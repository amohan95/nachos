#ifndef PASSPORT_OFFICE_LINE_H
#define PASSPORT_OFFICE_LINE_H

#include "synch.h"

#include <vector>

template<class T>
class Line {
 public:
  Line();
  ~Line();

  inline uint32_t size() const { return storage_.size(); }

  void push_back(const T& t);
  void remove(uint32_t index);
 private:
  Lock modification_lock_;
  std::vector<T> storage_;
};

template <class T>
Line<T>::Line() {
}

template <class T>
Line<T>::~Line() {
}

template <class T>
void Line<T>::push_back(const T& t) {
  modification_lock_.Acquire();
  storage_.push_back(t);
  modification_lock_.Release();
}

template <class T>
void Line<T>::remove(uint32_t index) {
  modification_lock_.Acquire();
  storage_.erase(storage_.begin() + index);
  modification_lock_.Release();
}

#endif
