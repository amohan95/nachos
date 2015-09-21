#include "clerks.h"
#include "customer.h"
#include <stdlib.h>

const char* Clerk::NameForClerkType(clerk_types::Type type) {
	static const* char* NAMES[] = {
		"ApplicationClerk",
		"PictureClerk",
		"PassportClerk",
		"Cashier"
	};
	return NAMES[type];
}

Clerk::Clerk(PassportOffice * passport_office, int identifier) 
		: lines_lock_cv_("Clerk Lines Condition"), 
		lines_lock_("Clerk Lines Lock"),
		bribe_line_lock_cv_("Clerk Bribe Line Condition"), 
		bribe_line_lock_("Clerk Bribe Line Lock"),
		regular_line_lock_cv_("Clerk Regular Line Condition"), 
		regular_line_lock_("Clerk Regular Line Lock"),
		wakeup_lock_cv_("Clerk Wakeup Condition"),
		wakeup_lock_("Clerk Wakeup Lock"),
		state_(clerk_states::kAvailable),
		collected_money_(0),
		customer_money_(0),
		customer_input_(false),
		passport_office_(passport_office),
		identifier_(identifier) {
}

Clerk::~Clerk() {}

int Clerk::CollectMoney() {
	money_lock_.Acquire()
	int money = collected_money_;
	collected_money_ = 0;
	money_lock_.Release();
	return money;
}

int Clerk::GetNumCustomersInLine() const {
	return passport_office_->line_counts_[type_][identifier_] +
		passport_office_->bribe_line_counts_[type_][identifier_];
}

void Clerk::GetNextCustomer() {
	lines_lock_.Acquire();
	if (passport_office_->bribe_line_counts_[type_][identifier_] > 0) {
		bribe_line_lock_cv_.Signal(&bribe_line_lock_);
		state_ = clerk_states::kBusy;
		std::cout << clerk_type_ << " [" << identifier_ 
			<< "] has signalled a Customer to come to their counter." << std::endl; 
	} else if (passport_office_->line_counts_[type_][identifier_] > 0) {
		regular_line_lock_cv_.Signal(&regular_line_lock_);
		state_ = clerk_states::kBusy;
		std::cout << clerk_type_ << " [" << identifier_ 
			<< "] has signalled a Customer to come to their counter." << std::endl; 
	} else {
		state_ = clerk_states::kAvailable;
	}
	lines_lock_.Release();
}

ApplicationClerk::ApplicationClerk(PassportOffice* passport_office, int identifier) 
		: Clerk(passport_office, identifier),
		clerk_type_("ApplicationClerk"),
		type_(clerk_types::kApplication) {
}

void Clerk::Run() {
	while (true) {
		GetNextCustomer();
	 	while (state_ == clerk_states::kBusy) {
	 		wakeup_lock_.Acquire();

	 		// Wait for customer to come to counter and give SSN.
	 		wakeup_lock_cv_.Wait(&wakeup_lock_);

	 		// Take Customer's SSN and verify passport.
	 		std::cout << clerk_type_ << " [" << identifier_ << "] has received SSN " << customer_ssn_ 
			<< " from Customer " << customer_ssn_ << std::endl;
	 		
	 		// Do work specific to the type of clerk.
	 		ClerkWork();
			
			// Collect bribe money.
			if (current_customer_->has_bribed()) {
				wakeup_lock_cv_.Signal(&wakeup_lock_);
				wakeup_lock_cv_.Wait(&wakeup_lock_);
				int bribe = customer_money_;
				customer_money_ = 0;
				money_lock_.Acquire()
				collected_money_ += bribe;
				money_lock_.Release()
				std::cout << clerk_type_ << " [" << identifier_ << "] has received $" << bribe 
						<< " from Customer " << customer_ssn_ << std::endl;
			}

	 		// Random delay.
	 		int random_time = rand() % 80 + 20;
	 		for (int i = 0; i < random_time; ++i) {
	 			currentThread->Yield();
	 		}

	 		// Wakeup customer.
	 		wakeup_lock_cv_.Signal(&wakeup_lock_);
	 		wakeup_lock_.Release();

	 		// Get next customer.
	 		GetNextCustomer();
	 	}

	 	// Wait until woken up.
	 	std::cout << clerk_type_ << " [" << identifier_ << "] is going on break" << std::endl;
		passport_office_->breaking_clerks_.push_back(this);
	 	wakeup_lock_cv_.Wait(&wakeup_lock_);
	 	std::cout << clerk_type_ << " [" << identifier_ << "] is coming off break" << std::endl;
	}
}

