#ifndef TEST_SIMULATION_H_
#define TEST_SIMULATION_H_

#include "../userprog/syscall.h"
#include "utilities.h"

#define NUM_CUSTOMERS 15
#define NUM_SENATORS 0
#define NUM_APPLICATION_CLERKS 3
#define NUM_PICTURE_CLERKS 3
#define NUM_PASSPORT_CLERKS 3
#define NUM_CASHIER_CLERKS 3
#define NUM_TOTAL_CLERKS NUM_APPLICATION_CLERKS + NUM_PICTURE_CLERKS + NUM_PASSPORT_CLERKS + NUM_CASHIER_CLERKS

int num_clerks_[NUM_CLERK_TYPES] 
    = {NUM_APPLICATION_CLERKS, NUM_PICTURE_CLERKS, NUM_PASSPORT_CLERKS, 
        NUM_CASHIER_CLERKS};

int application_clerk_init_lock_;
int picture_clerk_init_lock_;
int passport_clerk_init_lock_;
int cashier_clerk_init_lock_;
#ifndef NETWORK
int application_clerk_init_count_ = 0;
int picture_clerk_init_count_ = 0;
int passport_clerk_init_count_ = 0;
int cashier_clerk_init_count_ = 0;
int customers_init_size_ = 0;
#endif

int breaking_clerks_lock_;
int senator_lock_;
int senator_condition_;
int customer_count_lock_;
int customers_served_lock_;
int customers_served_cv_;
int num_customers_being_served_;
int num_customers_waiting_lock_;
int num_customers_waiting_;
int num_senators_lock_;
int num_senators_;
int outside_line_lock_;
int outside_line_cv_;

int line_locks_[NUM_CLERK_TYPES];
Customer customers_[NUM_CUSTOMERS + NUM_SENATORS];
int customers_size_;

Clerk clerks_[NUM_CLERK_TYPES][MAX_NUM_CLERKS];
#ifdef NETWORK
int line_counts_[NUM_CLERK_TYPES];
int bribe_line_counts_[NUM_CLERK_TYPES];
#else
int line_counts_[NUM_CLERK_TYPES][MAX_NUM_CLERKS];
int bribe_line_counts_[NUM_CLERK_TYPES][MAX_NUM_CLERKS];
#endif

Manager manager_;

#ifdef NETWORK
int customer_index_;
int clerk_index_[NUM_CLERK_TYPES];
int clerk_customer_ssn_[NUM_CLERK_TYPES];
int clerk_customer_money_[NUM_CLERK_TYPES];
int clerk_collected_money_[NUM_CLERK_TYPES];
int clerk_customer_input_[NUM_CLERK_TYPES];
int clerk_state_[NUM_CLERK_TYPES];
int customer_money_;
int customer_bribed_;
int customer_picture_taken_;
int customer_certified_;
int customer_completed_application_;
int customer_passport_verified_;
#endif

int NUM_INITIAL_MONEY_AMOUNTS = 4;
int INITIAL_MONEY_AMOUNTS[] = {100, 600, 1100, 1600};
int CURRENT_UNUSED_SSN = 0;
int SENATOR_UNUSED_SSN = 0;

void PrintCustomerIdentifierString(int customer_ssn);
void DoClerkWork(Customer* customer, Clerk* clerk);
void PrintLineJoin(Customer* customer, Clerk* clerk, int bribed);
void PrintSenatorIdentifierString(Customer* customer);
void SenatorRun(Customer* customer);
void CustomerRun(Customer* customer);
void PrintNameForClerkType(ClerkType type);
void PrintClerkIdentifierString(Clerk* clerk);
void DestroyClerk(Clerk* clerk);
void JoinLine(Clerk* clerk, int bribe);
void GetNextCustomer(Clerk* clerk);
void ApplicationClerkWork(Clerk* clerk);
void PictureClerkWork(Clerk* clerk);
void PassportClerkWork(Clerk* clerk);
void CashierClerkWork(Clerk* clerk);
void ClerkRun(Clerk* clerk);
void ManagerPrintMoneyReport(Manager* manager);
void ManagerRun(Manager* manager);

#ifndef NETWORK
void AddNewCustomer(int index);
void AddNewSenator(int index);
void DestroyCustomer(Customer* customer);
void RunManager();
void RunManagerMoneyReport();
void RunClerk();
void RunCustomer();
void RunSenator();
void StartPassportOffice();
#endif

void WakeWaitingCustomers();
void WakeClerksForSenator();

void SetupPassportOffice();
void WaitOnFinish();
void RunEntity(EntityType type, int entityId);
#endif
