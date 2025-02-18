#ifndef L1CACHE_H
#define L1CACHE_H

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

void initCache();
void accessL1(uint32_t, uint8_t *, uint32_t);

typedef struct CacheLine {
  uint8_t Valid;
  uint8_t Dirty;
  uint32_t Tag;
  /**
   * Adicionamos um array Data[BLOCK_SIZE] dentro de CacheLine, 
   * de modo a premitir que cada linha da cache armazene um bloco completo de dados.
   * Possibilitando que a cache retenha dados frequentemente acedidos
   */
  uint8_t Data[BLOCK_SIZE]; 
} CacheLine;

typedef struct Cache {
  uint32_t init;
  CacheLine lines[L1_LINES];
  /**
   * Agrupa todas as linhas da cache num vetor, com uma linha em cada posição.
   * Aumentando a probabilidade de encontrar os dados diretamente na cache (cache hit), 
   * o que reduz o tempo de acesso e melhora o desempenho geral
   */
} Cache;

/*********************** Interfaces *************************/

void read(uint32_t, uint8_t *);

void write(uint32_t, uint8_t *);

#endif
