cmake_minimum_required(VERSION 3.5)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
set(EXTRA_COMPONENT_DIRS
  "${CMAKE_CURRENT_LIST_DIR}/main"
)
project(esp-spi)

idf_build_get_property(APP_ELF EXECUTABLE)

add_custom_command(
  TARGET ${APP_ELF}
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E chdir ${CMAKE_CURRENT_LIST_DIR}
          bash ver.sh main/main.h
  VERBATIM
)

set(app_elf     "${PROJECT_BINARY_DIR}/${PROJECT_NAME}.elf")
set(app_disasm  "${PROJECT_BINARY_DIR}/${PROJECT_NAME}.s")

add_custom_command(
  OUTPUT "${app_disasm}"
  COMMAND "${CMAKE_OBJDUMP}" -S "${app_elf}" > "${app_disasm}"
  DEPENDS "${app_elf}"
  VERBATIM
)

add_custom_target(disasm ALL DEPENDS "${app_disasm}")
