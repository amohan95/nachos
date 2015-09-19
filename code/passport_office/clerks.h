#ifndef PASSPORT_OFFICE_CLERKS_H
#define PASSPORT_OFFICE_CLERKS_H

#include "synch.h";
#include "string.h";

class Clerk {
 public:
 	Clerk(PassportOffice* passport_office, int identifier);
 	~Clerk();
 	int CollectMoney();
 	void WakeUp();
 	virtual void Run() = 0;
 private:
 	void print_signalled_customer();
 	void print_received_ssn(std::string ssn);
 	void print_going_on_break();
 	void print_coming_off_break();
 	void get_next_customer();
 	PassportOffice* passport_office_;
 	string clerk_type_;
 	int collected_money_;
 	int identifier_;
 	Lock wakeup_condition_lock_;
 	Condition wakeup_condition_;
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