// Copyright (c) 2011 Google, Inc.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// CityHash, by Geoff Pike and Jyrki Alakuijala
//
// This file provides CityHash64() and related functions.
//
// It's probably possible to create even faster hash functions by
// writing a program that systematically explores some of the space of
// possible hash functions, by using SIMD instructions, or by
// compromising on hash quality.

#include "pch.h"
#include "Core/Hash/city.h"

#include <algorithm>
#include <cstring> // for memcpy and memset

static Uint64 UnalignedLoaD64(const char* P) {
    Uint64 Result{};
    std::memcpy(&Result, P, sizeof(Result));
    return Result;
}

static Uint32 UnalignedLoaD32(const char* P) {
    Uint32 Result{};
    std::memcpy(&Result, P, sizeof(Result));
    return Result;
}

#ifdef _MSC_VER

#include <stdlib.h>
#define bswap_32(x) _byteswap_ulong(x)
#define bswap_64(x) _byteswap_uint64(x)

#elif defined(__APPLE__)

// Mac OS X / Darwin features
#include <libkern/OSByteOrder.h>
#define bswap_32(x) OSSwapInt32(x)
#define bswap_64(x) OSSwapInt64(x)

#elif defined(__sun) || defined(sun)

#include <sys/byteorder.h>
#define bswap_32(x) BSWAP_32(x)
#define bswap_64(x) BSWAP_64(x)

#elif defined(__FreeBSD__)

#include <sys/endian.h>
#define bswap_32(x) bswap32(x)
#define bswap_64(x) bswap64(x)

#elif defined(__OpenBSD__)

#include <sys/types.h>
#define bswap_32(x) swap32(x)
#define bswap_64(x) swap64(x)

#elif defined(__NetBSD__)

#include <sys/types.h>
#include <machine/bswap.h>
#if defined(__BSWAP_RENAME) && !defined(__bswap_32)
#define bswap_32(x) bswap32(x)
#define bswap_64(x) bswap64(x)
#endif

#else

#include <byteswap.h>

#endif

#ifdef WORDS_BIGENDIAN
#define uint32_in_expected_order(x) (bswap_32(x))
#define uint64_in_expected_order(x) (bswap_64(x))
#else
#define uint32_in_expected_order(x) (x)
#define uint64_in_expected_order(x) (x)
#endif

#if !defined(LIKELY)
#if HAVE_BUILTIN_EXPECT
#define LIKELY(x) (__builtin_expect(!!(x), 1))
#else
#define LIKELY(x) (x)
#endif
#endif

static Uint64 Fetch64(const char* P) {
    return uint64_in_expected_order(UnalignedLoaD64(P));
}

static Uint32 Fetch32(const char* P) {
    return uint32_in_expected_order(UnalignedLoaD32(P));
}

// Some primes between 2^63 and 2^64 for various uses.
static const Uint64 K0{0xc3a5c85c97cb3127ULL};
static const Uint64 K1{0xb492b66fbe98f273ULL};
static const Uint64 K2{0x9ae16a3b2f90404fULL};

// Magic numbers for 32-bit hashing.  Copied from Murmur3.
static const Uint32 C1{0xcc9e2d51};
static const Uint32 C2{0x1b873593};

// A 32-bit to 32-bit integer hash copied from Murmur3.
static Uint32 Fmix(Uint32 H) {
    H ^= H >> 16;
    H *= 0x85ebca6b;
    H ^= H >> 13;
    H *= 0xc2b2ae35;
    H ^= H >> 16;
    return H;
}

static Uint32 Rotate32(Uint32 Val, int Shift) {
    // Avoid shifting by 32: doing so yields an undefined result.
    return Shift == 0 ? Val : ((Val >> Shift) | (Val << (32 - Shift)));
}

#undef PERMUTE3
#define PERMUTE3(a, b, c) \
    do {                  \
        std::swap(a, b);  \
        std::swap(a, c);  \
    } while (0)

static Uint32 Mur(Uint32 A, Uint32 H) {
    // Helper from Murmur3 for combining two 32-bit values.
    A *= C1;
    A = Rotate32(A, 17);
    A *= C2;
    H ^= A;
    H = Rotate32(H, 19);
    return H * 5 + 0xe6546b64;
}

