#include "HTTPDispatcher.h"
#include <memory>
#include "BellHTTPServer.h"
#include "civetweb.h"

using namespace euph;

HTTPDispatcher::HTTPDispatcher(std::shared_ptr<euph::Context> ctx) {
  this->ctx = ctx;
  this->responseSemaphore = std::make_unique<bell::WrappedSemaphore>(0);
  this->server = std::make_shared<bell::BellHTTPServer>(8080);
}

HTTPDispatcher::~HTTPDispatcher() {}

void HTTPDispatcher::initialize() {
  EUPH_LOG(debug, TAG, "Registering HTTP handlers");

  this->server->registerWS(
      "/events",
      [this](struct mg_connection* conn, char* data, size_t dataSize) {
        // Convert the data to a string
        std::string dataString(data, dataSize);

        // Post the event to the event bus
        auto event = std::make_unique<VmWebsocketEvent>(dataString);
        this->ctx->eventBus->postEvent(std::move(event));
      },
      [this](struct mg_connection* conn, bell::BellHTTPServer::WSState state) {
        switch (state) {
          case bell::BellHTTPServer::WSState::READY: {
            EUPH_LOG(debug, TAG, "Websocket connection open");
            std::scoped_lock lock(this->websocketConnectionsMutex);
            this->websocketConnections.push_back(conn);
            break;
          }
          case bell::BellHTTPServer::WSState::CLOSED: {
            EUPH_LOG(debug, TAG, "Websocket connection closed");
            std::scoped_lock lock(this->websocketConnectionsMutex);
            // remove the connection from the list
            this->websocketConnections.erase(find(websocketConnections.begin(),
                                                  websocketConnections.end(),
                                                  conn));
            break;
          }
          default:
            break;
        }
      });
}

std::shared_ptr<bell::BellHTTPServer> HTTPDispatcher::getServer() {
  return this->server;
}

void HTTPDispatcher::setupBindings() {
  EUPH_LOG(debug, TAG, "Setting up HTTP-scripting bindings");
  ctx->vm->export_this("_register_handler", this,
                       &HTTPDispatcher::_registerHandler, "http");
  ctx->vm->export_this("_read_body", this, &HTTPDispatcher::_readBody, "http");
  ctx->vm->export_this("_read_content_length", this,
                       &HTTPDispatcher::_readContentLength, "http");
  ctx->vm->export_this("_write_response", this, &HTTPDispatcher::_writeResponse,
                       "http");
  ctx->vm->export_this("_read_route_params", this,
                       &HTTPDispatcher::_readRouteParams, "http");
  ctx->vm->export_this("_broadcast_websocket", this,
                       &HTTPDispatcher::_broadcastWebsocket, "http");
}

std::string HTTPDispatcher::_readBody(int connId) {
  auto conn = this->bindConnections[connId];
  const struct mg_request_info* reqInfo = mg_get_request_info(conn);
  std::string body;
  body.reserve(reqInfo->content_length);

  mg_read(conn, body.data(), reqInfo->content_length);
  return body;
}

size_t HTTPDispatcher::_readContentLength(int connId) {
  auto conn = this->bindConnections[connId];
  const struct mg_request_info* reqInfo = mg_get_request_info(conn);
  return reqInfo->content_length;
}

void HTTPDispatcher::_registerHandler(int httpMethod, std::string path,
                                      int handlerId) {
  HTTPDispatcher::Method method =
      static_cast<HTTPDispatcher::Method>(httpMethod);
  switch (httpMethod) {
    case HTTPDispatcher::Method::GET:
      this->server->registerGet(
          path, [this, handlerId](struct mg_connection* conn) {
            this->bindConnections[this->nextBindId] = conn;

            // Prepare a response event to the scripting layer
            auto event = std::make_unique<HTTPDispatcher::VmEvent>(
                handlerId, this->nextBindId);
            this->ctx->eventBus->postEvent(std::move(event));

            this->nextBindId = (this->nextBindId + 1) % MAX_CONNECTION_BINDS;
            this->responseSemaphore->twait(HTTP_RESPONSE_TIMEOUT);

            return this->server->makeEmptyResponse();
          });
      break;
    case HTTPDispatcher::Method::POST:
      this->server->registerPost(
          path, [this, handlerId](struct mg_connection* conn) {
            this->bindConnections[this->nextBindId] = conn;

            // Prepare a response event to the scripting layer
            auto event = std::make_unique<HTTPDispatcher::VmEvent>(
                handlerId, this->nextBindId);
            this->ctx->eventBus->postEvent(std::move(event));

            this->nextBindId = (this->nextBindId + 1) % MAX_CONNECTION_BINDS;
            this->responseSemaphore->twait(HTTP_RESPONSE_TIMEOUT);

            return this->server->makeEmptyResponse();
          });
      break;
  }
}

void HTTPDispatcher::broadcastWebsocket(const std::string& body) {
  std::scoped_lock lock(this->websocketConnectionsMutex);

  // for each in this->websocketConnections
  for (auto&& conn : this->websocketConnections) {
    mg_websocket_write(conn, MG_WEBSOCKET_OPCODE_TEXT, body.data(), body.size());
  }
}

void HTTPDispatcher::_broadcastWebsocket(std::string body) {
  this->broadcastWebsocket(body);
}

void HTTPDispatcher::_writeResponse(int connId, std::string body,
                                    std::string contentType, int statusCode) {
  auto conn = this->bindConnections[connId];

  mg_printf(conn,
            "HTTP/1.1 %d OK\r\nContent-Type: "
            "%s\r\nAccess-Control-Allow-Origin: *\r\nConnection: close\r\n\r\n",
            statusCode, contentType.c_str());
  mg_write(conn, body.c_str(), body.size());
  this->responseSemaphore->give();
}

berry::map HTTPDispatcher::_readRouteParams(int connId) {
  auto conn = this->bindConnections[connId];
  auto params = bell::BellHTTPServer::extractParams(conn);

  berry::map result;
  for (auto&& param : params) {
    result[param.first] = param.second;
  }

  return result;
}