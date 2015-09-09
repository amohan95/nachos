#include "clerks.h";
#include "customer.h";
#include "stdlib.h";

Clerk::Clerk(PassportOffice * passport_office) 
		: wakeup_condition_("Clerk Wakeup Condition"), 
		wakeup_condition_lock_("Clerk Wakeup Condition Lock"),
		passport_office_(passport_office)
		collected_money_(0) {
}

Clerk::~Clerk() {}

int Clerk::CollectMoney() {
	int money = collected_money_;
	collected_money_ = 0;
	return money;
}

void Clerk::WakeUp() {
	wakeup_conditon_.Signal(wakeup_conditon_lock_);
}


void ApplicationClerk::Run() {
	while (true) {
		Customer * current_customer = passport_office->RemoveCustomerFromApplicationLine();
	 	while (current_customer != NULL) {

	 		// Take Customer's SSN and verify passport.
	 		current_customer->ssn();
	 		current_customer->verify_passport();

	 		// Collect bribe money.
	 		if (current_customer->has_bribed()) {
	 			collected_money_ += current_customer->GiveBribe();
	 		}

	 		// Random delay.
	 		int random_time = rand() % 80 + 20;
	 		for (int i = 0; i < random_time; ++i) {
	 			currentThread->Yield();
	 		}

	 		// Wakeup customer wait condition.
	 		current_customer->WakeUp();

	 		// Get next customer.
	 		current_customer = passport_office->RemoveCustomerFromApplicationLine();
	 	}

	 	// Wait until woken up.
	 	wakeup_conditon_.Wait(wakeup_conditon_lock_);
	}
}

void PictureClerk::Run() {
	while (true) {
		Customer * current_customer = passport_office->RemoveCustomerFromPictureLine();
	 	while (current_customer != NULL) {

	 		// Take Customer's picture until they like the picture.
	 		bool picture_accepted = current_customer->GetPictureTaken();
	 		while (!picture_accepted) {
	 			picture_accepted = current_customer->GetPictureTaken();
	 		}
	 		current_customer->set_picture_taken();

	 		// Collect bribe money.
	 		if (current_customer->has_bribed()) {
	 			collected_money_ += current_customer->GiveBribe();
	 		}

	 		// Random delay.
	 		int random_time = rand() % 80 + 20;
	 		for (int i = 0; i < random_time; ++i) {
	 			currentThread->Yield();
	 		}

	 		// Wakeup customer wait condition.
	 		current_customer->WakeUp();

	 		// Get next customer.
	 		current_customer = passport_office->RemoveCustomerFromPictureLine();
	 	}

	 	// Wait until woken up.
	 	wakeup_conditon_.Wait(wakeup_conditon_lock_);
	}
}

void PassportClerk::Run() {
	while (true) {
		Customer * current_customer = passport_office->RemoveCustomerFromPassportLine();
	 	while (current_customer != NULL) {

	 		// Check to make sure their picture has been taken and passport verified.
	 		if (!current_customer->picture_taken() || !current_customer->passport_verified()) {
	 			/* TODO (swillard13): Punish */
	 		}

	 		// Random delay.
	 		int random_time = rand() % 900 + 100;
	 		for (int i = 0; i < random_time; ++i) {
	 			currentThread->Yield();
	 		}

	 		// Wakeup customer wait condition.
	 		current_customer->WakeUp();

	 		// Get next customer.
	 		current_customer = passport_office->RemoveCustomerFromPassportLine();
	 	}

	 	// Wait until woken up.
	 	wakeup_conditon_.Wait(wakeup_conditon_lock_);
	}
}

void CashierClerk::Run() {
	while (true) {
		Customer * current_customer = passport_office->RemoveCustomerFromCashierLine();
	 	while (current_customer != NULL) {

	 		// Check to make sure they have been certified.
	 		if (!current_customer->certified()) {
	 			/* TODO (swillard13): Punish */
	 		}

	 		// Collect application fee.
	 		collected_money_ += current_customer->GiveApplicationFee();

	 		// Collect bribe money.
	 		if (current_customer->has_bribed()) {
	 			collected_money_ += current_customer->GiveBribe();
	 		}

	 		// Random delay.
	 		int random_time = rand() % 900 + 100;
	 		for (int i = 0; i < random_time; ++i) {
	 			currentThread->Yield();
	 		}

	 		// Wakeup customer wait condition.
	 		current_customer->WakeUp();

	 		// Get next customer.
	 		current_customer = passport_office->RemoveCustomerFromCashierLine();
	 	}

	 	// Wait until woken up.
	 	wakeup_conditon_.Wait(wakeup_conditon_lock_);
	}
}