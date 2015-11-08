#ifndef USERPROG_SWAP_H
#define USERPROG_SWAP_H

#include "../userprog/bitmap.h"

#define NUM_SWAP_PAGES 10000

class Swap {
 public:
	Swap();
	~Swap();

	int Write(int ppn);
	void Update(int ppn, int offset);
	void Read(int ppn, int offset);
	void Free(int offset);
 private:
	char swapFileName[11];
	OpenFile* file;
	BitMap pagesBitmap;
};
#endif
