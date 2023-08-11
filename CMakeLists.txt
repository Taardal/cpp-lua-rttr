cmake_minimum_required(VERSION 3.17)
project(lua_demo)

set(CMAKE_CXX_STANDARD 20)

add_executable(lua_demo main.cpp printlua.cpp)

set(INSTALL_DIR ${PROJECT_SOURCE_DIR})
set(BIN_DIR "${PROJECT_SOURCE_DIR}/bin")

set(RUNTIME_NAME "app")
set(RUNTIME_NAME_DEBUG "${RUNTIME_NAME}_debug")
set(RUNTIME_NAME_RELEASE "${RUNTIME_NAME}_release")

set_target_properties(
        ${PROJECT_NAME}
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${BIN_DIR}
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${BIN_DIR}/debug"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${BIN_DIR}/release"
        RUNTIME_OUTPUT_NAME ${RUNTIME_NAME}
        RUNTIME_OUTPUT_NAME_DEBUG ${RUNTIME_NAME_DEBUG}
        RUNTIME_OUTPUT_NAME_RELEASE ${RUNTIME_NAME_RELEASE}
)

install(TARGETS ${PROJECT_NAME} DESTINATION ${INSTALL_DIR})
install(TARGETS ${PROJECT_NAME} CONFIGURATIONS Debug DESTINATION ${INSTALL_DIR})
install(TARGETS ${PROJECT_NAME} CONFIGURATIONS Release DESTINATION ${INSTALL_DIR})

set(LUA_NAME lua)
set(LUA_DIR lib/lua-5.4.0)
add_subdirectory(${LUA_DIR})
target_include_directories(${PROJECT_NAME} PRIVATE ${LUA_DIR}/include)
target_link_libraries(${PROJECT_NAME} ${LUA_NAME})

set(RTTR_DIR lib/rttr-0.9.6/build/install/share/rttr/cmake)
find_package(RTTR CONFIG REQUIRED Core)
target_link_libraries(${PROJECT_NAME} RTTR::Core)