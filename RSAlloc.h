/*
* Copyright (C) 2024 ColleagueRiley
*
* libpng license
*
* This software is provided 'as-is', without any express or implied
* warranty.  In no event will the authors be held liable for any damages
* arising from the use of this software.
*
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
*
* 1. The origin of this software must not be misrepresented; you must not
*    claim that you wrote the original software. If you use this software
*    in a product, an acknowledgment in the product documentation would be
*    appreciated but is not required.
* 2. Altered source versions must be plainly marked as such, and must not be
*    misrepresented as being the original software.
* 3. This notice may not be removed or altered from any source distribution.
*
*
*/

/*
	(MAKE SURE RSA_IMPLEMENTATION is in exactly one header or you use -D RSA_IMPLEMENTATION)
	#define RSA_IMPLEMENTATION - makes it so source code is included with header
    #define RSA_BSS - use BSS memory instead of malloc
    #define RSA_DEBUG - show debug info
    #define RSA_MALLOC [x] / #define RSA_FREE [x] - define your own malloc/free functions to use
    #define RSA_DEFAULT_ARENA [x] - set default arena size
    #define RSA_CHUNK_COUNT [x] - set max chunk count
*/

#if (defined(_WIN32) || defined(_WIN64))
#define RSA_PRINT_U64 "%lli"
#else
#define RSA_PRINT_U64 "%li"
#endif

#ifdef RSA_DEBUG
#include <stdio.h>
#endif

#if !defined(RSA_MALLOC) && !defined(RSA_USE_MALLOC) && !defined(RSA_BSS) && (defined(__linux__) || defined(__unix__) || defined(__APPLE__))
#define RSA_MMAP

#elif !defined(RSA_MALLOC) && !defined(RSA_USE_MALLOC) && !defined(RSA_BSS) && (defined(_WIN32) || defined(_WIN64))
#define RSA_VIRTUAL_ALLOC

#elif !defined(RSA_BSS) && !defined(RSA_MMAP) && !defined(RSA_VIRTUAL_ALLOC)
#ifndef RSA_MALLOC
#include <stdlib.h>
#define RSA_MALLOC malloc
#define RSA_FREE free
#endif
#endif

#ifndef RSA_DEFAULT_ARENA
#define RSA_DEFAULT_ARENA 4194000 /* ~ 4 MB */
#endif

#ifndef RSA_CHUNK_COUNT
#define RSA_CHUNK_COUNT (RSA_DEFAULT_ARENA) / 8
#endif

#if !_MSC_VER
#ifndef inline
#ifndef __APPLE__
#define inline __inline
#endif
#endif
#endif

#ifndef RSADEF
#ifdef __APPLE__
#define RSADEF static inline
#else
#define RSADEF inline
#endif
#endif

#ifndef RSA_ENUM
#define RSA_ENUM(type, name) type name; enum
#endif

#ifndef RSA_UNUSED
#define RSA_UNUSED(x) (void)(x);
#endif

