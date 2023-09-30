//
// Created by TGG on 14/07/2022.
//

#include <new>
#include "SuccessCode.h"
#include "SystemStatus.h"
#include "SharedMemory.h"

// the pool is allocated as follows:
//  At the start of the pool there is an array of SharedMemoryArrayHeaders
//  This grows up as each new array is allocated.  When arrays are freed it will only shrink back if the last
//  array is freed; otherwise it leaves gaps marked with startIdx == SM_INVALID_START_IDX
//  The memory allocated for the arrays grows backwards from the end of the pool.
//  Whenever an array is freed, all the other arrays are defragmented immediately and the SharedMemoryArrayHeaders are
//   updated

const uint8_t SMA_MAX_ARRAY_COUNT = 0xfe;
const uint8_t SMA_INVALID = 0xff;
const uint16_t SM_INVALID_START_IDX = 0xffff;

SharedMemory::SharedMemory(int size) {
  m_arrayCount = 0;
  m_memoryPool = new uint8_t[size];
  m_memoryPoolSize = (m_memoryPool != nullptr) ? size : 0;
  m_frontOfAllocatedArrays = size;
}

SharedMemoryArray SharedMemory::allocate(uint8_t infoBlockSize, uint8_t sizeOfElement, uint8_t numberOfElements) {
  uint8_t newArrayIdx = firstFreeSMAidx();
  bool needExtraHeader = (newArrayIdx >= m_arrayCount);
  uint16_t arrayDataSize = infoBlockSize + (uint16_t)sizeOfElement * (uint16_t)numberOfElements;
  int extraSpaceNeeded;
  extraSpaceNeeded =  arrayDataSize + (needExtraHeader ? (uint16_t)sizeof(SharedMemoryArrayHeader) : 0);
  if (m_arrayCount >= SMA_MAX_ARRAY_COUNT ||  extraSpaceNeeded > freeSpaceLeft()) {
    return SharedMemoryArray(SMA_INVALID);
  }

  if (needExtraHeader) ++m_arrayCount;
  m_frontOfAllocatedArrays -= arrayDataSize;

  struct SharedMemoryArrayHeader &newSMAheader = ((SharedMemoryArrayHeader *)(m_memoryPool))[newArrayIdx];
  newSMAheader.startIdx = m_frontOfAllocatedArrays;
  newSMAheader.infoBlockSize = infoBlockSize;
  newSMAheader.sizeOfElement = sizeOfElement;
  newSMAheader.numberOfElements = numberOfElements;

  return SharedMemoryArray(newArrayIdx);
}

void SharedMemory::deallocate(SharedMemoryArray &sma) {
  if (!sma.isValid()) return;
  uint8_t smaIdx = sma.m_idx;
  if (smaIdx >= m_arrayCount) {
    assertFailureCode = ASSERT_SMA_DEALLOCATED_TWICE;
    return;
  }
  auto *smaHeaders = (SharedMemoryArrayHeader *)(m_memoryPool);
  struct SharedMemoryArrayHeader &smaHeader = smaHeaders[smaIdx];
  if (smaHeader.startIdx == SM_INVALID_START_IDX) {
    assertFailureCode = ASSERT_SMA_DEALLOCATED_TWICE;
    return;
  }

  defragment(smaHeader.startIdx,
             smaHeader.infoBlockSize + smaHeader.sizeOfElement * smaHeader.numberOfElements);
  sma.m_idx = SMA_INVALID;
  smaHeader.startIdx = SM_INVALID_START_IDX;

  // shrink the SMAheaders array if possible
  uint8_t i = m_arrayCount;
  while (i > 0 && smaHeaders[i-1].startIdx == SM_INVALID_START_IDX) {
    --i;
  }
  m_arrayCount = i;
}
uint16_t SharedMemory::freeSpaceLeft() const {
  return m_frontOfAllocatedArrays - m_arrayCount * (uint16_t)sizeof(SharedMemoryArrayHeader);
}

