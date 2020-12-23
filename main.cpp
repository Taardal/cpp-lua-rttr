#include "print.h"
#include <iostream>
#include <lua/lua.hpp>
#include <rttr/registration>

void FromLua()
{
    std::cout << "Hello World FromLua" << std::endl;
}

RTTR_REGISTRATION
{
    rttr::registration::method("FromLua", &FromLua);
}

int LuaFunction(lua_State* L)
{
    std::cout << "Lua Function" << std::endl;
    auto* method = (rttr::method*) lua_touserdata(L, lua_upvalueindex(1));
    rttr::instance instance = {};
    method->invoke(instance);
    return 0;
}

int main()
{
    lua_State* L = luaL_newstate();

    lua_newtable(L);
    lua_pushvalue(L, -1);
    lua_setglobal(L, "Global");

    lua_pushvalue(L, -1);
    for (const rttr::method& method : rttr::type::get_global_methods())
    {
        lua_pushstring(L, method.get_name().to_string().c_str());
        lua_pushlightuserdata(L, (void*) &method);
        lua_pushcclosure(L, LuaFunction, 1);
        lua_settable(L, -3);
    }

    const char* script = R"(
        Global.FromLua()
    )";

    if (luaL_dostring(L, script) != LUA_OK)
    {
        printf( "Error: %s\n", lua_tostring( L, -1 ) );
    }

    return 0;
}
