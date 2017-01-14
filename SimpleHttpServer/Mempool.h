#pragma once

#include <list>
#include <iostream>

#define DEBUG

/*
Mempool - a memory pool to avoid frequently syscall
=========================================================================
the kinds of slots:
	MIN_SLOT = 16Bytes ~ 32 * 16Bytes = 512Bytes
	A BITMAP_INDEX use 32Bytes indexing 254 MIN_SLOTS in 1 PAGE(4KB)
	A BITMAP_HEADER_INDEX use 32Bytes

	[TODO]MIDDLE_SLOT = 512Bytes ~ 4096Bytes
	[TODO]A BITMAP_INDEX use 256Bytes indexing 2047 MIDDLE_SLOT in 4096 PAGEs(8MB)
	[TODO]A BITMAP_HEADER_INDEX use 256Bytes

	[TODO]MAX_SLOT = MMAP

the bitmap index:
	the low bit represent low address of slot, eg:
	BITMAP INDEX: 0 0 0 0 0 0 0 0 1 1 1 0 0 0 1 1 of bmIndex(0)
		represents that slots[0], [1], [5], [6], [7] was used
the bitmap header index:
	the low bit represent low address of slot, eg:
	BITMAP INDEX: 0 0 0 0 0 0 0 0 1 0 1 0 0 0 1 1 of bmHeaderIndex(0)
		represents that slots[0], [1], [5], [6], [7] was used as first

Use case:
	frequent allocate few pieces of memory, and free them quickly;

Brenchmark:
	#define PARALLEL 1024
	#define LOOP_NUM (64*1024/PARALLEL)
	void* p[1024];
	auto h = analyse->suspend();
	for (auto i = 0; i < LOOP_NUM; i++) {
		for (auto j = 0; j < PARALLEL; j++)
			p[j] = malloc(sizeof(IOCPModule::PER_SOCKET_CONTEXT));
		for (auto j = 0; j < PARALLEL; j++)
			free(p[j]);
	}
	analyse->recover(h, "malloc");
	h = analyse->suspend();
	for (auto i = 0; i < LOOP_NUM; i++) {
		for (auto j = 0; j < PARALLEL; j++)
			p[j] = mempool->alloc(sizeof(IOCPModule::PER_SOCKET_CONTEXT));
		for (auto j = 0; j < PARALLEL; j++)
			mempool->free(p[j]);
	}
	analyse->recover(h, "pool");

	PARALLEL = 4095
	malloc                  1s+1.7664
	pool                    0s+0.177244

	PARALLEL = 1024
	malloc                  0s+0.496696
	pool                    0s+0.0430767

	PARALLERL = 64
	malloc                  0s+0.256422
	pool                    0s+0.00449659
*/

#define PAGE_SIZE (4096)
#define MIN_SLOT_SIZE (16)
#define MIN_SLOT_INDEX_BIT_NUM (PAGE_SIZE / MIN_SLOT_SIZE)
#define MIN_SLOT_INDEX_BYTE_NUM (MIN_SLOT_INDEX_BIT_NUM / 8)
#define MIN_SLOT_INDEX_UINT_NUM (MIN_SLOT_INDEX_BYTE_NUM / 4)
static_assert(MIN_SLOT_INDEX_BYTE_NUM % MIN_SLOT_SIZE == 0, "index bytes wastes");
static_assert(MIN_SLOT_INDEX_UINT_NUM == 8, "a");
#define PAGE_MIN_SLOT_NUM ((PAGE_SIZE - MIN_SLOT_INDEX_BYTE_NUM * 2) / MIN_SLOT_SIZE)
#define UINT_BIT_NUM (sizeof(unsigned int) * 8)
static_assert(UINT_BIT_NUM == 32, "this version only support x86");

class Mempool
{
public:
	Mempool();
	~Mempool();

	void *alloc(unsigned int size);
	void free(void* minSlot);
	void *allocPages(unsigned int size);
	void freePages(void* p);

	typedef unsigned int MIN_SLOT_BITMAP[MIN_SLOT_INDEX_UINT_NUM];
	typedef unsigned char MIN_SLOT[MIN_SLOT_SIZE];
	typedef struct _MIN_SLOT_MANAGER {
		MIN_SLOT_BITMAP bmIndex;
		MIN_SLOT_BITMAP bmHeaderIndex;
		MIN_SLOT slots[PAGE_MIN_SLOT_NUM];
	public:
		void* allocSlot(int nSize) {
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

		// p must be allocated by this manager
		void freeSlot(void* p) {
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

		void init() {
			bmIndex[0] = 0x0000000f; // The first 4 slots are used by bitmap index abd header index
			bmHeaderIndex[0] = 0x0000001; // The header is at slots[0]
		}
	private:
		// step1 = bmIndex[i] & index: discard head bits
		// return step1 == 0?slot:NULL and modify bmIndex if match
		inline MIN_SLOT* matchIndex(int i, unsigned int nSlots) {
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
	} MIN_SLOT_MANAGER;
	static_assert(sizeof(MIN_SLOT_MANAGER) == PAGE_SIZE, "manager struct error");

private:
	std::list<MIN_SLOT_MANAGER*> minSlotManagerList;
};

