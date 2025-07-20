//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
// MIS was written by Sergey Epishkin, and is placed in the public domain.
// The author hereby disclaims copyright to this source code.
//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////
// Just make a Github issue if you have any problems with my code.
// And don't sue me, it's your problems that you uses it. ಠ_ಠ
// Code is public because i really don't want to get money or popularity from this.
//////////////////////////////////////////////////////////////////////////////////////

#include <mis.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <mis_tables.h>

#include <stdint.h>  // for x64/x32 classification only
#include <threads.h> // for thread local fallbacks

#pragma region Caller location management

#define caller_location __FILE__, __LINE__
#define pass_caller_location caller_file, caller_line
#define has_caller_location const char* caller_file, int caller_line
#define field_caller_location const char* caller_file; int caller_line

#pragma endregion Caller location management

#pragma region Ghost types

#define f32   float
#define f64   double

#define i8    signed char
#define i16   signed short
#define i32   signed int
#define i64   signed long long
#define u8  unsigned char
#define u16 unsigned short
#define u32 unsigned int
#define u64 unsigned long long

#define uptr unsigned long long
#define iptr   signed long long

#define rune u32

#define bool int
#define true        (bool)1
#define false       (bool)0

#define i8min       (-127i8 - 1)
#define i16min      (-32767i16 - 1)
#define i32min      (-2147483647i32 - 1)
#define i64min      (-9223372036854775807i64 - 1)
#define i8max       127i8
#define i16max      32767i16
#define i32max      2147483647i32
#define i64max      9223372036854775807i64
#define u8max       0xffui8
#define u16max      0xffffui16
#define u32max      0xffffffffui32
#define u64max      0xffffffffffffffffui64
#define i8size      sizeof(i8)
#define i16size     sizeof(i16)
#define i32size     sizeof(i32)
#define i64size     sizeof(i64)
#define u8size      sizeof(u8)
#define u16size     sizeof(u16)
#define u32size     sizeof(u32)
#define u64size     sizeof(u64)
#define f32size     sizeof(f32)
#define f64size     sizeof(f64)
#define uptrsize    sizeof(uptr)
#define iptrsize    sizeof(iptr)
#define i8bitsize   (8*i8size)
#define i16bitsize  (8*i16size)
#define i32bitsize  (8*i32size)
#define i64bitsize  (8*i64size)
#define u8bitsize   (8*u8size)
#define u16bitsize  (8*u16size)
#define u32bitsize  (8*u32size)
#define u64bitsize  (8*u64size)
#define f32bitsize  (8*f32size)
#define f64bitsize  (8*f64size)
#define uptrbitsize (8*uptrsize)
#define iptrbitsize (8*iptrsize)

#pragma endregion Ghost types

#pragma region Memory debug tool

// put it on to debug memory leaks
// #define MIS_MEM_DBG

#ifdef MIS_MEM_DBG
struct dbg_alloc {
    bool is_aligned, freed;
    void* ptr;
    field_caller_location;
};
thread_local struct dbg_alloc dbg_allocs[1000000];
thread_local u64 dbg_allocs_count = 0;

void dbg_bump() {
    printf("\n-------- MEM DBG BUMP\n");
    for (u64 i = 0; i < dbg_allocs_count; i++) {
        struct dbg_alloc mem = dbg_allocs[i];
        printf(
            "%sBLOCK:\n"
            "-  ptr : %.8llX, aligned : %s\n"
            "-  alloc from %s:%u\n",
            mem.is_aligned ? "ALIGNED " : "",
            (u64)mem.ptr, mem.is_aligned ? "true" : "false",
            mem.caller_file, mem.caller_line
        );
    }
    printf("-------- STILL ALLOCATED %llu\n\n", dbg_allocs_count);
    fflush(stdout);
}

void dbg_alloc(void* ptr, bool align, has_caller_location) {
    dbg_allocs[dbg_allocs_count] = (struct dbg_alloc) { false, false, ptr, pass_caller_location };
    dbg_allocs_count++;
    #ifdef MIS_LOG_MEM_DBG
    printf(
        "%sALLOC:\n"
        "-  ptr : %.8llX, aligned : %s\n"
        "-  alloc from %s:%u\n",
        align ? "ALIGNED " : "",
        (u64)ptr, align ? "true" : "false",
        caller_file, caller_line
    );
    fflush(stdout);
    #endif
}

void dbg_free(void* ptr, bool align, has_caller_location) {
    if (!ptr) return;
    for (u64 i = 0; i < dbg_allocs_count; i++) {
        struct dbg_alloc* mem = &dbg_allocs[i];
        if (mem->ptr != ptr) continue;

        #ifdef MIS_LOG_MEM_DBG
        printf(
            "%sFREE:\n"
            "- %s ptr : %.8llX, aligned : %s\n"
            "-  allocated in %s:%u\n"
            "-  free from %s:%u\n",
            align ? "ALIGNED " : "",
            mem->freed ? " freed" : "", (u64)mem->ptr, mem->is_aligned ? "true" : "false",
            mem->caller_file, mem->caller_line,
            caller_file, caller_line
        );
        fflush(stdout);
        #endif
        *mem = dbg_allocs[--dbg_allocs_count]; 
        return;
    }
    #ifdef MIS_LOG_MEM_DBG
    printf(
        "%sUNDEFINED FREE:\n"
        "-  ptr : %.8llX, aligned : %s\n"
        "-  free from %s:%u\n",
        align ? "ALIGNED " : "",
        (u64)ptr, align ? "true" : "false",
        caller_file, caller_line
    );
    fflush(stdout);
    #endif
}

void* malloc_align(size_t size, size_t align, has_caller_location) {
    void* ptr = _aligned_malloc(size, align);
    dbg_alloc(ptr, true, pass_caller_location);
    return ptr;
}
void* malloc_(size_t size, has_caller_location) {
    void* ptr = malloc(size);
    dbg_alloc(ptr, false, pass_caller_location);
    return ptr;
}

void* realloc_(void* ptr, size_t size, has_caller_location) {
    dbg_free(ptr, false, pass_caller_location);

    ptr = realloc(ptr, size);
    if (size == 0) return ptr;
    dbg_alloc(ptr, false, pass_caller_location);
    return ptr;
}

void* realloc_align(void* ptr, size_t size, size_t align, has_caller_location) {
    dbg_free(ptr, true, pass_caller_location);

    ptr = _aligned_realloc(ptr, size, align);
    if (size == 0) return ptr;
    dbg_alloc(ptr, true, pass_caller_location);
    return ptr;
}

void* recalloc_align(void* ptr, size_t count, size_t size, size_t align, has_caller_location) {
    dbg_free(ptr, true, pass_caller_location);

    ptr = _aligned_recalloc(ptr, count, size, align);
    if (size == 0) return ptr;
    dbg_alloc(ptr, true, pass_caller_location);
    return ptr;
}

void free_align(void* ptr, has_caller_location) {
    dbg_free(ptr, false, pass_caller_location);
    _aligned_free(ptr);
}
void free_(void* ptr, has_caller_location) {
    dbg_free(ptr, false, pass_caller_location);
    free(ptr);
}

#define malloc(size) malloc_(size, caller_location)
#define realloc(ptr, size) realloc_(ptr, size, caller_location)
#define _aligned_realloc(ptr, size, align) realloc_align(ptr, size, align, caller_location)
#define _aligned_recalloc(ptr, count, size, align) recalloc_align(ptr, count, size, align, caller_location)
#define _aligned_malloc(size, align) malloc_align(size, align, caller_location)
#define _aligned_free(ptr) free_align(ptr, caller_location)
#define free(ptr) free_(ptr, caller_location)
#else
#define dbg_bump()
#endif

#pragma endregion Memory debug tool

#pragma region Computer specifics

#if defined(__i386__) || defined(__x86_64__) || defined(_M_AMD64) || defined(_M_IX86)
#define mis_x86_or_amd64
#endif

#if defined(MIS_BE)
// not le
#elif defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
#define mis_le
#elif defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
// not le
#elif defined(_WIN32) || defined(_WIN64) || defined(mis_x86_or_amd64)
#define mis_le
#else
#warning Could not determine endianness at compile time. Assuming Little-Endian. Define MIS_BE, if your device uses Big-Endian
#define mis_le
#endif

#if INTPTR_MAX == INT64_MAX
#define mis_x64
#elif INTPTR_MAX == INT32_MAX
// is x32
#else
#error MIS library supports only x32 and x64 bit architectures
#endif

#pragma endregion Computer specifics

#pragma region MIS settings

thread_local static MISFallbackType mis_fallback;

void mis_std_fallback(has_caller_location, const char* format, ...) {
    va_list valist;
    printf("%s:%d founded MIS Error: ", caller_file, caller_line);
    va_start(valist, format);
    vprintf(format, valist);
    va_end(valist);
    putchar('\n');
}

void mis_std_parse_fallback(MISParser prs, has_caller_location, const char* format, ...) {
    va_list valist;
    printf("%s:%d\n", caller_file, caller_line);
    printf("%s at %d founded MIS Error: ", prs.file, prs.ln);
    va_start(valist, format);
    vprintf(format, valist);
    va_end(valist);
    putchar('\n');
}

void mis_std_init() {
    mis_fallback = mis_std_fallback;
}

void mis_init(MISFallbackType _mis_fallback) {
    mis_fallback = _mis_fallback;
}

#pragma endregion MIS settings

//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
// Copyright 2018 Ulf Adams. All rights reserved.
// Boost Software License - Version 1.0 - August 17th, 2003
//
// Permission is hereby granted, free of charge, to any person or organization
// obtaining a copy of the software and accompanying documentation covered by
// this license (the "Software") to use, reproduce, display, distribute,
// execute, and transmit the Software, and to prepare derivative works of the
// Software, and to permit third-parties to whom the Software is furnished to
// do so, all subject to the following:
//
// The copyright notices in the Software and this entire statement, including
// the above license grant, this restriction and the following disclaimer,
// must be included in all copies of the Software, in whole or in part, and
// all derivative works of the Software, unless such copies or derivative
// works are solely in the form of machine-executable object code generated by
// a source language processor.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
// SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
// FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////

#pragma region Ryu double to fixed point string

#define ryu_double_mantissa_bits 52
#define ryu_double_exponent_bits 11
#define ryu_double_bias 1023
#define ryu_pow10_additional_bits 120

#if defined(__SIZEOF_INT128__) && !defined(_MSC_VER) && !defined(RYU_ONLY_64_BIT_OPS)
#define HAS_UINT128
#elif defined(_MSC_VER) && !defined(RYU_ONLY_64_BIT_OPS) && defined(_M_X64)
#define HAS_64_BIT_INTRINSICS
#endif

#include <intrin.h>

// Returns true if value is divisible by 2^p.
static inline bool multipleOfPowerOf2(const uint64_t value, const uint32_t p) {
    // __builtin_ctzll doesn't appear to be faster here.
    return (value & ((1ull << p) - 1)) == 0;
}

// Returns floor(log_10(2^e)); requires 0 <= e <= 1650.
static inline uint32_t log10Pow2(const int32_t e) {
    // The first value this approximation fails for is 2^1651 which is just greater than 10^297.
    return (((uint32_t) e) * 78913) >> 18;
}

static inline uint64_t double_to_bits(const double d) {
    uint64_t bits = 0;
    memcpy(&bits, &d, sizeof(double));
    return bits;
}

static inline int copy_special_str_printf(char* const result, const bool sign, const uint64_t mantissa) {
#if defined(_MSC_VER)
    if (sign) {
        result[0] = '-';
    }
    if (mantissa) {
        if (mantissa < (1ull << (ryu_double_mantissa_bits - 1))) {
        memcpy(result + sign, "nan(snan)", 9);
        return sign + 9;
        }
        memcpy(result + sign, "nan", 3);
        return sign + 3;
    }
#else
    if (mantissa) {
        memcpy(result, "nan", 3);
        return 3;
    }
    if (sign) {
        result[0] = '-';
    }
#endif
    memcpy(result + sign, "Infinity", 8);
    return sign + 8;
}

static inline uint32_t indexForExponent(const uint32_t e) {
    return (e + 15) / 16;
}

static inline uint32_t pow10BitsForIndex(const uint32_t idx) {
    return 16 * idx + ryu_pow10_additional_bits;
}

static inline uint32_t lengthForIndex(const uint32_t idx) {
    // +1 for ceil, +16 for mantissa, +8 to round up when dividing by 9
    return (log10Pow2(16 * (int32_t) idx) + 1 + 16 + 8) / 9;
}

#if defined(HAS_UINT128)
static inline uint128_t umul256(const uint128_t a, const uint64_t bHi, const uint64_t bLo, uint128_t* const productHi) {
    const uint64_t aLo = (uint64_t)a;
    const uint64_t aHi = (uint64_t)(a >> 64);

    const uint128_t b00 = (uint128_t)aLo * bLo;
    const uint128_t b01 = (uint128_t)aLo * bHi;
    const uint128_t b10 = (uint128_t)aHi * bLo;
    const uint128_t b11 = (uint128_t)aHi * bHi;

    const uint64_t b00Lo = (uint64_t)b00;
    const uint64_t b00Hi = (uint64_t)(b00 >> 64);

    const uint128_t mid1 = b10 + b00Hi;
    const uint64_t mid1Lo = (uint64_t)(mid1);
    const uint64_t mid1Hi = (uint64_t)(mid1 >> 64);

    const uint128_t mid2 = b01 + mid1Lo;
    const uint64_t mid2Lo = (uint64_t)(mid2);
    const uint64_t mid2Hi = (uint64_t)(mid2 >> 64);

    const uint128_t pHi = b11 + mid1Hi + mid2Hi;
    const uint128_t pLo = ((uint128_t)mid2Lo << 64) | b00Lo;

    *productHi = pHi;
    return pLo;
}

// Returns the high 128 bits of the 256-bit product of a and b.
static inline uint128_t umul256_hi(const uint128_t a, const uint64_t bHi, const uint64_t bLo) {
    // Reuse the umul256 implementation.
    // Optimizers will likely eliminate the instructions used to compute the
    // low part of the product.
    uint128_t hi;
    umul256(a, bHi, bLo, &hi);
    return hi;
}

// Unfortunately, gcc/clang do not automatically turn a 128-bit integer division
// into a multiplication, so we have to do it manually.
static inline uint32_t uint128_mod1e9(const uint128_t v) {
    // After multiplying, we're going to shift right by 29, then truncate to uint32_t.
    // This means that we need only 29 + 32 = 61 bits, so we can truncate to uint64_t before shifting.
    const uint64_t multiplied = (uint64_t) umul256_hi(v, 0x89705F4136B4A597u, 0x31680A88F8953031u);

    // For uint32_t truncation, see the mod1e9() comment in d2s_intrinsics.h.
    const uint32_t shifted = (uint32_t) (multiplied >> 29);

    return ((uint32_t) v) - 1000000000 * shifted;
}

// Best case: use 128-bit type.
static inline uint32_t mulShift_mod1e9(const uint64_t m, const uint64_t* const mul, const int32_t j) {
    const uint128_t b0 = ((uint128_t) m) * mul[0]; // 0
    const uint128_t b1 = ((uint128_t) m) * mul[1]; // 64
    const uint128_t b2 = ((uint128_t) m) * mul[2]; // 128
    // j: [128, 256)
    const uint128_t mid = b1 + (uint64_t) (b0 >> 64); // 64
    const uint128_t s1 = b2 + (uint64_t) (mid >> 64); // 128
    return uint128_mod1e9(s1 >> (j - 128));
}

#else // HAS_UINT128

