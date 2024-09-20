cmake_minimum_required(VERSION 3.30.1)
set(CMAKE_MODULE_PATH "/d/dev/Github/libczi-zeiss-ptahmose/buildmsys2/msys-mingw/vendor/eigen3/src/eigen_ext/cmake")
project(CheckFortran Fortran)
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/result.cmake"
  "set(CMAKE_Fortran_COMPILER \"${CMAKE_Fortran_COMPILER}\")\n"
  
  )