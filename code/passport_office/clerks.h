#ifndef PASSPORT_OFFICE_CLERKS_H
#define PASSPORT_OFFICE_CLERKS_H

#include "../threads/synch.h"
#include "string.h"
#include "passport_office.h"

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
 	Conditon wakeup_lock_cv_;
 	std::string customer_ssn_;
 	int customer_money_;
 	bool customer_input_;
 private:
 	void get_next_customer();
 	virtual void ClerkWork() = 0;
 	PassportOffice* passport_office_;
 	string clerk_type_;
 	clerk_type_::Type type_;
 	int collected_money_;
 	int identifier_;
};

class ApplicationClerk : Clerk {
 public:
 	ApplicationClerk(PassportOffice* passport_office, int identifier);
 	void ClerkWork();
};

class PictureClerk : Clerk {
 public:
 	PictureClerk(PassportOffice* passport_office, int identifier);
 	void ClerkWork();
};

class PassportClerk : Clerk {
 public:
 	PassportClerk(PassportOffice* passport_office, int identifier);
 	void ClerkWork();
};

class CashierClerk : Clerk {
 public:
 	CashierClerk(PassportOffice* passport_office, int identifier);
 	void ClerkWork();
};

#endif // PASSPORT_OFFICE_CLERK_H
