#include "stdafx.h"
#include "Mempool.h"
#include <windows.h>


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
		MIN_SLOT_MANAGER* newManager = (MIN_SLOT_MANAGER*)allocPages(sizeof(MIN_SLOT_MANAGER*));
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