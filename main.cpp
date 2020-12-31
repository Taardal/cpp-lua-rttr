#include "LuaBinding.h"
#include <lua/lua.hpp>
#include <rttr/registration>
#include <iostream>

extern void printLua(lua_State* L, const std::string& tag);

void HelloWorld()
{
    printf("--- Hello World from LUA\n");
}

void HelloWorldWithArguments(int x, int y)
{
    printf("--- Hello World from LUA (%d, %d)\n", x, y);
}

class Sprite
{
public:
    int x;
    int y;

    Sprite()
            : x(0),
              y(0)
    {
        printf("--- Created sprite\n");
    }

    ~Sprite()
    {
        printf("--- Destroyed sprite\n");
    }

    int Move(int distanceX, int distanceY)
    {
        x += distanceX;
        y += distanceY;
        printf("--- Moved sprite by [%dx, %dy] to [%dx, %dy]\n", distanceX, distanceY, x, y);
        return x + y;
    }

    void Draw()
    {
        printf("--- Drawed sprite\n");
    }
};

RTTR_REGISTRATION
{
    rttr::registration::method("HelloWorld", &HelloWorld);
    rttr::registration::method("HelloWorldWithArguments", &HelloWorldWithArguments);
    rttr::registration::class_<Sprite>("Sprite")
            .constructor()
            .method("Move", &Sprite::Move)
            .method("Draw", &Sprite::Draw)
            .property("x", &Sprite::x)
            .property("y", &Sprite::y);
}

union ArgumentValue
{
    int intValue;
    long longValue;
    float floatValue;
    double doubleValue;
    const char* stringValue;
};

int CreateType(lua_State* L, const rttr::variant& variant);

int PutOnLuaStack(lua_State* L, const rttr::variant& variant)
{
    const std::string& typeName = variant.get_type().get_name().to_string();
    printf("putting value of type [%s] on lua stack\n", typeName.c_str());

    int returnValueCount = 0;
    if (!variant.is_type<void>())
    {
        if (variant.is_type<int>())
        {
            int value = variant.get_value<int>();
            printf("pushing [%d] onto lua stack\n", value);
            lua_pushnumber(L, value);
            returnValueCount++;
        }
        else if (variant.get_type().is_class() || variant.get_type().is_pointer())
        {
            returnValueCount = CreateType(L, variant);
        }
        else
        {
            luaL_error(L, "could not put value of unsupported type [%s] on lua stack\n", typeName.c_str());
        }
    }
    printf("put [%d] values of type [%s] on lua stack\n", returnValueCount, typeName.c_str());
    return returnValueCount;
}

int GetMethodArgumentCount(lua_State* L, const rttr::array_range<rttr::parameter_info>& argumentInfos)
{
    int luaArgumentCount = 0;
    for (int i = lua_gettop(L); i > 0; i--)
    {
        if (lua_isuserdata(L, i))
        {
            break;
        }
        luaArgumentCount++;
    }
    int nativeArgumentCount = argumentInfos.size();
    if (luaArgumentCount != nativeArgumentCount)
    {
        luaL_error(L, "lua vs. native argument count mismatch [%d != %d]\n", luaArgumentCount, nativeArgumentCount);
    }
    return luaArgumentCount;
}

