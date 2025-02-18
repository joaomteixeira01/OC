#ifndef L2_2WCACHE_H
#define L2_2WCACHE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "Cache.h"

void resetTime();

uint32_t getTime();

/****************  RAM memory (byte addressable) ***************/
void accessDRAM(uint32_t, uint8_t *, uint32_t);

/*********************** Cache *************************/

void initCacheL1();
void accessL1(uint32_t, uint8_t *, uint32_t);

void initCacheL2();
void accessL2(uint32_t, uint8_t *, uint32_t);

void initCache();

typedef struct CacheLine {
  uint8_t Valid;
  uint8_t Dirty;
  uint32_t Tag;
  uint32_t Time;
  uint8_t Data[BLOCK_SIZE]; 
} CacheLine;

typedef struct CacheL1 {
  uint32_t init;
  CacheLine lines[L1_LINES];
} CacheL1;

typedef struct Set {
  uint32_t init;
  CacheLine lines[WAYS];
} Set;

typedef struct Cache {
  uint32_t init;
  Set sets[L2_LINES/WAYS];
} CacheL2;

CacheLine* calculateLRU(Set *set);

/*********************** Interfaces *************************/

void read(uint32_t, uint8_t *);

void write(uint32_t, uint8_t *);

#endif
