#include <iostream>
#include <lua/lua.hpp>
#include <rttr/registration>

extern void printLua(lua_State* L, const std::string& tag);

void TargetFunction(int x, int y)
{
    printf("Hello World from LUA (%d, %d)", x, y);
}

RTTR_REGISTRATION
{
    rttr::registration::method("TargetFunction", &TargetFunction);
}

union ArgumentValue
{
    int intValue;
    long longValue;
    float floatValue;
    double doubleValue;
    const char* stringValue;
};

int GetArgumentCount(lua_State* L, const rttr::array_range<rttr::parameter_info>& parameterInfos)
{
    int luaArgumentCount = lua_gettop(L);
    int nativeArgumentCount = parameterInfos.size();
    if (luaArgumentCount != nativeArgumentCount)
    {
        printf("lua vs. native argument count mismatch [%d != %d]\n", luaArgumentCount, nativeArgumentCount);
        assert(false);
    }
    return luaArgumentCount;
}

std::vector<rttr::argument> GetArguments(lua_State* L, rttr::method* method)
{
    const rttr::array_range<rttr::parameter_info>& nativeArguments = method->get_parameter_infos();
    int argumentCount = GetArgumentCount(L, nativeArguments);
    std::vector<ArgumentValue> argumentValues(argumentCount);
    std::vector<rttr::argument> arguments(argumentCount);
    auto nativeArgumentsIterator = nativeArguments.begin();
    for (int i = 0; i < argumentCount; i++, nativeArgumentsIterator++)
    {
        const rttr::type nativeArgumentType = nativeArgumentsIterator->get_type();
        int luaIndex = i + 1;
        int luaType = lua_type(L, luaIndex);
        if (luaType == LUA_TNUMBER)
        {
            if (nativeArgumentType == rttr::type::get<int>())
            {
                argumentValues[i].intValue = (int) lua_tonumber(L, luaIndex);
                arguments[i] = argumentValues[i].intValue;
            }
            else if (nativeArgumentType == rttr::type::get<long>())
            {
                argumentValues[i].longValue = (long) lua_tonumber(L, luaIndex);
                arguments[i] = argumentValues[i].longValue;
            }
            else if (nativeArgumentType == rttr::type::get<float>())
            {
                argumentValues[i].floatValue = (float) lua_tonumber(L, luaIndex);
                arguments[i] = argumentValues[i].floatValue;
            }
            else if (nativeArgumentType == rttr::type::get<double>())
            {
                argumentValues[i].doubleValue = (double) lua_tonumber(L, luaIndex);
                arguments[i] = argumentValues[i].doubleValue;
            }
            else
            {
                printf("unknown rttr type [%s] for lua number\n", nativeArgumentType.get_name().to_string().c_str());
            }
        }
        else if (luaType == LUA_TSTRING)
        {
            argumentValues[i].stringValue = lua_tostring(L, luaIndex);
            arguments[i] = argumentValues[i].stringValue;
        }
        else
        {
            printf("unknown lua type [%d]\n", luaType);
        }
    }
    return arguments;
}

int LuaFunction(lua_State* L)
{
    auto* method = (rttr::method*) lua_touserdata(L, lua_upvalueindex(1));
    std::vector<rttr::argument> arguments = GetArguments(L, method);
    rttr::instance instance = {};
    rttr::variant result = method->invoke_variadic(instance, arguments);
    if (!result.is_valid())
    {
        printf("unable to call method [%s]\n", method->get_name().to_string().c_str());
        assert(false);
    }
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
        int upvalueCount = 1;
        lua_pushcclosure(L, LuaFunction, upvalueCount);
        lua_settable(L, -3);
    }

    const char* script = R"(
        Global.TargetFunction(66, 99)
    )";

    if (luaL_dostring(L, script) != LUA_OK)
    {
        printf( "Error: %s\n", lua_tostring( L, -1 ) );
    }

    return 0;
}
