#ifndef PASSPORT_OFFICE_CLERKS_H
#define PASSPORT_OFFICE_CLERKS_H

#include "synch.h";
#include "string.h";

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
 	void WakeUp();
 	virtual void Run() = 0;

 	Lock lines_lock_;
 	Condition lines_lock_cv_;
 	Lock bribe_line_lock_;
 	Condition bribe_line_lock_cv_;
 	Lock regular_line_lock_;
 	Condition regular_line_lock_cv_;
 	Lock wakeup_lock_;
 	Conditon wakeup_lock_cv;
 private:
 	void print_signalled_customer();
 	void print_received_ssn(std::string ssn);
 	void print_going_on_break();
 	void print_coming_off_break();
 	void get_next_customer();
 	PassportOffice* passport_office_;
 	string clerk_type_;
 	clerk_type_::Type type_;
 	int collected_money_;
 	int identifier_;
};

class ApplicationClerk : Clerk {
 public:
 	ApplicationClerk(PassportOffice* passport_office, int identifier);
 	void Run();
};

class PictureClerk : Clerk {
 public:
 	PictureClerk(PassportOffice* passport_office, int identifier);
 	void Run();
};

class PassportClerk : Clerk {
 public:
 	PassportClerk(PassportOffice* passport_office, int identifier);
 	void Run();
};

class CashierClerk : Clerk {
 public:
 	CashierClerk(PassportOffice* passport_office, int identifier);
 	void Run();
};

#endif // PASSPORT_OFFICE_CLERK_H