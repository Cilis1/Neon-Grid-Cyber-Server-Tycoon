cmake_minimum_required(VERSION 3.24)

project(NeonGridCyberServerTycoon
  VERSION 1.0.0
  DESCRIPTION "Neon Grid: Cyber Server Tycoon - a cyberpunk server management game"
  LANGUAGES C CXX)

# resources.rc (icon + version info) needs the RC language on Windows
if(WIN32)
  enable_language(RC)
endif()

# Our code intentionally uses fopen/getenv; silence MSVC's C4996 deprecation noise
if(MSVC)
  add_compile_definitions(_CRT_SECURE_NO_WARNINGS NOMINMAX)
endif()

# ---------------------------------------------------------------------------
# Global settings
# ---------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

option(NEON_BUILD_GUI       "Build the SDL2/Dear ImGui game executable" ON)
option(NEON_BUILD_INSTALLER "Build the CyberServerTycoonSetup bootstrap installer" ON)
option(NEON_BUILD_TESTS     "Build Catch2 unit tests" ON)
option(NEON_USE_CURL        "Enable libcurl networking (installer downloader / version checker)" ON)
option(NEON_USE_LUA         "Enable Lua 5.4 scripting via sol2" ON)

include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

# Warning level for our own targets (MSVC /W3 per project policy).
if(MSVC)
  set(NEON_WARNING_FLAGS /W3 /permissive-)
else()
  set(NEON_WARNING_FLAGS -Wall -Wextra -Wpedantic)
endif()

# ---------------------------------------------------------------------------
# Core dependencies (always needed): nlohmann/json + spdlog (header-only)
# ---------------------------------------------------------------------------
FetchContent_Declare(nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.3
  GIT_SHALLOW TRUE)
