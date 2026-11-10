#pragma once

#define NULL ((void *) 0)

typedef unsigned long long uint64_t;
typedef unsigned int uint32_t;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;
typedef long long int64_t;
typedef int int32_t;
typedef short int16_t;
typedef char int8_t;

#ifdef __i386__
typedef uint32_t size_t;
typedef size_t uword_t;
typedef int32_t ssize_t;
typedef ssize_t word_t;
#endif