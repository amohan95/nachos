#ifndef PASSPORT_OFFICE_UTILITIES_H_
#define PASSPORT_OFFICE_UTILITIES_H_

struct clerk_states {
  typedef enum {
    kAvailable = 0,
    kBusy,
    kOnBreak,
  } State;
};

struct clerk_types {
  typedef enum {
    kApplication = 0,
    kPicture,
    kPassport,
    kCashier,
    Size,
  } Type;
};

struct customer_types {
  typedef enum {
    kCustomer = 0,
    kSenator,  
  } Type;
};

struct Clerk;
struct Customer;
struct Manager;

#define CLERK_BRIBE_AMOUNT 500
#define PASSPORT_FEE 100
#define CLERK_WAKEUP_THRESHOLD 3
#define MONEY_REPORT_INTERVAL 5000

typedef struct {
  int money_;
  int ssn_;
  int join_line_lock_;
  int join_line_lock_cv_;
  int bribed_;
  int certified_;
  int completed_application_;
  int passport_verified_;
  int picture_taken_;
  int running_;
  customer_types::Type type_;
} Customer;

typedef struct {
  int lines_lock_;
  int bribe_line_lock_cv_;
  int bribe_line_lock_;
  int regular_line_lock_cv_;
  int regular_line_lock_;
  int wakeup_lock_cv_;
  int wakeup_lock_;
  int money_lock_;
  int customer_ssn_;
  Customer* current_customer_;
  int customer_money_;
  int customer_input_;
  char* clerk_type_;
  clerk_types::Type type_;
  clerk_states::State state_;
  int collected_money_;
  int identifier_;
  int running_;
} Clerk;

typedef struct {
  int wakeup_condition_;
  int wakeup_condition_lock_;
  int elapsed_;
  int money_[clerk_types::Size];
  PassportOffice* passport_office_;
  int running_;
} Manager;

#endif