#if defined(HAS_64_BIT_INTRINSICS)
// Returns the low 64 bits of the high 128 bits of the 256-bit product of a and b.
static inline uint64_t umul256_hi128_lo64(
    const uint64_t aHi, const uint64_t aLo, const uint64_t bHi, const uint64_t bLo) {
    uint64_t b00Hi;
    const uint64_t b00Lo = _umul128(aLo, bLo, &b00Hi);
    uint64_t b01Hi;
    const uint64_t b01Lo = _umul128(aLo, bHi, &b01Hi);
    uint64_t b10Hi;
    const uint64_t b10Lo = _umul128(aHi, bLo, &b10Hi);
    uint64_t b11Hi;
    const uint64_t b11Lo = _umul128(aHi, bHi, &b11Hi);
    (void) b00Lo; // unused
    (void) b11Hi; // unused
    const uint64_t temp1Lo = b10Lo + b00Hi;
    const uint64_t temp1Hi = b10Hi + (temp1Lo < b10Lo);
    const uint64_t temp2Lo = b01Lo + temp1Lo;
    const uint64_t temp2Hi = b01Hi + (temp2Lo < b01Lo);
    return b11Lo + temp1Hi + temp2Hi;
}

static inline uint32_t uint128_mod1e9(const uint64_t vHi, const uint64_t vLo) {
    // After multiplying, we're going to shift right by 29, then truncate to uint32_t.
    // This means that we need only 29 + 32 = 61 bits, so we can truncate to uint64_t before shifting.
    const uint64_t multiplied = umul256_hi128_lo64(vHi, vLo, 0x89705F4136B4A597u, 0x31680A88F8953031u);

    // For uint32_t truncation, see the mod1e9() comment in d2s_intrinsics.h.
    const uint32_t shifted = (uint32_t) (multiplied >> 29);

    return ((uint32_t) vLo) - 1000000000 * shifted;
}
#endif // HAS_64_BIT_INTRINSICS

static inline uint32_t mulShift_mod1e9(const uint64_t m, const uint64_t* const mul, const int32_t j) {
    uint64_t high0;                                   // 64
    const uint64_t low0 = _umul128(m, mul[0], &high0); // 0
    uint64_t high1;                                   // 128
    const uint64_t low1 = _umul128(m, mul[1], &high1); // 64
    uint64_t high2;                                   // 192
    const uint64_t low2 = _umul128(m, mul[2], &high2); // 128
    const uint64_t s0low = low0;              // 0
    (void) s0low; // unused
    const uint64_t s0high = low1 + high0;     // 64
    const uint32_t c1 = s0high < low1;
    const uint64_t s1low = low2 + high1 + c1; // 128
    const uint32_t c2 = s1low < low2; // high1 + c1 can't overflow, so compare against low2
    const uint64_t s1high = high2 + c2;       // 192
#if defined(HAS_64_BIT_INTRINSICS)
    const uint32_t dist = (uint32_t) (j - 128); // dist: [0, 52]
    const uint64_t shiftedhigh = s1high >> dist;
    const uint64_t shiftedlow = __shiftright128(s1low, s1high, dist);
    return uint128_mod1e9(shiftedhigh, shiftedlow);
#else // HAS_64_BIT_INTRINSICS
    if (j < 160) { // j: [128, 160)
        const uint64_t r0 = mod1e9(s1high);
        const uint64_t r1 = mod1e9((r0 << 32) | (s1low >> 32));
        const uint64_t r2 = ((r1 << 32) | (s1low & 0xffffffff));
        return mod1e9(r2 >> (j - 128));
    } else { // j: [160, 192)
        const uint64_t r0 = mod1e9(s1high);
        const uint64_t r1 = ((r0 << 32) | (s1low >> 32));
        return mod1e9(r1 >> (j - 160));
    }
#endif // HAS_64_BIT_INTRINSICS
}
#endif // HAS_UINT128

// Returns the number of decimal digits in v, which must not contain more than 9 digits.
static inline uint32_t decimalLength9(const uint32_t v) {
    if (v >= 100000000) { return 9; }
    if (v >= 10000000) { return 8; }
    if (v >= 1000000) { return 7; }
    if (v >= 100000) { return 6; }
    if (v >= 10000) { return 5; }
    if (v >= 1000) { return 4; }
    if (v >= 100) { return 3; }
    if (v >= 10) { return 2; }
    return 1;
}

// Convert `digits` to a sequence of decimal digits. Append the digits to the result.
// The caller has to guarantee that:
//   10^(olength-1) <= digits < 10^olength
// e.g., by passing `olength` as `decimalLength9(digits)`.
static inline void append_n_digits(const uint32_t olength, uint32_t digits, char* const result) {
    uint32_t i = 0;
    while (digits >= 10000) {
    #ifdef __clang__ // https://bugs.llvm.org/show_bug.cgi?id=38217
        const uint32_t c = digits - 10000 * (digits / 10000);
    #else
        const uint32_t c = digits % 10000;
    #endif
        digits /= 10000;
        const uint32_t c0 = (c % 100) << 1;
        const uint32_t c1 = (c / 100) << 1;
        memcpy(result + olength - i - 2, ryu_digit_table + c0, 2);
        memcpy(result + olength - i - 4, ryu_digit_table + c1, 2);
        i += 4;
    }
    if (digits >= 100) {
        const uint32_t c = (digits % 100) << 1;
        digits /= 100;
        memcpy(result + olength - i - 2, ryu_digit_table + c, 2);
        i += 2;
    }
    if (digits >= 10) {
        const uint32_t c = digits << 1;
        memcpy(result + olength - i - 2, ryu_digit_table + c, 2);
    } else {
        result[0] = (char) ('0' + digits);
    }
}


// Convert `digits` to decimal and write the last `count` decimal digits to result.
// If `digits` contains additional digits, then those are silently ignored.
static inline void append_c_digits(const uint32_t count, uint32_t digits, char* const result) {
    // Copy pairs of digits from DIGIT_TABLE.
    uint32_t i = 0;
    for (; i < count - 1; i += 2) {
        const uint32_t c = (digits % 100) << 1;
        digits /= 100;
        memcpy(result + count - i - 2, ryu_digit_table + c, 2);
    }
    // Generate the last digit if count is odd.
    if (i < count) {
        const char c = (char) ('0' + (digits % 10));
        result[count - i - 1] = c;
    }
}

// Convert `digits` to decimal and write the last 9 decimal digits to result.
// If `digits` contains additional digits, then those are silently ignored.
static inline void append_nine_digits(uint32_t digits, char* const result) {
    if (digits == 0) {
        memset(result, '0', 9);
        return;
    }

    for (uint32_t i = 0; i < 5; i += 4) {
    #ifdef __clang__ // https://bugs.llvm.org/show_bug.cgi?id=38217
        const uint32_t c = digits - 10000 * (digits / 10000);
    #else
        const uint32_t c = digits % 10000;
    #endif
        digits /= 10000;
        const uint32_t c0 = (c % 100) << 1;
        const uint32_t c1 = (c / 100) << 1;
        memcpy(result + 7 - i, ryu_digit_table + c0, 2);
        memcpy(result + 5 - i, ryu_digit_table + c1, 2);
    }
    result[0] = (char) ('0' + digits);
    }

int d2fixed_buffered_n(double d, uint32_t precision, char* result) {
    const uint64_t bits = double_to_bits(d);

    // Decode bits into sign, mantissa, and exponent.
    const bool ieeeSign = ((bits >> (ryu_double_mantissa_bits + ryu_double_exponent_bits)) & 1) != 0;
    const uint64_t ieeeMantissa = bits & ((1ull << ryu_double_mantissa_bits) - 1);
    const uint32_t ieeeExponent = (uint32_t) ((bits >> ryu_double_mantissa_bits) & ((1u << ryu_double_exponent_bits) - 1));

    // Case distinction; exit early for the easy cases.
    if (ieeeExponent == ((1u << ryu_double_exponent_bits) - 1u)) {
        return copy_special_str_printf(result, ieeeSign, ieeeMantissa);
    }
    if (ieeeExponent == 0 && ieeeMantissa == 0) {
        int index = 0;
        if (ieeeSign) {
            result[index++] = '-';
        }
        result[index++] = '0';
        if (precision > 0) {
            result[index++] = '.';
            memset(result + index, '0', precision);
            index += precision;
        }
        return index;
    }

    int32_t e2;
    uint64_t m2;
    if (ieeeExponent == 0) {
        e2 = 1 - ryu_double_bias - ryu_double_mantissa_bits;
        m2 = ieeeMantissa;
    } else {
        e2 = (int32_t) ieeeExponent - ryu_double_bias - ryu_double_mantissa_bits;
        m2 = (1ull << ryu_double_mantissa_bits) | ieeeMantissa;
    }

    int index = 0;
    bool nonzero = false;
    if (ieeeSign) {
        result[index++] = '-';
    }
    if (e2 >= -52) {
        const uint32_t idx = e2 < 0 ? 0 : indexForExponent((uint32_t) e2);
        const uint32_t p10bits = pow10BitsForIndex(idx);
        const int32_t len = (int32_t) lengthForIndex(idx);
        for (int32_t i = len - 1; i >= 0; --i) {
            const uint32_t j = p10bits - e2;
            // Temporary: j is usually around 128, and by shifting a bit, we push it to 128 or above, which is
            // a slightly faster code path in mulShift_mod1e9. Instead, we can just increase the multipliers.
            const uint32_t digits = mulShift_mod1e9(m2 << 8, ryu_pow10_split[ryu_pow10_offset[idx] + i], (int32_t) (j + 8));
            if (nonzero) {
                append_nine_digits(digits, result + index);
                index += 9;
            } else if (digits != 0) {
                const uint32_t olength = decimalLength9(digits);
                append_n_digits(olength, digits, result + index);
                index += olength;
                nonzero = true;
            }
        }
    }
    if (!nonzero) {
        result[index++] = '0';
    }
    if (precision > 0) {
        result[index++] = '.';
    }
    if (e2 < 0) {
        const int32_t idx = -e2 / 16;
        const uint32_t blocks = precision / 9 + 1;
        // 0 = don't round up; 1 = round up unconditionally; 2 = round up if odd.
        int roundUp = 0;
        uint32_t i = 0;
        if (blocks <= ryu_min_block_2[idx]) {
            i = blocks;
            memset(result + index, '0', precision);
            index += precision;
        } else if (i < ryu_min_block_2[idx]) {
            i = ryu_min_block_2[idx];
            memset(result + index, '0', 9 * i);
            index += 9 * i;
        }
        for (; i < blocks; ++i) {
            const int32_t j = ryu_additional_bits_2 + (-e2 - 16 * idx);
            const uint32_t p = ryu_pow10_offset_2[idx] + i - ryu_min_block_2[idx];
            if (p >= ryu_pow10_offset_2[idx + 1]) {
                // If the remaining digits are all 0, then we might as well use memset.
                // No rounding required in this case.
                const uint32_t fill = precision - 9 * i;
                memset(result + index, '0', fill);
                index += fill;
                break;
            }
            // Temporary: j is usually around 128, and by shifting a bit, we push it to 128 or above, which is
            // a slightly faster code path in mulShift_mod1e9. Instead, we can just increase the multipliers.
            uint32_t digits = mulShift_mod1e9(m2 << 8, ryu_pow10_split_2[p], j + 8);
            if (i < blocks - 1) {
                append_nine_digits(digits, result + index);
                index += 9;
            } else {
                const uint32_t maximum = precision - 9 * i;
                uint32_t lastDigit = 0;
                for (uint32_t k = 0; k < 9 - maximum; ++k) {
                    lastDigit = digits % 10;
                    digits /= 10;
                }
                if (lastDigit != 5) {
                    roundUp = lastDigit > 5;
                } else {
                    // Is m * 10^(additionalDigits + 1) / 2^(-e2) integer?
                    const int32_t requiredTwos = -e2 - (int32_t) precision - 1;
                    const bool trailingZeros = requiredTwos <= 0
                        || (requiredTwos < 60 && multipleOfPowerOf2(m2, (uint32_t) requiredTwos));
                    roundUp = trailingZeros ? 2 : 1;
                }
                if (maximum > 0) {
                    append_c_digits(maximum, digits, result + index);
                    index += maximum;
                }
                break;
            }
        }
        if (roundUp != 0) {
            int roundIndex = index;
            int dotIndex = 0; // '.' can't be located at index 0
            while (true) {
                --roundIndex;
                char c;
                if (roundIndex == -1 || (c = result[roundIndex], c == '-')) {
                    result[roundIndex + 1] = '1';
                    if (dotIndex > 0) {
                        result[dotIndex] = '0';
                        result[dotIndex + 1] = '.';
                    }
                    result[index++] = '0';
                    break;
                }
                if (c == '.') {
                    dotIndex = roundIndex;
                    continue;
                } else if (c == '9') {
                    result[roundIndex] = '0';
                    roundUp = 1;
                    continue;
                } else {
                    if (roundUp == 2 && c % 2 == 0) {
                        break;
                    }
                    result[roundIndex] = c + 1;
                    break;
                }
            }
        }
    } else {
        memset(result + index, '0', precision);
        index += precision;
    }
    return index;
}

#pragma endregion Ryu double to fixed point string

//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
// MIS was written by Sergey Epishkin, and is placed in the public domain.
// The author hereby disclaims copyright to this source code.
//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////
// ---------------- MIS SERIALIZER ---------------- //
//////////////////////////////////////////////////////

#pragma region MIS Serializer

#pragma region Serialize helpers

#define freeop

#define expand(c) do { \
        if (build->length + c >= build->count) { \
            if (build->count == 0) build->count = 2; \
            else                   build->count = max(build->count * 2, build->count + c); \
            build->text = realloc(build->text, build->count); \
            if (!build->text) { mis_fallback(__FILE__, __LINE__, "Out of memory"); return false; } \
        } \
    } while (0)

bool __build_append_elem(struct __MISSerializerBuilder* build, char ch) {
    expand(1);
    build->text[build->length++] = ch;
    return true;
}

bool __build_append_elems(struct __MISSerializerBuilder* build, const char* elems, u32 count) {
    expand((i32)count);
    memcpy(build->text + build->length, elems, count);
    build->length += count;
    return true;
}

#define __build_append(build, v, ...) do { if (!_Generic((v), \
                                                    char*: __build_append_elems, \
                                                    const char*: __build_append_elems, \
                                                    default: __build_append_elem \
                                                )(build, v __VA_OPT__(, __VA_ARGS__))) { freeop; return 0; } } while (0)
#define __build_free(build) do { if (build.text) free(build.text); } while (0)

#undef expand

__inline bool __ser_check_root(MISSerializer* ser, has_caller_location) {
    if (ser->valid) {
        return true;
    }
    mis_fallback(pass_caller_location, "Serializer is not allowed to use, close action \"%s\" before", ser->action);
    ser->good = false;
    __build_free(ser->builder);
    return false;
}

__inline bool __ser_check_branch(MISBranchSerializer* ser, has_caller_location) {
    if (ser->valid) {
        return true;
    }
    mis_fallback(pass_caller_location, "Serializer is not allowed to use, close action \"%s\" before", ser->action);
    ser->root->good = false;
    __build_free(ser->root->builder);
    return false;
}

#define __ser_check(ser) do { if (!_Generic((ser), \
                                    MISSerializer*: __ser_check_root, \
                                    default: __ser_check_branch \
                                )(ser, pass_caller_location)) { freeop; return 0; } } while (0)
                                

bool __ser_end_fn(MISBranchSerializer* ser, has_caller_location) {
    if (!ser->endable) {
        mis_fallback(pass_caller_location, "Serializer action \"%s\" is not allowed to be ended", ser->action);
        free(ser->root->builder.text);
        return false;
    }
    __ser_check(ser);
    if (ser->back_valid) *ser->back_valid = true;
    return true;
}

#define __ser_end(ser) do { if (!__ser_end_fn(ser, pass_caller_location)) { freeop; return 0; } } while (0)

bool __ser_start_root(MISSerializer* ser, const char* action, has_caller_location) {
    __ser_check(ser);
    ser->valid = false;
    ser->action = action;
    return true;
}

bool __ser_start_branch(MISBranchSerializer* ser, const char* action, has_caller_location) {
    __ser_check(ser);
    ser->valid = false;
    ser->action = action;
    return true;
}

