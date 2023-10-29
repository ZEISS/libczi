 
 # Set properties for the imported target
   set_target_properties(CURL::libcurl PROPERTIES
    IMPORTED_LOCATION "${CMAKE_BINARY_DIR}/curl/lib"
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_BINARY_DIR}/curl/include"
    )