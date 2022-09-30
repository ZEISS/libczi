# SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
#
# SPDX-License-Identifier: LGPL-3.0-or-later

include(CheckCXXSourceRuns)

# Compile a small test program in order to determine whether Neon-intrinsics can be used.
# The specified variable will be set to 'TRUE' if Neon-intrinsics are operational with the
# current compiler; and to 'FALSE' otherwise.
macro (CHECK_CAN_USE_NEON_INTRINSICS VARIABLE)

  IF (NOT DEFINED ${VARIABLE})

    set (_CHECK_NEON_INTRINSICS_PROG "
        #include <stdio.h>
        #include <assert.h>

        #if defined(_WIN32) && (defined(_M_ARM) || defined(_M_ARM64))
        # include <Intrin.h>
        # include <arm_neon.h>
        # define CV_NEON 1
        #elif defined(__ARM_NEON__) || defined(__ARM_FEATURE_SIMD32) || (defined (__ARM_NEON) && defined(__aarch64__))
        #  include <arm_neon.h>
        #  define CV_NEON 1
        #endif

        #if defined CV_NEON
        int test()
        {
            const float32_t src[] = { 0.0f, 0.0f, 0.0f, 0.0f };
            float32x4_t val = vld1q_f32(src);
            return static_cast<int>(vgetq_lane_f32(val, 0));
        }
        #else
        #error \"NEON is not supported\"
        #endif

        int main()
        {
            int r = test();
            assert(r == 0);
            printf(\"%d\", r);
            return 0;
        }
")

    CHECK_CXX_SOURCE_RUNS("${_CHECK_NEON_INTRINSICS_PROG}" _NEON_INTRINSICS_RESULT)
    if (_NEON_INTRINSICS_RESULT EQUAL 1)
      set(${VARIABLE} TRUE CACHE INTERNAL "Result of test for 'Neon intrinsices supported'" FORCE)
    else()
      set(${VARIABLE} FALSE CACHE INTERNAL "Result of test for 'Neon intrinsices supported'" FORCE)
    endif()

  endif (NOT DEFINED ${VARIABLE})

endmacro (CHECK_CAN_USE_NEON_INTRINSICS VARIABLE)