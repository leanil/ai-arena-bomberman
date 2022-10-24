cmake_minimum_required(VERSION 3.16)
project(ai-arena-bomberman)

set(CMAKE_CXX_STANDARD 17)

if (MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -pedantic -Wno-unused-parameter)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bot")
    add_subdirectory(bot)
endif()
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/server")
    add_subdirectory(server)
endif()
