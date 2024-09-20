# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

if(EXISTS "/d/dev/Github/libczi-zeiss-ptahmose/buildmsys2/msys/vendor/eigen3/src/eigen_ext-stamp/eigen_ext-gitclone-lastrun.txt" AND EXISTS "/d/dev/Github/libczi-zeiss-ptahmose/buildmsys2/msys/vendor/eigen3/src/eigen_ext-stamp/eigen_ext-gitinfo.txt" AND
  "/d/dev/Github/libczi-zeiss-ptahmose/buildmsys2/msys/vendor/eigen3/src/eigen_ext-stamp/eigen_ext-gitclone-lastrun.txt" IS_NEWER_THAN "/d/dev/Github/libczi-zeiss-ptahmose/buildmsys2/msys/vendor/eigen3/src/eigen_ext-stamp/eigen_ext-gitinfo.txt")
  message(VERBOSE
    "Avoiding repeated git clone, stamp file is up to date: "
    "'/d/dev/Github/libczi-zeiss-ptahmose/buildmsys2/msys/vendor/eigen3/src/eigen_ext-stamp/eigen_ext-gitclone-lastrun.txt'"
  )
  return()
endif()

# Even at VERBOSE level, we don't want to see the commands executed, but
# enabling them to be shown for DEBUG may be useful to help diagnose problems.
cmake_language(GET_MESSAGE_LOG_LEVEL active_log_level)
if(active_log_level MATCHES "DEBUG|TRACE")
  set(maybe_show_command COMMAND_ECHO STDOUT)
else()
  set(maybe_show_command "")
endif()

execute_process(
  COMMAND ${CMAKE_COMMAND} -E rm -rf "/d/dev/Github/libczi-zeiss-ptahmose/buildmsys2/msys/vendor/eigen3/src/eigen_ext"
  RESULT_VARIABLE error_code
  ${maybe_show_command}
)
if(error_code)
  message(FATAL_ERROR "Failed to remove directory: '/d/dev/Github/libczi-zeiss-ptahmose/buildmsys2/msys/vendor/eigen3/src/eigen_ext'")
endif()

# try the clone 3 times in case there is an odd git clone issue
set(error_code 1)
set(number_of_tries 0)
while(error_code AND number_of_tries LESS 3)
  execute_process(
    COMMAND "/usr/bin/git.exe"
            clone --no-checkout --config "advice.detachedHead=false" "https://gitlab.com/libeigen/eigen.git" "eigen_ext"
    WORKING_DIRECTORY "/d/dev/Github/libczi-zeiss-ptahmose/buildmsys2/msys/vendor/eigen3/src"
    RESULT_VARIABLE error_code
    ${maybe_show_command}
  )
  math(EXPR number_of_tries "${number_of_tries} + 1")
endwhile()
if(number_of_tries GREATER 1)
  message(NOTICE "Had to git clone more than once: ${number_of_tries} times.")
endif()
if(error_code)
  message(FATAL_ERROR "Failed to clone repository: 'https://gitlab.com/libeigen/eigen.git'")
endif()

execute_process(
  COMMAND "/usr/bin/git.exe"
          checkout "3147391d946bb4b6c68edd901f2add6ac1f31f8c" --
  WORKING_DIRECTORY "/d/dev/Github/libczi-zeiss-ptahmose/buildmsys2/msys/vendor/eigen3/src/eigen_ext"
  RESULT_VARIABLE error_code
  ${maybe_show_command}
)
if(error_code)
  message(FATAL_ERROR "Failed to checkout tag: '3147391d946bb4b6c68edd901f2add6ac1f31f8c'")
endif()

set(init_submodules TRUE)
if(init_submodules)
  execute_process(
    COMMAND "/usr/bin/git.exe" 
            submodule update --recursive --init 
    WORKING_DIRECTORY "/d/dev/Github/libczi-zeiss-ptahmose/buildmsys2/msys/vendor/eigen3/src/eigen_ext"
    RESULT_VARIABLE error_code
    ${maybe_show_command}
  )
endif()
if(error_code)
  message(FATAL_ERROR "Failed to update submodules in: '/d/dev/Github/libczi-zeiss-ptahmose/buildmsys2/msys/vendor/eigen3/src/eigen_ext'")
endif()

# Complete success, update the script-last-run stamp file:
#
execute_process(
  COMMAND ${CMAKE_COMMAND} -E copy "/d/dev/Github/libczi-zeiss-ptahmose/buildmsys2/msys/vendor/eigen3/src/eigen_ext-stamp/eigen_ext-gitinfo.txt" "/d/dev/Github/libczi-zeiss-ptahmose/buildmsys2/msys/vendor/eigen3/src/eigen_ext-stamp/eigen_ext-gitclone-lastrun.txt"
  RESULT_VARIABLE error_code
  ${maybe_show_command}
)
if(error_code)
  message(FATAL_ERROR "Failed to copy script-last-run stamp file: '/d/dev/Github/libczi-zeiss-ptahmose/buildmsys2/msys/vendor/eigen3/src/eigen_ext-stamp/eigen_ext-gitclone-lastrun.txt'")
endif()
