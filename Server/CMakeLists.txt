cmake_minimum_required(VERSION 3.20)

project(ChatServer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/Zc:preprocessor>)

add_executable(${PROJECT_NAME} main.cc)

find_package(botan CONFIG REQUIRED)
if(${BUILD_SHARED_LIBS})
    target_link_libraries(${PROJECT_NAME} PRIVATE Botan::Botan)
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE Botan::Botan-static)
endif()
find_package(jwt-cpp CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE jwt-cpp::jwt-cpp)
find_package(fmt CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE fmt::fmt)
find_package(Drogon CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE Drogon::Drogon)
add_definitions(-DLIBASSERT_USE_FMT)
find_package(libassert CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE libassert::assert)

add_subdirectory(config)
target_include_directories(${PROJECT_NAME}
                           PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
                                   ${CMAKE_BINARY_DIR}/configured_files/include)

target_precompile_headers(${PROJECT_NAME} PRIVATE pch.h)

file(GLOB SRC_FILES "controllers/*.cc" "filters/*.cc" "plugins/*.cc" "models/*.cc")
target_sources(${PROJECT_NAME} PRIVATE ${SRC_FILES})
