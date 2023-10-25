
add_library(libCZIStreamsLib STATIC 
  "include/streamsFactory.h"
  "src/streamsFactory.cpp" 
  "src/curlhttpinputstream.cpp"
  "src/curlhttpinputstream.h" "include/include_libczi.h")

target_link_libraries(libCZIStreamsLib PRIVATE CURL::libcurl libCZIStatic)
