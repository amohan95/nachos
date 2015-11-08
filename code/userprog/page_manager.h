#ifndef USERPROG_PAGE_MANAGER_H_
#define USERPROG_PAGE_MANAGER_H_

#include "bitmap.h"
#include "machine.h"
#include "synch.h"

class Bitmap;
class Lock;

class PageManager {
 public:
  PageManager();
  ~PageManager();

  // Gets the next free available physical page. Returns -1 if there is no page
  // available.
  int ObtainFreePage();

  // Frees the page by the index.
  void FreePage(int page_num);

  int num_available_pages() const { return NumPhysPages - num_used_pages_; }

 private:
  // Any thread doing operations on the PageManager should hold this lock.
  Lock lock_;
  int num_used_pages_;
  BitMap page_map_;
};

#endif