static Uint32 Hash32Len13to24(const char* S, std::size_t Len) {
    Uint32 A{Fetch32(S - 4 + (Len >> 1))};
    Uint32 B{Fetch32(S + 4)};
    Uint32 C{Fetch32(S + Len - 8)};
    Uint32 D{Fetch32(S + (Len >> 1))};
    Uint32 E{Fetch32(S)};
    Uint32 F{Fetch32(S + Len - 4)};
    Uint32 H{static_cast<Uint32>(Len)};

    return Fmix(Mur(F, Mur(E, Mur(D, Mur(C, Mur(B, Mur(A, H)))))));
}

static Uint32 Hash32Len0to4(const char* S, std::size_t Len) {
    Uint32 B{0};
    Uint32 C{9};
    for (std::size_t I{0}; I < Len; I++) {
        signed char V{static_cast<signed char>(S[I])};
        B = B * C1 + static_cast<Uint32>(V);
        C ^= B;
    }
    return Fmix(Mur(B, Mur(static_cast<Uint32>(Len), C)));
}

static Uint32 Hash32Len5to12(const char* S, std::size_t Len) {
    Uint32 A{static_cast<Uint32>(Len)}, B{A * 5}, C{9}, D{B};
    A += Fetch32(S);
    B += Fetch32(S + Len - 4);
    C += Fetch32(S + ((Len >> 1) & 4));
    return Fmix(Mur(C, Mur(B, Mur(A, D))));
}

Uint32 CityHash32(const char* S, std::size_t Len) {
    if (Len <= 24) {
        return Len <= 12 ? (Len <= 4 ? Hash32Len0to4(S, Len) : Hash32Len5to12(S, Len)) : Hash32Len13to24(S, Len);
    }

    // len > 24
    Uint32 H{static_cast<Uint32>(Len)}, G{C1 * H}, F{G};
    Uint32 A0{Rotate32(Fetch32(S + Len - 4) * C1, 17) * C2};
    Uint32 A1{Rotate32(Fetch32(S + Len - 8) * C1, 17) * C2};
    Uint32 A2{Rotate32(Fetch32(S + Len - 16) * C1, 17) * C2};
    Uint32 A3{Rotate32(Fetch32(S + Len - 12) * C1, 17) * C2};
    Uint32 A4{Rotate32(Fetch32(S + Len - 20) * C1, 17) * C2};
    H ^= A0;
    H = Rotate32(H, 19);
    H = H * 5 + 0xe6546b64;
    H ^= A2;
    H = Rotate32(H, 19);
    H = H * 5 + 0xe6546b64;
    G ^= A1;
    G = Rotate32(G, 19);
    G = G * 5 + 0xe6546b64;
    G ^= A3;
    G = Rotate32(G, 19);
    G = G * 5 + 0xe6546b64;
    F += A4;
    F = Rotate32(F, 19);
    F = F * 5 + 0xe6546b64;
    std::size_t Iters{(Len - 1) / 20};
    do {
        Uint32 A0{Rotate32(Fetch32(S) * C1, 17) * C2};
        Uint32 A1{Fetch32(S + 4)};
        Uint32 A2{Rotate32(Fetch32(S + 8) * C1, 17) * C2};
        Uint32 A3{Rotate32(Fetch32(S + 12) * C1, 17) * C2};
        Uint32 A4{Fetch32(S + 16)};
        H ^= A0;
        H = Rotate32(H, 18);
        H = H * 5 + 0xe6546b64;
        F += A1;
        F = Rotate32(F, 19);
        F = F * C1;
        G += A2;
        G = Rotate32(G, 18);
        G = G * 5 + 0xe6546b64;
        H ^= A3 + A1;
        H = Rotate32(H, 19);
        H = H * 5 + 0xe6546b64;
        G ^= A4;
        G = bswap_32(G) * 5;
        H += A4 * 5;
        H = bswap_32(H);
        F += A0;
        PERMUTE3(F, H, G);
        S += 20;
    } while (--Iters != 0);
    G = Rotate32(G, 11) * C1;
    G = Rotate32(G, 17) * C1;
    F = Rotate32(F, 11) * C1;
    F = Rotate32(F, 17) * C1;
    H = Rotate32(H + G, 19);
    H = H * 5 + 0xe6546b64;
    H = Rotate32(H, 17) * C1;
    H = Rotate32(H + F, 19);
    H = H * 5 + 0xe6546b64;
    H = Rotate32(H, 17) * C1;
    return H;
}

