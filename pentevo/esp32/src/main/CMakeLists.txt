idf_component_register(SRCS "main.cpp" INCLUDE_DIRS ".")
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-error)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-missing-field-initializers)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-unused-but-set-variable)