#define __ser_start(ser, action) do { if (!_Generic((ser), \
                                            MISSerializer*: __ser_start_root, \
                                            default: __ser_start_branch \
                                        )(ser, action, pass_caller_location)) { freeop; return 0; } } while (0)

#pragma endregion Serialize helpers

MISSerializer mis_ser_create() {
    MISSerializer ser;
    ser.back_valid = NULL;
    ser.valid = true;
    ser.action = "serializer";
    ser.ender = 0;
    ser.endable = false;
    ser.good = true;
    ser.builder = (struct __MISSerializerBuilder){0};
    return ser;
}

bool __mis_ser_as_list(MISSerializer* ser, MISListSerializer* result, has_caller_location) {
    __ser_start(ser, "serialize as list");
    result->root = ser;
    result->first_elem = true;
    result->back_valid = &ser->valid;
    result->valid = true;
    result->action = "serialize as list";
    result->ender = 0;
    result->endable = false;
    ser->ender = ']';
    return true;
}

bool __mis_ser_fin(MISSerializer* ser, const char** result, has_caller_location) {
    if (ser->ender)
        __build_append(&ser->builder, ser->ender);
    __build_append(&ser->builder, 0);
    *result = ser->builder.text;
    return ser->good;
}

bool __mis_ser_end(MISBranchSerializer* ser, has_caller_location) {
    __ser_end(ser);
    if (ser->ender != 0) {
        __build_append(&ser->root->builder, ser->ender);
    }
    return true;
}

#pragma region Serialize values

#define __mis_add_start(ser) do { \
        if (ser->first_elem) ser->first_elem = false; \
        else                 __build_append(&ser->root->builder, ','); \
    } while (0)

#pragma region Serialize structures

bool __mis_ser_object(MISListSerializer* ser, MISObjectSerializer* result, has_caller_location) {
    __ser_start(ser, "object");
    __mis_add_start(ser);
    __build_append(&ser->root->builder, '[');

    result->root = ser->root;
    result->first_elem = true;
    result->back_valid = &ser->valid;
    result->valid = true;
    result->action = "object";
    result->ender = ']';
    result->endable = true;

    return true;
}

bool __mis_ser_list(MISListSerializer* ser, MISListSerializer* result, has_caller_location) {
    __ser_start(ser, "list");
    __mis_add_start(ser);
    __build_append(&ser->root->builder, '(');

    result->root = ser->root;
    result->first_elem = true;
    result->back_valid = &ser->valid;
    result->valid = true;
    result->action = "list";
    result->ender = ')';
    result->endable = true;

    return true;
}

#pragma endregion Serialize structures

#pragma region Serialize property

int __mis_ser_property_root_length(MISSerializer* ser, const char* name, int length, MISPropertySerializer* result, has_caller_location) {
    __ser_start(ser, "property");
    __build_append(&ser->builder, name, length);
    __build_append(&ser->builder, ':');

    result->root = ser;
    result->first_elem = true;
    result->back_valid = &ser->valid;
    result->valid = true;
    result->action = "property";
    result->ender = ';';
    result->endable = true;

    return true;
}

int __mis_ser_property_object_length(MISObjectSerializer* ser, const char* name, int length, MISPropertySerializer* result, has_caller_location) {
    __ser_start(ser, "property");
    __build_append(&ser->root->builder, name, length);
    __build_append(&ser->root->builder, ':');

    result->root = ser->root;
    result->first_elem = true;
    result->back_valid = &ser->valid;
    result->valid = true;
    result->action = "property";
    result->ender = ';';
    result->endable = true;

    return true;
}

int __mis_ser_property_root(MISSerializer* ser, const char* name, MISPropertySerializer* result, has_caller_location) {
    return __mis_ser_property_root_length(ser, name, (u32)strlen(name), result, pass_caller_location);
}

int __mis_ser_property_object(MISObjectSerializer* ser, const char* name, MISPropertySerializer* result, has_caller_location) {
    return __mis_ser_property_object_length(ser, name, (u32)strlen(name), result, pass_caller_location);
}

#pragma endregion Serialize property

#pragma region Serialize strings

int __mis_ser_add_string(MISBranchSerializer* ser, const char* value, has_caller_location) {
    return __mis_ser_add_string_length(ser, value, (u32)strlen(value), pass_caller_location);
}

int __mis_ser_add_string_length(MISBranchSerializer* ser, const char* value, int length, has_caller_location) {
    __mis_add_start(ser);

    i32 i, count;
    char ch;
    char* ptr = malloc(length*2);
    if (!ptr) return false;
    char* freeme = ptr;
    for (i = 0, count = 0; i < length; i++, ptr++, value++, count++) {
        ch = *value;
        #define match(a, b, c) case a: *ptr++ = b; *ptr = c; count++; break
        switch (ch)
        {
        match('\n', '\\', 'n');
        match('\t', '\\', 't');
        match('\b', '\\', 'b');
        match('\r', '\\', 'r');
        match('\a', '\\', 'a');
        match('\"', '\\', '"');
        match('\?', '\\', '?');
        match('\\', '\\', '\\');
        match('\f', '\\', 'f');
        match('\v', '\\', 'v');
        match(  0 , '\\', '0');
        default: *ptr = ch; break;
        }
        #undef match
    }
    #undef  freeop
    #define freeop free(freeme)
    
    __build_append(&ser->root->builder, '"');
    __build_append(&ser->root->builder, freeme, count);
    __build_append(&ser->root->builder, '"');

    free(freeme);
    return true;
}

#undef  freeop
#define freeop

#pragma endregion Serialize strings

#pragma region Serialize floating point

bool __mis_ser_add_float(MISBranchSerializer* ser, f32 value, has_caller_location) {
    __mis_add_start(ser);

    char str[20];
    u32 count = d2fixed_buffered_n(value, 5, str);
    u32 i;
    for (i = count-1; str[i] == '0'; i--, count--) {}
    if (str[i] == '.') count--;
    
    __build_append(&ser->root->builder, (char*)str, count);
    return true;
}

bool __mis_ser_add_double(MISBranchSerializer* ser, f64 value, has_caller_location) {
    __mis_add_start(ser);

    char str[40];
    u32 count = d2fixed_buffered_n(value, 5, str);
    u32 i;
    for (i = count-1; str[i] == '0'; i--, count--) {}
    if (str[i] == '.') count--;
    
    __build_append(&ser->root->builder, (char*)str, count);
    return true;
}

#pragma endregion Serialize floating point

// removes ide panic
#define get(a) a

#pragma region Serialize int

