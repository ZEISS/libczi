# Install script for directory: /d/dev/Github/libczi-zeiss-ptahmose/buildmsys2/msys/vendor/eigen3/src/eigen_ext/unsupported/Eigen

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set path to fallback-tool for dependency-resolution.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/usr/bin/objdump.exe")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Devel" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/eigen3/unsupported/Eigen" TYPE FILE FILES
    "/d/dev/Github/libczi-zeiss-ptahmose/buildmsys2/msys/vendor/eigen3/src/eigen_ext/unsupported/Eigen/AdolcForward"
    "/d/dev/Github/libczi-zeiss-ptahmose/buildmsys2/msys/vendor/eigen3/src/eigen_ext/unsupported/Eigen/AlignedVector3"
    "/d/dev/Github/libczi-zeiss-ptahmose/buildmsys2/msys/vendor/eigen3/src/eigen_ext/unsupported/Eigen/ArpackSupport"
    "/d/dev/Github/libczi-zeiss-ptahmose/buildmsys2/msys/vendor/eigen3/src/eigen_ext/unsupported/Eigen/AutoDiff"
    "/d/dev/Github/libczi-zeiss-ptahmose/buildmsys2/msys/vendor/eigen3/src/eigen_ext/unsupported/Eigen/BVH"
    "/d/dev/Github/libczi-zeiss-ptahmose/buildmsys2/msys/vendor/eigen3/src/eigen_ext/unsupported/Eigen/EulerAngles"
    "/d/dev/Github/libczi-zeiss-ptahmose/buildmsys2/msys/vendor/eigen3/src/eigen_ext/unsupported/Eigen/FFT"
    "/d/dev/Github/libczi-zeiss-ptahmose/buildmsys2/msys/vendor/eigen3/src/eigen_ext/unsupported/Eigen/IterativeSolvers"
    "/d/dev/Github/libczi-zeiss-ptahmose/buildmsys2/msys/vendor/eigen3/src/eigen_ext/unsupported/Eigen/KroneckerProduct"
    "/d/dev/Github/libczi-zeiss-ptahmose/buildmsys2/msys/vendor/eigen3/src/eigen_ext/unsupported/Eigen/LevenbergMarquardt"
    "/d/dev/Github/libczi-zeiss-ptahmose/buildmsys2/msys/vendor/eigen3/src/eigen_ext/unsupported/Eigen/MatrixFunctions"
    "/d/dev/Github/libczi-zeiss-ptahmose/buildmsys2/msys/vendor/eigen3/src/eigen_ext/unsupported/Eigen/MoreVectorization"
    "/d/dev/Github/libczi-zeiss-ptahmose/buildmsys2/msys/vendor/eigen3/src/eigen_ext/unsupported/Eigen/MPRealSupport"
    "/d/dev/Github/libczi-zeiss-ptahmose/buildmsys2/msys/vendor/eigen3/src/eigen_ext/unsupported/Eigen/NonLinearOptimization"
    "/d/dev/Github/libczi-zeiss-ptahmose/buildmsys2/msys/vendor/eigen3/src/eigen_ext/unsupported/Eigen/NumericalDiff"
    "/d/dev/Github/libczi-zeiss-ptahmose/buildmsys2/msys/vendor/eigen3/src/eigen_ext/unsupported/Eigen/OpenGLSupport"
    "/d/dev/Github/libczi-zeiss-ptahmose/buildmsys2/msys/vendor/eigen3/src/eigen_ext/unsupported/Eigen/Polynomials"
    "/d/dev/Github/libczi-zeiss-ptahmose/buildmsys2/msys/vendor/eigen3/src/eigen_ext/unsupported/Eigen/Skyline"
    "/d/dev/Github/libczi-zeiss-ptahmose/buildmsys2/msys/vendor/eigen3/src/eigen_ext/unsupported/Eigen/SparseExtra"
    "/d/dev/Github/libczi-zeiss-ptahmose/buildmsys2/msys/vendor/eigen3/src/eigen_ext/unsupported/Eigen/SpecialFunctions"
    "/d/dev/Github/libczi-zeiss-ptahmose/buildmsys2/msys/vendor/eigen3/src/eigen_ext/unsupported/Eigen/Splines"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Devel" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/eigen3/unsupported/Eigen" TYPE DIRECTORY FILES "/d/dev/Github/libczi-zeiss-ptahmose/buildmsys2/msys/vendor/eigen3/src/eigen_ext/unsupported/Eigen/src" FILES_MATCHING REGEX "/[^/]*\\.h$")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/d/dev/Github/libczi-zeiss-ptahmose/buildmsys2/msys/vendor/eigen3/src/eigen_ext-build/unsupported/Eigen/CXX11/cmake_install.cmake")

endif()

