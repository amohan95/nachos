#include "clerks.h";
#include "customer.h";
#include "stdlib.h";

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

void Clerk::print_signalled_customer() {
	std::cout << clerk_type_ << " [" << identifier_ 
			<< "] has signalled a Customer to come to their counter." << std::endl; 
}

void Clerk::print_received_ssn(string ssn {
	std::cout << clerk_type_ << " [" << identifier_ << "] has received SSN " << ssn 
			<< " from Customer " << ssn << std::endl;
}

void Clerk::print_going_on_break() {
	std::cout << clerk_type_ << " [" << identifier_ << "] is going on break" << std::endl;
}

void Clerk::print_coming_off_break() {
	std::cout << clerk_type_ << " [" << identifier_ << "] is coming off break" << std::endl;
}

void Clerk::get_next_customer() {
	Customer* current;
	lines_lock_->Acquire();
	if (passport_office_->bribe_line_counts_[type_][identifier_] > 0) {
		bribe_line_cv_.signal(bribe_line_lock);
		state_ = clerk_states_::kBusy;
		print_signalled_customer();
		current = ???; 
	} else if (passport_office_->line_counts_[type_][identifier_] > 0) {
		regular_line_cv.signal(regular_line_lock);
		state_ = clerk_states_::kBusy;
		print_signalled_customer();
		current = ???; 
	} else {
		state_ = clerk_states_::kAvailable;
		current = null;
	}
	lines_lock_->Release();
	return current;
}

ApplicationClerk::ApplicationClerk(PassportOffice* passport_office, int identifier) 
		: super(passport_office, identifier),
		clerk_type_("ApplicationClerk"),
		type(clerk_types::kApplication) {
}

void ApplicationClerk::Run() {
	while (true) {
		Customer * current_customer = get_next_customer();
	 	while (current_customer != NULL) {

	 		// Take Customer's SSN and verify passport.
	 		std::string ssn = current_customer->ssn();
	 		print_received_ssn(ssn);
	 		current_customer->verify_passport();
	 		std::cout << clerk_type_ << " [" << identifier_ 
	 				<< "] has recorded a completed application for Customer " << ssn << std::endl;

	 		// Collect bribe money.
	 		if (current_customer->has_bribed()) {
	 			int bribe = current_customer->GiveBribe();
	 			collected_money_ += bribe;
	 			std::cout << clerk_type_ << " [" << identifier_ << "] has received $" << bribe 
	 					<< " from Customer " << ssn << std::endl;
	 		}

	 		// Random delay.
	 		int random_time = rand() % 80 + 20;
	 		for (int i = 0; i < random_time; ++i) {
	 			currentThread->Yield();
	 		}

	 		// Wakeup customer wait condition.
	 		current_customer->WakeUp();

	 		// Get next customer.
	 		current_customer = get_next_customer();
	 	}

	 	// Wait until woken up.
	 	print_going_on_break();
	 	wakeup_lock_cv_.Wait(wakeup_lock_);
	 	print_coming_off_break();
	}
}

PictureClerk::PictureClerk(PassportOffice* passport_office, int identifier) 
		: super(passport_office, identifier),
		clerk_type_("PictureClerk"),
		type(clerk_types::kPicture) {
}

void PictureClerk::Run() {
	while (true) {
		Customer * current_customer = get_next_customer();
	 	while (current_customer != NULL) {

	 		std::string ssn = current_customer->ssn();
	 		print_received_ssn();
	 		// Take Customer's picture until they like the picture.
	 		bool picture_accepted = current_customer->GetPictureTaken();
	 		std::cout << clerk_type_ << " [" << identifier_ << "] has taken a picture of Customer " << ssn 
	 				<< std::endl;
	 		while (!picture_accepted) {
	 			std::cout << clerk_type_ << " [" << identifier_ << "] has been told that Customer[" << ssn
	 					<< "] does not like their picture" << std::endl;
	 			picture_accepted = current_customer->GetPictureTaken();
	 			td::cout << clerk_type_ << " [" << identifier_ << "] has taken a picture of Customer " 
	 					<< ssn << std::endl;
	 		}

	 		// Certify passport.
	 		std::cout << clerk_type_ << " [" << identifier_ << "] has been told that Customer[" << ssn 
	 				<< "] does like their picture" << std::endl;
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
	 		current_customer = get_next_customer();
	 	}

	 	// Wait until woken up.
	 	print_going_on_break();
	 	wakeup_lock_cv_.Wait(wakeup_lock_);
	 	print_coming_off_break();
	}
}

PassportClerk::PassportClerk(PassportOffice* passport_office, int identifier) 
		: super(passport_office, identifier),
		clerk_type_("PassportClerk"),
		type(clerk_types::kPassport) {
}

void PassportClerk::Run() {
	while (true) {
		Customer * current_customer = get_next_customer();
	 	while (current_customer != NULL) {
	 		std::string ssn = current_customer->ssn();
	 		print_received_ssn(ssn);

	 		// Check to make sure their picture has been taken and passport verified.
	 		if (!current_customer->picture_taken() || !current_customer->passport_verified()) {
	 			/* TODO (swillard13): Punish */
	 			std::cout << clerk_type_ << " [" << identifier_ << "] has determined that Customer[" 
	 					<< ssn << "] does not have both their application and picture completed" << std::endl;
	 		}
	 		std::cout << clerk_type_ << " [" << identifier_ << "] has determined that Customer[" << ssn 
	 				<< "] does have both their application and picture completed" << std::endl;
	 		
	 		current_customer->set_certified();
	 		std::cout << clerk_type_ << " [" << identifier_ << "] has recorded Customer[" << ssn 
	 				<< "] passport documentation" << std::endl;

	 		// Random delay.
	 		int random_time = rand() % 900 + 100;
	 		for (int i = 0; i < random_time; ++i) {
	 			currentThread->Yield();
	 		}

	 		// Wakeup customer wait condition.
	 		current_customer->WakeUp();

	 		// Get next customer.
	 		current_customer = get_next_customer();
	 	}

	 	// Wait until woken up.
	 	print_going_on_break();
	 	wakeup_lock_cv_.Wait(wakeup_lock_);
	 	print_coming_off_break();
	}
}

CashierClerk::CashierClerk(PassportOffice* passport_office, int identifier) 
		: super(passport_office, identifier),
		clerk_type_("Cashier"),
		type(clerk_types::kCashier) {
}

void CashierClerk::Run() {
	while (true) {
		Customer * current_customer = get_next_customer();
	 	while (current_customer != NULL) {
	 		std::string ssn = current_customer->ssn();
	 		print_received_ssn(ssn);

	 		// Collect application fee.
	 		collected_money_ += current_customer->GiveApplicationFee();

	 		// Check to make sure they have been certified.
	 		if (!current_customer->certified()) {
	 			/* TODO (swillard13): Punish */
	 			std::cout << clerk_type_ << " [" << identifier_ << "] has received the $100 from Customer[" << ssn << "] before certification. They are to go to the back of my line." << std::endl;
	 		}
	 		std::cout << clerk_type_ << " [" << identifier_ << "] has received the $100 from Customer[" << ssn << "] after certification." << std::endl;


	 		// Collect bribe money.
	 		if (current_customer->has_bribed()) {
	 			collected_money_ += current_customer->GiveBribe();
	 		}

	 		// Give customer passport.
	 		std::cout << clerk_type_ << " [" << identifier_ << "] has provided Customer[" << ssn << "] their completed passport." << std::endl;

	 		// Record passport was given to customer.
	 		std::cout << clerk_type_ << " [" << identifier_ << "] has recorded that Customer[" << ssn << "] has been given their completed passport." << std::endl;

	 		// Random delay.
	 		int random_time = rand() % 900 + 100;
	 		for (int i = 0; i < random_time; ++i) {
	 			currentThread->Yield();
	 		}

	 		// Wakeup customer wait condition.
	 		current_customer->WakeUp();

	 		// Get next customer.
	 		current_customer = get_next_customer();
	 	}

	 	// Wait until woken up.
	 	print_going_on_break();
	 	wakeup_lock_cv_.Wait(wakeup_lock_);
	 	print_coming_off_break();
	}
}
