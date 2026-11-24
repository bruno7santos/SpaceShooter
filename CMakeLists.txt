# CMake — alternativa ao Makefile (Windows/Linux/macOS, MinGW ou Visual Studio).
#   cmake -S . -B build
#   cmake --build build
# O executável é gerado na raiz do projeto. Se a raylib não estiver instalada,
# o CMake baixa e compila a raylib automaticamente (precisa de internet e do git).
cmake_minimum_required(VERSION 3.15)
project(SpaceShooter C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

file(GLOB GAME_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.c")
add_executable(${PROJECT_NAME} ${GAME_SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/src")

# Usa a raylib instalada no sistema; se não houver, baixa a versão 5.5.
find_package(raylib 5.0 QUIET)
if (NOT raylib_FOUND)
    include(FetchContent)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git
        GIT_TAG 5.5
        GIT_SHALLOW ON)
    FetchContent_MakeAvailable(raylib)
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE raylib)
if (UNIX AND NOT APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE m)
endif()
if (MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /utf-8)   # os textos do jogo usam acentos (UTF-8)
endif()

# O jogo procura highscore.dat e assets/ na pasta de trabalho: gere o .exe na raiz do projeto.
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "$<1:${CMAKE_SOURCE_DIR}>"
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
