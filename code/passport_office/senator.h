#ifndef PASSPORT_OFFICE_SENATOR_H
#define PASSPORT_OFFICE_SENATOR_H

#include <stdint.h>
#include <vector>

#include "../threads/synch.h"
#include "utilities.h"

class PassportOffice;
class Clerk;
class CashierClerk;

class Senator : public Customer {
 public:
  Senator(PassportOffice* passport_office) : Customer(passport_office) {}
  virtual ~Senator() {}

  virtual std::string IdentifierString() const;
  virtual void Run();
};

#endif
