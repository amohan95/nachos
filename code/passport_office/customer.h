#ifndef PASSPORT_OFFICE_CUSTOMER_H
#define PASSPORT_OFFICE_CUSTOMER_H

#include <atomic>
#include <cstdint>
#include <string>
#include <vector>

#include "passport_office.h"

class Customer {
 public:
  Customer();
  virtual ~Customer();

  inline bool certified() const { return certified_; }
  inline uint32_t money() const { return money_; }
  inline bool passport_verified() const { return passport_verified_; }
  inline bool picture_taken() const { return picture_taken_; }
  inline const std::string& ssn() const { return ssn_; }

  inline void set_certified(bool certified) { certified_ = certified; }
  inline void set_passport_verified(bool passport_verified) {
    passport_verified_ = passport_verified;
  }
  inline void set_picture_taken(bool picture_taken) { picture_taken_ = picture_taken; }

  void Run();
 private:
  static const std::vector<uint32_t> INITIAL_MONEY_AMOUNTS = {100, 600, 1100, 1600};
  static std::atomic<uint32_t> CURRENT_UNUSED_SSN;

  bool certified_;
  bool done_;
  uint32_t money_;
  bool passport_verified_;
  bool picture_taken_;
  const std::string ssn_;
  Condition wakeup_condition_;
  Lock wakeup_condition_lock_;
};

#endif
