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

#endif
