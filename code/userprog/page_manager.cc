#include "page_manager.h"

PageManager::PageManager() :
    lock_("Page Manager Lock"), num_used_pages_(0), page_map_(NumPhysPages) { }

int PageManager::ObtainFreePage() {
  int page_num = page_map_.Find();
  if (page_num != -1) {
    --num_used_pages_;
    return page_num;
  }
  return -1;
}

void PageManager::FreePage(int page_num) {
  if (page_map_.Test(page_num)) {
    page_map_.Clear(page_num);
    --num_used_pages_;
  }
}
