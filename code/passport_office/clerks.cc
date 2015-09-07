#include "clerks.h";
#include "stdlib.h";

Clerk::Clerk(PassportOffice * passport_office) {
	passport_office_ = passport_office;
	collected_money_ = 0;
	wakeup_conditon_ = Condition();
	wakeup_conditon_lock_ = Lock("Clerk Wakeup Condition Lock");
}

Clerk::~Clerk() {}

int Clerk::CollectMoney() {
	int money = collected_money_;
	collected_money_ = 0;
	return money;
}

void Clerk::WakeUp() {
	wakeup_conditon_.signal(wakeup_conditon_lock_);
}


void ApplicationClerk::run() {
	Customer * current_customer = passport_office->RemoveCustomerFromPictureLine();
 	while (current_customer != NULL) {

 		// Take Customer's SSN and verify passport.
 		current_customer->ssn();
 		current_customer->verify_passport();

 		// Collect bribe money.
 		if (current_customer->hasBribed()) {
 			collected_money_ += current_customer->GiveBribe();
 		}

 		// Random delay.
 		int random_time = rand() % 80 + 20;
 		for (int i = 0; i < random_time; i++) {
 			currentThread->Yield();
 		}

 		// Wakeup customer wait condition.
 		current_customer->WakeUp();

 		// Get next customer.
 		current_customer = passport_office->RemoveCustomerFromPictureLine();
 	}

 	// Wait until woken up.
 	wakeup_conditon_.wait(wakeup_conditon_lock_);
}

void PictureClerk::run() {
	Customer * current_customer = passport_office->RemoveCustomerFromPictureLine();
 	while (current_customer != NULL) {

 		// Take Customer's picture until they like the picture.
 		bool picture_accepted = current_customer->GetPictureTaken();
 		while (!picture_accepted) {
 			picture_accepted = current_customer->GetPictureTaken();
 		}
 		current_customer->set_picture_taken();

 		// Collect bribe money.
 		if (current_customer->hasBribed()) {
 			collected_money_ += current_customer->GiveBribe();
 		}

 		// Random delay.
 		int random_time = rand() % 80 + 20;
 		for (int i = 0; i < random_time; i++) {
 			currentThread->Yield();
 		}

 		// Wakeup customer wait condition.
 		current_customer->WakeUp();

 		// Get next customer.
 		current_customer = passport_office->RemoveCustomerFromPictureLine();
 	}

 	// Wait until woken up.
 	wakeup_conditon_.wait(wakeup_conditon_lock_);
}

void PassportClerk::run() {
	Customer * current_customer = passport_office->RemoveCustomerFromPictureLine();
 	while (current_customer != NULL) {

 		// Check to make sure their picture has been taken and passport verified.
 		if (!current_customer->picture_taken() || !current_customer->passport_verified()) {
 			/* TODO (swillard13): Punish */
 		}

 		// Random delay.
 		int random_time = rand() % 900 + 100;
 		for (int i = 0; i < random_time; i++) {
 			currentThread->Yield();
 		}

 		// Wakeup customer wait condition.
 		current_customer->WakeUp();

 		// Get next customer.
 		current_customer = passport_office->RemoveCustomerFromPictureLine();
 	}

 	// Wait until woken up.
 	wakeup_conditon_.wait(wakeup_conditon_lock_);
}

void CashierClerk::run() {
	Customer * current_customer = passport_office->RemoveCustomerFromPictureLine();
 	while (current_customer != NULL) {

 		// Check to make sure they have been certified.
 		if (!current_customer->certified()) {
 			/* TODO (swillard13): Punish */
 		}

 		// Collect application fee.
 		collected_money_ += current_customer->GiveApplicationFee();

 		// Collect bribe money.
 		if (current_customer->hasBribed()) {
 			collected_money_ += current_customer->GiveBribe();
 		}

 		// Random delay.
 		int random_time = rand() % 900 + 100;
 		for (int i = 0; i < random_time; i++) {
 			currentThread->Yield();
 		}

 		// Wakeup customer wait condition.
 		current_customer->WakeUp();

 		// Get next customer.
 		current_customer = passport_office->RemoveCustomerFromPictureLine();
 	}

 	// Wait until woken up.
 	wakeup_conditon_.wait(wakeup_conditon_lock_);
}