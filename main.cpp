#include "LuaBinding.h"
#include <lua/lua.hpp>
#include <rttr/registration>
#include <iostream>

extern void printLua(lua_State* L, const std::string& tag);

void HelloWorld()
{
    printf("Hello World from LUA\n");
}

void HelloWorldWithArguments(int x, int y)
{
    printf("Hello World from LUA (%d, %d)\n", x, y);
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
        printf("created sprite...\n");
    }

    ~Sprite()
    {
        printf("destroyed sprite...\n");
    }

    void Update()
    {
        printf("updated sprite...\n");
    }

    void Draw()
    {
        printf("drawed sprite...\n");
    }
};

RTTR_REGISTRATION
{
    rttr::registration::method("HelloWorld", &HelloWorld);
    rttr::registration::method("HelloWorldWithArguments", &HelloWorldWithArguments);
    rttr::registration::class_<Sprite>("Sprite")
            .constructor()
            .method("Update", &Sprite::Update)
            .method("Draw", &Sprite::Draw)
            .property("x", &Sprite::x)
            .property("y", &Sprite::y);
}

std::string GetMetatableName(const rttr::type& type)
{
    return type.get_name().to_string().append("__metatable");
}

int CreateType(lua_State* L)
{
    printf("creating native type from lua\n");

    auto* type = (rttr::type*) lua_touserdata(L, lua_upvalueindex(1));
    const std::string& typeName = type->get_name().to_string();
    printf("creating type [%s]\n", typeName.c_str());
    
    void* userData = lua_newuserdata(L, sizeof(rttr::variant));
    new(userData) rttr::variant(type->create());
    int userdataIndex = lua_gettop(L);
    printf("created userdata on index [%d] for type [%s]\n", userdataIndex, typeName.c_str());

    const std::string& metatableName = GetMetatableName(*type);
    luaL_getmetatable(L, metatableName.c_str());
    lua_setmetatable(L, userdataIndex);
    printf("bound metatable [%s] to userdata on index [%d] for type [%s]\n", metatableName.c_str(), userdataIndex, typeName.c_str());

    lua_newtable(L);
    lua_setuservalue(L, userdataIndex);
    printf("bound a new table to userdata on index [%d] for type [%s]\n", userdataIndex, typeName.c_str());

    constexpr int createdCount = 1;
    return createdCount;
}

int DestroyType(lua_State* L)
{
    printf("destroying native type from lua\n");
    auto* variant = (rttr::variant*) lua_touserdata(L, -1);
    variant->~variant();
    printf("destroyed native type [%s]\n", variant->get_type().get_name().to_string().c_str());
    return 0;
}

int InvokeMethodOnType(lua_State* L)
{
    printf("invoking method on type\n");

    auto& method = *(rttr::method*) lua_touserdata(L, lua_upvalueindex(1));
    const std::string& methodName = method.get_name().to_string();
    printf("invoking method [%s] on type\n", methodName.c_str());

    int userdataIndex = lua_gettop(L);
    if (!lua_isuserdata(L, userdataIndex))
    {
        luaL_error(L, "expected userdata on stack index [%d] when invoking method [%s] type\n", userdataIndex, methodName.c_str());
        assert(false);
    }
    auto& variant = *(rttr::variant*) lua_touserdata(L, userdataIndex);
    const std::string& typeName = variant.get_type().get_name().to_string();
    printf("invoking method [%s] on type [%s]\n", methodName.c_str(), typeName.c_str());

    rttr::variant result = method.invoke(variant);
    if (!result.is_valid())
    {
        luaL_error(L, "could not invoke method [%s] on type [%s]\n", methodName.c_str(), typeName.c_str());
        assert(false);
    }
    return 0;
}

int IndexType(lua_State* L)
{
    printf("indexing type from lua\n");

    const char* typeName = (const char*) lua_tostring(L, lua_upvalueindex(1));
    const rttr::type& type = rttr::type::get_by_name(typeName);
    printf("indexing type [%s]\n", typeName);

    int keyIndex = lua_gettop(L);
    if (!lua_isstring(L, keyIndex))
    {
        luaL_error(L, "expected name of a native property or method on stack index [%d] when indexing type [%s]\n", keyIndex, typeName);
        assert(false);
    }
    int userdataIndex = keyIndex - 1;
    if (!lua_isuserdata(L, userdataIndex))
    {
        luaL_error(L, "expected userdata on stack index [%d] when indexing type [%s]\n", userdataIndex, typeName);
        assert(false);
    }

    const char* key = lua_tostring(L, keyIndex);
    printf("indexing type [%s] by key [%s]\n", typeName, key);

    const rttr::method& method = type.get_method(key);
    if (method.is_valid())
    {
        const std::string& methodName = method.get_name().to_string();
        printf("found method [%s] to invoke on type [%s]\n", methodName.c_str(), typeName);

        void* methodUserdata = lua_newuserdata(L, sizeof(rttr::method));
        new (methodUserdata) rttr::method(method);
        printf("created userdata for method [%s] to invoke on type [%s]\n", methodName.c_str(), typeName);

        constexpr int upvalueCount = 1;
        lua_pushcclosure(L, InvokeMethodOnType, upvalueCount);
        printf("returning closure with method [%s] to invoke on type [%s] as upvalue\n", methodName.c_str(), typeName);

        constexpr int methodCount = 1;
        return methodCount;
    }

    const rttr::property& property = type.get_property(key);
    if (property.is_valid())
    {
        printf("INCOMPLETE! index property on type [%s] by key [%s]\n", typeName, key);
    }

    luaL_error(L, "could not index type [%s] by key [%s]", typeName, key);
    return 0;
}

int main()
{
    lua_State* L = luaL_newstate();

    LuaBinding luaBinding(L);
    luaBinding.BindGlobalMethods();

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
        }
    }

    const char* script = R"(
        --Global.HelloWorld()
        --Global.HelloWorldWithArguments(66, 99)
        local sprite = Sprite.new()
        sprite:Draw()
    )";

    if (luaL_dostring(L, script) != LUA_OK)
    {
        luaL_error(L, "Error: %s\n", lua_tostring(L, -1));
    }

    return 0;
}
