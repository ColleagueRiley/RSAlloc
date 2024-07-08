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

#ifndef RSA_DEFAULT_CHUNKS
#define RSA_DEFAULT_CHUNKS RSA_DEFAULT_ARENA / 8
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
#include <assert.h>

typedef struct RSA_chunk {
    size_t len;
    b8 used;
    struct RSA_chunk* next;
    struct RSA_chunk* prev;
} RSA_chunk;

RSADEF b8 RSA_init(size_t arenaSize);
RSADEF void* RSA_alloc(size_t size);
RSADEF void* RSA_calloc(size_t size, size_t typeSize);
RSADEF RSA_chunk RSA_getChunkInfo(void* ptr);
RSADEF b8 RSA_free(void* ptr);
RSADEF b8 RSA_deInit(void);

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

size_t RSA_arenaSize = 0;

RSA_chunk* RSA_freeList;

b8 RSA_init(size_t arenaSize) {
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

    #ifndef RSA_BSS
    if (RSA_memory == NULL) {
        #if defined(RSA_DEBUG)
        printf("RSA_INFO : failed to allocate memory\n");
        #endif
        return 0;
    }
    #endif

    RSA_freeList = (RSA_chunk*)RSA_memory;
    RSA_freeList->next = NULL;
    RSA_freeList->used = 0;
    RSA_freeList->prev = NULL;
    RSA_freeList->len = RSA_arenaSize;

    return 1;
}

void* RSA_alloc(size_t size) {
    size += sizeof(RSA_chunk);
    if (size == 0 || size > RSA_arenaSize) {
        #ifdef RSA_DEBUG
        printf("RSA_INFO: chunk size out of bounds\n");
        #endif

        return NULL;
    }


    #ifdef RSA_DEBUG
    b8 reused = 0;
    #endif

    RSA_chunk* findchunk = RSA_freeList;
    while (findchunk->next != NULL && (findchunk->len < size || findchunk->used)) {
        findchunk = findchunk->next;
        #ifdef RSA_DEBUG
        reused = 1;
        #endif
    }

    if (findchunk->next == NULL && (findchunk->len < size || findchunk->used)) {
        #ifdef RSA_DEBUG
        printf("RSA_INFO: no room for chunk found\n");
        #endif

        return NULL;
    }

    #ifdef RSA_DEBUG
    if (reused)
        printf("RSA_INFO: Reusing free'd memory\n");
    #endif

    if (RSA_freeList->next != NULL) {
        RSA_chunk* head = RSA_freeList->next;
        RSA_freeList->next = (RSA_chunk*)(RSA_freeList + size);
        RSA_freeList->next->next = head;
    } else {
        RSA_freeList->next = (RSA_chunk*)(RSA_freeList + size);
    }

    RSA_freeList->next->prev = RSA_freeList;
    RSA_freeList->next->len =  RSA_freeList->len - size;
    RSA_freeList->next->used = 0;
    RSA_freeList = RSA_freeList->next;

    #ifdef RSA_DEBUG
    printf("RSA_INFO : allocated " RSA_PRINT_U64 " sized chunk at %p\n", size, ((char*)findchunk));
    #endif

    findchunk->len = size;
    findchunk->used = 1;

    return (void*)(((char*)findchunk) + sizeof(RSA_chunk));
}

void* RSA_calloc(size_t size, size_t typeSize) {
    void* ptr = RSA_alloc(size * typeSize);
    assert(ptr != NULL);

    size_t i;
    for (i = 0; i < size * typeSize; i++) {
        ((u8*)ptr)[i] = 0;
    }

    return ptr;
}

RSA_chunk RSA_getChunkInfo(void* ptr) {
    return *((RSA_chunk*)ptr - 1);
}

b8 RSA_free(void* ptr) {
    assert(ptr != NULL);

    RSA_chunk* chunk = (RSA_chunk*)ptr - 1;
    chunk->used = 0;

    #ifdef RSA_DEBUG
    printf("RSA_INFO : freed " RSA_PRINT_U64 " sized chunk at %p\n", chunk->len, chunk);
    #endif

    if (chunk->prev != NULL && chunk->prev->used == 0) {
        #ifdef RSA_DEBUG
        printf("RSA_INFO : merging free chunk %p with %p to create a " RSA_PRINT_U64 " sized free chunk\n",
                        chunk->prev, chunk, chunk->prev->len + chunk->len);
        #endif

        chunk->prev->len += chunk->len;
        chunk = chunk->prev;
    }

    RSA_chunk* head = RSA_freeList;
    RSA_freeList = chunk;
    RSA_freeList->next = head;

    return 1;
}

b8 RSA_deInit(void) {
    b8 result = 1;

    #ifdef RSA_DEBUG
    size_t leaks = 0;
    /*RSA_chunk* findchunk = RSA_freeList;

    while (findchunk != NULL) {
        if (findchunk->used)
            leaks += 1;
        findchunk = findchunk->next;
    }*/

    printf("RSA_INFO : DeInit with " RSA_PRINT_U64 " possible memory leaks\n", leaks);
    #endif

    #ifdef RSA_MALLOC
    free(RSA_memory);
    #endif

    #ifdef RSA_MMAP
    result = (munmap(RSA_memory, RSA_arenaSize) == 0);
    #endif

    #ifdef RSA_VIRTUAL_ALLOC
    result = VirtualFreeEx(GetCurrentProcess(), RSA_memory, 0, MEM_RELEASE);
    #endif

    RSA_arenaSize = 0;
    RSA_freeList = NULL;
    return result;
}

#endif /* RSALLOC_IMPLEMENTATION */

#ifdef __cplusplus
}
#endif