#ifdef __cplusplus
extern "C" {
#endif

	/* makes sure the header file part is only defined once by default */
#ifndef RSA_HEADER

#define RSA_HEADER

#if !defined(u8)
	#if defined(_MSC_VER) || defined(__SYMBIAN32__)
		typedef unsigned char 	u8;
		typedef signed char		i8;
		typedef unsigned short  u16;
		typedef signed short 	i16;
		typedef unsigned int 	u32;
		typedef signed int		i32;
		typedef unsigned long	u64;
		typedef signed long		i64;
	#else
		#include <stdint.h>

		typedef uint8_t     u8;
		typedef int8_t      i8;
		typedef uint16_t   u16;
		typedef int16_t    i16;
		typedef uint32_t   u32;
		typedef int32_t    i32;
		typedef uint64_t   u64;
		typedef int64_t    i64;
	#endif
#endif

#if !defined(b8)
	typedef u8 b8;
#endif

#include <stddef.h>
#include <string.h>

typedef struct RSA_chunk {
    size_t start;
    size_t len;
    b8 used;
} RSA_chunk;

RSADEF void RSA_init(size_t arenaSize);
RSADEF void* RSA_alloc(size_t size);
RSADEF void* RSA_calloc(size_t size, size_t typeSize);
RSADEF void RSA_free(void* ptr);
RSADEF void RSA_deInit(void);

#endif /* RSA_HEADER */

#ifdef RSALLOC_IMPLEMENTATION

#ifdef RSA_VIRTUAL_ALLOC
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#elif defined(RSA_MMAP)
#include <sys/mman.h>
#endif


#ifndef RSA_BSS
void* RSA_memory;
#else
uintptr_t RSA_memory[RSA_DEFAULT_ARENA];
#endif

RSA_chunk RSA_chunks[RSA_CHUNK_COUNT] = {{0}};
size_t RSA_chunkLen = 0;
size_t RSA_freedChunks = 0;

size_t RSA_arenaSize = 0;

void RSA_init(size_t arenaSize) {
    #ifndef RSA_BSS
    if (arenaSize == 0)
    #endif
    {
        arenaSize = RSA_DEFAULT_ARENA;
    }
    
    RSA_arenaSize = arenaSize;

    #ifdef RSA_DEBUG
    printf("RSA_INFO : init'ing arena with size " RSA_PRINT_U64 " bytes\n", RSA_arenaSize);
    #endif

    #ifdef RSA_MALLOC
    RSA_memory = malloc(RSA_arenaSize);

    #ifdef RSA_DEBUG
    printf("RSA_INFO : init'd using malloc\n", RSA_memory);
    #endif
    #endif

    #ifdef RSA_MMAP
    RSA_memory = mmap(NULL, arenaSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);

    #ifdef RSA_DEBUG
    printf("RSA_INFO : init'd using unix's mmap\n");
    #endif
    #endif

    #ifdef RSA_VIRTUAL_ALLOC
    RSA_memory = VirtualAllocEx(GetCurrentProcess(), NULL, arenaSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

    #ifdef RSA_DEBUG
    printf("RSA_INFO : init'd using window's VirtualAlloc\n");
    #endif
    #endif

    #if defined(RSA_DEBUG) && defined(RSA_BSS)
    printf("RSA_INFO : init'd using bss\n");
    #endif

    #ifdef RSA_DEBUG
    printf("RSA_INFO : init'ed arena starting at %p\n", RSA_memory);
    #endif

    #if defined(RSA_DEBUG) && !RSA_BSS
    if (RSA_memory == NULL)
        printf("RSA_INFO : failed to allocate memory\n");
    #endif
}

void* RSA_alloc(size_t size) {
    size_t start = 0;

    if (RSA_chunkLen) {
        size_t i = 0; 
        for (i = 0; i < RSA_chunkLen; i++) {
            if (RSA_chunks[i].used) {
                start = RSA_chunks[i].start + RSA_chunks[i].len;
            }
            else if (RSA_chunks[i].len >= size) {
                start = RSA_chunks[i].start;
                
                #ifdef RSA_DEBUG
                    printf("RSA_INFO : Reusing free'd memory\n");
                #endif

                if (RSA_chunks[i].len - size || i == RSA_CHUNK_COUNT - 1) {
                    RSA_chunks[i].start += size;
                    RSA_chunks[i].len -= size;

                    #ifdef RSA_DEBUG
                    printf("RSA_INFO : Moving free chunk to %p\n", RSA_memory + start + size);
                    #endif

                    break;
                }

                #ifdef RSA_DEBUG
                printf("RSA_INFO : Deleting dead chunk at %p\n", RSA_memory + start);
                #endif

                memcpy(RSA_chunks + 1, &RSA_chunks[i + 1], RSA_chunkLen - i);
                RSA_chunks[RSA_CHUNK_COUNT - 1].used = 0;

                break;
            }
        }    
    }

    if (start + size >= RSA_arenaSize)
        return NULL;

    RSA_chunks[RSA_chunkLen].start = start;
    RSA_chunks[RSA_chunkLen].len = size;
    RSA_chunks[RSA_chunkLen].used = 1;
    RSA_chunkLen++;

    #ifdef RSA_DEBUG
    printf("RSA_INFO : allocated " RSA_PRINT_U64 " sized chunk at %p\n", size, RSA_memory + start);
    #endif

    return RSA_memory + start;
}

void* RSA_calloc(size_t size, size_t typeSize) {
    void* ptr = RSA_alloc(size * typeSize);
    if (ptr == NULL)
        return NULL;

    size_t i; 
    for (i = 0; i < size * typeSize; i++) {
        ((u8*)ptr)[i] = 0;  
    }

    return ptr;
}

void RSA_free(void* ptr) {
    size_t index = ptr - (void*)RSA_memory;

    size_t i = 0; 
    for (i = 0; i < RSA_chunkLen; i++) {
        if (index == RSA_chunks[i].start) {
            RSA_chunks[i].used = 0;
        
            #ifdef RSA_DEBUG
            printf("RSA_INFO : freed " RSA_PRINT_U64 " sized chunk at %p\n", RSA_chunks[i].len, RSA_memory + RSA_chunks[i].start);
            #endif

            if (i && RSA_chunks[i - 1].used == 0) {
                #ifdef RSA_DEBUG
                printf("RSA_INFO : merging free chunk %p with %p to create a " RSA_PRINT_U64 " sized free chunk\n", 
                    RSA_memory + RSA_chunks[i - 1].start,
                    RSA_memory + RSA_chunks[i].start,
                    RSA_chunks[i - 1].len + RSA_chunks[i].len
                );
                #endif

                RSA_chunks[i - 1].len += RSA_chunks[i].len;
                RSA_chunks[i].len = 0;
                
                memcpy(RSA_chunks + i, &RSA_chunks[i + 1], RSA_chunkLen - i);
                RSA_chunks[RSA_CHUNK_COUNT - 1].used = 0;
            }

            #ifdef RSA_DEBUG
            RSA_freedChunks += 1;
            #endif
        }
    }
}

void RSA_deInit(void) {
    #ifdef RSA_MALLOC
    free(RSA_memory);
    #endif

    #ifdef RSA_MMAP
    munmap(RSA_memory, RSA_arenaSize);
    #endif

    #ifdef RSA_VIRTUAL_ALLOC
    VirtualFreeEx(GetCurrentProcess(), RSA_memory, 0, MEM_RELEASE);
    #endif

    #ifdef RSA_DEBUG
    printf("RSA_INFO : DeInit with " RSA_PRINT_U64 " possible memory leaks\n", RSA_chunkLen - RSA_freedChunks);
    #endif

    RSA_chunkLen = 0;
    RSA_arenaSize = 0;
}

#endif /* RSALLOC_IMPLEMENTATION */

#ifdef __cplusplus
}
#endif