#ifndef PASSPORT_OFFICE_CLERKS_H
#define PASSPORT_OFFICE_CLERKS_H

#include <string>

#include "../threads/synch.h"
#include "passport_office.h"

class Lock;
class Condition;
class PassportOffice;

namespace clerk_types {
enum Type : unsigned int;
}

namespace clerk_states {

enum State {
  kAvailable = 0,
  kBusy,
  kOnBreak,
};

}  // namespace clerk_states

class Clerk {
 public:
 	Clerk(PassportOffice* passport_office, int identifier);
 	~Clerk();
 	int CollectMoney();
 	void Run();

 	Lock lines_lock_;
 	Condition lines_lock_cv_;
 	Lock bribe_line_lock_;
 	Condition bribe_line_lock_cv_;
 	Lock regular_line_lock_;
 	Condition regular_line_lock_cv_;
 	Lock wakeup_lock_;
 	Condition wakeup_lock_cv_;
 	std::string customer_ssn_;
 	int customer_money_;
 	bool customer_input_;
 protected:
 	void GetNextCustomer();
 	void CollectBribe();
 	virtual void ClerkWork() = 0;
 	PassportOffice* passport_office_;
 	std::string clerk_type_;
 	clerk_types::Type type_;
  clerk_states::State state_;
 	int collected_money_;
 	int identifier_;
};

class ApplicationClerk : public Clerk {
 public:
 	ApplicationClerk(PassportOffice* passport_office, int identifier);
 	void ClerkWork();
};

class PictureClerk : public Clerk {
 public:
 	PictureClerk(PassportOffice* passport_office, int identifier);
 	void ClerkWork();
};

class PassportClerk : public Clerk {
 public:
 	PassportClerk(PassportOffice* passport_office, int identifier);
 	void ClerkWork();
};

class CashierClerk : public Clerk {
 public:
 	CashierClerk(PassportOffice* passport_office, int identifier);
 	void ClerkWork();
};

#endif // PASSPORT_OFFICE_CLERK_H
