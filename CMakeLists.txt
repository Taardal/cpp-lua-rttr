cmake_minimum_required(VERSION 3.17)
project(lua_demo)

set(CMAKE_CXX_STANDARD 20)

add_executable(lua_demo main.cpp printlua.cpp LuaBinding.cpp LuaBinding.h)

set(LUA_NAME lua)
set(LUA_DIR lib/lua-5.4.0)
add_subdirectory(${LUA_DIR})
target_include_directories(${PROJECT_NAME} PRIVATE ${LUA_DIR}/include)
target_link_libraries(${PROJECT_NAME} ${LUA_NAME})

set(RTTR_DIR lib/rttr-0.9.6/build/install/share/rttr/cmake)
find_package(RTTR CONFIG REQUIRED Core)
target_link_libraries(${PROJECT_NAME} RTTR::Core)