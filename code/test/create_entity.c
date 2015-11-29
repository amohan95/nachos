#include "../userprog/syscall.h"

#define NETWORK

#include "simulation.c"

enum {
  CUSTOMER = 0,
  SENATOR,
  APPLICATION_CLERK,
  PASSPORT_CLERK,
  CASHIER_CLERK,
  PICTURE_CLERK,
  MANAGER,
  NUM_ENTITY_TYPES,
} EntityType;

int main(int argc, char* argv[]) {
  int type, entityId;
  if (argc != 3) {
    Print("Usage: create_entity <entity_type> <entity_id>\n", 47);
    Exit(1);
  }
  type = *argv[1] - '0';
  if (type < 0 || type >= NUM_ENTITY_TYPES) {
    Print("Illegal EntityType.\n", 20);
    Exit(1);
  }
  entityId = *argv[2] - '0';
  if (entityId < 0 || entityId > 9) {
    Print("Illegal entity id.\n", 17);
    Exit(1);
  }
  SetupPassportOffice();
  switch (type) {
    case CUSTOMER:
      CustomerRun(customers_ + entityId);
      break;
    case SENATOR:
      CustomerRun(customers_ + entityId);
      break;
    case APPLICATION_CLERK:
      ClerkRun(clerks_[kApplication] + entityId);
      break;
    case PASSPORT_CLERK:
      ClerkRun(clerks_[kPassport] + entityId);
      break;
    case CASHIER_CLERK:
      ClerkRun(clerks_[kCashier] + entityId);
      break;
    case PICTURE_CLERK:
      ClerkRun(clerks_[kPicture] + entityId);
      break;
    case MANAGER:
      ManagerRun(&manager_);
      break;
    default:
      break;
  }
  Exit(0);
}
