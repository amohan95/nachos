#ifndef TEST_UTILITIES_H_
#define TEST_UTILITIES_H_
#define NULL 0

#define MAX_NUM_CLERKS 5
#define NUM_CLERK_TYPES 4

typedef enum ClerkStates {
  kAvailable = 0,
  kBusy,
  kOnBreak,
} ClerkState;

typedef enum ClerkTypes {
  kApplication = 0,
  kPicture,
  kPassport,
  kCashier,
} ClerkType;

typedef enum CustomerTypes {
  kCustomer = 0,
  kSenator,  
} CustomerType;

struct Clerk;
struct Customer;
struct Manager;

#define CLERK_BRIBE_AMOUNT 500
#define PASSPORT_FEE 100
#define CLERK_WAKEUP_THRESHOLD 3
#define MONEY_REPORT_INTERVAL 5000

typedef struct Customers {
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
  CustomerType type_;
} Customer;

typedef struct Clerks {
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
  ClerkType type_;
  ClerkState state_;
  int collected_money_;
  int identifier_;
  int running_;
} Clerk;

typedef struct Managers {
  int wakeup_condition_;
  int wakeup_condition_lock_;
  int elapsed_;
  int running_;
} Manager;

#endif
