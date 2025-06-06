//  Copyright (c) 2011-present, Facebook, Inc.  All rights reserved.
//  This source code is licensed under both the GPLv2 (found in the
//  COPYING file in the root directory) and Apache 2.0 License
//  (found in the LICENSE.Apache file in the root directory).
//
/*
  Murmurhash from http://sites.google.com/site/murmurhash/

  All code is released to the public domain. For business purposes, Murmurhash
  is under the MIT license.
*/
#include "murmurhash.h"

#include "port/lang.h"

#if defined(__x86_64__)

// -------------------------------------------------------------------
//
// The same caveats as 32-bit MurmurHash2 apply here - beware of alignment
// and endian-ness issues if used across multiple platforms.
//
// 64-bit hash for 64-bit platforms

#ifdef ROCKSDB_UBSAN_RUN
#if defined(__clang__)
__attribute__((__no_sanitize__("alignment")))
#elif defined(__GNUC__)
__attribute__((__no_sanitize_undefined__))
#endif
#endif
// clang-format off
uint64_t MurmurHash64A ( const void * key, int len, unsigned int seed )
{
    const uint64_t m = 0xc6a4a7935bd1e995;
    const int r = 47;

    uint64_t h = seed ^ (len * m);

    const uint64_t * data = (const uint64_t *)key;
    const uint64_t * end = data + (len/8);

    while(data != end)
    {
        uint64_t k = *data++;

        k *= m;
        k ^= k >> r;
        k *= m;

        h ^= k;
        h *= m;
    }

    const unsigned char * data2 = (const unsigned char*)data;

    switch(len & 7)
    {
    case 7: h ^= ((uint64_t)data2[6]) << 48; FALLTHROUGH_INTENDED;
    case 6: h ^= ((uint64_t)data2[5]) << 40; FALLTHROUGH_INTENDED;
    case 5: h ^= ((uint64_t)data2[4]) << 32; FALLTHROUGH_INTENDED;
    case 4: h ^= ((uint64_t)data2[3]) << 24; FALLTHROUGH_INTENDED;
    case 3: h ^= ((uint64_t)data2[2]) << 16; FALLTHROUGH_INTENDED;
    case 2: h ^= ((uint64_t)data2[1]) << 8;  FALLTHROUGH_INTENDED;
    case 1: h ^= ((uint64_t)data2[0]);
        h *= m;
    }

    h ^= h >> r;
    h *= m;
    h ^= h >> r;

    return h;
}
// clang-format on

#elif defined(__i386__)

// -------------------------------------------------------------------
//
// Note - This code makes a few assumptions about how your machine behaves -
//
// 1. We can read a 4-byte value from any address without crashing
// 2. sizeof(int) == 4
//
// And it has a few limitations -
//
// 1. It will not work incrementally.
// 2. It will not produce the same results on little-endian and big-endian
//    machines.
// clang-format off
unsigned int MurmurHash2 ( const void * key, int len, unsigned int seed )
{
    // 'm' and 'r' are mixing constants generated offline.
    // They're not really 'magic', they just happen to work well.

    const unsigned int m = 0x5bd1e995;
    const int r = 24;

    // Initialize the hash to a 'random' value

    unsigned int h = seed ^ len;

    // Mix 4 bytes at a time into the hash

    const unsigned char * data = (const unsigned char *)key;

    while(len >= 4)
    {
        unsigned int k = *(unsigned int *)data;

        k *= m;
        k ^= k >> r;
        k *= m;

        h *= m;
        h ^= k;

        data += 4;
        len -= 4;
    }

    // Handle the last few bytes of the input array

    switch(len)
    {
    case 3: h ^= data[2] << 16; FALLTHROUGH_INTENDED;
    case 2: h ^= data[1] << 8;  FALLTHROUGH_INTENDED;
    case 1: h ^= data[0];
        h *= m;
    };

    // Do a few final mixes of the hash to ensure the last few
    // bytes are well-incorporated.

    h ^= h >> 13;
    h *= m;
    h ^= h >> 15;

    return h;
}
// clang-format on

#else

// -------------------------------------------------------------------
//
// Same as MurmurHash2, but endian- and alignment-neutral.
// Half the speed though, alas.
// clang-format off
unsigned int MurmurHashNeutral2 ( const void * key, int len, unsigned int seed )
{
    const unsigned int m = 0x5bd1e995;
    const int r = 24;

    unsigned int h = seed ^ len;

    const unsigned char * data = (const unsigned char *)key;

    while(len >= 4)
    {
        unsigned int k;

        k  = data[0];
        k |= data[1] << 8;
        k |= data[2] << 16;
        k |= data[3] << 24;

        k *= m;
        k ^= k >> r;
        k *= m;

        h *= m;
        h ^= k;

        data += 4;
        len -= 4;
    }

    switch(len)
    {
    case 3: h ^= data[2] << 16; FALLTHROUGH_INTENDED;
    case 2: h ^= data[1] << 8;  FALLTHROUGH_INTENDED;
    case 1: h ^= data[0];
        h *= m;
    };

    h ^= h >> 13;
    h *= m;
    h ^= h >> 15;

    return h;
}
// clang-format on

#endif