#define itos_buffered_n(tp, minlen, setmin) \
    u8 tp##tos_buffered_n(tp value, char** result) { \
        char* buffer = *result; \
        bool neg = value < 0; \
        if (neg) { \
            if (value == get(tp##min)) { \
                setmin \
                return minlen; \
            } \
            value = -value; \
        } \
		tp compute; \
        u8 count = 0; \
		for (u8 i = get(tp##len)-1; i < get(tp##len); i--) { \
			compute = value/10; \
			buffer[i] = '0' + (char)(value - compute*10); \
			count++; \
			if (!compute) break; \
			value = compute; \
		} \
        if (neg) buffer[tp##len - ++count] = '-'; \
        *result = &buffer[tp##len - count]; \
        return count; \
	}

#define i8len  4
#define i16len 6
#define i32len 11
#define i64len 20


#ifdef mis_x86_or_amd64
#define sb1(a, ...) *buffer = a; __VA_OPT__(buffer++; __VA_ARGS__)
#define sb2(a, b, ...) *((u16*)buffer) = (u16)a | ((u16)b << 8); __VA_OPT__(buffer += 2; __VA_ARGS__)
#define sb4(a, b, c, d, ...) *((u32*)buffer) = (u32)a | ((u32)b << 8) | ((u32)c << 16) | ((u32)d << 24); __VA_OPT__(buffer += 4; __VA_ARGS__)

itos_buffered_n(i8,  4,  sb4('-','1','2','8'))
itos_buffered_n(i16, 6,  sb4('-','3','2','7', sb2('6','8')))

#ifdef mis_x64
#define sb8(a, b, c, d, e, f, g, h, ...) *((u64*)buffer) = (u64)a | ((u64)b << 8) | ((u64)c << 16) | ((u64)d << 24) | ((u64)e << 32) | ((u64)f << 40) | ((u64)g << 48) | ((u64)h << 56); __VA_OPT__(buffer += 8; __VA_ARGS__)
itos_buffered_n(i32, 11, sb8('-','2','1','4','7','4','8','3', sb2('6','4', sb1('8'))))
itos_buffered_n(i64, 20, sb8('-','9','2','2','3','3','7','2', sb8('0','3','6','8','5','4','7','7', sb4('5','8','0','8'))))
#undef sb8
#else
itos_buffered_n(i32, 11, sb4('-','2','1','4', sb4('7','4','8','3', sb2('6','4', sb1('8')))))
itos_buffered_n(i64, 20, sb4('-','9','2','2', sb4('3','3','7','2', sb4('0','3','6','8', sb4('5','4','7','7', sb4('5','8','0','8'))))))
#endif
#undef sb1
#undef sb2
#undef sb4
#else
#define copy(str) memcpy(buffer, str, sizeof(str)-1);
itos_buffered_n(i8,  4,  copy("-128"))
itos_buffered_n(i16, 6,  copy("-32768"))
itos_buffered_n(i32, 11, copy("-2147483648"))
itos_buffered_n(i64, 20, copy("-9223372036854775808"))
#undef copy
#endif


bool __mis_ser_add_int8(MISBranchSerializer* ser, i8 value, has_caller_location) {
    __mis_add_start(ser);

    char str[i8len];
    char* buf = str;
    u8 count = i8tos_buffered_n(value, &buf);
    
    __build_append(&ser->root->builder, buf, count);
    return true;
}

bool __mis_ser_add_int16(MISBranchSerializer* ser, i16 value, has_caller_location) {
    __mis_add_start(ser);

    char str[i16len];
    char* buf = str;
    u8 count = i16tos_buffered_n(value, &buf);
    
    __build_append(&ser->root->builder, buf, count);
    return true;
}

bool __mis_ser_add_int32(MISBranchSerializer* ser, i32 value, has_caller_location) {
    __mis_add_start(ser);

    char str[i32len];
    char* buf = str;
    u8 count = i32tos_buffered_n(value, &buf);
    
    __build_append(&ser->root->builder, buf, count);
    return true;
}

bool __mis_ser_add_int64(MISBranchSerializer* ser, i64 value, has_caller_location) {
    __mis_add_start(ser);

    char str[i64len];
    char* buf = str;
    u8 count = i64tos_buffered_n(value, &buf);
    
    __build_append(&ser->root->builder, buf, count);
    return true;
}

#pragma endregion Serialize int

#pragma region Serialize uint

#define utos_buffered_n(tp) \
	u8 tp##tos_buffered_n(tp value, char** result) { \
        char* buffer = *result; \
		tp compute; \
        u8 count = 0; \
		for (u8 i = get(tp##len)-1; i < get(tp##len); i--) { \
			compute = value/10; \
			buffer[i] = '0' + (char)(value - compute*10); \
			count++; \
			if (!compute) break; \
			value = compute; \
		} \
        *result = &buffer[tp##len - count]; \
        return count; \
	}

#define u8len  3
#define u16len 5
#define u32len 10
#define u64len 20

utos_buffered_n(u8)
utos_buffered_n(u16)
utos_buffered_n(u32)
utos_buffered_n(u64)


bool __mis_ser_add_uint8(MISBranchSerializer* ser, u8 value, has_caller_location) {
    __mis_add_start(ser);

    char str[u8len];
    char* buf = str;
    u8 count = u8tos_buffered_n(value, &buf);
    
    __build_append(&ser->root->builder, buf, count);
    return true;
}

bool __mis_ser_add_uint16(MISBranchSerializer* ser, u16 value, has_caller_location) {
    __mis_add_start(ser);

    char str[u16len];
    char* buf = str;
    u8 count = u16tos_buffered_n(value, &buf);
    
    __build_append(&ser->root->builder, buf, count);
    return true;
}

bool __mis_ser_add_uint32(MISBranchSerializer* ser, u32 value, has_caller_location) {
    __mis_add_start(ser);

    char str[u32len];
    char* buf = str;
    u8 count = u32tos_buffered_n(value, &buf);
    
    __build_append(&ser->root->builder, buf, count);
    return true;
}

bool __mis_ser_add_uint64(MISBranchSerializer* ser, u64 value, has_caller_location) {
    __mis_add_start(ser);

    char str[u64len];
    char* buf = str;
    u8 count = u64tos_buffered_n(value, &buf);
    
    __build_append(&ser->root->builder, buf, count);
    return true;
}

bool __mis_ser_add_boolean(MISBranchSerializer* ser, bool value, has_caller_location) {
    __mis_add_start(ser);

    if (value) __build_append(&ser->root->builder, "true",  4);
    else       __build_append(&ser->root->builder, "false", 5);

    return true;
}

bool __mis_ser_add_fastdouble(MISBranchSerializer* ser, f64 value, has_caller_location) {
    __mis_add_start(ser);

    __build_append(&ser->root->builder, '#');
#ifdef mis_le
    __build_append(&ser->root->builder, (char*)&value, 8);
#elif
    char* ptr = (char*)&value;
    for (u32 i = f64size; i < f64size; i--)
        __build_append(&ser->root->builder, ptr[i]);
#endif

    return true;
}

#pragma endregion Serialize uint

#pragma endregion Serialize values

#pragma endregion MIS Serializer

//////////////////////////////////////////////////
// ---------------- MIS PARSER ---------------- //
//////////////////////////////////////////////////

#pragma region MIS parser

//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2024 Ginger Bill. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
//    contributors may be used to endorse or promote products derived from
//    this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////

// decimal code is borrowed from Odin standard library
#pragma region Decimal

#define decimaldigitslen 384

struct Decimal {
	u8   digits[decimaldigitslen]; // big-endian digits
	i32  count;
	i32  decimal_point;
	bool neg, trunc;
};

#pragma region Decimal utilities
#define __decimal_trim(a) do { \
        while (a->count > 0 && a->digits[a->count-1] == '0') \
            a->count--; \
        if (a->count == 0) \
            a->decimal_point = 0; \
    } while (0)

void __decimal_shift_right(struct Decimal* a, uptr k) {
	uptr r = 0; // read index
	uptr w = 0; // write index

	uptr n = 0;
	for (; (n>>k) == 0; r++) {
		if (r >= a->count) {
			if (n == 0) {
				// Just in case
				a->count = 0;
				return;
			}
			while ((n>>k) == 0) {
				n *= 10;
				r++;
			}
			break;
		}
		uptr c = (uptr)a->digits[r];
		n = n*10 + c - '0';
	}
	a->decimal_point -= (i32)r-1;

	uptr mask = (1<<k) - 1;

	for (; r < a->count; r++) {
		uptr c = (uptr)a->digits[r];
		uptr dig = n>>k;
		n &= mask;
		a->digits[w] = (u8)('0' + dig);
		w++;
		n = n*10 + c - '0';
	}

	while (n > 0) {
		uptr dig = n>>k;
		n &= mask;
		if (w < decimaldigitslen) {
			a->digits[w] = (u8)('0' + dig);
			w++;
		} else if (dig > 0) {
			a->trunc = true;
		}
		n *= 10;
	}


	a->count = (i32)w;
	__decimal_trim(a);
}

struct decimal_lsh {
    u8 delta;
    u32 cutoff_length;
    const char* cutoff;
};

#define elem(a, b) {a, sizeof(b)-1, b}
const struct decimal_lsh _shift_left_offsets[61] = {
	elem( 0, ""),
	elem( 1, "5"),
	elem( 1, "25"),
	elem( 1, "125"),
	elem( 2, "625"),
	elem( 2, "3125"),
	elem( 2, "15625"),
	elem( 3, "78125"),
	elem( 3, "390625"),
	elem( 3, "1953125"),
	elem( 4, "9765625"),
	elem( 4, "48828125"),
	elem( 4, "244140625"),
	elem( 4, "1220703125"),
	elem( 5, "6103515625"),
	elem( 5, "30517578125"),
	elem( 5, "152587890625"),
	elem( 6, "762939453125"),
	elem( 6, "3814697265625"),
	elem( 6, "19073486328125"),
	elem( 7, "95367431640625"),
	elem( 7, "476837158203125"),
	elem( 7, "2384185791015625"),
	elem( 7, "11920928955078125"),
	elem( 8, "59604644775390625"),
	elem( 8, "298023223876953125"),
	elem( 8, "1490116119384765625"),
	elem( 9, "7450580596923828125"),
	elem( 9, "37252902984619140625"),
	elem( 9, "186264514923095703125"),
	elem(10, "931322574615478515625"),
	elem(10, "4656612873077392578125"),
	elem(10, "23283064365386962890625"),
	elem(10, "116415321826934814453125"),
	elem(11, "582076609134674072265625"),
	elem(11, "2910383045673370361328125"),
	elem(11, "14551915228366851806640625"),
	elem(12, "72759576141834259033203125"),
	elem(12, "363797880709171295166015625"),
	elem(12, "1818989403545856475830078125"),
	elem(13, "9094947017729282379150390625"),
	elem(13, "45474735088646411895751953125"),
	elem(13, "227373675443232059478759765625"),
	elem(13, "1136868377216160297393798828125"),
	elem(14, "5684341886080801486968994140625"),
	elem(14, "28421709430404007434844970703125"),
	elem(14, "142108547152020037174224853515625"),
	elem(15, "710542735760100185871124267578125"),
	elem(15, "3552713678800500929355621337890625"),
	elem(15, "17763568394002504646778106689453125"),
	elem(16, "88817841970012523233890533447265625"),
	elem(16, "444089209850062616169452667236328125"),
	elem(16, "2220446049250313080847263336181640625"),
	elem(16, "11102230246251565404236316680908203125"),
	elem(17, "55511151231257827021181583404541015625"),
	elem(17, "277555756156289135105907917022705078125"),
	elem(17, "1387778780781445675529539585113525390625"),
	elem(18, "6938893903907228377647697925567626953125"),
	elem(18, "34694469519536141888238489627838134765625"),
	elem(18, "173472347597680709441192448139190673828125"),
	elem(19, "867361737988403547205962240695953369140625")
};
#undef  elem
#define __decimal_shift_left_prefix_less(res, b, b_len, s, s_len) do { \
        res = false; \
        for (u32 i = 0; i < s_len; i++) { \
            if (i >= (u32)b_len) { \
                res = true; \
                break; \
            } \
            if (b[i] != s[i]) { \
                res = b[i] < s[i]; \
                break; \
            } \
        } \
    } while (0)

void __decimal_shift_left(struct Decimal* a, uptr k) {
    struct decimal_lsh _shift = _shift_left_offsets[k];
	u8 delta = _shift.delta;
    bool res;
    __decimal_shift_left_prefix_less(res, a->digits, a->count, _shift.cutoff, _shift.cutoff_length);
	if (res) delta--;

	i32 read_index  = a->count;
    i32 write_index = a->count+delta;

	uptr n = 0;
	for (read_index -= 1; read_index >= 0; read_index--) {
		n += ((uptr)(a->digits[read_index]) - '0') << k;
		uptr quo = n/10;
		uptr rem = n - 10*quo;
		write_index--;
		if (write_index < decimaldigitslen)
			a->digits[write_index] = (u8)('0' + rem);
		else if (rem != 0)
			a->trunc = true;
		n = quo;
	}

	while (n > 0) {
		uptr quo = n/10;
		uptr rem = n - 10*quo;
		write_index--;
		if (write_index < decimaldigitslen)
			a->digits[write_index] = (u8)('0' + rem);
        else if (rem != 0)
			a->trunc = true;
		n = quo;
	}

	a->decimal_point += delta;

    #define clamp(v, min, max) (v < min ? min : v > max ? max : v)
	a->count = clamp(a->count+delta, 0, decimaldigitslen);
	__decimal_trim(a);
}

void __decimal_shift(struct Decimal* a, iptr i) {
	#define max_shift (uptrbitsize-4)
    
	if (a->count == 0)
		return; // no need to update
    
	if (i > 0) {
		while (i > max_shift) {
			__decimal_shift_left(a, max_shift);
			i -= max_shift;
		}
		__decimal_shift_left(a, (uptr)i);
        return;
    }

    if (i < 0) {
		while (i < -(iptr)max_shift) {
			__decimal_shift_right(a, max_shift);
			i += max_shift;
		}
		__decimal_shift_right(a, (uptr)-i);
	}
}

bool __decimal_can_round_up(struct Decimal* a, iptr nd) {
	if (nd < 0 || nd >= a->count) return false;
	if (a->digits[nd] == '5' && nd+1 == a->count) {
		if (a->trunc)
			return true;
		return nd > 0 && (a->digits[nd-1]-'0')%2 != 0;
	}

	return a->digits[nd] >= '5';
}

u64 __decimal_rounded_integer(struct Decimal* a) {
	if (a->decimal_point > 20)
		return u64max;
	iptr i = 0;
	uptr n = 0;
	i32  m = min(a->decimal_point, a->count);
	for (; i < m; i += 1) {
		n = n*10 + (u64)(a->digits[i]-'0');
	}
	for (; i < a->decimal_point; i += 1) {
		n *= 10;
	}
	if (__decimal_can_round_up(a, a->decimal_point)) {
		n++;
	}
	return n;
}

#pragma endregion Decimal utilities

#pragma region Decimal to double (f64)

#define __f64_mantbits 52
#define __f64_expbits 11
#define __f64_bias -1023

f64 __decimal_to_double_end(struct Decimal* d, u64 mant, iptr exp) {
    u64 bits = mant & ((1ui64 << __f64_mantbits) - 1);
    bits |= (u64)((exp-__f64_bias) & ((1ui64<<__f64_expbits) - 1)) << __f64_mantbits;
    if (d->neg) bits |= (1ui64 << __f64_mantbits) << __f64_expbits;
    return *(f64*)&bits;
}

f64 __decimal_to_double_overflow_end(struct Decimal* d) {
    u64  mant = 0;
    iptr exp  = (1i64 << __f64_expbits) - 1 + __f64_bias;
    return __decimal_to_double_end(d, mant, exp);
}

#define __decimal_to_double_power_table_size 9
const iptr __decimal_to_double_power_table[__decimal_to_double_power_table_size] = {1, 3, 6, 9, 13, 16, 19, 23, 26};

f64 __decimal_to_double(struct Decimal* d, bool* overflow) {
    *overflow = false;

	if (d->count == 0)
		return __decimal_to_double_end(d, 0, __f64_bias);


	if (d->decimal_point > 310) {
        *overflow = true;
		return __decimal_to_double_overflow_end(d);
	}
    else if (d->decimal_point < -330)
		return __decimal_to_double_end(d, 0, __f64_bias);

	iptr exp = 0;
	while (d->decimal_point > 0) {
		iptr n = (d->decimal_point >= __decimal_to_double_power_table_size)
                    ? 27
                    : __decimal_to_double_power_table[d->decimal_point];
		__decimal_shift(d, -n);
		exp += n;
	}
	while (d->decimal_point < 0 || d->decimal_point == 0 && d->digits[0] < '5') {
		uptr n = -d->decimal_point >= __decimal_to_double_power_table_size
                    ? 27
                    : __decimal_to_double_power_table[-d->decimal_point];
        __decimal_shift(d, n);
		exp -= n;
	}

	// go from [0.5, 1) to [1, 2)
	exp--;

	// Min rep exp is 1+bias
	if (exp < __f64_bias + 1) {
		iptr n = __f64_bias + 1 - exp;
		__decimal_shift(d, -n);
		exp += n;
	}

	if ((exp-__f64_bias) >= ((1i64 << __f64_expbits) - 1)) {
        *overflow = true;
        return __decimal_to_double_overflow_end(d);
	}

	// Extract 1 + mantbits
	__decimal_shift(d, 1 + __f64_mantbits);
	u64 mant = __decimal_rounded_integer(d);

	// Rounding for shift down
	if (mant == 2ui64 << __f64_mantbits) {
		mant >>= 1;
		exp += 1;
		if ((exp-__f64_bias) >= ((1i64 << __f64_expbits) - 1)) {
            *overflow = true;
			return __decimal_to_double_overflow_end(d);
		}
	}

	// Check for denormalized mantissa
	if ((mant & (1ui64 << __f64_mantbits)) == 0) {
		exp = __f64_bias;
	}

	return __decimal_to_double_end(d, mant, exp);
}

#pragma endregion Decimal to double (f64)

bool __decimal_set(struct Decimal* d, u32* count, const rune* s, u32 len) {
    *count = 0;
	if (len == 0)
		return false;
    
    *d = (struct Decimal) {0};

	u32 i = 0;
	switch (s[i]) {
	case '+': i += 1; *count += 1; break;
	case '-': i += 1; d->neg = true; *count += 1; break;
	}

	// digits
	bool saw_dot = false;
	bool saw_digits = false;
	for (; i < len; i++) {
        rune r = s[i];
        if ('0' <= r && r <= '9') {
            *count += 1;
			saw_digits = true;
			if (r == '0' && d->count == 0) {
				d->decimal_point--;
				continue;
			}
			if (d->count < decimaldigitslen) {
				d->digits[d->count] = r;
				d->count++;
			}
            else if (r != '0')
				d->trunc = true;
            continue;
        }
		if (r == '.') {
            *count += 1;
			if (saw_dot)
				return false;
			saw_dot = true;
			d->decimal_point = d->count;
			continue;
        }
        break;
	}
    if (!saw_digits)
        return false;
	if (!saw_dot)
		d->decimal_point = d->count;

	return true;
}

#pragma endregion Decimal

//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
// MIS was written by Sergey Epishkin, and is placed in the public domain.
// The author hereby disclaims copyright to this source code.
//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////

typedef struct {
    union {
        f64 number;
        bool boolean;
        MISList list;
        MISObject object;
        const char* string;
        MISKeyword keyword;
    };
    enum {
        MISU_NULL,

        MISU_F64,
        MISU_BOOL,

        MISU_LIST,
        MISU_LIST_LINK,
        MISU_UNAVAILABLE_LIST,

        MISU_OBJECT,
        MISU_OBJECT_LINK,

        MISU_STRING,
        MISU_STRING_LINK,

        MISU_KEYWORD,
        MISU_KEYWORD_LINK,

        MISU_PROPERTY,
    } type;
} MISUniversal;

#pragma region List macroses

#define list_for(_type, _for, _align, ...) \
    _for { _type* ptr; u32 count, length; }; \
    bool __mis_ctn(__mis_ctn(__list, __VA_OPT__(__VA_ARGS__)), _append)(_for* list, _type value) { \
        if (list->length + 1 >= list->count) { \
            if (list->count == 0) list->count = 2; \
            else                  list->count = max(list->count * 2, list->count); \
            list->ptr = _aligned_realloc(list->ptr, list->count * sizeof(_type), _align); \
            if (!list->ptr) { \
                mis_fallback(__FILE__, __LINE__, "Out of memory"); \
                return false; \
            } \
        } \
        list->ptr[list->length++] = value; \
        return true; \
    } \
    bool __mis_ctn(__mis_ctn(__list, __VA_OPT__(__VA_ARGS__)), _append_refed)(_for* list, _type** ref, _type value) { \
        if (list->length + 1 >= list->count) { \
            if (list->count == 0) list->count = 2; \
            else                  list->count = max(list->count * 2, list->count); \
            list->ptr = _aligned_realloc(list->ptr, list->count * sizeof(_type), _align); \
            if (!list->ptr) { \
                mis_fallback(__FILE__, __LINE__, "Out of memory"); \
                return false; \
            } \
        } \
        _type* innerref = &list->ptr[list->length++]; \
        *ref = innerref; \
        *innerref = value; \
        return true; \
    } \
    bool __mis_ctn(__mis_ctn(__list, __VA_OPT__(__VA_ARGS__)), _prepare)(_for* list, u32 count) { \
        u32 new = count; \
        list->count += new; \
        list->ptr = _aligned_realloc(list->ptr, list->count*sizeof(_type), _align); \
        if (!list->ptr) { \
            mis_fallback(__FILE__, __LINE__, "Out of memory"); \
            return false; \
        } \
        return true; \
    }

#define __list_create(type) ((struct type) {0})
#define __list_pop(list) ((list)->ptr[(list)->length -= 1])
#define __list_clear(list) (list)->length = 0;
#define __list_free(list) do { if ((list).count) _aligned_free((list).ptr); } while (0)
#define __list_foreach(type, i, list) u32 __mis_ctn(__i, __LINE__) = 0; if ((list)->ptr) for (type i = *((list)->ptr); __mis_ctn(__i, __LINE__) < (list)->length; i = (list)->ptr[++__mis_ctn(__i, __LINE__)])
#pragma endregion List macroses

#pragma region Lists

list_for(MISUniversal, struct __MISListBase, uptrsize);
list_for(rune, struct RuneList, 4, _rune);
list_for(u32, struct U32List, 4, _u32);

#pragma endregion Lists

//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2024 Ginger Bill. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
//    contributors may be used to endorse or promote products derived from
//    this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////

// utf-8 encode/decode code is borrowed from Odin standard library but has been rewrited to better performance
#pragma region UTF-8 encode and decode

const u8 accept_sizes[256] = {
    // ascii,    size 1
    0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,
    0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,
    0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,
    0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,
    0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,
    0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,
    0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,
    0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,
    0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,
    0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,
    0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,
    0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,
    0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,
    // invalid,  size 1
    0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,
    0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,
    0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,
    0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,
    0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,
    0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,
    0xf1,0xf1,0xf1,0xf1,0xf1,
    // accept 1, size 2
    0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,
    0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,
    0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,
    // accept 1, size 3
    0x13,
    // accept 0, size 3
    0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,
    0x03,
    // accept 2, size 3
    0x23,
    // accept 0, size 3
    0x03,
    // accept 3, size 4
    0x34,
    // accept 0, size 4
    0x04,0x04,
    // accept 4, size 4
    0x44,
    // ascii,    size 4
    0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,0xf1
};

struct accr { u8 lo, hi; };

struct accr accept_ranges[5] = {
	{0x80, 0xbf},
	{0xa0, 0xbf},
	{0x80, 0x9f},
	{0x90, 0xbf},
	{0x80, 0x8f},
};

__inline rune swap_rune_endian(rune val) {
    return ((val << 24) & 0xFF000000UL) |
           ((val << 8)  & 0x00FF0000UL) |
           ((val >> 8)  & 0x0000FF00UL) |
           ((val >> 24) & 0x000000FFUL);
}

bool chars_to_runes(const char* s, const rune** result, u32* count, u32* ln, u32* cl) {
    #define ASCII 0x80
    
    u32 n = (u32)strlen(s);
    struct RuneList runes = __list_create(RuneList);

    bool in_str = false, esc = false;
    *ln = *cl = 1;
    *count = 0;

    for (u32 i = 0; i < n;) {
        char si = s[i];
        if (si < ASCII) { // ascii
            if (si == '\n') {
                __list_rune_append(&runes, '\n');
                *ln += 1;
                *cl = 1;
                i++;
                continue;
            }
            else if (si == '"') {
                if (!esc) in_str = !in_str;
            }
            else if (!in_str && si == '#') {
                if (i+9 >= n) {
                    __list_free(runes);
                    return false;
                }
                __list_rune_append(&runes, '#');

                rune low_byte;
                rune high_byte;

                // Eww... Alignment... ;(
                memcpy(&low_byte, &s[i+1], sizeof(rune));
                memcpy(&high_byte, &s[i+5], sizeof(rune));

                #ifdef mis_le
                __list_rune_append(&runes, low_byte);
                __list_rune_append(&runes, high_byte);
                #else
                __list_rune_append(&runes, swap_rune_endian(high_byte));
                __list_rune_append(&runes, swap_rune_endian(low_byte));
                #endif
                i += 9;
                *cl += 3;
                continue;
            }
            else if (si == '\\' && !esc) esc = true;
            else                         esc = false;
            __list_rune_append(&runes, (rune)si);
            i++;
            *cl += 1;
            continue;
        }
        esc = false;
        u8 x = accept_sizes[si];
        if (x == 0xf1) {
            __list_rune_append(&runes, (rune)si);
            i++;
            *cl += 1;
            continue;
        }
        u32 size = (u32)(x & 7);
        if (i+size > n) {
            __list_rune_append(&runes, (rune)si);
            i++;
            *cl += 1;
            continue;
        }
        struct accr ar = accept_ranges[x>>4];
        char b = s[i+1];
        if (b < ar.lo || ar.hi < b) {
            size = 1;
        } else if (size == 2) {
            // Okay
        } else {
            char c = s[i+2];
            if (c < 0x80 || 0xbf < c) {
                size = 1;
            } else if (size == 3) {
                // Okay
            } else {
                char d = s[i+3];
                if (d < 0x80 || 0xbf < d) {
                    size = 1;
                }
            }
        }
        #ifdef mis_x86_or_amd64
        switch (size) {
        case 1:
            __list_rune_append(&runes, (rune)si);
            break;
        case 2:
            __list_rune_append(&runes, (rune)*(u16*)&s[i]);
            break;
        case 3:
            __list_rune_append(&runes, (*(rune*)&s[i]) & 0xFFFFFF);
            break;
        case 4:
            __list_rune_append(&runes, *(rune*)&s[i]);
            break;
        }
        #elif defined(mis_le)
        switch (size) {
        case 1:
            __list_rune_append(&runes, (rune)si);
            break;
        case 2:
            __list_rune_append(&runes, (rune)si | ((rune)s[i+1] << 8));
            break;
        case 3:
            __list_rune_append(&runes, (rune)si | ((rune)s[i+1] << 8) | ((rune)s[i+2] << 16));
            break;
        case 4:
            __list_rune_append(&runes, (rune)si | ((rune)s[i+1] << 8) | ((rune)s[i+2] << 16) | ((rune)s[i+3] << 24));
            break;
        }
        #else
        switch (size) {
        case 1:
            __list_rune_append(&runes, (rune)si);
            break;
        case 2:
            __list_rune_append(&runes, ((rune)si << 8) | s[i+1]);
            break;
        case 3:
            __list_rune_append(&runes, ((rune)si << 16) | ((rune)s[i+1] << 8) | (rune)s[i+2]);
            break;
        case 4:
            __list_rune_append(&runes, ((rune)si << 24) | ((rune)s[i+1] << 16) | ((rune)s[i+2] << 8) | (rune)s[i+3]);
            break;
        }
        #endif
        i += size;
        *cl += 1;
    }
    *count  = runes.length;
    *result = runes.ptr;
    return true;
}

u8 encode_rune_size(rune r) {
	return r <= (1<<7)-1 ? 1
        : r <= (1<<11)-1 ? 2
        : r <= (1<<16)-1 ? 3
        :                4;
}

u8 encode_rune(rune r, char* buf) {
    u32 i = r;

	u8 const mask = 0x3f;
	if (i <= (1<<7)-1) {
		buf[0] = (u8)r;
		return 1;
	}
	if (i <= (1<<11)-1) {
		buf[0] = 0xc0 | (u8)(r>>6);
		buf[1] = 0x80 | (u8)r & mask;
		return 2;
	}

	// Invalid or Surrogate range
	if (i > 0x0010ffff || (0xd800 <= i && i <= 0xdfff))
		r = 0xfffd;

	if (i <= (1<<16)-1) {
		buf[0] = 0xe0 | (u8)(r>>12);
		buf[1] = 0x80 | (u8)(r>>6) & mask;
		buf[2] = 0x80 | (u8)r      & mask;
		return 3;
	}

	buf[0] = 0xf0 | (u8)(r>>18);
	buf[1] = 0x80 | (u8)(r>>12) & mask;
	buf[2] = 0x80 | (u8)(r>>6)  & mask;
	buf[3] = 0x80 | (u8)r       & mask;
	return 4;
}

char* runes_to_chars(const rune* runes, u32 runes_length) {
    u8 size;
	u32 byte_count = 0;
	for (u32 i = 0; i < runes_length; i++) {
		size = encode_rune_size(runes[i]);
		byte_count += size;
	}

	char* bytes = malloc(byte_count + 1);
    if (!bytes) return NULL;
    char* ptr = bytes;
	for (u32 i = 0; i < runes_length; i++)
		ptr += encode_rune(runes[i], ptr);
    *ptr = 0;

	return bytes;
}

#pragma endregion UTF-8 encode and decode

//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
// MIS was written by Sergey Epishkin, and is placed in the public domain.
// The author hereby disclaims copyright to this source code.
//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////

#pragma region Object dictionary
struct __MISObjectDictMember {
    const rune* key;
    u32 key_length;
    struct __MISListBase data;
    u32 next;
};

list_for(struct __MISObjectDictMember, struct __MISObjectDictMemberList, uptrsize, _member);

struct __MISObjectDictBuck {
    u32 first, last;
    u32 min_key, max_key;
};

struct __MISObjectDict {
    // count is total buckets count
    u32 count, bitcount;
    struct __MISObjectDictMemberList members_pool;
    struct __MISObjectDictBuck* ptr;
};

#pragma region Mur-Mur Hashing
// MurmurHash was written by Austin Appleby. Thank you, Appleby.

#define murmurrotl(v, r) ((v << r) | (v >> (32 - r)))

u32 murmur3utf8(const char* key, u32 len, u32 seed) {
    u32 n = len;
    u32 i;
    u32 h = seed;
    u32 c1 = 0xcc9e2d51;
    u32 c2 = 0x1b873593;
    rune k;

    #define murmur_push(v) \
        k = v; \
        k *= c1; \
        k = murmurrotl(k,15); \
        k *= c2; \
        h ^= k; \
        h = murmurrotl(h,13); \
        h = h*5+0xe6546b64

    for (i = 0; i < n;) {
        char si = key[i];
        if (si < ASCII) { // ascii
            murmur_push((rune)si);
            i++;
            continue;
        }
        u8 x = accept_sizes[si];
        if (x == 0xf1) {
            murmur_push((rune)si);
            i++;
            continue;
        }
        u32 size = (u32)(x & 7);
        if (i+size > n) {
            murmur_push((rune)si);
            i++;
            continue;
        }
        struct accr ar = accept_ranges[x>>4];
        char b = key[i+1];
        if (b < ar.lo || ar.hi < b) {
            size = 1;
        } else if (size == 2) {
            // Okay
        } else {
            char c = key[i+2];
            if (c < 0x80 || 0xbf < c) {
                size = 1;
            } else if (size == 3) {
                // Okay
            } else {
                char d = key[i+3];
                if (d < 0x80 || 0xbf < d) {
                    size = 1;
                }
            }
        }
        #ifdef mis_x86_or_amd64
        switch (size) {
            murmur_push((rune)si);
            break;
        case 2:
            murmur_push((rune)si | ((rune)key[i+1] << 8));
            break;
        case 3:
            murmur_push((rune)si | ((rune)key[i+1] << 8) | ((rune)key[i+2] << 16));
            break;
        case 4:
            murmur_push((rune)si | ((rune)key[i+1] << 8) | ((rune)key[i+2] << 16) | ((rune)key[i+3] << 24));
            break;
        }
        #else
        switch (size) {
        case 1:
            murmur_push((rune)si);
            break;
        case 2:
            murmur_push(((rune)si << 8) | key[i+1]);
            break;
        case 3:
            murmur_push(((rune)si << 16) | ((rune)key[i+1] << 8)  | (rune)key[i+2]);
            break;
        case 4:
            murmur_push(((rune)si << 24) | ((rune)key[i+1] << 16) | ((rune)key[i+2] << 8) | (rune)s[i+3]);
            break;
        }
        #endif
        i += size;
    }

    h ^= h >> 16;
    h *= 0x85ebca6b;
    h ^= h >> 13;
    h *= 0xc2b2ae35;
    h ^= h >> 16;

    return h;
}

u32 murmur3(const rune* key, i32 len, u32 seed) {
    i32 i;
    u32 h = seed;
    u32 c1 = 0xcc9e2d51;
    u32 c2 = 0x1b873593;

    for(i = 0; i < len; i++) {
        u32 k = key[i];
        
        k *= c1;
        k = murmurrotl(k,15);
        k *= c2;
        
        h ^= k;
        h = murmurrotl(h,13); 
        h = h*5+0xe6546b64;
    }

    h ^= h >> 16;
    h *= 0x85ebca6b;
    h ^= h >> 13;
    h *= 0xc2b2ae35;
    h ^= h >> 16;

    return h;
}

#pragma endregion Mur-Mur Hashing

#define pooled(dict, id) ((dict)->members_pool.ptr + id)

struct __MISListBase* __dict_lookup(struct __MISObjectDict* dict, const rune* key, u32 key_length) {
    if (!dict->ptr)
        return NULL;

    u32 hash = murmur3(key, key_length, 0);
    u32 id   = hash & (dict->count - 1);
	struct __MISObjectDictBuck buck = dict->ptr[id];
	if (buck.first == -1)
        return NULL;

    if (key_length < buck.min_key)
        return NULL;
    if (key_length > buck.max_key)
        return NULL;

    if (buck.last == buck.first)
        return &pooled(dict, buck.first)->data;

	struct __MISObjectDictMember* cur = pooled(dict, buck.first);
	while (1) {
	    if (cur->key_length != key_length) {
            if (cur->next == -1) return NULL;
            cur = pooled(dict, cur->next);
            continue;
        }
        if (0 != memcmp(key, cur->key, key_length * sizeof(rune))) {
            if (cur->next == -1) return NULL;
            cur = pooled(dict, cur->next);
            continue;
        }
        return &cur->data;
    }
}

struct __MISListBase* __dict_utf8lookup(struct __MISObjectDict* dict, const char* key, u32 key_length) {
    if (!dict->ptr)
        return NULL;

    u32 hash = murmur3utf8(key, key_length, 0);
    u32 id   = hash & (dict->count - 1);
	struct __MISObjectDictBuck buck = dict->ptr[id];
	if (buck.first == -1)
        return NULL;

    if (key_length < buck.min_key)
        return NULL;
    if (key_length > buck.max_key)
        return NULL;

    if (buck.last == buck.first)
        return &pooled(dict, buck.first)->data;

	struct __MISObjectDictMember* cur = pooled(dict, buck.first);
	while (1) {
	    if (cur->key_length != key_length) {
            if (cur->next == -1) return NULL;
            cur = pooled(dict, cur->next);
            continue;
        }
        if (0 != memcmp(key, cur->key, key_length * sizeof(rune))) {
            if (cur->next == -1) return NULL;
            cur = pooled(dict, cur->next);
            continue;
        }
        return &cur->data;
    }
}

#define MIS_OBJECT_INITAL_SIZE 256
#define MIS_OBJECT_INITAL_BITCOUNT 8

struct __MISListBase* __dict_insert(struct __MISObjectDict* dict, const rune* key, u32 key_length, struct __MISListBase value) {
    if (!dict->ptr) {
        dict->count = MIS_OBJECT_INITAL_SIZE;
        dict->bitcount    = MIS_OBJECT_INITAL_BITCOUNT;
        u32 len = MIS_OBJECT_INITAL_SIZE * sizeof(struct __MISObjectDictBuck);
        dict->ptr = _aligned_realloc(NULL, len, uptrsize);
        memset(dict->ptr, -1, len);
    }
    else {
        u32 length = dict->members_pool.length;
        if (length + 1 >= (dict->count >> 1)) {
            // Set members pool length to zero, data is still here, but will be rewrited
            __list_clear(&dict->members_pool);
            
            // Get old bucks
            struct __MISObjectDictBuck* last = dict->ptr;

            // Resize bucks
            dict->count *= 2;
            dict->bitcount++;
            u32 len = dict->count * sizeof(struct __MISObjectDictBuck);
            dict->ptr = _aligned_realloc(NULL, len, uptrsize);
            memset(dict->ptr, -1, len);
            
            // Iterate all old elements
            // data from dict->elems will be readed before rewriting
            for (u32 i = 0; i < length; i++) {
                struct __MISObjectDictMember e = dict->members_pool.ptr[i];
                __dict_insert(dict, e.key, e.key_length, e.data);
            }
        }
    }

    struct __MISObjectDictMember* member;
    #define new_member u32 member_id = dict->members_pool.length; do { \
            struct __MISObjectDictMember tmp_member = {0}; \
            tmp_member.next = u32max; \
            tmp_member.data = value; \
            tmp_member.key = key; \
            tmp_member.key_length = key_length; \
            if (!__list_member_append(&dict->members_pool, tmp_member)) \
                return NULL; \
            member = &dict->members_pool.ptr[dict->members_pool.length-1]; \
        } while (0)

    u32 hash = murmur3(key, key_length, 0);
    u32 id   = hash & (dict->count - 1);
	struct __MISObjectDictBuck* buck = &dict->ptr[id];

    if (buck->first == -1) {
        new_member;
        buck->last = buck->first = member_id;
        buck->min_key = buck->max_key = key_length;
        return &member->data;
    }

    if (key_length < buck->min_key) {
        new_member;
        pooled(dict, buck->last)->next = member_id;
        buck->last = member_id;
        buck->min_key = key_length;
        return &member->data;
    }
    if (key_length > buck->max_key || buck->max_key == -1) {
        new_member;
        pooled(dict, buck->last)->next = member_id;
        buck->last = member_id;
        buck->max_key = key_length;
        return &member->data;
    }
    
	struct __MISObjectDictMember* last = NULL;
	struct __MISObjectDictMember* cur = pooled(dict, buck->first);
	while (1) {
        last = cur;
	    if (cur->key_length != key_length) {
            if (cur->next == -1) break;
            cur = pooled(dict, cur->next);
            continue;
        }
        if (0 != memcmp(key, cur->key, key_length * sizeof(rune))) {
            if (cur->next == -1) break;
            cur = pooled(dict, cur->next);
            continue;
        }
        cur->data = value;
        return &cur->data;
    }

    new_member;
    buck->last = last->next = member_id;
    return &member->data;
}

#define __dict_free(dict) do { _aligned_free(dict->ptr); __list_free(dict->members_pool); } while (0)
#define __dict_create(type) ((struct type) {0})
#define __dict_foreach(k, k_len, val, dict) \
        const rune* k; \
        u32 k_len; \
        struct __MISListBase* val; \
        __list_foreach(struct __MISObjectDictMember, __mis_ctn(__e, __LINE__), &(dict)->members_pool) \
            if (k = __mis_ctn(__e, __LINE__).key, \
                k_len = __mis_ctn(__e, __LINE__).key_length, \
                val = &__mis_ctn(__e, __LINE__).data, 1)

#pragma endregion Object dictionary

#pragma region Parser structures
struct ParserValue {
    MISProperty unabled;
    MISList unabled_list;
};

struct ParserObject {
    MISObject obj;
    bool endable;
    MISProperty prop;
    const rune* prop_name;
    u32 prop_name_length;
    enum {
        ParserObject_property,
        ParserObject_property_stable,
        ParserObject_base
    } inside;
};

struct ParserList {
    MISList lst;
    bool endable;
    enum {
        ParserList_base,
        ParserList_stable
    } inside;
};

struct ParserContext {
    MISUniversal* to;
    bool done;
    union {
        struct ParserList list;
        struct ParserObject object;
        struct ParserValue value;
    };
    enum {
        PARS_LIST,
        PARS_OBJ,
        PARS_VAL
    } type;
};
#pragma endregion Parser structures

void __mis_free_list(MISList val, has_caller_location);
void __mis_free_object(MISObject val, has_caller_location);
void __mis_free_uni(MISUniversal val, has_caller_location);

#pragma region MIS parse from
MISUniversal __parse(MISParser prs, MISParserRootType type, MISParseFallbackType fallback, has_caller_location);

bool __mis_parse_source_list(MISListContainer* list, const char* source, const char* file, MISParseFallbackType fallback, has_caller_location) {
    u32 ln, cl, length;
    rune* runes;
    if (!chars_to_runes(source, &runes, &length, &ln, &cl)) {
        fallback((MISParser) { pass_caller_location, NULL, 0, file, 0, ln, cl }, caller_location, "Invalid fast number is founded");
        return false;
    }
    MISParser prs = { pass_caller_location, runes, length, file, 0, 1, 1 };
    MISUniversal uni = __parse(prs, MIS_OBJECT, fallback, pass_caller_location);
    if (uni.type == MISU_NULL) {
        list->value = NULL;
        return false;
    }
    switch (uni.type) {
    case MISU_OBJECT:
        mis_free(uni.object);
        break;
    case MISU_LIST:
        list->value = uni.list;
        list->container = runes;
        return true;
    case MISU_STRING:
        free((void*)uni.string);
        break;
    case MISU_KEYWORD:
        free((void*)uni.keyword.value);
        break;
    }
    return false;
}

bool __mis_parse_file_list(MISListContainer* list, const char* file, MISParseFallbackType fallback, has_caller_location) {
    FILE* fi = fopen(file, "rb");
    if (!fi) {
        fallback((MISParser) { pass_caller_location, NULL, 0, file, 0, 1, 1 }, caller_location, "Rejected to read file \"%s\"", file);
        return false;
    }
    fseek(fi, 0, SEEK_END);
    u32 len = ftell(fi);
    rewind(fi);
    char* data = malloc(len);
    fread(data, 1, len, fi);
    fclose(fi);
    bool res = __mis_parse_source_list(list, data, file, fallback, pass_caller_location);
    free(data);
    return res;
}

bool __mis_parse_source_object(MISObjectContainer* object, const char* source, const char* file, MISParseFallbackType fallback, has_caller_location) {
    u32 ln, cl, length;
    rune* runes;
    if (!chars_to_runes(source, &runes, &length, &ln, &cl)) {
        fallback((MISParser) { pass_caller_location, NULL, 0, file, 0, ln, cl }, caller_location, "Invalid fast number is founded");
        return false;
    }
    MISParser prs = { pass_caller_location, runes, length, file, 0, 1, 1 };
    MISUniversal uni = __parse(prs, MIS_OBJECT, fallback, pass_caller_location);
    if (uni.type == MISU_NULL) {
        object->value = NULL;
        return false;
    }
    switch (uni.type) {
    case MISU_OBJECT:
        object->value = uni.object;
        object->container = runes;
        return true;
    case MISU_LIST:
        mis_free(uni.list);
        break;
    case MISU_STRING:
        free((void*)uni.string);
        break;
    case MISU_KEYWORD:
        free((void*)uni.keyword.value);
        break;
    }
    return false;
}

bool __mis_parse_file_object(MISObjectContainer* object, const char* file, MISParseFallbackType fallback, has_caller_location) {
    FILE* fi = fopen(file, "rb");
    if (!fi) {
        fallback((MISParser) { pass_caller_location, NULL, 0, file, 0, 1, 1 }, caller_location, "Rejected to read file \"%s\"", file);
        return false;
    }
    fseek(fi, 0, SEEK_END);
    u32 len = ftell(fi);
    rewind(fi);
    char* data = malloc(len);
    fread(data, 1, len, fi);
    fclose(fi);
    bool res = __mis_parse_source_object(object, data, file, fallback, pass_caller_location);
    free(data);
    return res;
}
#pragma endregion MIS parse from

#pragma region MIS free resource
void __mis_free_list(struct __MISListBase* val, has_caller_location) {
    __list_foreach(MISUniversal, a, val) __mis_free_uni(a, pass_caller_location);
    __list_free(*val);
    _aligned_free(val);
}

void __mis_free_object(MISObject val, has_caller_location) {
    __dict_foreach(key, key_length, elem, val) {
        __list_foreach(MISUniversal, uni, elem) {
            __mis_free_uni(uni, pass_caller_location);
        }
        __list_free(*elem);
    }
    __dict_free(val);
    _aligned_free(val);
}

void __mis_free_uni(MISUniversal val, has_caller_location) {
    switch (val.type) {
    case MISU_STRING:
        free((void*)val.string);
        break;
    case MISU_LIST:
        __mis_free_list(val.list, pass_caller_location);
        break;
    case MISU_OBJECT:
        __mis_free_object(val.object, pass_caller_location);
        break;
    case MISU_KEYWORD:
        free((void*)val.keyword.value);
        break;
    }
}

#pragma endregion MIS free resource

#pragma region Symbols specification
#define isL(c) (c <= 0x10FFFF && (symbols_flags_trie[symbols_flags_trie_leaf_addresses[symbols_flags_trie[symbols_flags_trie_main_addresses[symbols_flags_trie[(rune)c&0xFF]] + (((rune)c>>8)&0xFF)]] + (((rune)c>>16)&0xFF)]&1)!=0)
#define isWS(c) (c <= 0x10FFFF && (symbols_flags_trie[symbols_flags_trie_leaf_addresses[symbols_flags_trie[symbols_flags_trie_main_addresses[symbols_flags_trie[(rune)c&0xFF]] + (((rune)c>>8)&0xFF)]] + (((rune)c>>16)&0xFF)]&2)!=0)

#define isalpha(sym)   isL(sym)
#define isspace(sym)   isWS(sym)
#define is_alpha(sym)  isL(sym)
#define is_space(sym)  isWS(sym)
#define isnumeric(sym) (sym >= '0' && sym <= '9')
#pragma endregion Symbols specification

#pragma region Parser functions

#pragma region Range check parser functions

__inline bool is_end(MISParser* prs) {
    return prs->i >= prs->source_length;
}

#define inrange(prs, i) (rune)(i >= prs->source_length ? 0 : prs->source[i])

enum {
    END_CHECK_none,
    END_CHECK_eof,
    END_CHECK_done
};

#define end_check(v, end, prs) (\
        v->endable ? ( \
            current(prs, end) ? END_CHECK_done : \
            is_end(prs) ? END_CHECK_eof : END_CHECK_none \
        ) : is_end(prs) ? END_CHECK_done : END_CHECK_none \
    )

