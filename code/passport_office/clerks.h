#ifndef PASSPORT_OFFICE_CLERKS_H
#define PASSPORT_OFFICE_CLERKS_H

#include "synch.h";

class Clerk {
 public:
 	Clerk(PassportOffice* passport_office);
 	~Clerk();
 	int CollectMoney();
 	void WakeUp();
 	virtual void run() = 0;
 private:
 	PassportOffice* passport_office_;
 	int collected_money_;
 	Lock wakeup_condition_lock_;
 	Condition wakeup_condition_;
};

class ApplicationClerk extends Clerk {
 public:
 	void run();
};

class PictureClerk extends Clerk {
 public:
 	void run();
};

class PassportClerk extends Clerk {
 public:
 	void run();
};

class CashierClerk extends Clerk {
 public:
 	void run();
};

#endif // PASSPORT_OFFICE_CLERK_H