#include "stdafx.h"
#include "Mempool.h"



void* Mempool::MIN_SLOT_MANAGER::allocSlot(int nSize) {
	if (nSize == 0) return NULL;
	int nSlots = (nSize + MIN_SLOT_SIZE - 1) / MIN_SLOT_SIZE;
	if (nSize > UINT_BIT_NUM * MIN_SLOT_SIZE) return NULL;

	for (auto i = 0; i < MIN_SLOT_INDEX_UINT_NUM; i++) {
		if (!(~bmIndex[i])) {
			// This 32 slots are used
			continue;
		}
		else if (auto ret = matchIndex(i, nSlots))
			return (void*)ret;
	}
	return nullptr;
}

void Mempool::MIN_SLOT_MANAGER::freeSlot(void* p) {
	auto pSlot = (MIN_SLOT*)p;
	unsigned int offset = pSlot - (MIN_SLOT*)this; // the slot offset
	unsigned int bit = offset & 0x0000001f; // offset % 32
	unsigned int byte = offset >> 5; // offset / 32
#ifdef DEBUG
	if (((bmIndex[byte] >> bit) & 0x01) == 0 || ((bmHeaderIndex[byte] >> bit) & 0x01) == 0) {
		std::cout << "Error: free a not aloocated slot" << std::endl;
	}
#endif
	unsigned int index = bmHeaderIndex[byte] >> bit;
	unsigned int mask = 0xffffffff;
	unsigned int len = 0;
	for (len = 1; len < UINT_BIT_NUM - bit; len++) {
		if ((index >> len) & 0x01) break;
	}
	mask >>= UINT_BIT_NUM - len;
	mask <<= bit;
	bmIndex[byte] ^= mask;
	bmHeaderIndex[byte] &= (~mask);
}

void Mempool::MIN_SLOT_MANAGER::init() {
	bmIndex[0] = 0x0000000f; // The first 4 slots are used by bitmap index abd header index
	bmHeaderIndex[0] = 0x0000001; // The header is at slots[0]
}

inline Mempool::MIN_SLOT* Mempool::MIN_SLOT_MANAGER::matchIndex(int i, unsigned int nSlots) {
	// Example:
	// I want 7*min_slots, and the index is 0x0000007f (tail: ...0111 1111b)
	unsigned int index = 0xffffffff >> (UINT_BIT_NUM - nSlots);
	// TODO: link besides
	for (unsigned int t = 0; t < UINT_BIT_NUM - nSlots; t++) {
		if ((bmIndex[i] >> t) & index) continue;
		// bmIndex[i]: 0x0000f80f -> 0x0000ffff (t = 4)
		// tail: 1111 1000 0000 1111 -> 1111 1111 1111 1111
		bmIndex[i] = bmIndex[i] | (index << t);
		// bmHeaderIndex[i]: 0x00008001 -> 0x00008011
		// tail: 0000 1000 0000 0001 -> 0000 1000 0001 0001
		bmHeaderIndex[i] = bmHeaderIndex[i] | (1 << t);
		return &slots[(i << 5) + t - 4];
	}
	return nullptr;
}


Mempool::Mempool()
{
	unsigned char* managerArray = (unsigned char*)allocPages(sizeof(MIN_SLOT_MANAGER) * 64);
	for (auto i = 0; i < 64; i++) {
		((MIN_SLOT_MANAGER*)(managerArray + i * sizeof(MIN_SLOT_MANAGER)))->init();
		minSlotManagerList.push_front(((MIN_SLOT_MANAGER*)(managerArray + i * sizeof(MIN_SLOT_MANAGER))));
	}
	minSlotManagerList;
}


Mempool::~Mempool()
{
}

void *Mempool::alloc(unsigned int size) {
	if (size <= UINT_BIT_NUM * MIN_SLOT_SIZE) {
		for (auto l : minSlotManagerList) {
			void* ret = l->allocSlot(size);
			if (ret) return ret;
		}
		// alloc new slot manager
		MIN_SLOT_MANAGER* newManager = (MIN_SLOT_MANAGER*)allocPages(sizeof(MIN_SLOT_MANAGER));
		newManager->init();
		minSlotManagerList.push_front(newManager);
		return newManager->allocSlot(size);
	}
	return NULL;
}

void Mempool::free(void* minSlot) {
	for (auto l : minSlotManagerList) {
		if (((unsigned int)minSlot & ~(PAGE_SIZE - 1)) == (unsigned int)l)
			return l->freeSlot(minSlot);
	}
#ifdef DEBUG
	std::cout << "free(): not fount this slot!" << std::endl;
#endif
}

void *Mempool::allocPages(unsigned int size) {
	static_assert(((((7 * PAGE_SIZE + PAGE_SIZE - 1) >> 12) << 12) & (PAGE_SIZE-1)) == 0, "error");
	unsigned int alligned = ((size + PAGE_SIZE - 1) >> 12) << 12;
	auto ret = (void*)VirtualAlloc(NULL, alligned, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	ZeroMemory(ret, size);
	return ret;
}

void Mempool::freePages(void* p) {
	VirtualFree(p, 0, MEM_RELEASE);
}