FetchContent_Declare(spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG v1.14.1
  GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(nlohmann_json spdlog)

# ---------------------------------------------------------------------------
# Lua 5.4 + sol2 (optional scripting layer)
# ---------------------------------------------------------------------------
if(NEON_USE_LUA)
  FetchContent_Declare(lua_src
    GIT_REPOSITORY https://github.com/lua/lua.git
    GIT_TAG v5.4.7
    GIT_SHALLOW TRUE)
  FetchContent_Declare(sol2
    GIT_REPOSITORY https://github.com/ThePhD/sol2.git
    GIT_TAG v3.3.0
    GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(lua_src sol2)

  file(GLOB LUA_C_SOURCES CONFIGURE_DEPENDS "${lua_src_SOURCE_DIR}/*.c")
  list(REMOVE_ITEM LUA_C_SOURCES
    "${lua_src_SOURCE_DIR}/lua.c"
    "${lua_src_SOURCE_DIR}/luac.c"
    "${lua_src_SOURCE_DIR}/onelua.c")
  add_library(lua STATIC ${LUA_C_SOURCES})
  target_include_directories(lua PUBLIC "${lua_src_SOURCE_DIR}")
  set_target_properties(lua PROPERTIES POSITION_INDEPENDENT_CODE ON)

  # sol2's own CMakeLists defines the header-only `sol2` interface target
  # (single/include). We only need to add our lua build.
  if(NOT TARGET sol2)
    add_library(sol2 INTERFACE)
    target_include_directories(sol2 SYSTEM INTERFACE "${sol2_SOURCE_DIR}/single/include")
  endif()
  target_compile_features(sol2 INTERFACE cxx_std_17)
endif()

# ---------------------------------------------------------------------------
# libcurl (optional; static for the installer)
# ---------------------------------------------------------------------------
if(NEON_USE_CURL)
  find_package(CURL QUIET)
  if(NOT CURL_FOUND)
    set(_neon_saved_build_shared_libs "${BUILD_SHARED_LIBS}")
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(BUILD_CURL_EXE OFF CACHE BOOL "" FORCE)
    set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
    set(CURL_DISABLE_LDAP ON CACHE BOOL "" FORCE)
    set(CURL_DISABLE_LDAPS ON CACHE BOOL "" FORCE)
    if(WIN32)
      # Windows: SChannel gives HTTPS without external TLS deps (installer stays small)
      set(CURL_USE_SCHANNEL ON CACHE BOOL "" FORCE)
      set(CURL_USE_OPENSSL OFF CACHE BOOL "" FORCE)
    else()
      # Non-Windows fallback: no TLS backend needed for local/mock testing
      # (system libcurl is preferred via find_package above; on Windows the
      # installer uses SChannel for HTTPS release downloads).
      set(CURL_USE_SCHANNEL OFF CACHE BOOL "" FORCE)
      set(CURL_USE_OPENSSL OFF CACHE BOOL "" FORCE)
      set(CURL_USE_GNUTLS OFF CACHE BOOL "" FORCE)
      set(CURL_USE_MBEDTLS OFF CACHE BOOL "" FORCE)
      set(CURL_USE_WOLFSSL OFF CACHE BOOL "" FORCE)
    endif()
    set(CURL_DISABLE_BASIC_AUTH OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(curl
      GIT_REPOSITORY https://github.com/curl/curl.git
      GIT_TAG curl-8_10_1
      GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(curl)
    if(_neon_saved_build_shared_libs STREQUAL "")
      unset(BUILD_SHARED_LIBS CACHE)
    else()
      set(BUILD_SHARED_LIBS "${_neon_saved_build_shared_libs}" CACHE BOOL "" FORCE)
    endif()
  endif()
endif()

# ---------------------------------------------------------------------------
# miniz (zip extraction for the installer)
# ---------------------------------------------------------------------------
set(_neon_saved_build_shared_libs "${BUILD_SHARED_LIBS}")
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(MZ_BUILD_SHARED OFF CACHE BOOL "" FORCE)
FetchContent_Declare(miniz
  GIT_REPOSITORY https://github.com/richgel999/miniz.git
  GIT_TAG 3.0.2
  GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(miniz)
if(_neon_saved_build_shared_libs STREQUAL "")
  unset(BUILD_SHARED_LIBS CACHE)
else()
  set(BUILD_SHARED_LIBS "${_neon_saved_build_shared_libs}" CACHE BOOL "" FORCE)
endif()

# ---------------------------------------------------------------------------
# SDL2 + satellites + Dear ImGui (GUI only)
# ---------------------------------------------------------------------------
if(NEON_BUILD_GUI)
  set(SDL_SHARED ON CACHE BOOL "" FORCE)
  set(SDL_STATIC OFF CACHE BOOL "" FORCE)
  set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
  set(SDL2_DISABLE_INSTALL OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(SDL2
    GIT_SUBMODULES ""
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-2.30.9
    GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(SDL2)

  set(SDL2IMAGE_VENDORED ON CACHE BOOL "" FORCE)
  set(SDL2IMAGE_SAMPLES OFF CACHE BOOL "" FORCE)
  set(SDL2IMAGE_TESTS OFF CACHE BOOL "" FORCE)
  set(SDL2IMAGE_INSTALL OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(SDL2_image
    GIT_SUBMODULES ""
    GIT_REPOSITORY https://github.com/libsdl-org/SDL_image.git
    GIT_TAG release-2.8.2
    GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(SDL2_image)

  set(SDL2TTF_VENDORED ON CACHE BOOL "" FORCE)
  set(SDL2TTF_SAMPLES OFF CACHE BOOL "" FORCE)
  set(SDL2TTF_INSTALL OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(SDL2_ttf
    GIT_SUBMODULES "external/freetype"
    GIT_REPOSITORY https://github.com/libsdl-org/SDL_ttf.git
    GIT_TAG release-2.22.0
    GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(SDL2_ttf)

  set(SDL2MIXER_VENDORED ON CACHE BOOL "" FORCE)
  set(SDL2MIXER_DEPS_SHARED OFF CACHE BOOL "" FORCE) # static-link vendored deps into the DLL
  set(SDL2MIXER_SAMPLES OFF CACHE BOOL "" FORCE)
  set(SDL2MIXER_INSTALL OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(SDL2_mixer
    GIT_REPOSITORY https://github.com/libsdl-org/SDL_mixer.git
    GIT_TAG release-2.8.0
    GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(SDL2_mixer)

  FetchContent_Declare(imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG docking
    GIT_SHALLOW FALSE)
  FetchContent_GetProperties(imgui)
  if(NOT imgui_POPULATED)
    FetchContent_Populate(imgui)
  endif()
  add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer2.cpp)
  target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
  target_link_libraries(imgui PUBLIC SDL2::SDL2)
  set_target_properties(imgui PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()

# ---------------------------------------------------------------------------
# Icon generation (deterministic, via Pillow) if missing
# ---------------------------------------------------------------------------
find_package(Python3 COMPONENTS Interpreter QUIET)
if(Python3_Interpreter AND NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets/icon/icon.ico")
  add_custom_command(
    OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/assets/icon/icon.ico"
           "${CMAKE_CURRENT_SOURCE_DIR}/assets/icon/icon.png"
    COMMAND ${Python3_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/scripts/generate_icon.py"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "Generating cyberpunk icon with scripts/generate_icon.py"
    VERBATIM)
  add_custom_target(neon_generate_icon DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/icon/icon.ico"
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/icon/icon.png")
else()
  add_custom_target(neon_generate_icon)
endif()

# ---------------------------------------------------------------------------
# Core game logic library (no SDL / no ImGui -> unit-testable anywhere)
# ---------------------------------------------------------------------------
add_library(neongrid_core STATIC
  src/core/DataCatalog.cpp
  src/core/EventBus.cpp
  src/core/Game.cpp
  src/core/GameState.cpp
  src/core/LuaEngine.cpp
  src/core/SaveLoad.cpp
  src/core/TimeSystem.cpp
  src/hardware/HardwareComponent.cpp
  src/hardware/HardwareFactory.cpp
  src/hardware/Rack.cpp
  src/hardware/Server.cpp
  src/hardware/ServerRoom.cpp
  src/netrunning/Cyberdeck.cpp
  src/netrunning/GridMap.cpp
  src/netrunning/ICE.cpp
  src/netrunning/NetrunSession.cpp
  src/netrunning/Program.cpp
  src/economy/Client.cpp
  src/economy/Contract.cpp
  src/economy/ContractManager.cpp
  src/economy/Faction.cpp
  src/economy/Market.cpp
  src/techtree/Effect.cpp
  src/techtree/TechNode.cpp
  src/techtree/TechTree.cpp
  src/net/HttpClient.cpp
  src/net/VersionChecker.cpp
  src/utils/FileSystem.cpp
  src/utils/Logger.cpp
  src/utils/Random.cpp
  src/utils/StringUtils.cpp
  src/utils/WindowsShortcut.cpp
)
target_include_directories(neongrid_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(neongrid_core PUBLIC nlohmann_json::nlohmann_json spdlog::spdlog_header_only)
target_compile_options(neongrid_core PRIVATE ${NEON_WARNING_FLAGS})
if(NEON_USE_LUA)
  target_compile_definitions(neongrid_core PUBLIC NEON_USE_LUA=1)
  target_link_libraries(neongrid_core PUBLIC sol2 lua)
endif()
if(NEON_USE_CURL)
  target_compile_definitions(neongrid_core PUBLIC NEON_USE_CURL=1)
  if(TARGET CURL::libcurl)
    target_link_libraries(neongrid_core PUBLIC CURL::libcurl)
  endif()
endif()
set_target_properties(neongrid_core PROPERTIES POSITION_INDEPENDENT_CODE ON)

# ---------------------------------------------------------------------------
# Game executable (SDL2 + Dear ImGui UI)
# ---------------------------------------------------------------------------
if(NEON_BUILD_GUI)
  add_executable(NeonGrid
    src/main.cpp
    src/ui/UITheme.cpp
    src/ui/MainMenu.cpp
    src/ui/ServerRoomView.cpp
    src/ui/TechTreeView.cpp
    src/ui/NetrunView.cpp
    src/ui/ContractView.cpp
    src/ui/MarketView.cpp
    src/ui/StatusPanel.cpp
    src/ui/LogWindow.cpp
  )
  if(WIN32)
    target_sources(NeonGrid PRIVATE src/resources.rc)
  endif()
  target_link_libraries(NeonGrid PRIVATE
    neongrid_core
    imgui
    SDL2::SDL2
    SDL2_image::SDL2_image
    SDL2_ttf::SDL2_ttf
    SDL2_mixer::SDL2_mixer)
  target_compile_options(NeonGrid PRIVATE ${NEON_WARNING_FLAGS})
  set_target_properties(NeonGrid PROPERTIES OUTPUT_NAME "NeonGrid")
  add_dependencies(NeonGrid neon_generate_icon)

  # Installation / packaging rules (used by CI to build game.zip)
  install(TARGETS NeonGrid RUNTIME DESTINATION .)
  install(FILES
    $<TARGET_FILE:SDL2::SDL2>
    $<TARGET_FILE:SDL2_image::SDL2_image>
    $<TARGET_FILE:SDL2_ttf::SDL2_ttf>
    $<TARGET_FILE:SDL2_mixer::SDL2_mixer>
    DESTINATION .)
  install(DIRECTORY data/ DESTINATION data FILES_MATCHING PATTERN "*.json")
  install(DIRECTORY assets/ DESTINATION assets)
  install(DIRECTORY scripts/lua/ DESTINATION scripts/lua FILES_MATCHING PATTERN "*.lua")
  install(FILES README.md DESTINATION . RENAME README.txt OPTIONAL)
endif()

# ---------------------------------------------------------------------------
# Bootstrap installer
# ---------------------------------------------------------------------------
if(NEON_BUILD_INSTALLER)
  add_subdirectory(installer)
endif()

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------
if(NEON_BUILD_TESTS)
  enable_testing()
  FetchContent_Declare(Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.7.1
    GIT_SHALLOW TRUE)
  set(CATCH_INSTALL_DOCS OFF CACHE BOOL "" FORCE)
  set(CATCH_INSTALL_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(CATCH_BUILD_TESTING OFF CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(Catch2)
  list(APPEND CMAKE_MODULE_PATH "${catch2_SOURCE_DIR}/extras")
  include(CTest)
  include(Catch)
  add_subdirectory(tests)
endif()