void ApplicationClerk::ClerkWork() {
	// Wait for customer to put passport on counter.
	wakeup_lock_cv_.Signal(&wakeup_lock_);
	wakeup_lock_cv_.Wait(&wakeup_lock_);
	current_customer_->verify_passport();
	std::cout << clerk_type_ << " [" << identifier_ 
			<< "] has recorded a completed application for Customer " << customer_ssn_ << std::endl;
}

PictureClerk::PictureClerk(PassportOffice* passport_office, int identifier) 
		: Clerk(passport_office, identifier),
		clerk_type_("PictureClerk"),
		type_(clerk_types::kPicture) {
}

void PictureClerk::ClerkWork() {
	// Take Customer's picture and wait to hear if they like it.
	wakeup_lock_cv_.Signal(&wakeup_lock_);
	wakeup_lock_cv_.Wait(&wakeup_lock_);
	bool picture_accepted = customer_input_;
	std::cout << clerk_type_ << " [" << identifier_ << "] has taken a picture of Customer " << customer_ssn_ 
			<< std::endl;

	// Take picture until they like it.
	while (!picture_accepted) {
		std::cout << clerk_type_ << " [" << identifier_ << "] has been told that Customer[" << customer_ssn_
				<< "] does not like their picture" << std::endl;

		// Take Customer's picture again and wait to hear if they like it.
		wakeup_lock_cv_.Signal(&wakeup_lock_);
		wakeup_lock_cv_.Wait(&wakeup_lock_);
		picture_accepted = customer_input_;
		std::cout << clerk_type_ << " [" << identifier_ << "] has taken a picture of Customer " 
				<< customer_ssn_ << std::endl;
	}

	// Set picture taken.
	current_customer_->set_picture_taken();
}

PassportClerk::PassportClerk(PassportOffice* passport_office, int identifier) 
		: Clerk(passport_office, identifier),
		clerk_type_("PassportClerk"),
		type_(clerk_types::kPassport) {
}

void PassportClerk::ClerkWork() {
	// Wait for customer to show whether or not they got their picture taken and passport verified.
	wakeup_lock_cv_.Signal(&wakeup_lock_);
	wakeup_lock_cv_.Wait(&wakeup_lock_);
	bool picture_taken_and_passport_verified = customer_input_;

	// Check to make sure their picture has been taken and passport verified.
	if (!picture_taken_and_passport_verified) {
		std::cout << clerk_type_ << " [" << identifier_ << "] has determined that Customer[" 
				<< customer_ssn_ << "] does not have both their application and picture completed" << std::endl;
	} else {
		std::cout << clerk_type_ << " [" << identifier_ << "] has determined that Customer[" << customer_ssn_ 
				<< "] does have both their application and picture completed" << std::endl;
	
		current_customer_->set_certified();
		std::cout << clerk_type_ << " [" << identifier_ << "] has recorded Customer[" << customer_ssn_ 
				<< "] passport documentation" << std::endl;
	}
}

CashierClerk::CashierClerk(PassportOffice* passport_office, int identifier) 
		: Clerk(passport_office, identifier),
		clerk_type_("Cashier"),
		type_(clerk_types::kCashier) {
}

void CashierClerk::ClerkWork() {
	// Collect application fee.
	wakeup_lock_cv_.Signal(&wakeup_lock_);
	wakeup_lock_cv_.Wait(&wakeup_lock_);
	money_lock_.Acquire()
	collected_money_ += customer_money_;
	customer_money_ = 0;
	money_lock_.Release()

	// Wait for the customer to show you that they are certified.
	wakeup_lock_cv_.Signal(&wakeup_lock_);
	wakeup_lock_cv_.Wait(&wakeup_lock_);
	bool certified = customer_input_;

	// Check to make sure they have been certified.
	if (!certified) {
		std::cout << clerk_type_ << " [" << identifier_ << "] has received the $100 from Customer[" 
				<< customer_ssn_ << "] before certification. They are to go to the back of my line." 
				<< std::endl;

		// Give money back.
		money_lock_.Acquire()
		collected_money_ -= 100;
		money_lock_.Release()
		current_customer_->money_ += 100;
	} else {
		std::cout << clerk_type_ << " [" << identifier_ << "] has received the $100 from Customer[" 
			<< customer_ssn_ << "] after certification." << std::endl;

		// Give customer passport.
		std::cout << clerk_type_ << " [" << identifier_ << "] has provided Customer[" << customer_ssn_ << "] their completed passport." << std::endl;

		// Record passport was given to customer.
		std::cout << clerk_type_ << " [" << identifier_ << "] has recorded that Customer[" << customer_ssn_ << "] has been given their completed passport." << std::endl;
	}
}
