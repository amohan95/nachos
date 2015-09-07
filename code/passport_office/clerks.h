#ifndef PASSPORT_OFFICE_CLERKS_H
#define PASSPORT_OFFICE_CLERKS_H

#include "synch.h";

class Clerk {
 public:
 	Clerk(PassportOffice* passport_office);
 	~Clerk();
 	int CollectMoney();
 	void WakeUp();
 	virtual void Run() = 0;
 private:
 	PassportOffice* passport_office_;
 	int collected_money_;
 	Lock wakeup_condition_lock_;
 	Condition wakeup_condition_;
};

class ApplicationClerk : Clerk {
 public:
 	void Run();
};

class PictureClerk : Clerk {
 public:
 	void Run();
};

class PassportClerk : Clerk {
 public:
 	void Run();
};

class CashierClerk : Clerk {
 public:
 	void Run();
};

#endif // PASSPORT_OFFICE_CLERK_H