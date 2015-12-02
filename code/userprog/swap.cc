#include "swap.h"

#include "../threads/system.h"

Swap::Swap() : pagesBitmap(NUM_SWAP_PAGES) {
	#ifdef NETWORK
	sprintf(swapFileName, "%d.swap", machineId);
	#else
	sprintf(swapFileName, "%s", "local.swap");
	#endif
	if (!fileSystem->Create(swapFileName, 0)) {
		printf("Swap file %s already open.", swapFileName);
	}
	file = fileSystem->Open(swapFileName);
}

Swap::~Swap() {
	delete file;
}

int Swap::Write(int ppn) {
	int spn = pagesBitmap.Find();
	// this should never fail -> swap file has "infinite" pages

	file->WriteAt(machine->mainMemory + ppn * PageSize, PageSize, spn * PageSize);

	return spn * PageSize;
}

void Swap::Update(int ppn, int offset) {
	int spn = offset / PageSize;
	file->WriteAt(machine->mainMemory + ppn * PageSize, PageSize, spn * PageSize);
}

void Swap::Read(int ppn, int offset) {
	file->ReadAt(machine->mainMemory + ppn * PageSize, PageSize, offset);
}

void Swap::Free(int offset) {
	int spn = offset / PageSize;
	pagesBitmap.Clear(spn);
}

