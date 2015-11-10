#ifndef USERPROG_SWAPPABLE_TRANSLATION_ENTRY_H
#define USERPROG_SWAPPABLE_TRANSLATION_ENTRY_H

#include "translate.h"

enum DiskLocation {
	NEITHER = 0,
	EXECUTABLE,
	SWAP,
	NUM_DISK_LOCATIONS
};

class SwappableTranslationEntry : public TranslationEntry {
 public:
	int byteOffset;
	DiskLocation diskLocation;
};

#endif
