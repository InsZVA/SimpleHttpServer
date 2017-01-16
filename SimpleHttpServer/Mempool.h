#pragma once

#include <list>
#include <iostream>
#define    WIN32_LEAN_AND_MEAN
#include <windows.h>
//#define DEBUG

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
	void* p[4096];
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

	PARALLEL = 4096
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
		void* allocSlot(int nSize);

		// p must be allocated by this manager
		void freeSlot(void* p);

		void init();
	private:
		// step1 = bmIndex[i] & index: discard head bits
		// return step1 == 0?slot:NULL and modify bmIndex if match
		inline MIN_SLOT* matchIndex(int i, unsigned int nSlots);
	} MIN_SLOT_MANAGER;
	static_assert(sizeof(MIN_SLOT_MANAGER) == PAGE_SIZE, "manager struct error");

private:
	std::list<MIN_SLOT_MANAGER*> minSlotManagerList;
};