int InvokeMethod(lua_State* L, const rttr::method& method, const rttr::instance& instance)
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
        int luaIndex = lua_gettop(L) - i;
        int luaType = lua_type(L, luaIndex);
        const rttr::type& argumentType = argumentInfoIterator->get_type();

        const char* luaTypeName = lua_typename(L, luaType);
        const std::string& argumentTypeName = argumentType.get_name().to_string();
        printf("parsing argument on lua index [%d] of lua type [%s] and native type [%s]\n", luaIndex, luaTypeName, argumentTypeName.c_str());

        if (luaType == LUA_TNUMBER)
        {
            if (argumentType == rttr::type::get<int>())
            {
                auto intValue = (int) lua_tonumber(L, luaIndex);
                printf("parsed int [%d]\n", intValue);
                argumentValues[i].intValue = intValue;
                arguments[i] = argumentValues[i].intValue;
            }
            else if (argumentType == rttr::type::get<long>())
            {
                auto longValue = (long) lua_tonumber(L, luaIndex);
                printf("parsed long [%ld]\n", longValue);
                argumentValues[i].longValue = longValue;
                arguments[i] = argumentValues[i].longValue;
            }
            else if (argumentType == rttr::type::get<float>())
            {
                auto floatValue = (float) lua_tonumber(L, luaIndex);
                printf("parsed float [%f]\n", floatValue);
                argumentValues[i].floatValue = floatValue;
                arguments[i] = argumentValues[i].floatValue;
            }
            else if (argumentType == rttr::type::get<double>())
            {
                auto doubleValue = (double) lua_tonumber(L, luaIndex);
                printf("parsed double [%f]\n", doubleValue);
                argumentValues[i].doubleValue = doubleValue;
                arguments[i] = argumentValues[i].doubleValue;
            }
            else
            {
                luaL_error(L, "unknown native type [%s] for lua type [%s]\n", argumentTypeName.c_str(), luaTypeName);
            }
        }
        else if (luaType == LUA_TSTRING)
        {
            const char* stringValue = lua_tostring(L, luaIndex);
            printf("parsed string [%s]\n", stringValue);
            argumentValues[i].stringValue = stringValue;
            arguments[i] = argumentValues[i].stringValue;
        }
        else
        {
            luaL_error(L, "unknown lua type [%s]\n", luaTypeName);
        }
    }
    std::reverse(arguments.begin(), arguments.end());

    const rttr::variant& result = method.invoke_variadic(instance, arguments);
    if (!result.is_valid())
    {
        luaL_error(L, "could not invoke method [%s] with [%d] arguments\n", methodName.c_str(), (int) arguments.size());
    }
    printf("invoked method [%s] with [%d] arguments\n", methodName.c_str(), (int) arguments.size());

    const std::string& returnTypeName = result.get_type().get_name().to_string();
    printf("return type from method [%s] is [%s]\n", methodName.c_str(), returnTypeName.c_str());

    int returnValueCount = PutOnLuaStack(L, result);
    printf("returning [%d] values of type [%s] from method [%s]\n", returnValueCount, returnTypeName.c_str(), methodName.c_str());
    return returnValueCount;
}

int InvokeGlobalMethod(lua_State* L)
{
    printf("invoking global method from lua\n");

    const auto& method = *(rttr::method*) lua_touserdata(L, lua_upvalueindex(1));
    const std::string& methodName = method.get_name().to_string();
    printf("invoking global method [%s]\n", methodName.c_str());

    const rttr::instance& instance = {};
    return InvokeMethod(L, method, instance);
}

std::string GetMetatableName(const rttr::type& type)
{
    std::string typeName;
    if (type.is_pointer())
    {
        typeName = type.get_raw_type().get_name().to_string();
    }
    else
    {
        typeName = type.get_name().to_string();
    }
    return typeName.append("__metatable");
}

int CreateType(lua_State* L)
{
    printf("creating native type from lua\n");

    const auto& type = *(rttr::type*) lua_touserdata(L, lua_upvalueindex(1));
    const std::string& typeName = type.get_name().to_string();
    printf("creating type [%s]\n", typeName.c_str());

    void* userData = lua_newuserdata(L, sizeof(rttr::variant));
    new(userData) rttr::variant(type.create());
    int userdataIndex = lua_gettop(L);
    printf("created userdata on lua index [%d] for type [%s]\n", userdataIndex, typeName.c_str());

    const std::string& metatableName = GetMetatableName(type);
    luaL_getmetatable(L, metatableName.c_str());
    lua_setmetatable(L, userdataIndex);
    printf("bound metatable [%s] to userdata on lua index [%d] for type [%s]\n", metatableName.c_str(), userdataIndex, typeName.c_str());

    lua_newtable(L);
    lua_setuservalue(L, userdataIndex);
    printf("bound a new table to userdata on lua index [%d] for type [%s]\n", userdataIndex, typeName.c_str());

    constexpr int createdCount = 1;
    return createdCount;
}

int CreateType(lua_State* L, const rttr::variant& variant)
{
    printf("creating native type from lua\n");

    rttr::type type = variant.get_type();
    const std::string& typeName = type.get_name().to_string();
    printf("creating type [%s]\n", typeName.c_str());

    void* userData = lua_newuserdata(L, sizeof(rttr::variant));
    new(userData) rttr::variant(variant);
    int userdataIndex = lua_gettop(L);
    printf("created userdata on lua index [%d] for type [%s]\n", userdataIndex, typeName.c_str());

    const std::string& metatableName = GetMetatableName(type);
    luaL_getmetatable(L, metatableName.c_str());
    lua_setmetatable(L, userdataIndex);
    printf("bound metatable [%s] to userdata on lua index [%d] for type [%s]\n", metatableName.c_str(), userdataIndex, typeName.c_str());

    lua_newtable(L);
    lua_setuservalue(L, userdataIndex);
    printf("bound a new table to userdata on lua index [%d] for type [%s]\n", userdataIndex, typeName.c_str());

    constexpr int createdCount = 1;
    return createdCount;
}

