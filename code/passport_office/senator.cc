#include "senator.h"
#include "utilities.h"

Senator::Senator(PassportOffice* passport_office) : 
    Customer(passport_office, customer_types::kSenator) {}

void Senator::Run() {
  running_ = true;
  // Increment the number of senators in the office so that others know
  // that a senator is there.
  passport_office_->num_senators_lock_.Acquire();
  ++passport_office_->num_senators_;
  passport_office_->num_senators_lock_.Release();

  passport_office_->senator_lock_->Acquire();

  // Wake up customers that are currently in line in the passport office so that
  // they can join the outside line.
  passport_office_->customers_served_lock_.Acquire();
  passport_office_->manager_->WakeWaitingCustomers();
  // Wait until all customers have left the building.
  if (passport_office_->num_customers_being_served_ > 0) {
    passport_office_->customers_served_cv_.Wait(
        &passport_office_->customers_served_lock_);
  }
  passport_office_->customers_served_lock_.Release();

  // Reset the line counts to 0 since there are no customers in the office
  // at this point.
  for (unsigned i = 0; i < passport_office_->line_counts_.size(); ++i) {
    passport_office_->line_locks_[i]->Acquire();
    for (unsigned j = 0; j < passport_office_->line_counts_[i].size(); ++j) {
      passport_office_->line_counts_[i][j] = 0;
      passport_office_->bribe_line_counts_[i][j] = 0;      
    }
    passport_office_->line_locks_[i]->Release();
  }

  // Ask the manager to wake up the necessary clerks for the senator - they will
  // stay awake until the senator leaves.
  passport_office_->manager_->WakeClerksForSenator();
  
  // Wait for all clerks to get off of their breaks, if necessary.
  for (int i = 0; i < 500; ++i) { currentThread->Yield(); }

  while (running_ &&
        (!passport_verified() || !picture_taken() ||
         !completed_application() || !certified())) {
    clerk_types::Type next_clerk;
    if (!completed_application() && !picture_taken() && 
        passport_office_->clerks_[clerk_types::kApplication].size() > 0 &&
        passport_office_->clerks_[clerk_types::kPicture].size() > 0) {
      next_clerk = static_cast<clerk_types::Type>(rand() % 2); // either kApplication (0) or kPicture (1)
    } else if (!completed_application()) {
      next_clerk = clerk_types::kApplication;
    } else if (!picture_taken()) {
      next_clerk = clerk_types::kPicture;
    } else if (!certified()) {
      next_clerk = clerk_types::kPassport;
    } else {
      next_clerk = clerk_types::kCashier;
    }
    if (passport_office_->clerks_[next_clerk].empty()) {
      break;
    }
    Clerk* clerk = passport_office_->clerks_[next_clerk][0];
    PrintLineJoin(clerk, bribed_);

    DoClerkWork(clerk);

    clerk->current_customer_ = NULL;
    clerk->wakeup_lock_cv_.Signal(&clerk->wakeup_lock_);
    clerk->wakeup_lock_.Release();
  }

  if (passport_verified()) {
    std::cout << IdentifierString() << " is leaving the Passport Office." << std::endl;
  } else {
    std::cout << IdentifierString() << " terminated early because it is impossible to get a passport." << std::endl;
  }

  // Leaving the office - if there are no more senators left waiting, then
  // tell all the customers outside to come back in.
  --passport_office_->num_senators_;
  if (passport_office_->num_senators_ == 0) {
    passport_office_->outside_line_cv_.Broadcast(
        &passport_office_->outside_line_lock_);
  }
  passport_office_->senator_lock_->Release();
}

std::string Senator::IdentifierString() const {
  std::stringstream ss;
  ss << "Senator [" << ssn_ << ']';
  return ss.str();
}