// Bitwise right rotate.  Normally this will compile to a single
// instruction, especially if the shift is a manifest constant.
static Uint64 Rotate(Uint64 Val, int Shift) {
    // Avoid shifting by 64: doing so yields an undefined result.
    return Shift == 0 ? Val : ((Val >> Shift) | (Val << (64 - Shift)));
}

static Uint64 ShiftMix(Uint64 Val) {
    return Val ^ (Val >> 47);
}

static Uint64 HashLen16(Uint64 U, Uint64 V) {
    return Hash128to64(Uint128{U, V});
}

static Uint64 HashLen16(Uint64 U, Uint64 V, Uint64 Mul) {
    // Murmur-inspired hashing.
    Uint64 A{(U ^ V) * Mul};
    A ^= (A >> 47);
    Uint64 B{(V ^ A) * Mul};
    B ^= (B >> 47);
    B *= Mul;
    return B;
}

static Uint64 HashLen0to16(const char* S, std::size_t Len) {
    if (Len >= 8) {
        Uint64 Mul{K2 + Len * 2};
        Uint64 A{Fetch64(S) + K2};
        Uint64 B{Fetch64(S + Len - 8)};
        Uint64 C{Rotate(B, 37) * Mul + A};
        Uint64 D{(Rotate(A, 25) + B) * Mul};
        return HashLen16(C, D, Mul);
    }
    if (Len >= 4) {
        Uint64 Mul{K2 + Len * 2};
        Uint64 A{Fetch32(S)};
        return HashLen16(Len + (A << 3), Fetch32(S + Len - 4), Mul);
    }
    if (Len > 0) {
        Uint8 A{static_cast<Uint8>(S[0])};
        Uint8 B{static_cast<Uint8>(S[Len >> 1])};
        Uint8 C{static_cast<Uint8>(S[Len - 1])};
        Uint32 Y{static_cast<Uint32>(A) + (static_cast<Uint32>(B) << 8)};
        Uint32 Z{static_cast<Uint32>(Len) + (static_cast<Uint32>(C) << 2)};
        return ShiftMix(Y * K2 ^ Z * K0) * K2;
    }
    return K2;
}

// This probably works well for 16-byte strings as well, but it may be overkill
// in that case.
static Uint64 HashLen17to32(const char* S, std::size_t Len) {
    Uint64 Mul{K2 + Len * 2};
    Uint64 A{Fetch64(S) * K1};
    Uint64 B{Fetch64(S + 8)};
    Uint64 C{Fetch64(S + Len - 8) * Mul};
    Uint64 D{Fetch64(S + Len - 16) * K2};
    return HashLen16(Rotate(A + B, 43) + Rotate(C, 30) + D, A + Rotate(B + K2, 18) + C, Mul);
}

// Return a 16-byte hash for 48 bytes.  Quick and dirty.
// Callers do best to use "random-looking" values for a and b.
static std::pair<Uint64, Uint64> WeakHashLen32WithSeeds(Uint64 W, Uint64 X, Uint64 Y, Uint64 Z, Uint64 A, Uint64 B) {
    A += W;
    B = Rotate(B + A + Z, 21);
    Uint64 C{A};
    A += X;
    A += Y;
    B += Rotate(A, 44);
    return std::make_pair(A + Z, B + C);
}

// Return a 16-byte hash for s[0] ... s[31], a, and b.  Quick and dirty.
static std::pair<Uint64, Uint64> WeakHashLen32WithSeeds(const char* S, Uint64 A, Uint64 B) {
    return WeakHashLen32WithSeeds(Fetch64(S), Fetch64(S + 8), Fetch64(S + 16), Fetch64(S + 24), A, B);
}

// Return an 8-byte hash for 33 to 64 bytes.
static Uint64 HashLen33to64(const char* S, std::size_t Len) {
    Uint64 Mul{K2 + Len * 2};
    Uint64 A{Fetch64(S) * K2};
    Uint64 B{Fetch64(S + 8)};
    Uint64 C{Fetch64(S + Len - 24)};
    Uint64 D{Fetch64(S + Len - 32)};
    Uint64 E{Fetch64(S + 16) * K2};
    Uint64 F{Fetch64(S + 24) * 9};
    Uint64 G{Fetch64(S + Len - 8)};
    Uint64 H{Fetch64(S + Len - 16) * Mul};
    Uint64 U{Rotate(A + G, 43) + (Rotate(B, 30) + C) * 9};
    Uint64 V{((A + G) ^ D) + F + 1};
    Uint64 W{bswap_64((U + V) * Mul) + H};
    Uint64 X{Rotate(E + F, 42) + C};
    Uint64 Y{(bswap_64((V + W) * Mul) + G) * Mul};
    Uint64 Z{E + F + C};
    A = bswap_64((X + Z) * Mul + Y) + B;
    B = ShiftMix((Z + A) * Mul + D + H) * Mul;
    return B + X;
}

