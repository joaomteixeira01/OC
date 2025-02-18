#include "L2_2WCache.h"

uint8_t DRAM[DRAM_SIZE];
uint32_t time;
CacheL1 L1Cache;
CacheL2 L2Cache;

/**************** Time Manipulation ***************/
void resetTime() { time = 0; }

uint32_t getTime() { return time; }

/**************** Aux ****************/
CacheLine* calculateLRU(Set *set) {
  uint32_t time0 = set->lines[0].Time;
  uint32_t lruLine = 0;

  for (int i = 0; i < WAYS; i++) {
    uint32_t timei = set->lines[i].Time;
    
    if (timei > time0) {
      time0 = timei; 
      lruLine = i;
    }
  }

  return (CacheLine*)&set->lines[lruLine];
}

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

/********************** Inits **********************/

void initCache() {
  initCacheL1();
  initCacheL2();
}

void initCacheL1() {
  for (int b = 0; b < L1_LINES; b++) {
    L1Cache.lines[b].Valid = 0;
    L1Cache.lines[b].Dirty = 0;
    L1Cache.lines[b].Tag = 0;
    L1Cache.lines[b].Time = 0;
    
    for (int w = 0; w < BLOCK_SIZE; w += WORD_SIZE) {
      L1Cache.lines[b].Data[w] = 0;
    }
  }
  L1Cache.init = 1;
}

void initCacheL2() {
  for (int s = 0; s < (L2_LINES / WAYS); s++) {
      
    for (int b = 0; b < WAYS; b++) {
      L2Cache.sets[s].lines[b].Valid = 0;
      L2Cache.sets[s].lines[b].Dirty = 0;
      L2Cache.sets[s].lines[b].Tag = 0;
      L2Cache.sets[s].lines[b].Time = 0;
      
      for (int w = 0; w < BLOCK_SIZE; w += WORD_SIZE) {
        L2Cache.sets[s].lines[b].Data[w] = 0;
      }
      
    }
    L2Cache.sets[s].init = 0;

  }
  L2Cache.init = 1;

}
/*********************** L1 cache *************************/

void accessL1(uint32_t address, uint8_t *data, uint32_t mode) {
  //printf("address: %d\n", address);
  //printf("value: %d\n", *data);
  //printf("points to address: %p\n", data);
  //printf("\n");

  uint32_t Offset;  // Offset??
  uint32_t Index, Tag, L2Address;
  uint8_t TempBlock[BLOCK_SIZE];
  
  uint32_t OffsetMask = (1 << L1_OFFSET_BITS) - 1; // 00...00111111 (6 1's)
  uint32_t IndexMask = (1 << L1_INDEX_BITS) - 1; // 00...0001111111 (8 1's)

  Offset = address & OffsetMask; // extract Offset (maybe needed in the future??)
  Index = (address >> L1_OFFSET_BITS) & IndexMask; // extract Index
  Tag = (address >> (L1_INDEX_BITS + L1_OFFSET_BITS)); // extract Tag
  //printf("Index: %d\n", Index);
  //printf("Tag: %d\n", Tag);


  CacheLine *Line = &L1Cache.lines[Index];

  L2Address = address >> L2_OFFSET_BITS; // clear Offset bits
  L2Address = L2Address << L2_OFFSET_BITS; // address of the block in memory
  //printf("MemAddress: %d\n", MemAddress);


  /* access Cache*/

  if (!Line->Valid || Line->Tag != Tag) {         // if block not present - miss
    accessL2(L2Address, TempBlock, MODE_READ); // get new block from L2

    if ((Line->Valid) && (Line->Dirty)) { // line has dirty block
      L2Address = Line->Tag << (L2_INDEX_BITS + L2_OFFSET_BITS); // get address of the block in L2
      L2Address = L2Address | (Index << L2_OFFSET_BITS); // ""
      accessL2(L2Address, Line->Data, MODE_WRITE); // then write back old block
    }

    memcpy(Line->Data, TempBlock,
           BLOCK_SIZE); // copy new block to cache line
    Line->Valid = 1;
    Line->Tag = Tag;
    Line->Dirty = 0;
  } // if miss, then replaced with the correct block


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

/*********************** L2 cache *************************/

void accessL2(uint32_t address, uint8_t *data, uint32_t mode) {
  //printf("address: %d\n", address);
  //printf("value: %d\n", *data);
  //printf("points to address: %p\n", data);
  //printf("\n");

  uint32_t /*Offset,*/ Index, Tag, MemAddress; 
  uint8_t TempBlock[BLOCK_SIZE];
  
  //uint32_t OffsetMask = (1 << L2_OFFSET_BITS) - 1; // 00...00111111 (6 1's)
  uint32_t IndexMask = (1 << L2_INDEX_BITS) - 1; // 00...0111111111 (8 1's)

  //Offset = address & OffsetMask; // extract Offset (maybe needed in the future??)
  Index = (address >> L2_OFFSET_BITS) & IndexMask; // extract Index
  Tag = (address >> (L2_INDEX_BITS + L2_OFFSET_BITS)); // extract Tag
  //printf("Index: %d\n", Index);
  //printf("Tag: %d\n", Tag);

  MemAddress = address >> L2_OFFSET_BITS; // clear Offset bits
  MemAddress = MemAddress << L2_OFFSET_BITS; // address of the block in memory
  //printf("MemAddress: %d\n", MemAddress);

  /* access Cache*/

  Set *Set = &L2Cache.sets[Index];

  for (int i = 0; i < WAYS; i++) {
    CacheLine *Line = &Set->lines[i];

    if (Line->Valid && Line->Tag == Tag) { // Hit in one of the set lines
      
      if (mode == MODE_READ) { // read data from cache line
        memcpy(data, &(Line->Data), BLOCK_SIZE);
        time += L2_READ_TIME;
      }

      if (mode == MODE_WRITE) { // write data from cache line
        memcpy(&(Line->Data), data, BLOCK_SIZE);
        time += L2_WRITE_TIME;
        Line->Dirty = 1;
      }

      Line->Time = getTime();
      return;
    }
  }

  // Miss

  CacheLine *lruLine = calculateLRU(Set);

  if ((lruLine->Valid) && (lruLine->Dirty)) { // line has dirty block
    MemAddress = lruLine->Tag << (L2_INDEX_BITS + L2_OFFSET_BITS); // get address of the block in L2
    MemAddress = MemAddress | (Index << L2_OFFSET_BITS); // ""
    accessDRAM(MemAddress, lruLine->Data, MODE_WRITE); // then write back old block
  }
  
  accessDRAM(MemAddress, TempBlock, MODE_READ); // get new block from DRAM

  memcpy(lruLine->Data, TempBlock,
          BLOCK_SIZE); // copy new block to cache line
  lruLine->Valid = 1;
  lruLine->Tag = Tag;
  lruLine->Dirty = 0;
  lruLine->Time = getTime();
  // if miss, then replaced with the correct block

  if (mode == MODE_READ) {    // read data from cache line
    memcpy(data, &(lruLine->Data), BLOCK_SIZE); // ????????? Word/Byte acess? with Offset?
    time += L2_READ_TIME;
  } 

  if (mode == MODE_WRITE) { // write data from cache line
    memcpy(&(lruLine->Data), data, BLOCK_SIZE); // ????????? Word/Byte acess? with Offset?
    time += L2_WRITE_TIME;
    lruLine->Dirty = 1;
  }
}
