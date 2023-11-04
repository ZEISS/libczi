add_executable(jpgxrdecodetest 
                main.cpp)

set_target_properties(jpgxrdecodetest  PROPERTIES CXX_STANDARD 11)
target_compile_definitions(jpgxrdecodetest  PRIVATE _LIBCZISTATICLIB)

target_link_libraries(jpgxrdecodetest PRIVATE libCZIStatic)