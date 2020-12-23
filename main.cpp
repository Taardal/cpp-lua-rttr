#include "print.h"
#include <iostream>
#include <lua/lua.hpp>
#include <rttr/registration>

void f()
{
    std::cout << "Hello World RTTR" << std::endl;
}

RTTR_REGISTRATION
{
    rttr::registration::method("f", &f);
}

int main()
{
    std::cout << "Hello World C++" << std::endl;

    rttr::type::invoke("f", {});

    lua_State* L = luaL_newstate();

    const char* script = R"(
        foo = "Hello World Lua"
    )";

    luaL_dostring(L, script);
    lua_getglobal(L, "foo");
    const char* foo = lua_tostring(L, -1);
    std::cout << foo << std::endl;

    return 0;
}
