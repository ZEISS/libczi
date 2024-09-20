 
if(NOT EXISTS "/d/dev/Github/libczi-zeiss-ptahmose/buildmsys2/msys/install_manifest.txt")
  message(FATAL_ERROR "Cannot find install manifest: /d/dev/Github/libczi-zeiss-ptahmose/buildmsys2/msys/install_manifest.txt")
endif()

file(READ "/d/dev/Github/libczi-zeiss-ptahmose/buildmsys2/msys/install_manifest.txt" files)
string(REGEX REPLACE "\n" ";" files "${files}")
foreach(file ${files})
  message(STATUS "Uninstalling $ENV{DESTDIR}${file}")
  if(IS_SYMLINK "$ENV{DESTDIR}${file}" OR EXISTS "$ENV{DESTDIR}${file}")
    exec_program(
      "/usr/bin/cmake.exe" ARGS "-E remove \"$ENV{DESTDIR}${file}\""
      OUTPUT_VARIABLE rm_out
      RETURN_VALUE rm_retval
      )
    if(NOT "${rm_retval}" STREQUAL 0)
      message(FATAL_ERROR "Problem when removing $ENV{DESTDIR}${file}")
    endif()
  else()
    message(STATUS "File $ENV{DESTDIR}${file} does not exist.")
  endif()
endforeach()