Uint64 CityHash64(const char* S, std::size_t Len) {
    if (Len <= 32) {
        if (Len <= 16) {
            return HashLen0to16(S, Len);
        } else {
            return HashLen17to32(S, Len);
        }
    } else if (Len <= 64) {
        return HashLen33to64(S, Len);
    }

    // For strings over 64 bytes we hash the end first, and then as we
    // loop we keep 56 bytes of state: v, w, x, y, and z.
    Uint64 X{Fetch64(S + Len - 40)};
    Uint64 Y{Fetch64(S + Len - 16) + Fetch64(S + Len - 56)};
    Uint64 Z{HashLen16(Fetch64(S + Len - 48) + Len, Fetch64(S + Len - 24))};
    std::pair<Uint64, Uint64> V{WeakHashLen32WithSeeds(S + Len - 64, Len, Z)};
    std::pair<Uint64, Uint64> W{WeakHashLen32WithSeeds(S + Len - 32, Y + K1, X)};
    X = X * K1 + Fetch64(S);

    // Decrease len to the nearest multiple of 64, and operate on 64-byte chunks.
    Len = (Len - 1) & ~static_cast<std::size_t>(63);
    do {
        X = Rotate(X + Y + V.first + Fetch64(S + 8), 37) * K1;
        Y = Rotate(Y + V.second + Fetch64(S + 48), 42) * K1;
        X ^= W.second;
        Y += V.first + Fetch64(S + 40);
        Z = Rotate(Z + W.first, 33) * K1;
        V = WeakHashLen32WithSeeds(S, V.second * K1, X + W.first);
        W = WeakHashLen32WithSeeds(S + 32, Z + W.second, Y + Fetch64(S + 16));
        std::swap(Z, X);
        S += 64;
        Len -= 64;
    } while (Len != 0);
    return HashLen16(HashLen16(V.first, W.first) + ShiftMix(Y) * K1 + Z, HashLen16(V.second, W.second) + X);
}

Uint64 CityHash64WithSeed(const char* S, std::size_t Len, Uint64 Seed) {
    return CityHash64WithSeeds(S, Len, K2, Seed);
}

Uint64 CityHash64WithSeeds(const char* S, std::size_t Len, Uint64 Seed0, Uint64 Seed1) {
    return HashLen16(CityHash64(S, Len) - Seed0, Seed1);
}

// A subroutine for CityHash128().  Returns a decent 128-bit hash for strings
// of any length representable in signed long.  Based on City and Murmur.
static Uint128 CityMurmur(const char* S, std::size_t Len, Uint128 Seed) {
    Uint64 A{Uint128Low64(Seed)};
    Uint64 B{Uint128High64(Seed)};
    Uint64 C{0};
    Uint64 D{0};
    if (Len <= 16) {
        A = ShiftMix(A * K1) * K1;
        C = B * K1 + HashLen0to16(S, Len);
        D = ShiftMix(A + (Len >= 8 ? Fetch64(S) : C));
    } else {
        C = HashLen16(Fetch64(S + Len - 8) + K1, A);
        D = HashLen16(B + Len, C + Fetch64(S + Len - 16));
        A += D;
        // len > 16 here, so do...while is safe
        do {
            A ^= ShiftMix(Fetch64(S) * K1) * K1;
            A *= K1;
            B ^= A;
            C ^= ShiftMix(Fetch64(S + 8) * K1) * K1;
            C *= K1;
            D ^= C;
            S += 16;
            Len -= 16;
        } while (Len > 16);
    }
    A = HashLen16(A, C);
    B = HashLen16(D, B);
    return Uint128{A ^ B, HashLen16(B, A)};
}