uint8_t SharedMemory::firstFreeSMAidx() {
  auto *smaHeaders = (SharedMemoryArrayHeader *)(m_memoryPool);

  for (uint8_t i = 0; i < m_arrayCount; ++i) {
    if (smaHeaders[i].startIdx == SM_INVALID_START_IDX) {
      return i;
    }
  }
  return m_arrayCount;
}

// shuffle up all allocated arrays to eliminate the gap
void SharedMemory::defragment(uint16_t startOfGap, uint16_t sizeOfGap) {
  const int srcIdxMoveEnd = startOfGap - 1;
  const int srcIdxMoveStart = m_frontOfAllocatedArrays;
  int dstIdxMove = startOfGap + sizeOfGap - 1;

  for (int i = srcIdxMoveEnd; i >= srcIdxMoveStart; --i) {
    m_memoryPool[dstIdxMove--] = m_memoryPool[i];
  }

  auto *smaHeaders = (SharedMemoryArrayHeader *)(m_memoryPool);
  for (int i = 0; i < m_arrayCount; ++i) {
    if (smaHeaders[i].startIdx != SM_INVALID_START_IDX && smaHeaders[i].startIdx < startOfGap) {
      smaHeaders[i].startIdx += sizeOfGap;
    }
  }
  m_frontOfAllocatedArrays += sizeOfGap;
}

uint8_t *SharedMemory::getArrayElement(uint8_t smaidx, uint8_t elementnumber) {
  if (smaidx == SMA_INVALID || smaidx >= m_arrayCount) {
    assertFailureCode = ASSERT_SMA_USED_WHILE_INVALID;
    return nullptr;
  }
  auto *smaHeaders = (SharedMemoryArrayHeader *)(m_memoryPool);
  struct SharedMemoryArrayHeader &smaHeader = smaHeaders[smaidx];
  if (smaHeader.startIdx == SM_INVALID_START_IDX) {
    assertFailureCode = ASSERT_SMA_USED_WHILE_INVALID;
    return nullptr;
  }
  if (elementnumber >= smaHeader.numberOfElements) {
    assertFailureCode = ASSERT_SMA_ELEMENT_OUT_OF_BOUNDS;
    return nullptr;
  }
  return m_memoryPool + smaHeader.startIdx + smaHeader.infoBlockSize + smaHeader.sizeOfElement * elementnumber;
}

void SharedMemory::deallocateAll() {
  m_frontOfAllocatedArrays = m_memoryPoolSize;
  m_arrayCount = 0;
}

uint8_t *SharedMemory::invalidSMAentry() {
  // return a spot somewhere in the unallocated region and hope for the best
  return m_memoryPool + (m_arrayCount * sizeof(SharedMemoryArrayHeader) + m_frontOfAllocatedArrays)/2;
}

void SharedMemory::shrink(SharedMemoryArrayHeader &smah, uint8_t newNumberOfElements) {
  int spaceRemoved = (smah.numberOfElements - newNumberOfElements) * (int)smah.sizeOfElement;
  smah.numberOfElements = newNumberOfElements;
  defragment(smah.startIdx + smah.infoBlockSize + smah.numberOfElements * (int)smah.sizeOfElement, spaceRemoved);
}

SuccessCode SharedMemory::expand(SharedMemoryArrayHeader &smah, uint8_t newNumberOfElements) {
  int extraSpaceNeeded = (newNumberOfElements - smah.numberOfElements) * (int)smah.sizeOfElement;
  if (extraSpaceNeeded > freeSpaceLeft()) return {ErrorCode::InsufficientMemory};
  insertSpace(smah.startIdx + smah.infoBlockSize + smah.numberOfElements * (int)smah.sizeOfElement, extraSpaceNeeded);
  smah.numberOfElements = newNumberOfElements;
  return SuccessCode::success();
}

