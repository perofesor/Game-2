cmake_minimum_required(VERSION 3.16)
project(ArcaneAscendant CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
set(CMAKE_CXX_FLAGS_RELEASE "-O2")

# ---- sources ----
file(GLOB GAME_SOURCES "${CMAKE_SOURCE_DIR}/src/*.cpp")
set(GLAD_SOURCE "${CMAKE_SOURCE_DIR}/external/glad.c")

if(WIN32 OR MINGW)
    enable_language(RC)
    set(APP_RESOURCES "${CMAKE_SOURCE_DIR}/src/app.rc")
else()
    set(APP_RESOURCES "")
endif()

add_executable(ArcaneAscendant ${GAME_SOURCES} ${GLAD_SOURCE} ${APP_RESOURCES})

target_include_directories(ArcaneAscendant PRIVATE
    "${CMAKE_SOURCE_DIR}/include"
    "${CMAKE_SOURCE_DIR}/src"
)

if(WIN32 OR MINGW)
    # ---- Windows (MinGW cross-compile) ----
    message(STATUS "Configuring for Windows (MinGW)")
    target_link_directories(ArcaneAscendant PRIVATE "${CMAKE_SOURCE_DIR}/libs/win64")
    target_link_libraries(ArcaneAscendant PRIVATE glfw3 opengl32 gdi32 winmm imm32 ole32 oleaut32 version uuid)
    # Static link the runtime so the exe is portable (double-click, no DLLs needed)
    target_link_options(ArcaneAscendant PRIVATE -static -static-libgcc -static-libstdc++ -mwindows)
else()
    # ---- Linux (for local testing) ----
    message(STATUS "Configuring for Linux (native test)")
    find_package(OpenGL REQUIRED)
    find_library(GLFW_LIB glfw)
    target_link_libraries(ArcaneAscendant PRIVATE ${GLFW_LIB} OpenGL::GL dl pthread X11)
endif()
