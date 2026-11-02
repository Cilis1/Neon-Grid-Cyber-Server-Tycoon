# Bootstrap installer: CyberServerTycoonSetup.exe
# Downloads game.zip from the latest GitHub release, verifies SHA-256, extracts
# to %LOCALAPPDATA%\NeonGrid\game, creates a desktop shortcut and launches.

add_library(installer_util STATIC
  sha256.cpp
  unzipper.cpp
  downloader.cpp
)
target_include_directories(installer_util PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(installer_util PUBLIC neongrid_core miniz)
if(TARGET CURL::libcurl)
  target_link_libraries(installer_util PUBLIC CURL::libcurl)
endif()
target_compile_options(installer_util PRIVATE ${NEON_WARNING_FLAGS})
set_target_properties(installer_util PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_executable(CyberServerTycoonSetup bootstrap.cpp)
if(WIN32)
  target_sources(CyberServerTycoonSetup PRIVATE resources.rc)
endif()
target_link_libraries(CyberServerTycoonSetup PRIVATE installer_util)
target_compile_options(CyberServerTycoonSetup PRIVATE ${NEON_WARNING_FLAGS})
set_target_properties(CyberServerTycoonSetup PROPERTIES OUTPUT_NAME "CyberServerTycoonSetup")
if(WIN32)
  # GUI subsystem: double-clicking shows message boxes, not a console window.
  set_target_properties(CyberServerTycoonSetup PROPERTIES WIN32_EXECUTABLE TRUE)
endif()
add_dependencies(CyberServerTycoonSetup neon_generate_icon)

if(WIN32)
  # Static-ish runtime for the bootstrapper: keep it small and dependency-free
  # except for system DLLs. Link ole32/uuid for COM shortcut creation.
  target_link_libraries(CyberServerTycoonSetup PRIVATE ole32 uuid shell32)
endif()

install(TARGETS CyberServerTycoonSetup RUNTIME DESTINATION installer)