#pragma endregion Range check parser functions

__inline bool skipspaces(MISParser* prs, MISParseFallbackType fallback, has_caller_location) {
    while (true) {
        rune c = inrange(prs, prs->i);
        if (c == '\n') {
            prs->ln++;
            prs->cl = 1;
            prs->i++;
        }
        else if (isspace(c)) {
            prs->cl++;
            prs->i++;
        }
        else if (c == '{') {
            prs->i++;
            u32 cnt = 1;
            while (cnt != 0) {
                if (is_end(prs)) {
                    fallback(*prs, pass_caller_location, "Comment is not closed by '}'");
                    return true;
                }
                c = inrange(prs, prs->i);
                if (c == '{') cnt++;
                else if (c == '}') cnt--;
                else if (c == '\n') {
                    prs->cl = 0;
                    prs->ln++;
                }
                prs->cl++;
                prs->i++;
            }
        }
        else return false;
    }
    return false;
}

__inline bool current(MISParser* prs, rune r) {
    if (is_end(prs)) return false;
    if (prs->source[prs->i] == r) {
        prs->cl++;
        prs->i++;
        return true;
    }
    return false;
}

__inline bool get_string(MISParser* prs, const rune** result, u32* length, bool* must_panic, MISParseFallbackType fallback, has_caller_location) {
    *must_panic = false;
    {
        if (inrange(prs, prs->i) != '"') return false;
        prs->cl++;
        prs->i++;
    }

    *must_panic = true;

    u32 start = prs->i;
    u32 specials = 0;
    bool last_special = false;
    while (true) {
        if (is_end(prs)) {
            fallback(*prs, pass_caller_location, "String is not closed by '\"'");
            return true;
        }
        rune c = inrange(prs, prs->i);
        if (last_special) specials++;
        else if (c == '"') {
            prs->i++;
            break;
        }
        if (last_special) last_special = false;
        else              last_special = c == '\\';

        prs->cl++;
        prs->i++;
    }
    last_special = false;
    *length = prs->i - start - specials - 1;
    rune* runes = _aligned_malloc(*length * sizeof(rune), sizeof(rune));
    for (int i = start, j = 0; i < prs->i-1; i++) {
        rune c = prs->source[i];
        if (last_special) {
            #define match(ch, rl) case ch: runes[j] = rl; break

            switch (c) {
            match('n', '\n');
            match('t', '\t');
            match('b', '\b');
            match('r', '\r');
            match('a', '\a');
            match('"', '\"');
            match('?', '\?');
            match('\\', '\\');
            match('f', '\f');
            match('v', '\v');
            match('0', 0);
            default:
                _aligned_free(runes);
                fallback(*prs, pass_caller_location, "Invalid escape character '\\%c'", c);
                return true;
            }
        }
        else runes[j] = c;
        if (last_special) last_special = false;
        else              last_special = c == '\\';
        if (!last_special) j++;
    }
    *result = runes;
    *must_panic = false;
    return true;
}

