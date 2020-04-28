#include <sp2/script/environment.h>
#include <sp2/script/luaBindings.h>
#include <sp2/assert.h>

static void luaInstructionCountHook(lua_State *L, lua_Debug *ar)
{
    luaL_error(L, "Instruction count exceeded.");
}

static void* luaAlloc(void *ud, void *ptr, size_t osize, size_t nsize)
{
    sp::script::Environment::AllocInfo* info = static_cast<sp::script::Environment::AllocInfo*>(ud);
    if (ptr)
        info->total -= osize;
    if (osize < nsize && info->total + nsize > info->max)
    {
        info->total += osize;
        return nullptr;
    }
    info->total += nsize;
    if (nsize == 0)
    {
        free(ptr);
        return nullptr;
    }
    return realloc(ptr, nsize);
}

namespace sp {
namespace script {

Environment::Environment()
{
    if (!script::global_lua_state)
        script::global_lua_state = script::createLuaState();
    lua = script::global_lua_state;

    //Create a new lua environment.
    //REGISTY[this] = {"metatable": {"__index": _G, "environment_ptr": this}}
    lua_newtable(lua); //environment

    lua_newtable(lua); //environment metatable
    lua_pushstring(lua, "[environment]");
    lua_setfield(lua, -2, "__metatable");
    lua_pushglobaltable(lua);
    lua_setfield(lua, -2, "__index");
    //Set the ptr in the metatable.
    lua_pushlightuserdata(lua, this);
    lua_setfield(lua, -2, "environment_ptr");

    lua_setmetatable(lua, -2);

    lua_rawsetp(lua, LUA_REGISTRYINDEX, this);

    sp2assert(lua_gettop(lua) == 0, "Lua stack incorrect");
}

Environment::Environment(const SandboxConfig& sandbox_config)
{
    alloc_info.total = 0;
    alloc_info.max = sandbox_config.memory_limit;

    lua = lua_newstate(luaAlloc, &alloc_info);
    sp2assert(lua, "Failed to create lua state for sandboxed environment. Not giving enough memory for basic state?");
    lua = script::createLuaState(lua);
    lua_sethook(lua, luaInstructionCountHook, LUA_MASKCOUNT, sandbox_config.instruction_limit);
    //Create a new lua environment.
    //REGISTY[this] = {"metatable": {"__index": _G, "environment_ptr": this}}
    lua_pushglobaltable(lua); //environment

    lua_newtable(lua); //environment metatable
    lua_pushstring(lua, "[environment]");
    lua_setfield(lua, -2, "__metatable");
    //Set the ptr in the metatable.
    lua_pushlightuserdata(lua, this);
    lua_setfield(lua, -2, "environment_ptr");

    lua_setmetatable(lua, -2);

    lua_rawsetp(lua, LUA_REGISTRYINDEX, this);

    sp2assert(lua_gettop(lua) == 0, "Lua stack incorrect");
}

Environment::~Environment()
{
    //Remove our environment from the registry.
    //REGISTRY[this] = nil
    lua_pushnil(lua);
    lua_rawsetp(lua, LUA_REGISTRYINDEX, this);

    sp2assert(lua_gettop(lua) == 0, "Lua stack incorrect");

    if (lua != script::global_lua_state)
        lua_close(lua);
}

void Environment::setGlobal(const string& name, lua_CFunction function)
{
    //Get the environment table from the registry.
    lua_rawgetp(lua, LUA_REGISTRYINDEX, this);

    //Set our variable in this environment table, with our environment as first upvalue.
    if (function)
    {
        lua_pushvalue(lua, -1);
        lua_pushcclosure(lua, function, 1);
    }
    else
    {
        lua_pushnil(lua);
    }
    lua_setfield(lua, -2, name.c_str());

    //Pop the table
    lua_pop(lua, 1);
}

void Environment::setGlobal(const string& name, ScriptBindingObject* ptr)
{
    //Get the environment table from the registry.
    lua_rawgetp(lua, LUA_REGISTRYINDEX, this);

    //Set our variable in this environment table
    pushToLua(lua, ptr);
    lua_setfield(lua, -2, name.c_str());

    //Pop the table
    lua_pop(lua, 1);
}

void Environment::setGlobal(const string& name, P<ScriptBindingObject> ptr)
{
    setGlobal(name, *ptr);
}

void Environment::setGlobal(const string& name, bool value)
{
    //Get the environment table from the registry.
    lua_rawgetp(lua, LUA_REGISTRYINDEX, this);
    
    //Set our variable in this environment table
    lua_pushboolean(lua, value);
    lua_setfield(lua, -2, name.c_str());
    
    //Pop the table
    lua_pop(lua, 1);
}

void Environment::setGlobal(const string& name, int value)
{
    //Get the environment table from the registry.
    lua_rawgetp(lua, LUA_REGISTRYINDEX, this);
    
    //Set our variable in this environment table
    lua_pushinteger(lua, value);
    lua_setfield(lua, -2, name.c_str());
    
    //Pop the table
    lua_pop(lua, 1);
}

void Environment::setGlobal(const string& name, const string& value)
{
    //Get the environment table from the registry.
    lua_rawgetp(lua, LUA_REGISTRYINDEX, this);
    
    //Set our variable in this environment table
    lua_pushstring(lua, value.c_str());
    lua_setfield(lua, -2, name.c_str());
    
    //Pop the table
    lua_pop(lua, 1);
}

bool Environment::load(const string& resource_name)
{
    io::ResourceStreamPtr stream = io::ResourceProvider::get(resource_name);
    if (!stream)
    {
        LOG(Warning, "Failed to find script resource:", resource_name);
        return false;
    }
    return _load(stream, resource_name);
}

bool Environment::load(io::ResourceStreamPtr resource)
{
    return _load(resource, "=[resource]");
}

bool Environment::run(const string& code)
{
    return _run(code, "=[string]");
}

CoroutinePtr Environment::runCoroutine(const string& code)
{
    lua_State* L = lua_newthread(lua);

    last_error = "";
    if (luaL_loadbufferx(L, code.c_str(), code.length(), "=[string]", "t"))
    {
        last_error = luaL_checkstring(L, -1);
        LOG(Error, "LUA: load:", last_error);
        lua_pop(L, 1);
        lua_pop(lua, 1);
        return nullptr;
    }

    //Get the environment table from the registry.
    lua_rawgetp(L, LUA_REGISTRYINDEX, this);
    //set the environment table it as 1st upvalue
    lua_setupvalue(L, -2, 1);

    //Set the hook as it was already, so the internal counter gets reset for sandboxed environments.
    lua_sethook(L, lua_gethook(L), lua_gethookmask(L), lua_gethookcount(L));

    int result = lua_resume(L, nullptr, 0);
    if (result != LUA_OK && result != LUA_YIELD)
    {
        last_error = lua_tostring(L, -1);
        LOG(Error, "LUA: Run:", last_error);
        lua_pop(lua, 1); //remove coroutine
        return nullptr;
    }
    if (result == LUA_OK) //Coroutine didn't yield. So no state to store for it.
    {
        lua_pop(lua, 1); //remove coroutine
        return nullptr;
    }
    std::shared_ptr<Coroutine> coroutine = std::make_shared<Coroutine>(lua, L);
    //coroutine is removed by constructor of Coroutine object.
    return coroutine;
}


bool Environment::_load(io::ResourceStreamPtr resource, const string& name)
{
    if (!resource)
        return false;

    return _run(resource->readAll(), "@" + name);
}

bool Environment::_run(const string& code, const string& name)
{
    last_error = "";
    if (luaL_loadbufferx(lua, code.c_str(), code.length(), name.c_str(), "t"))
    {
        last_error = luaL_checkstring(lua, -1);
        LOG(Error, "LUA: load:", last_error);
        lua_pop(lua, 1);
        return false;
    }

    //Get the environment table from the registry.
    lua_rawgetp(lua, LUA_REGISTRYINDEX, this);
    //set the environment table it as 1st upvalue
    lua_setupvalue(lua, -2, 1);
    
    //Set the hook as it was already, so the internal counter gets reset for sandboxed environments.
    lua_sethook(lua, lua_gethook(lua), lua_gethookmask(lua), lua_gethookcount(lua));
    //Call the actual code.
    if (lua_pcall(lua, 0, 0, 0))
    {
        last_error = luaL_checkstring(lua, -1);
        LOG(Error, "LUA: run:", last_error);
        lua_pop(lua, 1);
        return false;
    }
    return true;
}

}//namespace script
}//namespace sp
