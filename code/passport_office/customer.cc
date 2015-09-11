#include "Customer.h"

#include <cstdlib>

uint32_t Customer::CURRENT_UNUSED_SSN = 0;

Customer::Customer() :
  certified_(false),
  money_(INITIAL_MONEY_AMOUNTS[rand() % INITIAL_MONEY_AMOUNTS.size()]),
  passport_verified_(false),
  picture_taken_(false),
  ssn_(std::to_string(CURRENT_UNUSED_SSN++)) {
}

Customer::~Customer() {
}
