#ifndef PASSPORT_OFFICE_PASSPORT_OFFICE_H_
#define PASSPORT_OFFICE_PASSPORT_OFFICE_H_

#include <vector>

#include "clerks.h"
#include "customer.h"
#include "manager.h"
#include "senator.h"
#include "../threads/synch.h"
#include "../threads/thread.h"

namespace clerk_types {

enum Type {
  kApplication = 0,
  kPicture,
  kPassport,
  kCashier,
  Size,
};

}  // namespace clerk_types

class PassportOffice {
 public:
  PassportOffice(int num_application_clerks, int num_picture_clerks,
                 int num_passport_clerks, int num_cashier_clerks);
  
  ~PassportOffice() {
    for (int i = 0; i < line_locks_.size(); ++i) {
      delete line_locks_[i];
    }
    for (int i = 0; i < thread_list_.size(); ++i) {
      delete thread_list_[i];
    }
    for (int i = 0; i < clerks_.size(); ++i) {
      for (int j = 0; j < clerks_[i].size(); ++j) {
        delete clerks_[i][j];
      }
    }
    delete manager_;
  }
  
  void Start();

  void AddNewCustomer(Customer* customer);
  void AddNewSenator(Senator* senator);

  Manager* manager_;

  std::vector<Lock*> line_locks_;
  std::vector<std::vector<Clerk*>> clerks_;

  std::vector<Clerk*> breaking_clerks_;
  Lock* breaking_clerks_lock_;
 private:
  Thread manager_thread_;
  std::vector<Thread*> thread_list_;
};

#endif  // PASSPORT_OFFICE_PASSPORT_OFFICE_H_
