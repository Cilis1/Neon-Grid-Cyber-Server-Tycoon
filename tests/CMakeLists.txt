set(NEON_TEST_SOURCES
  test_tech_tree.cpp
  test_economy.cpp
  test_hardware.cpp
  test_netrunning.cpp
  test_save_load.cpp
  test_integration.cpp
)

foreach(test_src ${NEON_TEST_SOURCES})
  get_filename_component(test_name ${test_src} NAME_WE)
  add_executable(${test_name} ${test_src})
  target_link_libraries(${test_name} PRIVATE neongrid_core Catch2::Catch2WithMain)
  target_compile_definitions(${test_name} PRIVATE NEON_TEST_DATA_DIR="${CMAKE_SOURCE_DIR}/data")
  target_compile_options(${test_name} PRIVATE ${NEON_WARNING_FLAGS})
  catch_discover_tests(${test_name} WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
endforeach()

# Installer utility tests (sha256 + unzip round-trip) if the installer is built
if(TARGET installer_util)
  add_executable(test_installer test_installer.cpp)
  target_link_libraries(test_installer PRIVATE installer_util neongrid_core Catch2::Catch2WithMain)
  target_compile_options(test_installer PRIVATE ${NEON_WARNING_FLAGS})
  catch_discover_tests(test_installer WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
endif()
