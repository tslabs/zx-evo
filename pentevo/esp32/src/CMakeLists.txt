cmake_minimum_required(VERSION 3.5)

set(APP_VERSION_STAMP "${CMAKE_BINARY_DIR}/app_version_bump.stamp" CACHE INTERNAL "")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

set(EXTRA_COMPONENT_DIRS
  "${CMAKE_CURRENT_LIST_DIR}/main"
)

project(esp-spi)

idf_build_get_property(APP_ELF EXECUTABLE)

file(GLOB_RECURSE APP_VERSION_INPUTS
  "${CMAKE_CURRENT_LIST_DIR}/main/*.c"
  "${CMAKE_CURRENT_LIST_DIR}/main/*.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/main/*.h"
  "${CMAKE_CURRENT_LIST_DIR}/ver.sh"
)

add_custom_command(
  OUTPUT "${APP_VERSION_STAMP}"
  COMMAND ${CMAKE_COMMAND} -E chdir "${CMAKE_CURRENT_LIST_DIR}"
          bash ver.sh main/main.h
  COMMAND ${CMAKE_COMMAND} -E touch "${APP_VERSION_STAMP}"
  DEPENDS ${APP_VERSION_INPUTS}
  VERBATIM
)

add_custom_target(app_version_bump DEPENDS "${APP_VERSION_STAMP}")

add_dependencies(${APP_ELF} app_version_bump)

set(app_elf     "${PROJECT_BINARY_DIR}/${PROJECT_NAME}.elf")
set(app_disasm  "${PROJECT_BINARY_DIR}/${PROJECT_NAME}.s")

add_custom_command(
  OUTPUT "${app_disasm}"
  COMMAND "${CMAKE_OBJDUMP}" -S "${app_elf}" > "${app_disasm}"
  DEPENDS "${app_elf}"
  VERBATIM
)

add_custom_target(disasm ALL DEPENDS "${app_disasm}")
