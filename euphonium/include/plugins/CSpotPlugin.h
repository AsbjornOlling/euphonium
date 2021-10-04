#ifndef EUPHONIUM_CSPOT_PLUGIN_H
#define EUPHONIUM_CSPOT_PLUGIN_H

#include "ScriptLoader.h"
#include <sol.hpp>

#include "Module.h"

class CSpotPlugin: public Module {
    public:
    CSpotPlugin();
    void loadScript(std::shared_ptr<ScriptLoader> scriptLoader, std::shared_ptr<sol::state> luaState);
    void setupLuaBindings(std::shared_ptr<sol::state> luaState);
    void startAudioThread();
};

#endif