int DestroyType(lua_State* L)
{
    printf("destroying native type from lua\n");
    const auto& variant = *(rttr::variant*) lua_touserdata(L, -1);
    variant.~variant();
    printf("destroyed native type [%s]\n", variant.get_type().get_name().to_string().c_str());
    return 0;
}

int InvokeMethodOnType(lua_State* L)
{
    printf("invoking method on type\n");

    auto& method = *(rttr::method*) lua_touserdata(L, lua_upvalueindex(1));
    const std::string& methodName = method.get_name().to_string();
    printf("invoking method [%s] on type\n", methodName.c_str());

    constexpr int bottomOfLuaStackIndex = 1;
    int userdataIndex = bottomOfLuaStackIndex;
    if (!lua_isuserdata(L, userdataIndex))
    {
        luaL_error(L, "expected userdata on lua index [%d] when invoking method [%s] on type\n", userdataIndex, methodName.c_str());
    }
    const auto& variant = *(rttr::variant*) lua_touserdata(L, userdataIndex);
    const std::string& typeName = variant.get_type().get_name().to_string();
    printf("invoking method [%s] on type [%s]\n", methodName.c_str(), typeName.c_str());

    rttr::instance instance(variant);
    return InvokeMethod(L, method, instance);
}

int IndexType(lua_State* L)
{
    printf("indexing type from lua\n");

    const char* typeName = (const char*) lua_tostring(L, lua_upvalueindex(1));
    const rttr::type& type = rttr::type::get_by_name(typeName);
    printf("indexing type [%s]\n", typeName);

    constexpr int bottomOfLuaStackIndex = 1;
    int userdataIndex = bottomOfLuaStackIndex;
    int keyIndex = userdataIndex + 1;

    if (!lua_isuserdata(L, userdataIndex))
    {
        luaL_error(L, "expected userdata on lua index [%d] when indexing type [%s]\n", userdataIndex, typeName);
    }
    if (!lua_isstring(L, keyIndex))
    {
        luaL_error(L, "expected name of a native property or method on lua index [%d] when indexing type [%s]\n", keyIndex, typeName);
    }

    const char* key = lua_tostring(L, keyIndex);
    printf("indexing type [%s] by key [%s]\n", typeName, key);

    const rttr::method& method = type.get_method(key);
    if (method.is_valid())
    {
        const std::string& methodName = method.get_name().to_string();
        printf("found method [%s] to invoke on type [%s]\n", methodName.c_str(), typeName);

        void* methodUserdata = lua_newuserdata(L, sizeof(rttr::method));
        new(methodUserdata) rttr::method(method);
        printf("created userdata for method [%s] to be invoked on type [%s]\n", methodName.c_str(), typeName);

        constexpr int upvalueCount = 1;
        lua_pushcclosure(L, InvokeMethodOnType, upvalueCount);
        printf("returning closure with method [%s] to be invoked on type [%s] as upvalue\n", methodName.c_str(), typeName);

        int indexedMethodsCount = 1;
        return indexedMethodsCount;
    }

    const rttr::property& property = type.get_property(key);
    if (property.is_valid())
    {
        const std::string& propertyName = property.get_name().to_string();
        printf("found property [%s] to read from type [%s]\n", propertyName.c_str(), typeName);

        const rttr::variant& instance = *(rttr::variant*) lua_touserdata(L, bottomOfLuaStackIndex);
        const rttr::variant& propertyValue = property.get_value(instance);
        const std::string& propertyValueType = propertyValue.get_type().get_name().to_string();
        printf("reading property [%s] of type [%s] from type [%s]\n", propertyName.c_str(), propertyValueType.c_str(), typeName);

        int indexedPropertiesCount = PutOnLuaStack(L, propertyValue);
        return indexedPropertiesCount;
    }

    printf("getting uservalue (i.e. table) for userdata on index [%d]\n", userdataIndex);
    lua_getuservalue(L, userdataIndex);

    printf("getting key for value in table on index [%d]\n", keyIndex);
    lua_pushvalue(L, keyIndex);

    printf("getting value on key in table\n");
    lua_gettable(L, -2);

    printf("returning value found on key on index [%d] in uservalue (i.e. table) on index [%d]\n", keyIndex, userdataIndex);
    int indexedValuesCount = 1;
    return indexedValuesCount;
}

