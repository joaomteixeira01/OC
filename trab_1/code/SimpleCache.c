#include "L1Cache.h"

uint8_t L1Cache[L1_SIZE];
uint8_t L2Cache[L2_SIZE];
uint8_t DRAM[DRAM_SIZE];
uint32_t time;
Cache SimpleCache;
CacheLine LineL2;

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
  SimpleCache.init = 0;
  LineL2.Valid = 0;  // Inicializar L2 cache
}

void accessL1(uint32_t address, uint8_t *data, uint32_t mode) {
  //printf("address: %d\n", address);
  //printf("value: %d\n", *data);
  //printf("points to address: %p\n", data);
  //printf("\n");

  uint32_t Offset;
  uint32_t Index, Tag, MemAddress; // Offset??
  uint8_t TempBlock[BLOCK_SIZE];

  /* init cache */
  if (SimpleCache.init == 0) {
    for (int i = 0; i < L1_LINES; i++) {
      SimpleCache.lines[i].Valid = 0;
    }
    SimpleCache.init = 1;
  }
  
  uint32_t OffsetMask = (1 << L1_OFFSET_BITS) - 1; // 00...00111111 (6 1's)
  uint32_t IndexMask = (1 << L1_INDEX_BITS) - 1; // 00...0011111111 (8 1's)

  Offset = address & OffsetMask; // extract Offset (maybe needed in the future??)
  Index = (address >> L1_OFFSET_BITS) & IndexMask; // extract Index
  Tag = (address >> (L1_INDEX_BITS + L1_OFFSET_BITS)); // extract Tag
  //printf("Index: %d\n", Index);
  //printf("Tag: %d\n", Tag);


  CacheLine *Line = &SimpleCache.lines[Index];

  MemAddress = address >> L1_OFFSET_BITS; // clear Offset bits
  MemAddress = MemAddress << L1_OFFSET_BITS; // address of the block in memory
  //printf("MemAddress: %d\n", MemAddress);


  /* access Cache*/

  if (!Line->Valid || Line->Tag != Tag) {         // if block not present - miss
    //accessDRAM(MemAddress, TempBlock, MODE_READ); // get new block from DRAM
    accessL2(MemAddress, TempBlock, MODE_READ);

    if ((Line->Valid) && (Line->Dirty)) { // line has dirty block
      MemAddress = Line->Tag << (L1_INDEX_BITS + L1_OFFSET_BITS); // get address of the block in memory
      MemAddress = MemAddress | (Index << L1_OFFSET_BITS); // ""
      accessDRAM(MemAddress, &(L1Cache[Index * BLOCK_SIZE]), MODE_WRITE); // then write back old block
    }

    memcpy(&(L1Cache[Index * BLOCK_SIZE]), TempBlock, BLOCK_SIZE); // copy new block to cache line
    Line->Valid = 1;
    Line->Tag = Tag;
    Line->Dirty = 0;
  } // if miss, then replaced with the correct block


  if (mode == MODE_READ) {    // read data from cache line

    if (Offset % WORD_SIZE != 0) {
        printf("Error: Unaligned word read at address %u\n", address);
        exit(-1);  // Stop program ( DOESNT SUPPORT BYTE/BLOCK ACCESS!)
    }

    memcpy(data, &(L1Cache[Index * BLOCK_SIZE + Offset]), WORD_SIZE); // ????????? Word/Byte acess? with Offset?
    time += L1_READ_TIME;
  } 

  if (mode == MODE_WRITE) { // write data from cache line
    
    if (Offset % WORD_SIZE != 0) {
        printf("Error: Unaligned word write at address %u\n", address);
        exit(-1);  // Stop program ( DOESNT SUPPORT BYTE/BLOCK ACCESS!)
    }
    
    memcpy(&(L1Cache[Index * BLOCK_SIZE + Offset]), data, WORD_SIZE); // ????????? Word/Byte acess? with Offset?
    time += L1_WRITE_TIME;
    Line->Dirty = 1;
  } 
}

/*********************** L2 cache *************************/

void accessL2(uint32_t address, uint8_t *data, uint32_t mode) {

  uint32_t Tag, MemAddress;
  uint8_t TempBlock[BLOCK_SIZE];
  
  Tag = address >> 4; // Ajustar o tag para a cache L2
  MemAddress = address >> 4;
  MemAddress = MemAddress << 4; // Endereço do bloco na memória

  /* access L2 Cache */
  if (!LineL2.Valid || LineL2.Tag != Tag) {       // Miss no L2
    accessDRAM(MemAddress, TempBlock, MODE_READ); // Acessar DRAM se falha no L2
    
    if (LineL2.Valid && LineL2.Dirty) {                       // Linha com bloco sujo
      accessDRAM(LineL2.Tag << 4, &(L2Cache[0]), MODE_WRITE); // Escreve o bloco sujo para a DRAM
    }

    memcpy(&(L2Cache[0]), TempBlock, BLOCK_SIZE); // Copiar bloco para cache L2
    LineL2.Valid = 1;
    LineL2.Tag = Tag;
    LineL2.Dirty = 0;
  }

  if (mode == MODE_READ) { // Leitura da cache L2
    memcpy(data, &(L2Cache[address % BLOCK_SIZE]), WORD_SIZE);
    time += L2_READ_TIME;
  }

  if (mode == MODE_WRITE) { // Escrita na cache L2
    memcpy(&(L2Cache[address % BLOCK_SIZE]), data, WORD_SIZE);
    time += L2_WRITE_TIME;
    LineL2.Dirty = 1;
  }
}

/*********************** Interface ***********************/

void read(uint32_t address, uint8_t *data) {
  accessL1(address, data, MODE_READ);
}

void write(uint32_t address, uint8_t *data) {
  accessL1(address, data, MODE_WRITE);
}
