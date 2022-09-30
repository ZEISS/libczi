// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "stdafx.h"
#include <stdexcept>
#include <cstdint>
#include "inc_libCZI_Config.h"
#include "utilities.h"

using namespace std;

#if LIBCZI_HAS_AVXINTRINSICS

// Note: On x86/x64 (and GCC/Clang) this module is compiled with the switch "-mavx2", which means that AVX may be used
//        for code-generation (aside from intrinsics). We employ the model of "runtime detection of AVX-capabilities" here,
//        so there must not be any AVX-code in any execution path. So, we must be careful that no code in this part is executed
//        without a prior runtime detection of AVX-capabilities.

#if defined(_MSC_VER)
# include <intrin.h>
#endif

#include <immintrin.h>

// from https://software.intel.com/content/www/us/en/develop/articles/how-to-detect-new-instruction-support-in-the-4th-generation-intel-core-processor-family.html
static void run_cpuid(uint32_t eax, uint32_t ecx, uint32_t* abcd)
{
#if defined(_MSC_VER)
    __cpuidex(reinterpret_cast<int*>(abcd), eax, ecx);
#else
    uint32_t ebx, edx;
# if defined( __i386__ ) && defined ( __PIC__ )
    /* in case of PIC under 32-bit EBX cannot be clobbered */
    __asm__("movl %%ebx, %%edi \n\t cpuid \n\t xchgl %%ebx, %%edi" : "=D" (ebx),
# else
    __asm__("cpuid" : "+b" (ebx),
# endif
        "+a" (eax), "+c" (ecx), "=d" (edx));
    abcd[0] = eax; abcd[1] = ebx; abcd[2] = ecx; abcd[3] = edx;
#endif
}

static int check_xcr0_ymm()
{
    uint32_t xcr0;
#if defined(_MSC_VER)
    xcr0 = static_cast<uint32_t>(_xgetbv(0));  /* min VS2010 SP1 compiler is required */
#else
    __asm__("xgetbv" : "=a" (xcr0) : "c" (0) : "%edx");
#endif
    return ((xcr0 & 6) == 6); /* checking if xmm and ymm state are enabled in XCR0 */
}


static int check_4th_gen_intel_core_features()
{
    uint32_t abcd[4];
    constexpr uint32_t fma_movbe_osxsave_mask = ((1 << 12) | (1 << 22) | (1 << 27));
    constexpr uint32_t avx2_bmi12_mask = (1 << 5) | (1 << 3) | (1 << 8);

    /* CPUID.(EAX=01H, ECX=0H):ECX.FMA[bit 12]==1   &&
       CPUID.(EAX=01H, ECX=0H):ECX.MOVBE[bit 22]==1 &&
       CPUID.(EAX=01H, ECX=0H):ECX.OSXSAVE[bit 27]==1 */
    run_cpuid(1, 0, abcd);
    if ((abcd[2] & fma_movbe_osxsave_mask) != fma_movbe_osxsave_mask)
        return 0;

    if (!check_xcr0_ymm())
        return 0;

    /*  CPUID.(EAX=07H, ECX=0H):EBX.AVX2[bit 5]==1  &&
        CPUID.(EAX=07H, ECX=0H):EBX.BMI1[bit 3]==1  &&
        CPUID.(EAX=07H, ECX=0H):EBX.BMI2[bit 8]==1  */
    run_cpuid(7, 0, abcd);
    if ((abcd[1] & avx2_bmi12_mask) != avx2_bmi12_mask)
        return 0;

    /* CPUID.(EAX=80000001H):ECX.LZCNT[bit 5]==1 */
    run_cpuid(0x80000001, 0, abcd);
    if ((abcd[2] & (1 << 5)) == 0)
        return 0;

    return 1;
}

static bool CheckWhetherCpuSupportsAVX2()
{
    static int avx2Supported = -1;

    if (avx2Supported == 0)
    {
        return false;
    }
    else if (avx2Supported == 1)
    {
        return true;
    }
    else
    {
        avx2Supported = (check_4th_gen_intel_core_features() > 0) ? 1 : 0;
        return (avx2Supported == 1);
    }
}

class LoHiBytePackUnpackAvx : public LoHiBytePackUnpack
{
public:
    typedef void(*pfnLoHiByteUnpackStrided_t)(const void*, std::uint32_t, std::uint32_t, std::uint32_t, void*);
    typedef void(*pfnLoHiBytePackStrided_t)(const void*, size_t, std::uint32_t, std::uint32_t, std::uint32_t, void*);

    static pfnLoHiByteUnpackStrided_t pfnLoHiByteUnpackStrided;
    static pfnLoHiBytePackStrided_t pfnLoHiBytePackStrided;

    static void LoHiByteUnpackStrided_Choose(const void* ptrSrc, std::uint32_t wordCount, std::uint32_t stride, std::uint32_t lineCount, void* ptrDst);
    static void LoHiBytePackStrided_Choose(const void* ptrSrc, size_t sizeSrc, std::uint32_t width, std::uint32_t height, std::uint32_t stride, void* dest);

    static void LoHiByteUnpackStrided_AVX(const void* source, std::uint32_t wordCount, std::uint32_t stride, std::uint32_t lineCount, void* dest);
    static void LoHiBytePackStrided_AVX(const void* ptrSrc, size_t sizeSrc, std::uint32_t width, std::uint32_t height, std::uint32_t stride, void* dest);
};

LoHiBytePackUnpackAvx::pfnLoHiByteUnpackStrided_t LoHiBytePackUnpackAvx::pfnLoHiByteUnpackStrided = &LoHiBytePackUnpackAvx::LoHiByteUnpackStrided_Choose;
LoHiBytePackUnpackAvx::pfnLoHiBytePackStrided_t LoHiBytePackUnpackAvx::pfnLoHiBytePackStrided = &LoHiBytePackUnpackAvx::LoHiBytePackStrided_Choose;

/*static*/void LoHiBytePackUnpack::LoHiByteUnpackStrided(const void* ptrSrc, std::uint32_t wordCount, std::uint32_t stride, std::uint32_t lineCount, void* ptrDst)
{
    LoHiBytePackUnpack::CheckLoHiByteUnpackArgumentsAndThrow(wordCount, stride, ptrSrc, ptrDst);
    (*LoHiBytePackUnpackAvx::pfnLoHiByteUnpackStrided)(ptrSrc, wordCount, stride, lineCount, ptrDst);
}

/*static*/void LoHiBytePackUnpack::LoHiBytePackStrided(const void* ptrSrc, size_t sizeSrc, std::uint32_t width, std::uint32_t height, std::uint32_t stride, void* dest)
{
    LoHiBytePackUnpack::CheckLoHiBytePackArgumentsAndThrow(ptrSrc, sizeSrc, width, height, stride, dest);
    (*LoHiBytePackUnpackAvx::pfnLoHiBytePackStrided)(ptrSrc, sizeSrc, width, height, stride, dest);
}

/*static*/void LoHiBytePackUnpackAvx::LoHiByteUnpackStrided_AVX(const void* source, std::uint32_t wordCount, std::uint32_t stride, std::uint32_t lineCount, void* dest)
{
    static const __m128i shuffleConst128 = _mm_set_epi8(
        0xf, 0xd, 0xb, 0x9,
        0x7, 0x5, 0x3, 0x1,
        0xe, 0xc, 0xa, 0x8,
        0x6, 0x4, 0x2, 0x0);

    uint8_t* pDst = static_cast<uint8_t*>(dest);
    const size_t halfLength = (static_cast<size_t>(wordCount) * 2 * lineCount) / 2;
    const uint32_t widthOver16 = wordCount / 16;
    const uint32_t widthModulo16 = wordCount % 16;
    const __m256i shuffleConst = _mm256_broadcastsi128_si256(shuffleConst128);

    for (uint32_t y = 0; y < lineCount; ++y)
    {
        const uint16_t* pSrc = reinterpret_cast<const uint16_t*>(static_cast<const uint8_t*>(source) + y * static_cast<size_t>(stride));

        for (uint32_t i = 0; i < widthOver16; ++i)
        {
            const __m256i d = _mm256_lddqu_si256((const __m256i*)pSrc);
            const __m256i shuffled = _mm256_permute4x64_epi64(_mm256_shuffle_epi8(d, shuffleConst), 0xd8);
            _mm256_storeu2_m128i(reinterpret_cast<__m128i*>(pDst + halfLength), reinterpret_cast<__m128i*>(pDst), shuffled);

            pSrc += 16;  // we do 16 words = 32 bytes per loop
            pDst += 16;
        }

        for (uint32_t i = 0; i < widthModulo16; ++i)
        {
            const uint16_t v = *pSrc++;
            *pDst = static_cast<uint8_t>(v);
            *(pDst + halfLength) = static_cast<uint8_t>(v >> 8);
            pDst++;
        }
    }

    _mm256_zeroall();
}

/*static*/void LoHiBytePackUnpackAvx::LoHiBytePackStrided_AVX(const void* ptrSrc, size_t sizeSrc, std::uint32_t width, std::uint32_t height, std::uint32_t stride, void* dest)
{
    const uint8_t* pSrc = static_cast<const uint8_t*>(ptrSrc);

    const size_t halfLength = sizeSrc / 2;
    const uint32_t widthOver16 = width / 16;
    const uint32_t widthRemainder = width % 16;

    for (uint32_t y = 0; y < height; ++y)
    {
        __m256i* pDst = reinterpret_cast<__m256i*>(static_cast<uint8_t*>(dest) + static_cast<size_t>(y) * stride);

        for (uint32_t x = 0; x < widthOver16; ++x)
        {
            const __m256i a = _mm256_permute4x64_epi64(_mm256_castsi128_si256(_mm_lddqu_si128(reinterpret_cast<const __m128i*>(pSrc))), 0x50);
            const __m256i b = _mm256_permute4x64_epi64(_mm256_castsi128_si256(_mm_lddqu_si128(reinterpret_cast<const __m128i*>(pSrc + halfLength))), 0x50);
            const __m256i packed = _mm256_unpacklo_epi8(a, b);
            _mm256_storeu_si256(pDst++, packed);
            pSrc += 16;
        }

        uint16_t* pDstWord = reinterpret_cast<uint16_t*>(pDst);
        for (uint32_t x = 0; x < widthRemainder; ++x)
        {
            const uint16_t v = *pSrc | (static_cast<uint16_t>(*(pSrc + halfLength)) << 8);
            *pDstWord++ = v;
            ++pSrc;
        }
    }

    _mm256_zeroall();
}

/*static*/void LoHiBytePackUnpackAvx::LoHiByteUnpackStrided_Choose(const void* ptrSrc, std::uint32_t wordCount, std::uint32_t stride, std::uint32_t lineCount, void* ptrDst)
{
    if (CheckWhetherCpuSupportsAVX2())
    {
        LoHiBytePackUnpackAvx::pfnLoHiByteUnpackStrided = LoHiBytePackUnpackAvx::LoHiByteUnpackStrided_AVX;
    }
    else
    {
        LoHiBytePackUnpackAvx::pfnLoHiByteUnpackStrided = LoHiBytePackUnpackAvx::LoHiByteUnpackStrided_C;
    }

    (*LoHiBytePackUnpackAvx::pfnLoHiByteUnpackStrided)(ptrSrc, wordCount, stride, lineCount, ptrDst);
}

/*static*/void LoHiBytePackUnpackAvx::LoHiBytePackStrided_Choose(const void* ptrSrc, size_t sizeSrc, std::uint32_t width, std::uint32_t height, std::uint32_t stride, void* dest)
{
    if (CheckWhetherCpuSupportsAVX2())
    {
        LoHiBytePackUnpackAvx::pfnLoHiBytePackStrided = LoHiBytePackUnpackAvx::LoHiBytePackStrided_AVX;
    }
    else
    {
        LoHiBytePackUnpackAvx::pfnLoHiBytePackStrided = LoHiBytePackUnpackAvx::LoHiBytePackStrided_C;
    }

    (*LoHiBytePackUnpackAvx::pfnLoHiBytePackStrided)(ptrSrc, sizeSrc, width, height, stride, dest);
}

#elif LIBCZI_HAS_NEOININTRINSICS

#include <arm_neon.h>

/*static*/void LoHiBytePackUnpack::LoHiByteUnpackStrided(const void* ptrSrc, std::uint32_t wordCount, std::uint32_t stride, std::uint32_t lineCount, void* ptrDst)
{
    LoHiBytePackUnpack::CheckLoHiByteUnpackArgumentsAndThrow(wordCount, stride, ptrSrc, ptrDst);

    uint8_t* pDst = static_cast<uint8_t*>(ptrDst);
    const uint32_t widthOver8 = wordCount / 8;
    const uint32_t widthModulo8 = wordCount % 8;
    const size_t halfLength = (static_cast<size_t>(wordCount) * 2 * lineCount) / 2;
    for (uint32_t y = 0; y < lineCount; ++y)
    {
        const uint16_t* pSrc = reinterpret_cast<const uint16_t*>(static_cast<const uint8_t*>(ptrSrc) + y * static_cast<size_t>(stride));

        for (uint32_t i = 0; i < widthOver8; ++i)
        {
            const uint8x8x2_t data = vld2_u8(reinterpret_cast<const uint8_t*>(pSrc));
            vst1_u8(pDst, data.val[0]);
            vst1_u8(pDst + halfLength, data.val[1]);

            pSrc += 8;  // we do 8 words = 16 bytes per loop
            pDst += 8;
        }

        for (uint32_t i = 0; i < widthModulo8; ++i)
        {
            const uint16_t v = *pSrc++;
            *pDst = static_cast<uint8_t>(v);
            *(pDst + halfLength) = static_cast<uint8_t>(v >> 8);
            pDst++;
        }
    }
}

/*static*/void LoHiBytePackUnpack::LoHiBytePackStrided(const void* ptrSrc, size_t sizeSrc, std::uint32_t width, std::uint32_t height, std::uint32_t stride, void* dest)
{
    LoHiBytePackUnpack::CheckLoHiBytePackArgumentsAndThrow(ptrSrc, sizeSrc, width, height, stride, dest);

    const uint8_t* pSrc = static_cast<const uint8_t*>(ptrSrc);

    const size_t halfLength = sizeSrc / 2;
    const uint32_t widthOver8 = width / 8;
    const uint32_t widthRemainder = width % 8;

    for (uint32_t y = 0; y < height; ++y)
    {
        uint8_t* pDst = static_cast<uint8_t*>(dest) + static_cast<size_t>(y) * stride;
        for (uint32_t x = 0; x < widthOver8; ++x)
        {
            const uint8x8_t a = vld1_u8(pSrc);
            const uint8x8_t b = vld1_u8(pSrc + halfLength);
            uint8x8x2_t c{ a,b };
            vst2_u8(pDst, c);
            pSrc += 8;
            pDst += 16;
        }

        uint16_t* pDstWord = reinterpret_cast<uint16_t*>(pDst);
        for (uint32_t x = 0; x < widthRemainder; ++x)
        {
            const uint16_t v = *pSrc | (static_cast<uint16_t>(*(pSrc + halfLength)) << 8);
            *pDstWord++ = v;
            ++pSrc;
        }
    }
}
#endif