#ifndef USERPROG_INVERTED_TRANSLATION_ENTRY_H
#define USERPROG_INVERTED_TRANSLATION_ENTRY_H

#include "translate.h"

class AddrSpace;

class InvertedTranslationEntry : public TranslationEntry {
 public:
	AddrSpace* owner;
};

#endif