Uint128 CityHash128WithSeed(const char* S, std::size_t Len, Uint128 Seed) {
    if (Len < 128) {
        return CityMurmur(S, Len, Seed);
    }

    // We expect len >= 128 to be the common case.  Keep 56 bytes of state:
    // v, w, x, y, and z.
    std::pair<Uint64, Uint64> V{}, W{};
    Uint64 X{Uint128Low64(Seed)};
    Uint64 Y{Uint128High64(Seed)};
    Uint64 Z{Len * K1};
    V.first = Rotate(Y ^ K1, 49) * K1 + Fetch64(S);
    V.second = Rotate(V.first, 42) * K1 + Fetch64(S + 8);
    W.first = Rotate(Y + Z, 35) * K1 + X;
    W.second = Rotate(X + Fetch64(S + 88), 53) * K1;

    // This is the same inner loop as CityHash64(), manually unrolled.
    do {
        X = Rotate(X + Y + V.first + Fetch64(S + 8), 37) * K1;
        Y = Rotate(Y + V.second + Fetch64(S + 48), 42) * K1;
        X ^= W.second;
        Y += V.first + Fetch64(S + 40);
        Z = Rotate(Z + W.first, 33) * K1;
        V = WeakHashLen32WithSeeds(S, V.second * K1, X + W.first);
        W = WeakHashLen32WithSeeds(S + 32, Z + W.second, Y + Fetch64(S + 16));
        std::swap(Z, X);
        S += 64;
        X = Rotate(X + Y + V.first + Fetch64(S + 8), 37) * K1;
        Y = Rotate(Y + V.second + Fetch64(S + 48), 42) * K1;
        X ^= W.second;
        Y += V.first + Fetch64(S + 40);
        Z = Rotate(Z + W.first, 33) * K1;
        V = WeakHashLen32WithSeeds(S, V.second * K1, X + W.first);
        W = WeakHashLen32WithSeeds(S + 32, Z + W.second, Y + Fetch64(S + 16));
        std::swap(Z, X);
        S += 64;
        Len -= 128;
    } while (LIKELY(Len >= 128));
    X += Rotate(V.first + Z, 49) * K0;
    Y = Y * K0 + Rotate(W.second, 37);
    Z = Z * K0 + Rotate(W.first, 27);
    W.first *= 9;
    V.first *= K0;
    // If 0 < len < 128, hash up to 4 chunks of 32 bytes each from the end of s.
    for (std::size_t TailDone{0}; TailDone < Len;) {
        TailDone += 32;
        Y = Rotate(X + Y, 42) * K0 + V.second;
        W.first += Fetch64(S + Len - TailDone + 16);
        X = X * K0 + W.first;
        Z += W.second + Fetch64(S + Len - TailDone);
        W.second += V.first;
        V = WeakHashLen32WithSeeds(S + Len - TailDone, V.first + Z, V.second);
        V.first *= K0;
    }
    // At this point our 56 bytes of state should contain more than
    // enough information for a strong 128-bit hash.  We use two
    // different 56-byte-to-8-byte hashes to get a 16-byte final result.
    X = HashLen16(X, V.first);
    Y = HashLen16(Y + Z, W.first);
    return Uint128{HashLen16(X + V.second, W.second) + Y, HashLen16(X + W.second, Y + V.second)};
}

Uint128 CityHash128(const char* S, std::size_t Len) {
    return Len >= 16 ? CityHash128WithSeed(S + 16, Len - 16, Uint128{Fetch64(S), Fetch64(S + 8) + K0}) : CityHash128WithSeed(S, Len, Uint128{K0, K1});
}

#ifdef __SSE4_2__
#include <citycrc.h>
#include <nmmintrin.h>

