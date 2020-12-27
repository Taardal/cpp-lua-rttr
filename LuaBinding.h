#pragma once

#include <lua/lua.hpp>
#include <rttr/registration>

class LuaBinding
{
private:
    union ArgumentValue
    {
        int intValue;
        long longValue;
        float floatValue;
        double doubleValue;
        const char* stringValue;
    };

private:
    lua_State* L;

public:
    explicit LuaBinding(lua_State* L);

    void BindGlobalMethods();

private:
    static int InvokeGlobalMethod(lua_State* L);

    static std::vector<rttr::argument> GetMethodArguments(lua_State* L, const rttr::method& method);

    static int GetMethodArgumentCount(lua_State* L, const rttr::array_range<rttr::parameter_info>& argumentInfos);
};