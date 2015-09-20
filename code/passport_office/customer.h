#ifndef PASSPORT_OFFICE_CUSTOMER_H
#define PASSPORT_OFFICE_CUSTOMER_H

#include <stdint.h>
#include <string>
#include <vector>

#include "passport_office.h"

#define CLERK_BRIBE_AMOUNT 500
#define PASSPORT_FEE 100

class Customer {
 public:
  Customer();
  virtual ~Customer();

  inline bool certified() const { return certified_; }
  inline uint32_t money() const { return money_; }
  inline bool passport_verified() const { return passport_verified_; }
  inline bool picture_taken() const { return picture_taken_; }
  inline uint32_t ssn() const { return ssn_; }

  inline void set_certified(bool certified__) { certified_ = certified__; }
  inline void set_passport_verified(bool passport_verified__) {
    passport_verified_ = passport_verified__;
  }
  inline void set_picture_taken(bool picture_taken__) {
    picture_taken_ = picture_taken__;
  }

  bool CanBribe() const;
  void GivePassportFee(CashierClerk* clerk);
  std::string IdentifierString() const;
  void Run();
 private:
  static const uint32_t* INITIAL_MONEY_AMOUNTS;
  static const uint32_t NUM_INITIAL_MONEY_AMOUNTS = 4;
  static uint32_t CURRENT_UNUSED_SSN;

  void GiveBribe(Clerk* clerk);
  void PrintLineJoin(Clerk* clerk, bool bribed) const;

  bool certified_;
  uint32_t money_;
  bool passport_verified_;
  bool picture_taken_;
  const uint32_t ssn_;
  Condition wakeup_condition_;
  Lock wakeup_condition_lock_;
};

#endif
