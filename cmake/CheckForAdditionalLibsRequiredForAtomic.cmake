# SPDX-FileCopyrightText: 2023 Carl Zeiss Microscopy GmbH
#
# SPDX-License-Identifier: MIT

# On some platforms it seems necessary to link against additional libraries ('atomic' in particular) when using C++'s 'std::atomic'.
# E.g. on Risc-V this seems to be the case currently. This function checks whether this is necessary and sets the variable 'ADDITIONAL_LIBS_TO_LINK' accordingly.
# This variable can then be used in the 'target_link_libraries' command.
# C.f. https://github.com/advancedtelematic/aktualizr/issues/1427

include(CheckCXXSourceRuns)

function(CheckForAdditionalLibsRequiredForAtomic VARIABLE)

  if (NOT DEFINED ${ADDITIONAL_LIBS_TO_LINK})

     # define a small test program which uses 'std::atomic' with a couple of types. It seems, with the Risc-V case, 'uchar' is the only type which fails.
     set (_CHECK_ATOMIC_PROG "
     #include <cstdint>
     #include <atomic>

     int main()
     {
       std::atomic_bool atomic_bool{false};
       std::atomic<std::uint32_t> atomic_uint32{0};
       std::atomic<std::uint64_t> atomic_uint64{0};
       std::atomic<unsigned char> atomic_uchar{0};

       atomic_bool.store(true);
       ++atomic_uint32;
       ++atomic_uint64;
       ++atomic_uchar;

       bool is_correct = atomic_bool.load() == true && atomic_uint32.load() == 1 && atomic_uint64.load() == 1 && atomic_uchar.load() == 1;

       return is_correct ? 0 : 1;
     }")

     CHECK_CXX_SOURCE_RUNS("${_CHECK_ATOMIC_PROG}" _ATOMIC_TEST_RESULT)

     if (_ATOMIC_TEST_RESULT EQUAL 1)
        # if this compile succeeds, we don't need to link against any additional libraries
        set(${VARIABLE} "" CACHE INTERNAL "Additional libraries to link for using 'atomic'" FORCE)
     else()
        # if this compile fails, we try linking against 'atomic' and check again. Note that we need to unset the '_ATOMIC_TEST_RESULT' variable 
        # before the check, otherwise the check will not run again. Also, we need to use the 'CMAKE_REQUIRED_LIBRARIES' variable in order
        # to pass the 'atomic' library to the linker. We restore the original value of 'CMAKE_REQUIRED_LIBRARIES' after the check.
        set(BEFORE_CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES})
        set(CMAKE_REQUIRED_LIBRARIES "${CMAKE_REQUIRED_LIBRARIES};atomic")

        unset(_ATOMIC_TEST_RESULT CACHE)        
        CHECK_CXX_SOURCE_RUNS("${_CHECK_ATOMIC_PROG}" _ATOMIC_TEST_RESULT)
        
        # Unset the libraries after the check
        set(CMAKE_REQUIRED_LIBRARIES ${BEFORE_CMAKE_REQUIRED_LIBRARIES})

        if (_ATOMIC_TEST_RESULT EQUAL 1)
          set(${VARIABLE} "atomic" CACHE INTERNAL "Additional libraries to link for using 'atomic'" FORCE)
        else()
          message(FATAL_ERROR "Unable to compile code which uses 'atomic'.")
        endif()
     endif()
  endif (NOT DEFINED ${ADDITIONAL_LIBS_TO_LINK})

endfunction(CheckForAdditionalLibsRequiredForAtomic VARIABLE)