#ifndef PASSPORT_OFFICE_CUSTOMER_H
#define PASSPORT_OFFICE_CUSTOMER_H

#include <stdint.h>
#include <vector>

#include "../threads/synch.h"
#include "utilities.h"

#define CLERK_BRIBE_AMOUNT 500
#define PASSPORT_FEE 100

class PassportOffice;
class Clerk;
class CashierClerk;

class Customer {
 public:
  Customer(PassportOffice* passport_office);
  virtual ~Customer();

	inline bool certified() const { return certified_; }
  inline bool completed_application() const { return completed_application_; }
  inline uint32_t money() const { return money_; }
  inline bool passport_verified() const { return passport_verified_; }
  inline bool picture_taken() const { return picture_taken_; }
  inline uint32_t ssn() const { return ssn_; }

	inline void set_certified(bool certified__) { certified_ = certified__; }
  inline void set_completed_application(bool completed_application__) { completed_application_ = completed_application__; }
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
  static const uint32_t NUM_INITIAL_MONEY_AMOUNTS = 4;
  static uint32_t CURRENT_UNUSED_SSN;

  void GiveBribe(Clerk* clerk);
  void PrintLineJoin(Clerk* clerk, bool bribed) const;

  PassportOffice* passport_office_;
  bool certified_;
  bool completed_application_;
  uint32_t money_;
  bool passport_verified_;
  bool picture_taken_;
  const uint32_t ssn_;
};

class Senator : public Customer {
  Senator(PassportOffice* passport_office) : Customer(passport_office) {}
  virtual ~Senator() {}
};
#endif