// Requires len >= 240.
static void CityHashCrc256Long(const char* S, std::size_t Len, Uint32 Seed, Uint64* Result) {
    Uint64 A{Fetch64(S + 56) + K0};
    Uint64 B{Fetch64(S + 96) + K0};
    Uint64 C{Result[0] = HashLen16(B, Len)};
    Uint64 D{Result[1] = Fetch64(S + 120) * K0 + Len};
    Uint64 E{Fetch64(S + 184) + Seed};
    Uint64 F{0};
    Uint64 G{0};
    Uint64 H{C + D};
    Uint64 X{Seed};
    Uint64 Y{0};
    Uint64 Z{0};

    // 240 bytes of input per iter.
    std::size_t Iters{Len / 240};
    Len -= Iters * 240;
    do {
#undef CHUNK
#define CHUNK(R)                 \
    PERMUTE3(X, Z, Y);           \
    B += Fetch64(S);             \
    C += Fetch64(S + 8);         \
    D += Fetch64(S + 16);        \
    E += Fetch64(S + 24);        \
    F += Fetch64(S + 32);        \
    A += B;                      \
    H += F;                      \
    B += C;                      \
    F += D;                      \
    G += E;                      \
    E += Z;                      \
    G += X;                      \
    Z = _mm_crc32_u64(Z, B + G); \
    Y = _mm_crc32_u64(Y, E + H); \
    X = _mm_crc32_u64(X, F + A); \
    E = Rotate(E, R);            \
    C += E;                      \
    S += 40

        CHUNK(0);
        PERMUTE3(A, H, C);
        CHUNK(33);
        PERMUTE3(A, H, F);
        CHUNK(0);
        PERMUTE3(B, H, F);
        CHUNK(42);
        PERMUTE3(B, H, D);
        CHUNK(0);
        PERMUTE3(B, H, E);
        CHUNK(33);
        PERMUTE3(A, H, E);
    } while (--Iters > 0);

    while (Len >= 40) {
        CHUNK(29);
        E ^= Rotate(A, 20);
        H += Rotate(B, 30);
        G ^= Rotate(C, 40);
        F += Rotate(D, 34);
        PERMUTE3(C, H, G);
        Len -= 40;
    }
    if (Len > 0) {
        S = S + Len - 40;
        CHUNK(33);
        E ^= Rotate(A, 43);
        H += Rotate(B, 42);
        G ^= Rotate(C, 41);
        F += Rotate(D, 40);
    }
    Result[0] ^= H;
    Result[1] ^= G;
    G += H;
    A = HashLen16(A, G + Z);
    X += Y << 32;
    B += X;
    C = HashLen16(C, Z) + H;
    D = HashLen16(D, E + Result[0]);
    G += E;
    H += HashLen16(X, F);
    E = HashLen16(A, D) + G;
    Z = HashLen16(B, C) + A;
    Y = HashLen16(G, H) + C;
    Result[0] = E + Z + Y + X;
    A = ShiftMix((A + Y) * K0) * K0 + B;
    Result[1] += A + Result[0];
    A = ShiftMix(A * K0) * K0 + C;
    Result[2] = A + Result[1];
    A = ShiftMix((A + E) * K0) * K0;
    Result[3] = A + Result[2];
}

// Requires len < 240.
static void CityHashCrc256Short(const char* S, std::size_t Len, Uint64* Result) {
    char Buffer[240]{};
    std::memcpy(Buffer, S, Len);
    std::memset(Buffer + Len, 0, 240 - Len);
    CityHashCrc256Long(Buffer, 240, ~static_cast<Uint32>(Len), Result);
}

void CityHashCrc256(const char* S, std::size_t Len, Uint64* Result) {
    if (LIKELY(Len >= 240)) {
        CityHashCrc256Long(S, Len, 0, Result);
    } else {
        CityHashCrc256Short(S, Len, Result);
    }
}

Uint128 CityHashCrc128WithSeed(const char* S, std::size_t Len, Uint128 Seed) {
    if (Len <= 900) {
        return CityHash128WithSeed(S, Len, Seed);
    } else {
        Uint64 Result[4]{};
        CityHashCrc256(S, Len, Result);
        Uint64 U{Uint128High64(Seed) + Result[0]};
        Uint64 V{Uint128Low64(Seed) + Result[1]};
        return Uint128{HashLen16(U, V + Result[2]),
                       HashLen16(Rotate(V, 32), U * K0 + Result[3])};
    }
}

Uint128 CityHashCrc128(const char* S, std::size_t Len) {
    if (Len <= 900) {
        return CityHash128(S, Len);
    } else {
        Uint64 Result[4]{};
        CityHashCrc256(S, Len, Result);
        return Uint128{Result[2], Result[3]};
    }
}

#endif

Uint64 Uint128Low64(const Uint128& X) {
    return X.first;
}

Uint64 Uint128High64(const Uint128& X) {
    return X.second;
}

Uint64 Hash128to64(const Uint128& X) {
    // Murmur-inspired hashing.
    const Uint64 KMul{0x9ddfea08eb382d69ULL};
    Uint64 A{(Uint128Low64(X) ^ Uint128High64(X)) * KMul};
    A ^= (A >> 47);
    Uint64 B{(Uint128High64(X) ^ A) * KMul};
    B ^= (B >> 47);
    B *= KMul;
    return B;
}