int NewIndexType(lua_State* L)
{
    printf("indexing type by unknown key from lua\n");

    const char* typeName = (const char*) lua_tostring(L, lua_upvalueindex(1));
    const rttr::type& type = rttr::type::get_by_name(typeName);
    printf("indexing type [%s] by unknown key\n", typeName);

    constexpr int bottomOfLuaStackIndex = 1;
    int userdataIndex = bottomOfLuaStackIndex;
    int keyIndex = userdataIndex + 1;
    int valueIndex = keyIndex + 1;

    if (!lua_isuserdata(L, userdataIndex))
    {
        luaL_error(L, "expected userdata on lua index [%d] when indexing type [%s]\n", userdataIndex, typeName);
    }
    if (!lua_isstring(L, keyIndex))
    {
        luaL_error(L, "expected name of a native property or method on lua index [%d] when indexing type [%s]\n", keyIndex, typeName);
    }

    const char* keyName = lua_tostring(L, keyIndex);
    const rttr::property& property = type.get_property(keyName);
    if (property.is_valid())
    {
        const std::string& propertyName = property.get_name().to_string();
        printf("found property [%s] to write to on type [%s]\n", propertyName.c_str(), typeName);

        const rttr::variant& instance = *(rttr::variant*) lua_touserdata(L, userdataIndex);
        const std::string& instanceTypeName = instance.get_type().get_name().to_string();
        printf("writing to property [%s] on instance of type [%s]\n", propertyName.c_str(), instanceTypeName.c_str());

        int valueLuaType = lua_type(L, valueIndex);
        const char* valueLuaTypeName = lua_typename(L, valueLuaType);
        printf("writing value of lua type [%d: %s] to property [%s] on instance of type [%s]\n", valueLuaType, valueLuaTypeName, propertyName.c_str(),
               instanceTypeName.c_str());

        bool didSetValueOnProperty = false;
        if (valueLuaType == LUA_TNUMBER)
        {
            if (property.get_type() == rttr::type::get<int>())
            {
                auto value = (int) lua_tonumber(L, valueIndex);
                printf("setting value [%d] on property [%s]\n", value, propertyName.c_str());
                didSetValueOnProperty = property.set_value(instance, value);
            }
            else if (property.get_type() == rttr::type::get<short>())
            {
                auto value = (short) lua_tonumber(L, valueIndex);
                printf("setting value [%d] on property [%s]\n", value, propertyName.c_str());
                didSetValueOnProperty = property.set_value(instance, value);
            }
            else
            {
                const std::string& propertyTypeName = property.get_type().get_name().to_string();
                luaL_error(L, "unknown native type [%s] for lua type [%s]\n", propertyTypeName.c_str(), valueLuaTypeName);
            }
        }
        else
        {
            luaL_error(L, "unknown lua type [%s]\n", valueLuaTypeName);
        }
        if (!didSetValueOnProperty)
        {
            luaL_error(L, "could not set value on property [%s] on type [%s]\n", propertyName.c_str(), typeName);
        }
        return 0;
    }


    printf("getting uservalue (i.e. table) for userdata on index [%d]\n", userdataIndex);
    lua_getuservalue(L, userdataIndex);

    printf("getting key [%s] for value in table on index [%d]\n", keyName, keyIndex);
    lua_pushvalue(L, keyIndex);

    printf("getting value for key [%s] in table on index [%d]\n", keyName, valueIndex);
    lua_pushvalue(L, valueIndex);

    printf("setting value on index [%d] on key on index [%d] on uservalue (i.e. table) on index [%d]\n", valueIndex, keyIndex, userdataIndex);
    lua_settable(L, -3);

    int valuesIndexedCount = 1;
    return valuesIndexedCount;
}

int PutMethodArgumentsOnLuaStack(lua_State* L)
{
    return 0;
}

template<typename T>
int PutMethodArgumentsOnLuaStack(lua_State* L, T& argument)
{
    const rttr::type& type = rttr::type::get<T>();
    if (type.is_class())
    {
        rttr::variant variant(&argument);
        return PutOnLuaStack(L, variant);
    }
    else
    {
        rttr::variant variant(argument);
        return PutOnLuaStack(L, variant);
    }
}

template<typename T, typename... E>
int PutMethodArgumentsOnLuaStack(lua_State* L, T& argument, E& ... arguments)
{
    return PutMethodArgumentsOnLuaStack(L, argument) + PutMethodArgumentsOnLuaStack(L, arguments...);
}

