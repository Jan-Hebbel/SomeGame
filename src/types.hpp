#ifndef PCH_H
#define PCH_H

#include <stdint.h>

#define internal_function static
#define local_persist static
#define global_variable static

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef int32 bool32;
typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef float real32;
typedef double real64;

typedef unsigned int uint;

constexpr bool32 GAME_SUCCESS = 0;
constexpr bool32 GAME_FAILURE = 1;

#endif
