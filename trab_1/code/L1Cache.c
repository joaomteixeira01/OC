#include "L1Cache.h"

uint8_t DRAM[DRAM_SIZE];
uint32_t time;
Cache L1Cache;

/**************** Time Manipulation ***************/
void resetTime() { time = 0; }

uint32_t getTime() { return time; }

/****************  RAM memory (byte addressable) ***************/
void accessDRAM(uint32_t address, uint8_t *data, uint32_t mode) {

  if (address >= DRAM_SIZE - WORD_SIZE + 1)
    exit(-1);

  if (mode == MODE_READ) {
    memcpy(data, &(DRAM[address]), BLOCK_SIZE);
    time += DRAM_READ_TIME;
  }

  if (mode == MODE_WRITE) {
    memcpy(&(DRAM[address]), data, BLOCK_SIZE);
    time += DRAM_WRITE_TIME;
  }
}

/*********************** L1 cache *************************/

void initCache() { 
  for (int b = 0; b < L1_LINES; b++) {
    L1Cache.lines[b].Valid = 0;
    L1Cache.lines[b].Dirty = 0;
    L1Cache.lines[b].Tag = 0;
    
    for (int w = 0; w < BLOCK_SIZE; w += WORD_SIZE) {
      L1Cache.lines[b].Data[w] = 0;
    }
  }
  L1Cache.init = 1;
}

void accessL1(uint32_t address, uint8_t *data, uint32_t mode) {

  uint32_t Offset, Index, Tag, MemAddress; 
  uint8_t TempBlock[BLOCK_SIZE];
  
  uint32_t OffsetMask = (1 << L1_OFFSET_BITS) - 1; // 00...00111111 (6 1's)
  uint32_t IndexMask = (1 << L1_INDEX_BITS) - 1; // 00...0011111111 (8 1's)

  Offset = address & OffsetMask; // extract Offset (maybe needed in the future??)
  Index = (address >> L1_OFFSET_BITS) & IndexMask; // extract Index
  Tag = (address >> (L1_INDEX_BITS + L1_OFFSET_BITS)); // extract Tag

  MemAddress = address >> L1_OFFSET_BITS; // clear Offset bits
  MemAddress = MemAddress << L1_OFFSET_BITS; // address of the block in memory

  /* access Cache*/

  CacheLine *Line = &L1Cache.lines[Index];

  if (!Line->Valid || Line->Tag != Tag) {         // if block not present - miss
    accessDRAM(MemAddress, TempBlock, MODE_READ); // get new block from DRAM
    
    if ((Line->Valid) && (Line->Dirty)) { // line has dirty block
      /**
       * Dados "sujos" na cache são escritos de volta na DRAM antes de se carrrgar um bloco novo
       */
      MemAddress = Line->Tag << (L1_INDEX_BITS + L1_OFFSET_BITS); // get address of the block in memory
      MemAddress = MemAddress | (Index << L1_OFFSET_BITS); // ""
      accessDRAM(MemAddress, Line->Data, MODE_WRITE); // then write back old block
    }

    memcpy(Line->Data, TempBlock,
           BLOCK_SIZE); // copy new block to cache line
    Line->Valid = 1;
    Line->Tag = Tag;
    Line->Dirty = 0;
  } // if miss, then replaced with the correct block (previous dirty block, if exists)

  if (mode == MODE_READ) {    // read data from cache line
    memcpy(data, &(Line->Data[Offset]), WORD_SIZE); // ????????? Word/Byte acess? with Offset?
    time += L1_READ_TIME;
  } 

  if (mode == MODE_WRITE) { // write data from cache line
    memcpy(&(Line->Data[Offset]), data, WORD_SIZE); // ????????? Word/Byte acess? with Offset?
    time += L1_WRITE_TIME;
    Line->Dirty = 1;
  } 
}

void read(uint32_t address, uint8_t *data) {
  accessL1(address, data, MODE_READ);
}

void write(uint32_t address, uint8_t *data) {
  accessL1(address, data, MODE_WRITE);
}