template<typename... T>
void CallMethod(lua_State* L, const char* methodName, T& ... arguments)
{
    lua_getglobal(L, methodName);
    int methodIndex = -1;
    if (lua_type(L, methodIndex) != LUA_TFUNCTION)
    {
        luaL_error(L, "expected method [%s] on lua stack index [%d]", methodName, methodIndex);
    }
    int argumentCount = PutMethodArgumentsOnLuaStack(L, arguments...);
    constexpr int resultsCount = 0;
    constexpr int messageHandlerIndex = 0;
    if (lua_pcall(L, argumentCount, resultsCount, messageHandlerIndex) != LUA_OK)
    {
        luaL_error(L, "could not call method [%s] on lua stack index [%d]: %s", methodName, methodIndex, lua_tostring(L, -1));
    }
}

lua_State* CreateState()
{
    lua_State* L = luaL_newstate();

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

    for (const auto& type : rttr::type::get_types())
    {
        const std::string& typeName = type.get_name().to_string();
        if (type.is_class())
        {
            //printf("binding class type [%s] to lua\n", typeName.c_str());

            lua_newtable(L);
            lua_pushvalue(L, -1);
            lua_setglobal(L, typeName.c_str());
            //printf("created global [%s]\n", typeName.c_str());

            lua_pushlightuserdata(L, (void*) &type);
            constexpr int newUpvalueCount = 1;
            lua_pushcclosure(L, CreateType, newUpvalueCount);
            lua_setfield(L, -2, "new");
            //printf("added new/create function with upvalue [%s]\n", typeName.c_str());

            const std::string& metatableName = GetMetatableName(type);
            luaL_newmetatable(L, metatableName.c_str());
            //printf("created metatable [%s]\n", metatableName.c_str());

            lua_pushstring(L, "__gc");
            lua_pushcfunction(L, DestroyType);
            lua_settable(L, -3);
            //printf("added garbage collect function to metatable [%s]\n", metatableName.c_str());

            lua_pushstring(L, "__index");
            lua_pushstring(L, typeName.c_str());
            constexpr int indexUpvalueCount = 1;
            lua_pushcclosure(L, IndexType, indexUpvalueCount);
            lua_settable(L, -3);
            //printf("added index function with upvalue [%s] to metatable [%s]\n", typeName.c_str(), metatableName.c_str());

            lua_pushstring(L, "__newindex");
            lua_pushstring(L, typeName.c_str());
            constexpr int newindexUpvalueCount = 1;
            lua_pushcclosure(L, NewIndexType, newindexUpvalueCount);
            lua_settable(L, -3);
            //printf("added newindex function with upvalue [%s] to metatable [%s]\n", typeName.c_str(), metatableName.c_str());
        }
    }

    return L;
}

void Load(lua_State* L, const char* script)
{
    if (luaL_loadstring(L, script) != LUA_OK)
    {
        luaL_error(L, "could not load lua script: %s", lua_tostring(L, -1));
    }
}

void Run(lua_State* L)
{
    constexpr int argumentCount = 0;
    constexpr int resultCount = LUA_MULTRET;
    constexpr int messageHandlerIndex = 0;
    if (lua_pcall(L, argumentCount, resultCount, messageHandlerIndex) != LUA_OK)
    {
        luaL_error(L, "could not run lua with loaded script: %s", lua_tostring(L, -1));
    }
}

const char* script = R"(
        Global.HelloWorld()
        Global.HelloWorldWithArguments(66, 99)

        local sprite = Sprite.new()
        sprite:Draw()

        local distance = sprite:Move(1, 1)
        sprite:Move(distance, 10)

        local x = sprite.x
        sprite:Move(x, 10)

        sprite.z = 100
        local z = sprite.z
        sprite:Move(z, 10)

        sprite.x = 0
        sprite:Move(sprite.x, 10)

        function Foo(x, y)
            Global.HelloWorldWithArguments(x, y)
        end

        function Bar(x)
            Global.HelloWorldWithArguments(x, x)
        end

        function Asd()
            Global.HelloWorldWithArguments(0, 0)
        end

        function Update(sprite)
            sprite.x = sprite.x + 10
            sprite:Move(0, 5)
        end
    )";

int main()
{
    lua_State* L = CreateState();

    Load(L, script);
    Run(L);

    int i = 1;
    int j = 2;
    CallMethod(L, "Foo", i, j);
    CallMethod(L, "Bar", i);
    CallMethod(L, "Asd");

    Sprite sprite;
    sprite.x = 100;
    CallMethod(L, "Update", sprite);
    CallMethod(L, "Update", sprite);

    return 0;
}
