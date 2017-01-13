#pragma once

/*
Mempool - a memory pool to avoid frequently syscall
=========================================================================
the kinds of slots:
	MIN_SLOT = 16Bytes For 
	MIDDLE_SLOT = 4096Bytes
	MAX_SLOT = MMAP

	4 pages: 16380 Bytes
		BITMAP_INDEX 64Byte 256Bits
		63 * MIN_SLOT + 64 * MIN_SLOT + 64 * MIN_SLOT + 64 * MIN_SLOT

	1 pages: 8 * SMALL_SLOT
*/

#define PAGE_SIZE (4096)
#define MIN_SLOT_SIZE (16)
#define INDEX_BIT_NUM (PAGE_SIZE / MIN_SLOT_SIZE)
#define INDEX_BYTE_NUM (INDEX_BIT_NUM / 8)
#define PAGE_MIN_SLOT_NUM ((PAGE_SIZE - INDEX_BYTE_NUM) / MIN_SLOT_SIZE)

class Mempool
{
public:
	Mempool();
	~Mempool();

	typedef unsigned int MIN_SLOT_BITMAP[INDEX_BYTE_NUM];
	typedef unsigned int MIN_SLOT[MIN_SLOT_SIZE];
	typedef struct _MIN_SLOT_MANAGER {
		MIN_SLOT_BITMAP bmIndex;
		MIN_SLOT slots[PAGE_MIN_SLOT_NUM];
	public:
		MIN_SLOT* allocSlot(int nSize) {
			int nSlots = (nSize + MIN_SLOT_SIZE - 1) / MIN_SLOT_SIZE;
			for (auto i = 0; i < INDEX_BYTE_NUM; i++) {
				if (~bmIndex[i]) {
					continue;
				}
				else {

				}
			}
		}
	} MIN_SLOT_MANAGER;
};

