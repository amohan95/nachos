#ifndef PASSPORT_OFFICE_CLERKS_H
#define PASSPORT_OFFICE_CLERKS_H

#include "../threads/synch.h"
#include "utilities.h"

class PassportOffice;
class Customer;

class Clerk {
 public:
	static const char* NameForClerkType(clerk_types::Type type);
 	Clerk(PassportOffice* passport_office, int identifier);
 	virtual ~Clerk();
 	int CollectMoney();
	int GetNumCustomersInLine() const;
  void Run();
	void JoinLine(bool bribe);
	std::string IdentifierString() const;

  inline void set_running(bool running__) { running_ = running__; }
  Lock lines_lock_;
  Condition bribe_line_lock_cv_;
  Lock bribe_line_lock_;
  Condition regular_line_lock_cv_;
  Lock regular_line_lock_;
  Condition wakeup_lock_cv_;
  Lock wakeup_lock_;
  Lock money_lock_;
  uint32_t customer_ssn_;
  Customer* current_customer_;
  int customer_money_;
  bool customer_input_;
  std::string clerk_type_;
  clerk_types::Type type_;
  clerk_states::State state_;
 protected:
  void GetNextCustomer();
  virtual void ClerkWork() = 0;
  PassportOffice* passport_office_;
  int collected_money_;
  int identifier_;
  bool running_;
};

class ApplicationClerk : public Clerk {
 public:
  ApplicationClerk(PassportOffice* passport_office, int identifier);
  virtual ~ApplicationClerk() { }
  void ClerkWork();
};

class PictureClerk : public Clerk {
 public:
  PictureClerk(PassportOffice* passport_office, int identifier);
  virtual ~PictureClerk() { }
  void ClerkWork();
};

class PassportClerk : public Clerk {
 public:
  PassportClerk(PassportOffice* passport_office, int identifier);
  virtual ~PassportClerk() { }
  void ClerkWork();
};

class CashierClerk : public Clerk {
 public:
  CashierClerk(PassportOffice* passport_office, int identifier);
  virtual ~CashierClerk() { }
  void ClerkWork();
};

#endif // PASSPORT_OFFICE_CLERK_H