__inline bool get_keyword(MISParser* prs, const rune** result, u32* length) {
    u32 start = prs->i;
    {
        rune s = inrange(prs, prs->i);
        if (!isalpha(s) && s != '_') return false;
        prs->cl++;
        prs->i++;
    }
    while (true) {
        rune c = inrange(prs, prs->i);
        bool alph = !isalpha(c);
        bool num = !isnumeric(c);
        bool down = c != '_';
        if (alph && num && down) break;
        prs->cl++;
        prs->i++;
    }
    *result = &prs->source[start];
    *length = prs->i - start;
    return true;
}

enum get_number { get_number_fast, get_number_normal, get_number_mimic, get_number_overflow };

__inline bool get_number(MISParser* prs, enum get_number* type, f64* result) {
    u32 start = prs->i;
    {
        rune s = inrange(prs, prs->i);
        if (s == '#') {
            #ifdef mis_x86_or_amd64
            f64 tmp = *(f64*)&prs->source[prs->i+1];
            #else
            rune a = prs->source[prs->i+1];
            rune b = prs->source[prs->i+2];
            u64  c = a & (b << 32);
            f64 tmp = *(f64*)&c;
            #endif
            prs->cl += 3;
            prs->i += 3;
            *result = tmp;
            *type = get_number_fast;
            return true;
        }

        if (!isnumeric(s)) {
            rune r = inrange(prs, prs->i+1);
            if (!((s == '.' || s == '-') && isnumeric(r))) return false;
        }
    }

    u32 count;
    struct Decimal decimal;
    
    *type = __decimal_set(&decimal, &count, &prs->source[start], prs->source_length-start) ? get_number_normal : get_number_mimic;
    prs->i += count;
    prs->cl += count;

    bool overflow;
    *result = __decimal_to_double(&decimal, &overflow);
    if (overflow) *type = get_number_overflow;
    return true;
}

bool get_index(MISParser* prs, u32* result) {
    u32 start = prs->i;
    {
        rune s = inrange(prs, prs->i);
        if (!isnumeric(s)) return false;
        *result = s - '0';
        prs->cl++;
        prs->i++;
    }
    while (true) {
        rune c = inrange(prs, prs->i);
        if (!isnumeric(c)) break;
        *result = *result * 10 + (c - '0');
        prs->cl++;
        prs->i++;
    }
    return true;
}
    
#pragma endregion Parser functions

list_for(struct ParserContext, struct ParserContextList, uptrsize, _ctx);

