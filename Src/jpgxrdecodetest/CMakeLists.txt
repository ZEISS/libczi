add_executable(jpgxrdecodetest 
                "main.cpp"
                "save_bitmap.cpp" 
                "save_bitmap.h")

set_target_properties(jpgxrdecodetest  PROPERTIES CXX_STANDARD 11)
target_compile_definitions(jpgxrdecodetest  PRIVATE _LIBCZISTATICLIB)

target_link_libraries(jpgxrdecodetest PRIVATE libCZIStatic)

 target_link_libraries(jpgxrdecodetest PRIVATE Windowscodecs)