SuccessCode SharedMemory::resize(SharedMemoryArray sma, uint8_t newNumberOfElements) {
  if (sma.m_idx == SMA_INVALID || sma.m_idx >= m_arrayCount) {
    assertFailureCode = ASSERT_SMA_USED_WHILE_INVALID;
    return {ErrorCode::AssertionFailed};
  }
  auto *smaHeaders = (SharedMemoryArrayHeader *)(m_memoryPool);
  struct SharedMemoryArrayHeader &smaHeader = smaHeaders[sma.m_idx];
  if (smaHeader.startIdx == SM_INVALID_START_IDX) {
    assertFailureCode = ASSERT_SMA_USED_WHILE_INVALID;
    return SuccessCode(ErrorCode::AssertionFailed);
  }
  if (smaHeader.numberOfElements == newNumberOfElements) return SuccessCode::success();
  if (newNumberOfElements < smaHeader.numberOfElements) {
    shrink(smaHeader, newNumberOfElements);
    return SuccessCode::success();
  }
  return expand(smaHeader, newNumberOfElements);
}

void SharedMemory::insertSpace(uint16_t placeToInsert, uint16_t numberToInsert) {
  const int srcIdxMoveEnd = placeToInsert - 1;
  const int srcIdxMoveStart = m_frontOfAllocatedArrays;
  int dstIdxMove = srcIdxMoveStart - numberToInsert;

  for (int i = srcIdxMoveStart; i <= srcIdxMoveEnd; ++i) {
    m_memoryPool[dstIdxMove++] = m_memoryPool[i];
  }

  auto *smaHeaders = (SharedMemoryArrayHeader *)(m_memoryPool);
  for (int i = 0; i < m_arrayCount; ++i) {
    if (smaHeaders[i].startIdx != SM_INVALID_START_IDX && smaHeaders[i].startIdx < placeToInsert) {
      smaHeaders[i].startIdx -= numberToInsert;
    }
  }
  m_frontOfAllocatedArrays -= numberToInsert;
}

uint8_t SharedMemory::getAllocatedNumberOfElements(uint8_t smaidx) {
  if (smaidx == SMA_INVALID || smaidx >= m_arrayCount) {
    assertFailureCode = ASSERT_SMA_USED_WHILE_INVALID;
    return 0;
  }
  auto *smaHeaders = (SharedMemoryArrayHeader *) (m_memoryPool);
  struct SharedMemoryArrayHeader &smaHeader = smaHeaders[smaidx];
  if (smaHeader.startIdx == SM_INVALID_START_IDX) {
    assertFailureCode = ASSERT_SMA_USED_WHILE_INVALID;
    return 0;
  }
  return smaHeader.numberOfElements;
}

uint8_t *SharedMemory::getInfoBlock(uint8_t smaidx) {
  if (smaidx == SMA_INVALID || smaidx >= m_arrayCount) {
    assertFailureCode = ASSERT_SMA_USED_WHILE_INVALID;
    return nullptr;
  }
  auto *smaHeaders = (SharedMemoryArrayHeader *) (m_memoryPool);
  struct SharedMemoryArrayHeader &smaHeader = smaHeaders[smaidx];
  if (smaHeader.startIdx == SM_INVALID_START_IDX) {
    assertFailureCode = ASSERT_SMA_USED_WHILE_INVALID;
    return nullptr;
  }
  return m_memoryPool + smaHeader.startIdx;
}

//---------------

SharedMemoryArray::SharedMemoryArray() {
  m_idx = SMA_INVALID;
}
// callers should check for nullptr
uint8_t *SharedMemoryArray::getElement(SharedMemory &sharedMemory, uint8_t index) {
  return sharedMemory.getArrayElement(m_idx, index);
}

bool SharedMemoryArray::isValid() const {
  return m_idx != SMA_INVALID;
}

uint8_t SharedMemoryArray::getAllocatedNumberOfElements(SharedMemory &sharedMemory) const {
  return sharedMemory.getAllocatedNumberOfElements(m_idx);
}

uint8_t *SharedMemoryArray::getInfoBlock(SharedMemory &sharedMemory) {
  return sharedMemory.getInfoBlock(m_idx);
}
