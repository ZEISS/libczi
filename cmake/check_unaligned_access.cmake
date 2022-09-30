# SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
#
# SPDX-License-Identifier: LGPL-3.0-or-later

include(CheckCXXSourceRuns)

# Compile and run a small test application in order to check whether an int can be written and read to/from an
# unaligned address. On systems where this is not possible (e.g. Sparc) this will crash. The specified variable
# will be set to 'TRUE' if the program did not compile or crashed, and 'FALSE' otherwise.
macro (CHECK_UNALIGNED_ACCESS VARIABLE)

  IF (NOT DEFINED ${VARIABLE})

    set (_CHECK_UNALIGNED_PROG "
    #include <stdlib.h>
    #include <stdint.h>

    int main()
    {
            char* bufOrig = reinterpret_cast<char*>(malloc(sizeof(uint32_t)*2));
            char* buf = bufOrig;
            uint32_t* ptrInt = reinterpret_cast<uint32_t*>(buf);
            buf[0]=0x12;
            buf[1]=0x34;
            buf[2]=0x56;
            buf[3]=0x78;
            if (*ptrInt != 0x12345678 && *ptrInt != 0x78563412)
            {
                    free(bufOrig);
                    return 1;
            }

            buf = buf+1;
            ptrInt = reinterpret_cast<uint32_t*>(buf);
            buf[0]=0x12;
            buf[1]=0x34;
            buf[2]=0x56;
            buf[3]=0x78;
            if (*ptrInt != 0x12345678 && *ptrInt != 0x78563412)
            {
                    free(bufOrig);
                    return 1;
            }

            buf = buf+1;
            ptrInt = reinterpret_cast<uint32_t*>(buf);
            buf[0]=0x12;
            buf[1]=0x34;
            buf[2]=0x56;
            buf[3]=0x78;
            if (*ptrInt != 0x12345678 && *ptrInt != 0x78563412)
            {
                    free(bufOrig);
                    return 1;
            }

            buf = buf+1;
            ptrInt = reinterpret_cast<uint32_t*>(buf);
            buf[0]=0x12;
            buf[1]=0x34;
            buf[2]=0x56;
            buf[3]=0x78;
            if (*ptrInt != 0x12345678 && *ptrInt != 0x78563412)
            {
                    free(bufOrig);
                    return 1;
            }

            free(bufOrig);
            return 0;
    }")


    CHECK_CXX_SOURCE_RUNS("${_CHECK_UNALIGNED_PROG}" _UNALIGNED_ACCESS_RESULT)
    if (_UNALIGNED_ACCESS_RESULT EQUAL 1)
      set(${VARIABLE} FALSE CACHE INTERNAL "Result of test for 'CPU supports unaligned read/write'" FORCE)
    else()
      set(${VARIABLE} TRUE CACHE INTERNAL "Result of test for 'CPU supports unaligned read/write'" FORCE)
    endif()

  endif (NOT DEFINED ${VARIABLE})

endmacro (CHECK_UNALIGNED_ACCESS VARIABLE)