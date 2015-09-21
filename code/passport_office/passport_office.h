#ifndef PASSPORT_OFFICE_PASSPORT_OFFICE_H_
#define PASSPORT_OFFICE_PASSPORT_OFFICE_H_

#include <vector>
#include <set>
#include <string>

#include "../threads/synch.h"
#include "clerks.h"
#include "customer.h"
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
	void Stop();

  // Creates and forks a new thread for the new member being added.
  void AddNewCustomer(Customer* customer);
  void AddNewSenator(Senator* senator);

  Manager* manager_;

  std::vector<Lock*> line_locks_;
  std::vector<std::vector<Clerk*> > clerks_;
  std::vector<Clerk*> breaking_clerks_;

  std::vector<std::vector<int> > line_counts_;
  std::vector<std::vector<int> > bribe_line_counts_;

  Lock* breaking_clerks_lock_;
  Lock* senator_lock_;
  Condition* senator_condition_;
	Lock customer_count_lock_;
	std::set<Customer*> customers_;
 private:
  Thread manager_thread_;
  std::vector<Thread*> thread_list_;
};

#endif  // PASSPORT_OFFICE_PASSPORT_OFFICE_H_
