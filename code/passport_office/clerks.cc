#include "clerks.h"
#include "customer.h"
#include "stdlib.h"

Clerk::Clerk(PassportOffice * passport_office, int identifier) 
		: lines_lock_cv_("Clerk Lines Condition"), 
		lines_lock_("Clerk Lines Lock"),
		bribe_line_lock_cv_("Clerk Bribe Line Condition"), 
		bribe_line_lock_("Clerk Bribe Line Lock"),
		regular_line_lock_cv_("Clerk Regular Line Condition"), 
		regular_line_lock_("Clerk Regular Line Lock"),
		wakeup_lock_cv_("Clerk Wakeup Condition"),
		wakeup_lock_("Clerk Wakeup Lock")),
		passport_office_(passport_office),
		collected_money_(0),
		identifier_(identifier),
		clerk_type_("Clerk") {
}

Clerk::~Clerk() {}

int Clerk::CollectMoney() {
	int money = collected_money_;
	collected_money_ = 0;
	return money;
}

void Clerk::get_next_customer() {
	lines_lock_->Acquire();
	if (passport_office_->bribe_line_counts_[type_][identifier_] > 0) {
		bribe_line_cv_.signal(bribe_line_lock);
		state_ = clerk_states_::kBusy;
		std::cout << clerk_type_ << " [" << identifier_ 
			<< "] has signalled a Customer to come to their counter." << std::endl; 
	} else if (passport_office_->line_counts_[type_][identifier_] > 0) {
		regular_line_cv.signal(regular_line_lock);
		state_ = clerk_states_::kBusy;
		std::cout << clerk_type_ << " [" << identifier_ 
			<< "] has signalled a Customer to come to their counter." << std::endl; 
	} else {
		state_ = clerk_states_::kAvailable;
	}
	lines_lock_->Release();
}

ApplicationClerk::ApplicationClerk(PassportOffice* passport_office, int identifier) 
		: super(passport_office, identifier),
		clerk_type_("ApplicationClerk"),
		type(clerk_types::kApplication) {
}

void Clerk::Run() {
	while (true) {
		get_next_customer();
	 	while (state_ == clerk_states_::kBusy) {
	 		wakeup_lock_.Acquire();

	 		// Wait for customer to come to counter and give SSN.
	 		wakeup_lock_cv_.Wait(wakeup_lock_);

	 		// Take Customer's SSN and verify passport.
	 		std::cout << clerk_type_ << " [" << identifier_ << "] has received SSN " << customer_ssn_ 
			<< " from Customer " << customer_ssn_ << std::endl;
	 		
	 		// Do work specific to the type of clerk.
	 		ClerkWork();

	 		// Random delay.
	 		int random_time = rand() % 80 + 20;
	 		for (int i = 0; i < random_time; ++i) {
	 			currentThread->Yield();
	 		}

	 		// Wakeup customer.
	 		wakeup_lock_cv_.Signal(wakeup_lock_);
	 		wakeup_lock_.Release();

	 		// Get next customer.
	 		get_next_customer();
	 	}

	 	// Wait until woken up.
	 	std::cout << clerk_type_ << " [" << identifier_ << "] is going on break" << std::endl;
	 	wakeup_lock_cv_.Wait(wakeup_lock_);
	 	std::cout << clerk_type_ << " [" << identifier_ << "] is coming off break" << std::endl;
	}
}

void ApplicationClerk::ClerkWork() {
	current_customer_->verify_passport();
	std::cout << clerk_type_ << " [" << identifier_ 
			<< "] has recorded a completed application for Customer " << customer_ssn_ << std::endl;

	// Collect bribe money.
	if (current_customer_->has_bribed()) {
		int bribe = current_customer_->GiveBribe();
		collected_money_ += bribe;
		std::cout << clerk_type_ << " [" << identifier_ << "] has received $" << bribe 
				<< " from Customer " << customer_ssn_ << std::endl;
	}
}

PictureClerk::PictureClerk(PassportOffice* passport_office, int identifier) 
		: super(passport_office, identifier),
		clerk_type_("PictureClerk"),
		type(clerk_types::kPicture) {
}

void PictureClerk::ClerkWork() {
	// Take Customer's picture until they like the picture.
	bool picture_accepted = current_customer_->GetPictureTaken();
	std::cout << clerk_type_ << " [" << identifier_ << "] has taken a picture of Customer " << customer_ssn_ 
			<< std::endl;
	while (!picture_accepted) {
		std::cout << clerk_type_ << " [" << identifier_ << "] has been told that Customer[" << customer_ssn_
				<< "] does not like their picture" << std::endl;
		picture_accepted = current_customer_->GetPictureTaken();
		td::cout << clerk_type_ << " [" << identifier_ << "] has taken a picture of Customer " 
				<< customer_ssn_ << std::endl;
	}

	// Set picture taken.
	current_customer_->set_picture_taken();

	// Collect bribe money.
	if (current_customer_->has_bribed()) {
		collected_money_ += current_customer_->GiveBribe();
	}
}

PassportClerk::PassportClerk(PassportOffice* passport_office, int identifier) 
		: super(passport_office, identifier),
		clerk_type_("PassportClerk"),
		type(clerk_types::kPassport) {
}

void PassportClerk::ClerkWork() {
	// Check to make sure their picture has been taken and passport verified.
	if (!current_customer_->picture_taken() || !current_customer_->passport_verified()) {
		/* TODO (swillard13): Punish */
		std::cout << clerk_type_ << " [" << identifier_ << "] has determined that Customer[" 
				<< customer_ssn_ << "] does not have both their application and picture completed" << std::endl;
	}
	std::cout << clerk_type_ << " [" << identifier_ << "] has determined that Customer[" << customer_ssn_ 
			<< "] does have both their application and picture completed" << std::endl;
	
	current_customer_->set_certified();
	std::cout << clerk_type_ << " [" << identifier_ << "] has recorded Customer[" << customer_ssn_ 
			<< "] passport documentation" << std::endl;
}

CashierClerk::CashierClerk(PassportOffice* passport_office, int identifier) 
		: super(passport_office, identifier),
		clerk_type_("Cashier"),
		type(clerk_types::kCashier) {
}

void CashierClerk::ClerkWork() {
	// Collect application fee.
	collected_money_ += current_customer_->GiveApplicationFee();

	// Check to make sure they have been certified.
	if (!current_customer_->certified()) {
		/* TODO (swillard13): Punish */
		std::cout << clerk_type_ << " [" << identifier_ << "] has received the $100 from Customer[" << customer_ssn_ << "] before certification. They are to go to the back of my line." << std::endl;
	}
	std::cout << clerk_type_ << " [" << identifier_ << "] has received the $100 from Customer[" << customer_ssn_ << "] after certification." << std::endl;


	// Collect bribe money.
	if (current_customer_->has_bribed()) {
		collected_money_ += current_customer_->GiveBribe();
	}

	// Give customer passport.
	std::cout << clerk_type_ << " [" << identifier_ << "] has provided Customer[" << customer_ssn_ << "] their completed passport." << std::endl;

	// Record passport was given to customer.
	std::cout << clerk_type_ << " [" << identifier_ << "] has recorded that Customer[" << customer_ssn_ << "] has been given their completed passport." << std::endl;

}
