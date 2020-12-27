#include "LuaBinding.h"

LuaBinding::LuaBinding(lua_State* L)
        : L(L)
{
}

void LuaBinding::BindGlobalMethods()
{
    lua_newtable(L);
    lua_pushvalue(L, -1);
    lua_setglobal(L, "Global");
    for (const auto& method : rttr::type::get_global_methods())
    {
        lua_pushstring(L, method.get_name().to_string().c_str());
        lua_pushlightuserdata(L, (void*) &method);
        constexpr int upvalueCount = 1;
        lua_pushcclosure(L, InvokeGlobalMethod, upvalueCount); 
        lua_settable(L, -3);
    }
}

int LuaBinding::InvokeGlobalMethod(lua_State* L)
{
    printf("invoking global method from lua\n");
    
    auto& method = *(rttr::method*) lua_touserdata(L, lua_upvalueindex(1));
    const std::string& methodName = method.get_name().to_string();
    printf("invoking global method [%s]\n", methodName.c_str());

    const rttr::instance& instance = {};
    const std::vector<rttr::argument>& arguments = GetMethodArguments(L, method);
    const rttr::variant& result = method.invoke_variadic(instance, arguments);
    if (!result.is_valid())
    {
        luaL_error(L, "could not invoke method [%s] with [%d] arguments\n", methodName.c_str(), (int) arguments.size());
    }
    printf("invoked global method [%s] with [%d] arguments\n", methodName.c_str(), (int) arguments.size());

    const std::string& returnTypeName = result.get_type().get_name().to_string();
    printf("return type from global method [%s] is [%s]\n", methodName.c_str(), returnTypeName.c_str());
    
    int returnValueCount = 0;
    if (!result.is_type<void>())
    {
        if (result.is_type<int>())
        {
            lua_pushnumber(L, result.get_value<int>());
            returnValueCount++;
        }
        else
        {
            luaL_error(L, "unsupported return type [%s] from global method [%s]\n", returnTypeName.c_str(), methodName.c_str());
        }
    }
    printf("returning [%d] values of type [%s] from global method [%s]\n", returnValueCount, returnTypeName.c_str(), methodName.c_str());
    return returnValueCount;
}

std::vector<rttr::argument> LuaBinding::GetMethodArguments(lua_State* L, const rttr::method& method)
{
    const std::string& methodName = method.get_name().to_string();
    printf("getting arguments for method [%s]\n", methodName.c_str());

    const rttr::array_range<rttr::parameter_info>& argumentInfos = method.get_parameter_infos();
    int argumentCount = GetMethodArgumentCount(L, argumentInfos);
    printf("getting [%d] arguments for method [%s]\n", argumentCount, methodName.c_str());

    std::vector<ArgumentValue> argumentValues(argumentCount);
    std::vector<rttr::argument> arguments(argumentCount);
    auto argumentInfoIterator = argumentInfos.begin();
    for (int i = 0; i < argumentCount; i++, argumentInfoIterator++)
    {
        int luaIndex = i + 1;
        int luaType = lua_type(L, luaIndex);
        const rttr::type argumentType = argumentInfoIterator->get_type();

        const char* luaTypeName = lua_typename(L, luaType);
        const std::string& argumentTypeName = argumentType.get_name().to_string();
        printf("parsing argument on lua index [%d] of lua type [%s] and native type [%s]\n", luaIndex, luaTypeName, argumentTypeName.c_str());

        if (luaType == LUA_TNUMBER)
        {
            if (argumentType == rttr::type::get<int>())
            {
                argumentValues[i].intValue = (int) lua_tonumber(L, luaIndex);
                arguments[i] = argumentValues[i].intValue;
            }
            else if (argumentType == rttr::type::get<long>())
            {
                argumentValues[i].longValue = (long) lua_tonumber(L, luaIndex);
                arguments[i] = argumentValues[i].longValue;
            }
            else if (argumentType == rttr::type::get<float>())
            {
                argumentValues[i].floatValue = (float) lua_tonumber(L, luaIndex);
                arguments[i] = argumentValues[i].floatValue;
            }
            else if (argumentType == rttr::type::get<double>())
            {
                argumentValues[i].doubleValue = (double) lua_tonumber(L, luaIndex);
                arguments[i] = argumentValues[i].doubleValue;
            }
            else
            {
                luaL_error(L, "unknown native type [%s] for lua type [%s]\n", argumentTypeName.c_str(), luaTypeName);
            }
        }
        else if (luaType == LUA_TSTRING)
        {
            argumentValues[i].stringValue = lua_tostring(L, luaIndex);
            arguments[i] = argumentValues[i].stringValue;
        }
        else
        {
            luaL_error(L, "unknown lua type [%s]\n", luaTypeName);
        }
    }

    printf("returning [%d] arguments for method [%s]\n", (int) arguments.size(), methodName.c_str());
    return arguments;
}

int LuaBinding::GetMethodArgumentCount(lua_State* L, const rttr::array_range<rttr::parameter_info>& argumentInfos)
{
    int luaArgumentCount = lua_gettop(L);
    int nativeArgumentCount = argumentInfos.size();
    if (luaArgumentCount != nativeArgumentCount)
    {
        luaL_error(L, "lua vs. native argument count mismatch [%d != %d]\n", luaArgumentCount, nativeArgumentCount);
    }
    return luaArgumentCount;
}