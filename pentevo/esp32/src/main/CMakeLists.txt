file(GLOB_RECURSE SOURCES "../../src/*.c" "../../src/*.cpp")

idf_component_register(SRCS ${SOURCES} INCLUDE_DIRS ".")

idf_build_set_property(COMPILE_OPTIONS "-ggdb;-g3" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-Wno-missing-field-initializers" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-Wno-implicit-fallthrough" APPEND)

# target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-error=format)
# target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-error)
# target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-unused-but-set-variable)
# target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-unused-function)
