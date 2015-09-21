#ifndef PASSPORT_OFFICE_UTILITIES_H_
#define PASSPORT_OFFICE_UTILITIES_H_

namespace clerk_states {

enum State {
  kAvailable = 0,
  kBusy,
  kOnBreak,
};

}  // namespace clerk_states

namespace clerk_types {

enum Type {
  kApplication = 0,
  kPicture,
  kPassport,
  kCashier,
  Size,
};

}  // namespace clerk_types

#endif
