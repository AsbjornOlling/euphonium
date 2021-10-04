#ifndef EUPHONIUM_CORE_H
#define EUPHONIUM_CORE_H

#include <vector>
#include <memory>
#include <iostream>
#include "AudioOutput.h"
#include "HTTPModule.h"
#include "plugins/CSpotPlugin.h"
#include "Module.h"
#include "ScriptLoader.h"
#include "EventBus.h"

#define AUDIO_BUFFER_SIZE 4096 * 16

class Core: public EventSubscriber {
private:
    std::shared_ptr<AudioOutput> currentOutput;
    std::shared_ptr<sol::state> luaState;
    std::vector<std::shared_ptr<Module>> requiredModules;
    std::vector<std::shared_ptr<Module>> registeredPlugins;
    bool outputConnected = false;

public:
    Core();
    ~Core() {};
    void selectAudioOutput(std::shared_ptr<AudioOutput> output);
    void loadPlugins(std::shared_ptr<ScriptLoader> loader);
    void handleLuaThread();
    void handleServerThread();
    void handleAudioOutputThread();
    void handleEvent(std::unique_ptr<Event> event);
    void setupBindings();
    void startAudioThreadForPlugin(std::string pluginName);

    std::shared_ptr<EventBus> luaEventBus;
    std::shared_ptr<CircularBuffer> audioBuffer;
};

#endif