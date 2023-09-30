//
// Created by TGG on 14/07/2022.
//The purpose of SharedMemory is to allow dynamic memory allocation in an embedded system with little RAM:
//1) Define a memory pool with a fixed size, to ensure that the program behaves deterministically i.e. the program can
//   be prevented from allocating so much memory that the code crashes
//2) Allow allocation and freeing of chunks of memory without having to change references into the memory
//
//Similar to malloc, with the difference that the memory pool is automatically defragmented
//
//The usage is similar to :
//
//1) Create a SharedMemory instance, specifying the number of bytes that should be allocated to it
//2) Create one or more SharedMemoryArrays using SharedMemory.getArray.  Each SharedMemoryArray consists of
//   a) an infoblock, used by the caller to store information about the array
//   b) an array of elements
//3a) Access the infoblock using SharedMemoryArray.getInfoBlock
//3b) Access the elements in the SharedMemoryArray using SharedMemoryArray.getElement
//Example:
//SharedMemory sm(200);
//MyBitOfData *mbod;
//SharedMemoryArray sma = sm.allocate(sizeof(MyBitOfData), 20);
//mbod = (MyBitOfData *)sma.getElement();
//sm.deallocate(sma);
//
// This class sets assertFailureCode in some cases of error
//

#ifndef WATERINGSYSTEMTESTHARNESS_SHAREDMEMORY_H
#define WATERINGSYSTEMTESTHARNESS_SHAREDMEMORY_H
#include <Arduino.h>
#include "SuccessCode.h"

class SharedMemoryArray;

class SharedMemory {
public:
  // size is the number of bytes to allocate to the pool.  Include 5 bytes overhead for each array you need.
  explicit SharedMemory(int size);
  ~SharedMemory() {delete[] m_memoryPool;}
  SharedMemory(const SharedMemory&) = delete;
  void operator=(const SharedMemory&) = delete;

  // allocate an array
  // you must check the allocated array before using it (isValid), because the allocation may fail:
  //   - there are too many arrays already defined
  //   - there isn't enough memory in the pool for the request
  SharedMemoryArray allocate(uint8_t infoBlockSize, uint8_t sizeOfElement, uint8_t numberOfElements);

  // resizes the allocated array (smaller, or larger if enough space)
  // preserves the contents
  // if insufficient room, returns failure code and does not resize the array.
  SuccessCode resize(SharedMemoryArray sma, uint8_t newNumberOfElements);

  // deallocate the array.  The sma is changed to be invalid
  void deallocate(SharedMemoryArray &sma);
  void deallocateAll();

private:
  struct SharedMemoryArrayHeader {
    uint16_t startIdx;          // index of the start of the allocated space for the array
    uint8_t infoBlockSize;      // number of bytes in the infoblock
    uint8_t sizeOfElement;      // size of each array element (bytes)
    uint8_t numberOfElements;   // number of array elements
  };

  friend class SharedMemoryArray;

  uint16_t freeSpaceLeft() const; // how many bytes are left in the pool?
  uint8_t firstFreeSMAidx(); // returns the first free SMA index;
  void defragment(uint16_t startOfGap, uint16_t sizeOfGap);  // shuffle up arrays to eliminate the given gap from the pool
  void insertSpace(uint16_t placeToInsert, uint16_t numberToInsert);  // insert numberToInsert byes at placeToInsert.  Assumes there is sufficient space.
  SuccessCode expand(SharedMemoryArrayHeader &smah, uint8_t newNumberOfElements);
  void shrink(SharedMemoryArrayHeader &smah, uint8_t newNumberOfElements);

  uint8_t *invalidSMAentry();  // returns the equivalent of a null pointer. Tries to return an unused entry in the pool
  uint8_t *getInfoBlock(uint8_t smaidx); // return the address of the infoblock.  DO NOT CACHE
  uint8_t *getArrayElement(uint8_t smaidx, uint8_t elementnumber); // return the first element of the array.  DO NOT CACHE

  uint8_t getAllocatedNumberOfElements(uint8_t smaidx); // how many elements are in this array?  0 if invalid

  uint8_t *m_memoryPool;  // array
  uint16_t m_memoryPoolSize; // size of allocated memory pool - including overheads
  uint16_t m_frontOfAllocatedArrays; // the index of the first uint8_t allocated from the pool (the pool is
                                     //  allocated backwards i.e. grows backwards from the end towards the start)
  uint8_t m_arrayCount; // number of arrays allocated
};

class SharedMemoryArray {
public:
    // get a single element from the array
    // if isValid() is false, the result of getElement is undefined.
    // Do not cache the pointer, you should dereference it immediately, because it may become invalid if any other
    //  arrays are deallocated from the pool i.e. if SharedMemory.deallocate is called.
  uint8_t *getElement(SharedMemory &sharedMemory, uint8_t index);
  uint8_t *getInfoBlock(SharedMemory &sharedMemory);

  // returns true if the sma is valid.  If not valid, the results of getElement are undefined
  // an sma will be invalid if it couldn't be allocated, or if it has been deallocated
  bool isValid() const;
  uint8_t getAllocatedNumberOfElements(SharedMemory &sharedMemory) const; // how many elements are in this array?  0 if invalid

  SharedMemoryArray();  // return an empty (invalid) SharedMemoryArray

  bool operator==(const SharedMemoryArray &rhs) const {return m_idx == rhs.m_idx;}  // shallow compare

protected:
  explicit SharedMemoryArray(uint8_t idx) : m_idx(idx) {};
  uint8_t m_idx;  // if == 0xff, the array is invalid

private:
  friend class SharedMemory;
};

#endif //WATERINGSYSTEMTESTHARNESS_SHAREDMEMORY_H
