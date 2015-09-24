#ifndef PASSPORT_OFFICE_PASSPORT_OFFICE_H_
#define PASSPORT_OFFICE_PASSPORT_OFFICE_H_

#include <vector>
#include <set>
#include <string>

#include "../threads/synch.h"
#include "clerks.h"
#include "customer.h"
#include "senator.h"
#include "manager.h"
#include "utilities.h"

class PassportOffice {
 public:
  PassportOffice(int num_application_clerks, int num_picture_clerks,
                 int num_passport_clerks, int num_cashier_clerks);

	virtual ~PassportOffice() {
    for (unsigned i = 0; i < line_locks_.size(); ++i) {
      delete line_locks_[i];
    }
    for (unsigned i = 0; i < thread_list_.size(); ++i) {
      delete thread_list_[i];
    }
    for (unsigned i = 0; i < clerks_.size(); ++i) {
      for (unsigned j = 0; j < clerks_[i].size(); ++j) {
        delete clerks_[i][j];
      }
    }
    delete manager_;
  }

  void Start();
  void WaitOnFinish();
	void Stop();

  void AddNewCustomer(Customer* customer);
  void AddNewSenator(Senator* senator);

  int GetNumCustomersForClerkType(clerk_types::Type type) const;

  Manager* manager_;

  // Locks for each type of clerk - Acquire when needing to check data on a 
  // class of clerk.
  std::vector<Lock*> line_locks_;
  // A map of clerk_types::Type => List of clerks of that type.
  std::vector<std::vector<Clerk*> > clerks_;

  // A list of the clerks currently on break and its lock.
  std::vector<Clerk*> breaking_clerks_;
  Lock* breaking_clerks_lock_;

  // A map of clerk_types::Type => List of numbers indicating how many people
  // are in the line of that type and individual clerk identifier.
  std::vector<std::vector<int> > line_counts_;

  std::vector<std::vector<int> > bribe_line_counts_;

  // Lock and condition for when a senator enters the building. Held by the
  // senator while they are doing their work.
  Lock* senator_lock_;
  Condition* senator_condition_;
	
  // Lock for the total number of customers.
  Lock customer_count_lock_;
  
  // Lock and condition guarding the number of customers currently doing
  // work at the office.
  Lock customers_served_lock_;
  Condition customers_served_cv_;
  unsigned int num_customers_being_served_;
  
  // Lock and variable checking how many customers are waiting to be served
  // by the passport office. Used to see if it is necessary to terminate if
  // it is impossible for customers to be served.
  Lock num_customers_waiting_lock_;
  unsigned int num_customers_waiting_;
  
  // Lock and variable for how many senators are currently waiting to finish.
  // Only one senator can be in the office at a time , so this is incremented
  // at the beginning of Senator::Run and decremented once at the end.
  Lock num_senators_lock_;
  int num_senators_;

  // Lock and condition simulating the customers waiting outside of the office
  // when a senator arrives.
  Lock outside_line_lock_;
  Condition outside_line_cv_;

  // A set of the customers currently needing to be served by the office.
  // Customers are added to it in the PassportOffice::AddNewCustomer method, and
  // customers remove themselves from this set once they are finished.
  std::set<Customer*> customers_;
 private:
  Thread manager_thread_;

  // List of all threads that have started so that they can be released once the
  // passport office finishes. 
  std::vector<Thread*> thread_list_;
};

#endif  // PASSPORT_OFFICE_PASSPORT_OFFICE_H_