MISUniversal __parse(MISParser prs, MISParserRootType type, MISParseFallbackType fallback, has_caller_location) {
    #define nulluniversal (MISUniversal) {0}

    #define std_parser_list(...) (struct ParserList) { \
            _aligned_recalloc(NULL, 1, sizeof(struct __MISListBase), uptrsize), \
            __VA_OPT__(1?__VA_ARGS__:)true, \
            ParserList_base \
        }

    #define std_parser_object(...) (struct ParserObject) { \
            _aligned_recalloc(NULL, 1, sizeof(struct __MISObjectDict), uptrsize), \
            __VA_OPT__(1?__VA_ARGS__:)true, \
            (MISProperty) {0}, \
            NULL, \
            0, \
            ParserObject_base \
        }

    #define fall_cleanup do { \
            _aligned_free(prs.source); \
            __list_free(ctxs); \
            __mis_free_uni(res, pass_caller_location); \
        } while (0)

    #define __err(...) fallback(prs, caller_location, __VA_ARGS__)

    MISUniversal res = nulluniversal;
    struct ParserContextList ctxs = __list_create(ParserContextList);
    __list_ctx_prepare(&ctxs, 4);
    struct ParserContext ctx;
    ctx.to = &res;
    ctx.done = false;
    if (type == MIS_OBJECT) {
        ctx.type = PARS_OBJ;
        ctx.object = std_parser_object(false);
        if (!ctx.object.obj)
            fall_cleanup;
    }
    else {
        ctx.type = PARS_LIST;
        ctx.list = std_parser_list(false);
        if (!ctx.list.lst)
            fall_cleanup;
    }

    while (1) {
        next_state:
        #define NEXT_state goto next_state

        if (ctx.done) {
            if (ctxs.length == 0) break;
            ctxs.length--;
            ctx = ctxs.ptr[ctxs.length];
        }

        if (skipspaces(&prs, fallback, pass_caller_location)) {
            if (ctx.type == PARS_VAL && ctx.value.unabled_list) __mis_free_list(ctx.value.unabled_list, caller_location);
            fall_cleanup;
            return nulluniversal;
        }

        switch (ctx.type) {
        case PARS_LIST: {
            #define list_fail do {\
                    __mis_free_list(v->lst, caller_location); \
                    fall_cleanup; \
                    return nulluniversal; \
                } while (0)
            #define list_good do { \
                    ctx.to->list = v->lst; \
                    ctx.to->type = MISU_LIST; \
                    ctx.done = true; \
                    NEXT_state; \
                } while (0)

            struct ParserList* v = &ctx.list; 
            switch (end_check(v, ')', &prs)) {
            case END_CHECK_eof:
                __err("List expecting ',' between values or ')' on end");
                list_fail;
            case END_CHECK_done:
                list_good;
            }

            bool inter = false;
            switch (v->inside) {
            case ParserList_base:
                v->inside = ParserList_stable;
                inter = true;
            case ParserList_stable:
                if (inter) {
                    if (current(&prs, ')'))
                        list_good;
                }
                else if (current(&prs, ',')) {
                    if (skipspaces(&prs, fallback, caller_location))
                        list_fail;
                    if (current(&prs, ')'))
                        list_good;
                }
                else {
                    __err("List expecting ',' between values");
                    list_fail;
                }

                ctx.to->type = MISU_UNAVAILABLE_LIST;
                if (!__list_ctx_append(&ctxs, ctx))
                    list_fail;
                if (!__list_append_refed(v->lst, &ctx.to, nulluniversal))
                    list_fail;
                ctx.done = false;
                ctx.type = PARS_VAL;
                ctx.value.unabled = NULL;
                ctx.value.unabled_list = v->lst;

                NEXT_state;
            }
        } break;
        case PARS_OBJ: {
            struct ParserObject* v = &ctx.object; 
            bool inter = false;
            switch (v->inside) {
            case ParserObject_base:
                switch (end_check(v, ']', &prs)) {
                case END_CHECK_eof:
                    __err("Object expecting ';' between properties or ']' on end");
                    fall_cleanup;
                    return nulluniversal;
                case END_CHECK_done:
                    ctx.to->object = v->obj;
                    ctx.to->type = MISU_OBJECT;
                    ctx.done = true;
                    NEXT_state;
                }

                const rune* kw;
                u32 kw_len;
                bool has = get_keyword(&prs, &kw, &kw_len);
                if (!has) {
                    __err("Object expecting parameter name");
                    fall_cleanup;
                    return nulluniversal;
                }
                if (skipspaces(&prs, fallback, caller_location)) {
                    fall_cleanup;
                    return nulluniversal;
                }
                if (!current(&prs, ':')) {
                    char* str = runes_to_chars(kw, kw_len);
                    __err("Object expecting ':' after parameter \"%s\"", str);
                    free(str);
                    fall_cleanup;
                    return nulluniversal;
                }
                if (skipspaces(&prs, fallback, caller_location)) {
                    fall_cleanup;
                    return nulluniversal;
                }
                struct __MISListBase* member =  __dict_lookup(v->obj, kw, kw_len);
                if (member != NULL) {
                    char* str = runes_to_chars(kw, kw_len);
                    __err("Object cannot have properties with same name \"%s\"", str);
                    free(str);
                    fall_cleanup;
                    return nulluniversal;
                }
                struct __MISListBase* to = __dict_insert(v->obj, kw, kw_len, __list_create(__MISListBase));
                v->inside = ParserObject_property;
                v->prop_name = kw;
                v->prop_name_length = kw_len;
                v->prop = to;
                NEXT_state;
            case ParserObject_property:
                v->inside = ParserObject_property_stable;
                inter = true;
            case ParserObject_property_stable:
                ctx.to->object = v->obj;
                ctx.to->type = MISU_OBJECT;
                if (inter) {
                    if (current(&prs, ';')) {
                        char* str = runes_to_chars(v->prop_name, v->prop_name_length);
                        __err("Property \"%s\" is totaly empty", str);
                        free(str);
                        fall_cleanup;
                        return nulluniversal;
                    }
                }
                else if (!current(&prs, ',')) {
                    if (current(&prs, ';')) {
                        v->inside = ParserObject_base;
                        NEXT_state;
                    }
                    printf("%c\n", prs.source[prs.i]);
                    char* str = runes_to_chars(v->prop_name, v->prop_name_length);
                    __err("Property \"%s\" expecting ',' between values", str);
                    free(str);
                    fall_cleanup;
                    return nulluniversal;
                }
                if (!__list_ctx_append(&ctxs, ctx)) {
                    fall_cleanup;
                    return nulluniversal;
                }
                if (!__list_append_refed(v->prop, &ctx.to, nulluniversal)) {
                    fall_cleanup;
                    return nulluniversal;
                }
                ctx.done = false;
                ctx.type = PARS_VAL;
                ctx.value.unabled = v->prop;
                ctx.value.unabled_list = NULL;

                NEXT_state;
            }
        } break;
        case PARS_VAL: {
            #define try_free_list(lst) do { \
                    if (lst) __mis_free_list(lst, caller_location); \
                } while (0)

            struct ParserValue* v = &ctx.value; 

            if (current(&prs, ',')) {
                try_free_list(v->unabled_list);
                __err("Comma cannot be in start of list, remove it");
                fall_cleanup;
                return nulluniversal;
            }

            if (current(&prs, '[')) {
                ctx.object = std_parser_object();
                if (!ctx.object.obj) {
                    try_free_list(v->unabled_list);
                    fall_cleanup;
                }
                ctx.type = PARS_OBJ;

                ctx.to->object = ctx.object.obj;
                ctx.to->type = MISU_OBJECT;
                NEXT_state;
            }

            if (current(&prs, '(')) {
                ctx.list = std_parser_list();
                if (!ctx.list.lst) {
                    try_free_list(v->unabled_list);
                    fall_cleanup;
                }
                ctx.type = PARS_LIST;
                NEXT_state;
            }

            f64 num;
            enum get_number num_type;
            bool isnum = get_number(&prs, &num_type, &num);
            if (isnum) {
                switch (num_type)
                {
                case get_number_mimic:
                    __err("Founded invalid number-like value");
                    try_free_list(v->unabled_list);
                    fall_cleanup;
                    return nulluniversal;
                case get_number_overflow:
                    __err("Founded too big number value");
                    try_free_list(v->unabled_list);
                    fall_cleanup;
                    return nulluniversal;
                default:
                    break;
                }
                ctx.to->number = num;
                ctx.to->type = MISU_F64;
                ctx.done = true;
                NEXT_state;
            }

            u32 kw_len;
            const rune* kw;
            bool iskw = get_keyword(&prs, &kw, &kw_len);
            if (iskw) {
                if (    kw_len == 4 &&
                        kw[0] == 't' &&
                        kw[1] == 'r' &&
                        kw[2] == 'u' &&
                        kw[3] == 'e') {
                    ctx.to->boolean = true;
                    ctx.to->type = MISU_BOOL;
                }
                else if (kw_len == 5 &&
                         kw[0] == 'f' &&
                         kw[1] == 'a' &&
                         kw[2] == 'l' &&
                         kw[3] == 's' &&
                         kw[4] == 'e') {
                    ctx.to->boolean = false;
                    ctx.to->type = MISU_BOOL;
                }
                else {
                    ctx.to->keyword.value = runes_to_chars(kw, kw_len);
                    ctx.to->type = MISU_KEYWORD;
                }
                ctx.done = true;
                NEXT_state;
            }

            bool must_panic;
            u32 str_len;
            rune* str;
            bool isstr = get_string(&prs, &str, &str_len, &must_panic, fallback, caller_location);
            if (isstr) {
                if (must_panic) {
                    try_free_list(v->unabled_list);
                    fall_cleanup;
                    return nulluniversal;
                }
                ctx.to->string = runes_to_chars(str, str_len);
                ctx.to->type = MISU_STRING;
                _aligned_free(str);
                ctx.done = true;
                NEXT_state;
            }
            
            if (current(&prs, '>')) {
                #define check(to, cur) do {\
                        to = false; \
                        switch (cur.type) { \
                        case MISU_PROPERTY: \
                            if (v->unabled == cur.list) { \
                                __err("Property cannot reference itself"); \
                                try_free_list(v->unabled_list); \
                                fall_cleanup; \
                                to = true; \
                                break; \
                            } \
                            break; \
                        case MISU_UNAVAILABLE_LIST: \
                            __err("List cannot reference itself"); \
                            try_free_list(v->unabled_list); \
                            fall_cleanup; \
                            to = true; \
                            break; \
                        } \
                    } while (0)

                bool ch;
                MISUniversal cur = res;

                { // BEGIN loop
                    next_loop:
                    #define NEXT_loop goto next_loop

                    check(ch, cur);
                    if (ch) {
                        return nulluniversal;
                    }
                    if (skipspaces(&prs, fallback, caller_location)) {
                        try_free_list(v->unabled_list);
                        fall_cleanup;
                        return nulluniversal;
                    }
                    #define tani "Trying to access named indexation in "
                    #undef  match
                    #define match(_case, _name) \
                        _case: \
                            __err(_name); \
                            try_free_list(v->unabled_list); \
                            fall_cleanup; \
                            return nulluniversal
                    
                    u32 kw_len;
                    const rune* kw;
                    bool iskw = get_keyword(&prs, &kw, &kw_len);
                    if (iskw) {
                        switch (cur.type) {
                        case MISU_PROPERTY:
                            cur = cur.list->ptr[0];
                        case MISU_OBJECT_LINK:
                        case MISU_OBJECT:
                            cur.list = __dict_lookup(cur.object, kw, kw_len);
                            cur.type = MISU_PROPERTY;
                            if (!cur.list) {
                                char* str = runes_to_chars(kw, kw_len);
                                try_free_list(v->unabled_list);
                                __err("Trying to access not available name \"%s\" in object", str);
                                free(str);
                                fall_cleanup;
                                return nulluniversal;
                            }
                            NEXT_loop;
                        match(case MISU_F64, tani "number");
                        match(case MISU_BOOL, tani "boolean");

                        match(case MISU_LIST, tani "list");
                        match(case MISU_LIST_LINK, tani "list");

                        match(case MISU_STRING, tani "string");
                        match(case MISU_STRING_LINK, tani "string");

                        match(case MISU_KEYWORD, tani "keyword");
                        match(case MISU_KEYWORD_LINK, tani "keyword");

                        match(case MISU_UNAVAILABLE_LIST, "List cannot reference itself");
                        match(default, "Undefined branch");
                        }
                    }

                    u32 idx;
                    bool isidx = get_index(&prs, &idx);
                    if (isidx) {
                        switch (cur.type) {
                        case MISU_PROPERTY:
                            if (idx == 0) {
                                __err("Trying to access not available index 0 in property. MIS links indexation starts from 1");
                                try_free_list(v->unabled_list);
                                fall_cleanup;
                                return nulluniversal;
                            }
                            idx--;
                            
                            if (idx >= (u32)cur.list->length) {
                                __err("Trying to access not available index %u in property", idx);
                                try_free_list(v->unabled_list);
                                fall_cleanup;
                                return nulluniversal;
                            }
                            cur = cur.list->ptr[idx];
                            NEXT_loop;
                        case MISU_LIST_LINK:
                        case MISU_LIST:
                            if (idx == 0) {
                                __err("Trying to access not available index 0 in list. MIS links indexation starts from 1");
                                try_free_list(v->unabled_list);
                                fall_cleanup;
                                return nulluniversal;
                            }
                            idx--;
                        
                            if (idx >= (u32)cur.list->length) {
                                __err("Trying to access not available index %u in list", idx);
                                try_free_list(v->unabled_list);
                                fall_cleanup;
                                return nulluniversal;
                            }
                            cur = cur.list->ptr[idx];
                            NEXT_loop;
                        match(case MISU_F64, tani "number");
                        match(case MISU_BOOL, tani "boolean");
                        
                        match(case MISU_OBJECT, tani "object");
                        match(case MISU_OBJECT_LINK, tani "object");
                        
                        match(case MISU_STRING, tani "string");
                        match(case MISU_STRING_LINK, tani "string");
                        
                        match(case MISU_KEYWORD, tani "keyword");
                        match(case MISU_KEYWORD_LINK, tani "keyword");

                        match(case MISU_UNAVAILABLE_LIST, "List cannot reference itself");
                        match(default, "Undefined branch");
                        }
                    }
                } // END loop

                check(ch, cur);
                if (ch) return nulluniversal;
                switch (cur.type)
                {
                case MISU_LIST:    cur.type = MISU_LIST_LINK;    break;
                case MISU_OBJECT:  cur.type = MISU_OBJECT_LINK;  break;
                case MISU_STRING:  cur.type = MISU_STRING_LINK;  break;
                case MISU_KEYWORD: cur.type = MISU_KEYWORD_LINK; break;
                }
                ctx.done = true;
                NEXT_state;
            }

            __err("Invalid value founded");
            try_free_list(v->unabled_list);
            fall_cleanup;
            return nulluniversal;
        } break;
        }
        __err("End is reached. Is not possible if there's no bugs in library");
        fall_cleanup;
        return nulluniversal;
    }

    __list_free(ctxs);
    return res;
}

#pragma endregion MIS parser


///////////////////////////////////////////////////
// ---------------- MIS GETTERS ---------------- //
///////////////////////////////////////////////////

#define __on(_do, ...) __VA_OPT__(_do)
#define __opt(on, ...) __on(__VA_ARGS__, on)
#define __pass_args(...) __VA_OPT__(, __VA_ARGS__)
#define __variant_contain(def, ctn_type, val_type, val, others, ...) def(val_type val others) __VA_ARGS__ __mis_ctn(def, _container)(ctn_type __mis_ctn(__, __mis_ctn(val, __LINE__)) others) { val_type val = __mis_ctn(__, __mis_ctn(val, __LINE__)).value; __VA_ARGS__ }
#define __get_for_from(name, _for, cast, from, from_other, from_val) __variant_contain(int __mis_ctn(__mis_get_, name), MISListContainer, struct __MISListBase*, list, __pass_args(int i, _for* result), { \
        MISUniversal uni = list->ptr[i]; \
        if (uni.type != from __opt(from_other, && uni.type != from_other)) return false; \
        *result = __opt(cast, (cast))uni.from_val; \
        return true; \
    })

__variant_contain(MISProperty __mis_extract, MISObjectContainer, MISObject, object, __pass_args(const char* key), {
    return __dict_utf8lookup(object, key, (int)strlen(key));
})

__variant_contain(MISProperty __mis_extract_length, MISObjectContainer, MISObject, object, __pass_args(const char* key, int key_length), {
    return __dict_utf8lookup(object, key, key_length);
})

__variant_contain(int __mis_len, MISListContainer, struct __MISListBase*, list,, {
    return list->length;
})

__get_for_from(int8,   i8,  i8,  MISU_F64,, number)
__get_for_from(int16,  i16, i16, MISU_F64,, number)
__get_for_from(int32,  i32, i32, MISU_F64,, number)
__get_for_from(int64,  i64, i64, MISU_F64,, number)
__get_for_from(uint8,  u8,  u8,  MISU_F64,, number)
__get_for_from(uint16, u16, u16, MISU_F64,, number)
__get_for_from(uint32, u32, u32, MISU_F64,, number)
__get_for_from(uint64, u64, u64, MISU_F64,, number)
__get_for_from(float,  f32, f32, MISU_F64,, number)
__get_for_from(double, f64, f64, MISU_F64,, number)

__get_for_from(boolean, bool,        bool,      MISU_BOOL,    ,                  boolean)
__get_for_from(list,    MISList,     MISList,   MISU_LIST,    MISU_LIST_LINK,    list)
__get_for_from(object,  MISObject,   MISObject, MISU_OBJECT,  MISU_OBJECT_LINK,  object)
__get_for_from(keyword, MISKeyword,  ,          MISU_KEYWORD, MISU_KEYWORD_LINK, keyword)
__get_for_from(string,  const char*, ,          MISU_STRING,  MISU_STRING_LINK,  string)

////////////////////////////////////////////
// ---------------- TEST ---------------- //
////////////////////////////////////////////

#pragma region Print MIS resource

void print_uni(MISUniversal uni, u32 ident);

#define ident_print for (u32 i = 0; i < ident; i++) printf(" ")

void print_list(MISList list, u32 ident) {
    printf("(\n");
    ident++;
    __list_foreach(MISUniversal, e, list) {
        ident_print;
        for (u32 i = 0; i < ident; i++)
            putc(' ', stdout);
        print_uni(e, ident+1);
        printf(",\n");
    }
    ident--;
    ident_print;
    printf(")");
}

void print_object(MISObject object, u32 ident) {
    printf("[\n");
    ident++;
    __dict_foreach(key, key_len, prop, object) {
        ident_print;
        for (u32 i = 0; i < key_len; i++)
            putwchar(key[i]);
        putchar(':');
        bool first = true;
        __list_foreach(MISUniversal, uni, prop) {
            if (!first) putchar(',');
            putchar(' ');
            first = false;
            print_uni(uni, ident+1);
        }
        printf(";\n");
    }
    ident--;
    ident_print;
    printf("]");
}

void print_uni(MISUniversal uni, u32 ident) {
    switch (uni.type)
    {
    case MISU_BOOL:
        if (uni.boolean) printf("(bool) true");
        else             printf("(bool) false");
        break;
    case MISU_F64:
        printf("(number) ");

        char str[40];
        u32 count = d2fixed_buffered_n(uni.number, 5, str);
        u32 i;
        for (i = count-1; str[i] == '0'; i--, count--) {}
        if (str[i] == '.') count--;

        for (u32 i = 0; i < count; i++)
            printf("%c", str[i]);
        break;
    case MISU_KEYWORD:
    case MISU_KEYWORD_LINK:
        printf("(keyword) ");

        printf(uni.keyword.value);
        break;
    case MISU_STRING:
    case MISU_STRING_LINK:
        printf("(string) ");

        printf("\"%s\"", uni.string);
        break;
    case MISU_LIST:
    case MISU_LIST_LINK:
        print_list(uni.list, ident);
        break;
    case MISU_OBJECT:
    case MISU_OBJECT_LINK:
        print_object(uni.object, ident);
        break;
    
    default:
        break;
    }
}

