#include "page_manager.h"

PageManager::PageManager() :
    lock_("Page Manager Lock"), num_used_pages_(0), page_map_(NumPhysPages), next_page_(0) { }

int PageManager::ObtainFreePage() {
  MutexLock l(&lock_);
  int page_num = page_map_.Find();
  if (page_num != -1) {
    ++num_used_pages_;
    return page_num;
  }
  return -1;
}

void PageManager::FreePage(int page_num) {
  MutexLock l(&lock_);
  if (page_map_.Test(page_num)) {
    page_map_.Clear(page_num);
    --num_used_pages_;
  }
}

int PageManager::NextAllocatedPage() {
	int p = next_page_;
	next_page_ = (next_page_ + 1) % NumPhysPages;
	return p;
}