#pragma endregion Print MIS resource


#if defined(_WIN32) || defined(_WIN64)
extern int __stdcall QueryPerformanceFrequency(u64* lpFrequency);
extern int __stdcall QueryPerformanceCounter(u64* lpPerformanceCount);

typedef struct { u64 start, end, frc; } Bcmk;

Bcmk bcmk_create() {
    Bcmk bcmk;
    QueryPerformanceFrequency(&bcmk.frc);
    return bcmk;
}

void bcmk_start_in(u64* bcmk) {
   QueryPerformanceCounter(bcmk);
}

void bcmk_end_in(u64* bcmk) {
    QueryPerformanceCounter(bcmk);
}

void bcmk_start(Bcmk* bcmk) {
    QueryPerformanceCounter(&bcmk->start);
}

void bcmk_end(Bcmk* bcmk) {
    QueryPerformanceCounter(&bcmk->end);
}
#endif

#define let auto
#define assert(a, ...) do { if (!(a)) {mis_fallback(caller_location, __VA_ARGS__); } } while (0)
#define begin_test(name) puts("==== TEST '"name"'"); bcmk_start(&bcmk)
#define end_test(name) bcmk_end(&bcmk); puts(result); printf("==== TEST '"name"' DONE IN %llut with %llutps\n", bcmk.end - bcmk.start, bcmk.frc)

#undef isalpha
#undef isspace
#include "ctype.h"

void mis_test() {
    // do some operations to get real results after
    {
        printf("Some text, just to do smth");
        printf("Some text, just to do smth");
        printf("Some text, just to do smth");
        printf("Some text, just to do smth");
        printf("Some text, just to do smth");
        printf("Some text, just to do smth");
        printf("Some text, just to do smth");
    }

    // benchmarks
    Bcmk bcmk = bcmk_create(); 
    
    const char* result = "";
    
    mis_fallback(caller_location, "==== MIS TESTS");
    
    begin_test("cast checks");
    {
        rune* runes;
        u32 length, cl, ln;
        chars_to_runes("a, b, c", &runes, &length, &cl, &ln);
        for (u32 i = 0; i < length; i++)
            putchar(runes[i]);
        putchar('\n');
        char* chars = runes_to_chars(runes, length);
        puts(chars);
        free(chars);
        _aligned_free(runes);
    }
    end_test("cast checks");
    dbg_bump();
    
    begin_test("ASCII characters unicode checks");
    {
        u64 total = 0, tmp0, tmp1;
        for (u64 j = 0; j < 1000000; j++) {
            bcmk_start_in(&tmp0);
            for (i32 i = 0; i <= 127; ++i) {
                rune c = (rune)i;

                if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) {
                    if (!is_alpha(c)) assert(false, "fail isalpha for '%c' (%d)", c, i);
                }
                else if (is_alpha(c)) {
                    assert(false, "fail !isalpha for '%c' (%d)", c, i);
                }

                if (c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r') {
                    if (!is_space(c)) assert(false, "fail isspace for '%c' (%d)", c, i);
                }
                else if (c == '\x1C' || c == '\x1D' || c == '\x1E' || c == '\x1F') {
                    if (!is_space(c)) assert(false, "fail isspace for '%c' (%d)", c, i);
                }
                else if (is_space(c)) {
                    assert(false, "fail !isspace for '%c' (%d)", c, i);
                }
            }
            bcmk_end_in(&tmp1);
            total += (tmp1 - tmp0);
        }
        printf("MIS variant 1000000 iters : 1 time ~%llut, full %llut with %llutps\n", total / 1000000UL, total, bcmk.frc);
        total = 0;
        for (u64 j = 0; j < 1000000; j++) {
            bcmk_start_in(&tmp0);
            for (i32 i = 0; i <= 127; ++i) {
                rune c = (rune)i;

                if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) {
                    if (!iswalpha(c)) assert(false, "fail isalpha for '%c' (%d)", c, i);
                }
                else if (iswalpha(c)) {
                    assert(false, "fail !isalpha for '%c' (%d)", c, i);
                }

                if (c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r') {
                    if (!iswspace(c)) assert(false, "fail isspace for '%c' (%d)", c, i);
                }
                else if (c == '\x1C' || c == '\x1D' || c == '\x1E' || c == '\x1F') {
                    if (iswspace(c)) assert(false, "fail !isspace for '%c' (%d)", c, i);
                }
                else if (iswspace(c)) {
                    assert(false, "fail !isspace for '%c' (%d)", c, i);
                }
            }
            bcmk_end_in(&tmp1);
            total += (tmp1 - tmp0);
        }
        printf("Microsoft variant 1000000 iters : 1 time ~%llut, full %llut with %llutps\n", total / 1000000UL, total, bcmk.frc);
    }
    end_test("ASCII characters unicode checks");
    dbg_bump();

    begin_test("Full MIS method hangul characters unicode checks");
    u32 i;
    for (i = 0x3131; i <= 0x314E; i++) if (!is_alpha(i)) { printf("wrong!\n"); scanf(""); }
    for (i = 0xAC00; i <= 0xD7A3; i++) if (!is_alpha(i)) { printf("wrong!\n"); scanf(""); }
    end_test("Full MIS method hangul characters unicode checks");

    begin_test("Full Microsoft method hangul characters unicode checks");
    for (i = 0x3131; i <= 0x314E; i++) if (!iswalpha(i)) { printf("wrong!\n"); scanf(""); }
    for (i = 0xAC00; i <= 0xD7A3; i++) if (!iswalpha(i)) { printf("wrong!\n"); scanf(""); }
    end_test("Full Microsoft method hangul characters unicode checks");

    begin_test("basic serialize");
    {
        MISSerializer ser = mis_ser_create();
        MISPropertySerializer a;
        assert(mis_ser_property(&ser, &a, "a"),       "fail on 'a' property");
        assert(mis_ser_end     (&a),                  "fail on 'a' property end");
        assert(mis_ser_fin     (&ser, &result),       "fail on final");
        assert(0 == strcmp(result, "a:;"), "fail on final string");
    }
    end_test("basic serialize");
    free((void*)result);
    dbg_bump();
    
    begin_test("primitives serialize");
    {
        MISSerializer ser = mis_ser_create();
        MISPropertySerializer a;
        assert(mis_ser_property      (&ser, &a, "a"),        "fail on 'a' property");
        assert(mis_ser_add_int8      (&a, i8min),            "fail on write signed int8 min");
        assert(mis_ser_add_int8      (&a, i8max),            "fail on write signed int8 max");
        assert(mis_ser_add_int8      (&a, -100),             "fail on write signed int8 '-100'");
        assert(mis_ser_add_uint8     (&a,  100),             "fail on write unsigned int8 '100'");
        assert(mis_ser_add_int16     (&a, i16min),           "fail on write signed int16 min");
        assert(mis_ser_add_int16     (&a, i16max),           "fail on write signed int16 max");
        assert(mis_ser_add_int16     (&a, -433),             "fail on write signed int16 '-433'");
        assert(mis_ser_add_uint16    (&a,  433),             "fail on write unsigned int16 '433'");
        assert(mis_ser_add_int32     (&a, i32min),           "fail on write signed int32 min");
        assert(mis_ser_add_int32     (&a, i32max),           "fail on write signed int32 max");
        assert(mis_ser_add_int32     (&a, -56523),           "fail on write signed int32 '-56523'");
        assert(mis_ser_add_uint32    (&a,  56523),           "fail on write unsigned int32 '56523'");
        assert(mis_ser_add_int64     (&a, i64min),           "fail on write signed int64 min");
        assert(mis_ser_add_int64     (&a, i64max),           "fail on write signed int64 max");
        assert(mis_ser_add_int64     (&a, -46345677678),     "fail on write signed int64 '-46345677678'");
        assert(mis_ser_add_uint64    (&a,  46345677678),     "fail on write unsigned int64 '46345677678'");
        assert(mis_ser_add_float     (&a, 54.56f),           "fail on write float '54.56'");
        assert(mis_ser_add_double    (&a, 54.56),            "fail on write double '54.56'");
        assert(mis_ser_add_boolean   (&a, true),             "fail on write bool 'true'");
        assert(mis_ser_add_boolean   (&a, false),            "fail on write bool 'false'");
        assert(mis_ser_add_fastdouble(&a, 0.45),             "fail on write fast double '0.45'");
        assert(mis_ser_add_string    (&a, "Hello!\nWorld!"), "fail on write string 'Hello!\nWorld!'");
        assert(mis_ser_end           (&a),                   "fail on 'a' property end");
        assert(mis_ser_fin           (&ser, &result),        "fail on final");
    }
    end_test("primitives serialize");
    free((void*)result);
    dbg_bump();
    
    begin_test("complex serialize");
    {
        MISSerializer ser = mis_ser_create();
        MISPropertySerializer a;
        assert(mis_ser_property      (&ser, &a, "a"),        "fail on 'a' property");
        assert(mis_ser_add_int8      (&a, i8min),            "fail on write signed int8 min");
        assert(mis_ser_add_int8      (&a, i8max),            "fail on write signed int8 max");
        assert(mis_ser_add_int8      (&a, -100),             "fail on write signed int8 '-100'");
        assert(mis_ser_add_uint8     (&a,  100),             "fail on write unsigned int8 '100'");
        assert(mis_ser_add_int16     (&a, i16min),           "fail on write signed int16 min");
        assert(mis_ser_add_int16     (&a, i16max),           "fail on write signed int16 max");
        assert(mis_ser_add_int16     (&a, -433),             "fail on write signed int16 '-433'");
        assert(mis_ser_add_uint16    (&a,  433),             "fail on write unsigned int16 '433'");
        assert(mis_ser_add_int32     (&a, i32min),           "fail on write signed int32 min");
        assert(mis_ser_add_int32     (&a, i32max),           "fail on write signed int32 max");
        assert(mis_ser_add_int32     (&a, -56523),           "fail on write signed int32 '-56523'");
        assert(mis_ser_add_uint32    (&a,  56523),           "fail on write unsigned int32 '56523'");
        assert(mis_ser_add_int64     (&a, i64min),           "fail on write signed int64 min");
        assert(mis_ser_add_int64     (&a, i64max),           "fail on write signed int64 max");
        assert(mis_ser_add_int64     (&a, -46345677678),     "fail on write signed int64 '-46345677678'");
        assert(mis_ser_add_uint64    (&a,  46345677678),     "fail on write unsigned int64 '46345677678'");
        assert(mis_ser_add_float     (&a, 54.56f),           "fail on write float '54.56'");
        assert(mis_ser_add_double    (&a, 54.56),            "fail on write double '54.56'");
        assert(mis_ser_add_boolean   (&a, true),             "fail on write bool 'true'");
        assert(mis_ser_add_boolean   (&a, false),            "fail on write bool 'false'");
        assert(mis_ser_add_fastdouble(&a, 0.45),             "fail on write fast double '0.45'");
        assert(mis_ser_add_string    (&a, "Hello!\nWorld!"), "fail on write string 'Hello!\nWorld!'");

        MISListSerializer list;
        assert(mis_ser_add_list   (&a, &list),               "fail on 'a' list");
        assert(mis_ser_add_boolean(&list, true),             "fail on write bool 'true'");
        assert(mis_ser_add_boolean(&list, false),            "fail on write bool 'false'");
        assert(mis_ser_end        (&list),                   "fail on list end");
        
        MISObjectSerializer obj;
        assert(mis_ser_add_object(&a, &obj),              "fail on 'a' object");

        MISPropertySerializer obj_a;
        assert(mis_ser_property   (&obj, &obj_a, "a"),    "fail on 'obj.a' property");
        assert(mis_ser_add_fastdouble(&obj_a, 0.45),         "fail on write fast double '0.45'");
        assert(mis_ser_add_double    (&obj_a, 54.56),        "fail on write double '54.56'");
        assert(mis_ser_end        (&obj_a),               "fail on 'obj.a' property end");

        assert(mis_ser_end(&obj),                         "fail on list end");

        assert(mis_ser_end(&a),                           "fail on 'a' property end");
        assert(mis_ser_fin(&ser, &result),                "fail on final");
    }
    end_test("complex serialize");
    const char* complex = result;
    dbg_bump();
    result = "";

    #define prs_test(times, name, prs) \
        begin_test(name); \
        { \
            MISObjectContainer obj; \
            for (u32 i = 0; i < times; i++) \
                assert(mis_parse(&obj, prs, "meta", mis_std_parse_fallback), "fail on parse"); \
            end_test(name); \
            if (obj.value) print_object(obj.value, 0); \
            mis_free_container(obj);\
            dbg_bump(); \
            printf("\nPARSED TEXT " #times " times: %s\n", prs); \
        }

    #define prs_err_test(name, prs) \
        begin_test(name); \
        { \
            MISObjectContainer obj; \
            assert(!mis_parse(&obj, prs, "meta", mis_std_parse_fallback), "fail on error parse"); \
            end_test(name); \
            mis_free_container(obj);\
            dbg_bump(); \
            printf("\nPARSED TEXT for error: %s\n", prs); \
        }

    prs_test(1, "basic parse", "a: 0;");
    prs_test(1, "2 values parse", "a: 0, 0;");
    prs_test(1, "3 values parse", "a: 0, 0, 0;");
    prs_test(1, "keyword parse", "a: keyword;");
    prs_test(1, "true parse", "a: true;");
    prs_test(1, "false parse", "a: false;");
    prs_test(1, "easy string parse", "a: \"string\";");
    prs_test(1, "string parse", "a: \"string!! \\n Lelz #pop\";");
    prs_test(1, "primitive parse", "a: 0, keyword, true, false, \"\\\"String\\\", World!\", -87.65;");
    prs_test(1, "list parse", "a: (1, 2, 3);");
    prs_test(1, "list with comma parse", "a: (1, 2, 3,);");
    prs_test(1, "object parse", "a: [a: 1; b:2; c:3;];");
    prs_test(1, "comment parse", "{Hello!} a: xxx;");
    prs_test(1, "internal comment parse", "{Hello! {World!}} a: xxx;");
    prs_test(1, "complex parse", complex);
    free((void*)complex);
    
    prs_err_test("total empty error", "a:;");
    prs_err_test("comma at start error", "a:,;");
    prs_err_test("not closed comment error", "a:{ wow!;");
    prs_err_test("not closed string error", "a: \"wow!;");
    prs_err_test("not closed property error", "a: xxx");
    prs_err_test("not closed list error", "a: (xxx");
    prs_err_test("not closed object error", "a: [xxx:a;");
    prs_err_test("no delimiter property error", "a: xxx 0;");
    prs_err_test("no delimiter list error", "a: (xxx 0);");
    prs_err_test("invalid escape error", "a: \"\\~\";");
    prs_err_test("number mimic error", "a: 24.34.14;");

    prs_test(1, "crazy test", "{ Comment! { inner comment! } }\n"
                                  "property_number: 1;\n"
                                  "property_keyword: some_keyword; {keyword is a special type for not closed string literals, used to define some things like a types, enums, etc}\n"
                                  "property_string: \"String, World!\";\n"
                                  "property_boolean: true;\n"
                                  "property_list: (1, 2, 3); {lists can have not only one type, list (1, \"string\", key) - is totaly valid}\n"
                                  "property_object: [ {no bracket style difference between lists and objects}\n"
                                  "inner_property: 1;\n"
                                  "];\n"
                                  "{properties is not just a keyed fields, no, its a PROPERTIES, they support multiply values like a list, but a little more hard-typed for API}\n"
                                  "property_example: 1, keyword, [inner: 1, 2, 3;];\n"
                                  "{MIS supports \"links\" in builtin, but isn't a links, it's \"reference\", where is the different? Reference copies value at path to place it to self}\n"
                                  "property_link: > property_example 3 inner; {that path can be readed as: get property \"property_example\", get third value from the property, get property \"inner\" of getted value}");

    fflush(stdout);
    while (1){}
}
