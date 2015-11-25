// nettest.cc 
//	Test out message delivery between two "Nachos" machines,
//	using the Post Office to coordinate delivery.
//
//	Two caveats:
//	  1. Two copies of Nachos must be running, with machine ID's 0 and 1:
//		./nachos -m 0 -o 1 &
//		./nachos -m 1 -o 0 &
//
//	  2. You need an implementation of condition variables,
//	     which is *not* provided as part of the baseline threads 
//	     implementation.  The Post Office won't work without
//	     a correct implementation of condition variables.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#include "system.h"
#include "network.h"
#include "post.h"
#include "interrupt.h"
#include "../network/network_utility.h"
#include "string.h"
#include "deque.h"
#include "map.h"

#include <sstream>
#include <iostream>

struct Message {
  PacketHeader packetHdr;
  MailHeader mailHdr;
  char * data;
  Message(PacketHeader p, MailHeader m, char * d, int len) {
    packetHdr = p;
    mailHdr = m;
    data = new char[len];
    strcpy(data, d);
  }
};

struct ServerLock {
  bool busy;
  int machineID;
  std::string name;
  bool toBeDeleted;
  int numWaitingOnCV;
  std::deque<Message> waitQ;
  ServerLock(std::string n) {
    machineID = -1;
    name = n;
    busy = false;
    toBeDeleted = false;
    numWaitingOnCV = 0;
  }
  void addToWaitQ(Message m) {
    waitQ.push_back(m);
  }
};

struct ServerCV {
  int lockID;
  std::string name;
  bool toBeDeleted;
  std::deque<Message> waitQ;
  ServerCV(std::string n) {
    name = n;
    lockID = -1;
    toBeDeleted = false;
  }
  void addToWaitQ(Message m) {
    waitQ.push_back(m);
  }
};

struct ServerMV {
  std::vector<int> value;
  std::string name;
  ServerMV(std::string n, int size) {
    name = n;
    for (int i = 0; i < size; ++i) {
      value.push_back(0);
    }
  }
};

void create_lock(PacketHeader outPktHdr, MailHeader outMailHdr, 
  int & currentLock, std::map<int, ServerLock> & locks, string lock_name);

void acquire_lock(PacketHeader outPktHdr, MailHeader outMailHdr, 
    PacketHeader inPktHdr, std::map<int, ServerLock> & locks, int lockID);

void release_lock(PacketHeader outPktHdr, MailHeader outMailHdr, 
    PacketHeader inPktHdr, std::map<int, ServerLock> & locks, int lockID);

void destroy_lock(PacketHeader outPktHdr, MailHeader outMailHdr, 
    std::map<int, ServerLock> & locks, int lockID);

void create_cv(PacketHeader outPktHdr, MailHeader outMailHdr, int & currentCV,
    std::map<int, ServerCV> & cvs, string cv_name);

void wait_cv(PacketHeader outPktHdr, MailHeader outMailHdr, PacketHeader inPktHdr,
    std::map<int, ServerLock> & locks, std::map<int, ServerCV> & cvs, int cvID,
    int lockID);

void signal_cv(PacketHeader outPktHdr, MailHeader outMailHdr, PacketHeader inPktHdr,
    std::map<int, ServerLock> & locks, std::map<int, ServerCV> & cvs, int cvID,
    int lockID);

void broadcast_cv(PacketHeader outPktHdr, MailHeader outMailHdr, PacketHeader inPktHdr,
    std::map<int, ServerLock> & locks, std::map<int, ServerCV> & cvs, int cvID,
    int lockID);

void destroy_cv(PacketHeader outPktHdr, MailHeader outMailHdr, 
    std::map<int, ServerCV> & cvs, int cvID);

void create_mv(PacketHeader outPktHdr, MailHeader outMailHdr, int & currentMV,
    std::map<int, ServerMV> & mvs, int size, string mv_name);

void set_mv(PacketHeader outPktHdr, MailHeader outMailHdr, 
    std::map<int, ServerMV> & mvs, int mvID, int index, int value);

void get_mv(PacketHeader outPktHdr, MailHeader outMailHdr,
    std::map<int, ServerMV> & mvs, int mvID, int index);

void destroy_mv(PacketHeader outPktHdr, MailHeader outMailHdr, 
    std::map<int, ServerMV> & mvs, int mvID);

void release_lock_helper(PacketHeader outPktHdr, MailHeader outMailHdr, 
    PacketHeader inPktHdr, std::map<int, ServerLock> & locks, int lockID, 
    ServerLock* temp_lock, bool send);

// Server for Project 3 Part 3
void Server() {
  DEBUG('R', "In server function\n");
  int currentLock = 0;
  int currentCV = 0;
  int currentMV = 0;
  std::map<int, ServerLock> locks;
  std::map<int, ServerCV> cvs;
  std::map<int, ServerMV> mvs;

  for (;;) {
    // Receive a message
    PacketHeader outPktHdr, inPktHdr;
    MailHeader outMailHdr, inMailHdr;
    char buffer[MaxMailSize];
    postOffice->Receive(SERVER_MAILBOX, &inPktHdr, &inMailHdr, buffer);

    outPktHdr.to = inPktHdr.from;
    outMailHdr.to = inMailHdr.from;
    outMailHdr.from = SERVER_MAILBOX;

    DEBUG('R', "Received packet\n");

    // Parse the message
    int s;
    stringstream ss(buffer);
    DEBUG('R', "MESSAGE: ");
    DEBUG('R', buffer);
    DEBUG('R', "\n");
    ss >> s;

    // Process the message and Send a reply… maybe
    switch (s) {
      case CREATE_LOCK: {
        create_lock(outPktHdr, outMailHdr, currentLock, locks, ss.str());
        break;
      }
      case ACQUIRE_LOCK: {
        int lockID;
        ss >> lockID;
        acquire_lock(outPktHdr, outMailHdr, inPktHdr, locks, lockID);
        break;
      }
      case RELEASE_LOCK: {
        int lockID;
        ss >> lockID;
        release_lock(outPktHdr, outMailHdr, inPktHdr, locks, lockID);
        break;
      }
      case DESTROY_LOCK: {
        int lockID;
        ss >> lockID;
        destroy_lock(outPktHdr, outMailHdr, locks, lockID);
        break;
      }
      case CREATE_CV: {
        create_cv(outPktHdr, outMailHdr, currentCV, cvs, ss.str());
        break;
      }
      case WAIT_CV: {
        int cvID, lockID;
        ss >> cvID;
        ss >> lockID;
        wait_cv(outPktHdr, outMailHdr, inPktHdr, locks, cvs, cvID, lockID);
        break;
      }
      case SIGNAL_CV: {
        int cvID, lockID;
        ss >> cvID;
        ss >> lockID;
        signal_cv(outPktHdr, outMailHdr, inPktHdr, locks, cvs, cvID, lockID);
        break;
      }
      case BROADCAST_CV: {
        int cvID, lockID;
        ss >> cvID;
        ss >> lockID;
        broadcast_cv(outPktHdr, outMailHdr, inPktHdr, locks, cvs, cvID, lockID);
        break;
      }
      case DESTROY_CV: {
        int cvID;
        ss >> cvID;
        destroy_cv(outPktHdr, outMailHdr, cvs, cvID);
        break;
      }
      case CREATE_MV: {
        int size;
        ss >> size;
        std::string mv_name = ss.str();
        create_mv(outPktHdr, outMailHdr, currentMV, mvs, size, mv_name);
        break;
      }
      case SET_MV: {
        int mvID, index, value;
        ss >> mvID;
        ss >> index;
        ss >> value;
        set_mv(outPktHdr, outMailHdr, mvs, mvID, index, value);
        break;
      }
      case GET_MV: {
        int mvID, index;
        ss >> mvID;
        ss >> index;
        get_mv(outPktHdr, outMailHdr, mvs, mvID, index);
        break;
      }
      case DESTROY_MV: {
        int mvID;
        ss >> mvID;
        destroy_mv(outPktHdr, outMailHdr, mvs, mvID);
        break;
      }
    }
  }

  // Then we're done!
  interrupt->Halt();
}

// Returns the lock ID for the given
void create_lock(PacketHeader outPktHdr, MailHeader outMailHdr, 
    int & currentLock, std::map<int, ServerLock> & locks, string lock_name) {
  DEBUG('R', "Create lock on server starting\n");
  int lockID;
  bool found = false;
  for (std::map<int, ServerLock>::iterator it = locks.begin(); 
      it != locks.end(); ++it) {
    if (it->second.name == lock_name) {
      lockID = it->first;
      found = true;
      break;
    }
  } 
  if (!found) {
    if (locks.size() >= NUM_SYSTEM_LOCKS) {
      outMailHdr.length = 3;
      postOffice->Send(outPktHdr, outMailHdr, "-1");
    }
    lockID = currentLock;
    ServerLock lock(lock_name);
    locks.insert(std::pair<int, ServerLock>(currentLock++, lock));
  }
  stringstream ss;
  ss << (lockID);
  outMailHdr.length = ss.str().length() + 1;
  char *cstr = new char[ss.str().length() + 1];
  strcpy(cstr, ss.str().c_str());
  DEBUG('R',"Create lock on server sending: %s\n", cstr);
  postOffice->Send(outPktHdr, outMailHdr, cstr);
}

// Returns 0 if lock doesn't exist, 1 if it does and is acquired.
void acquire_lock(PacketHeader outPktHdr, MailHeader outMailHdr, 
    PacketHeader inPktHdr, std::map<int, ServerLock> & locks, int lockID) {
  DEBUG('R', "Acquiring lock on server starting\n");
  outMailHdr.length = 2;
  if (locks.find(lockID) != locks.end()) {
    ServerLock *temp_lock = &(locks.find(lockID)->second);
    if (temp_lock->busy) {
      DEBUG('R', "Found lock and busy\n");
      Message m(outPktHdr, outMailHdr, "1", 2);
      temp_lock->addToWaitQ(m);
    } else {
      DEBUG('R', "Found lock and not busy\n");
      temp_lock->busy = true;
      temp_lock->machineID = inPktHdr.from;
      postOffice->Send(outPktHdr, outMailHdr, "1");
    }
  } else {
    DEBUG('R', "Couldn't find lock\n");
    postOffice->Send(outPktHdr, outMailHdr, "0");
  }
}

// Returns 0 if lock doesn't exist, 1 if it does and is acquired.
void release_lock(PacketHeader outPktHdr, MailHeader outMailHdr, 
    PacketHeader inPktHdr, std::map<int, ServerLock> & locks, int lockID) {
  DEBUG('R', "Releasing lock on server starting\n");
  outMailHdr.length = 2;
  if (locks.find(lockID) != locks.end()) {
    ServerLock* temp_lock = &(locks.find(lockID)->second);
    release_lock_helper(outPktHdr, outMailHdr, inPktHdr, locks, lockID, temp_lock, true);
  } else {
    DEBUG('R', "Couldn't find lock\n");
    postOffice->Send(outPktHdr, outMailHdr, "0");
  }
}

// Returns 0 if lock doesn't exist, 1 if it does and is acquired.
void destroy_lock(PacketHeader outPktHdr, MailHeader outMailHdr, 
    std::map<int, ServerLock> & locks, int lockID) {
  DEBUG('R', "Destroying lock on server starting\n");
  outMailHdr.length = 2;
  if (locks.find(lockID) != locks.end()) {
    ServerLock* temp_lock = &(locks.find(lockID)->second);
    if (temp_lock->busy || temp_lock->numWaitingOnCV > 0) {
      DEBUG('R', "In use so, marking for deletion\n");
      temp_lock->toBeDeleted = true;
      postOffice->Send(outPktHdr, outMailHdr, "1");
    } else {
      DEBUG('R', "Successfully deleted\n");
      locks.erase(locks.find(lockID));
      postOffice->Send(outPktHdr, outMailHdr, "1");
    }
  } else {
    DEBUG('R', "Couldn't find lock\n");
    postOffice->Send(outPktHdr, outMailHdr, "0");
  }
}

// Returns the cv ID for the given CV.
void create_cv(PacketHeader outPktHdr, MailHeader outMailHdr, int & currentCV,
    std::map<int, ServerCV> & cvs, string cv_name) {
  DEBUG('R', "Create cv on server starting\n");
  int cvID;
  bool found = false;
  for (std::map<int, ServerCV>::iterator it = cvs.begin(); it != cvs.end(); ++it) {
    if (it->second.name == cv_name) {
      cvID = it->first;
      found = true;
      break;
    }
  } 
  if (!found) {
    if (cvs.size() >= NUM_SYSTEM_CONDITIONS) {
      outMailHdr.length = 3;
      postOffice->Send(outPktHdr, outMailHdr, "-1");
    }
    cvID = currentCV;
    ServerCV cv(cv_name);
    cvs.insert(std::pair<int, ServerCV>(currentCV++, cv));
  }
  stringstream ss;
  ss << (cvID);
  outMailHdr.length = ss.str().length() + 1;
  char *cstr = new char[ss.str().length() + 1];
  strcpy(cstr, ss.str().c_str());
  DEBUG('R', "Create cv on server sending: %s\n", cstr);
  postOffice->Send(outPktHdr, outMailHdr, cstr);
}

void wait_cv(PacketHeader outPktHdr, MailHeader outMailHdr, PacketHeader inPktHdr,
    std::map<int, ServerLock> & locks, std::map<int, ServerCV> & cvs, int cvID,
    int lockID) {
  DEBUG('R', "Waiting on cv on server starting\n");
  outMailHdr.length = 2;
  if (cvs.find(cvID) != cvs.end() && locks.find(lockID) != locks.end()) {
    ServerCV* temp_cv = &(cvs.find(cvID)->second);
    ServerLock* temp_lock = &(locks.find(lockID)->second);
    if ((temp_cv->lockID != -1 && temp_cv->lockID != lockID) 
        || temp_lock->machineID != inPktHdr.from) {
      DEBUG('R', "Trying to wait on lock that doesn't belong to cv or machine. real: %d request: %d\n", temp_lock->machineID, inPktHdr.from);
      postOffice->Send(outPktHdr, outMailHdr, "0");
    } else {
      DEBUG('R', "Adding to CV waitQ\n");
      ++temp_lock->numWaitingOnCV;
      Message m(outPktHdr, outMailHdr, "1", 2);
      temp_cv->addToWaitQ(m);
      temp_cv->lockID = lockID;

      // Release lock
      release_lock_helper(outPktHdr, outMailHdr, inPktHdr, locks, lockID, temp_lock, false);
    }
  } else {
    DEBUG('R', "Couldn't find cv or lock\n");
    postOffice->Send(outPktHdr, outMailHdr, "0");
  }
}

void signal_cv(PacketHeader outPktHdr, MailHeader outMailHdr, PacketHeader inPktHdr,
    std::map<int, ServerLock> & locks, std::map<int, ServerCV> & cvs, int cvID,
    int lockID) {
  DEBUG('R', "Signalling on cv on server starting\n");
  outMailHdr.length = 2;
  if (cvs.find(cvID) != cvs.end() && locks.find(lockID) != locks.end()) {
    ServerCV* temp_cv = &(cvs.find(cvID)->second);
    ServerLock* temp_lock = &(locks.find(lockID)->second);
    if ((temp_cv->lockID != -1 && temp_cv->lockID != lockID) 
        || temp_lock->machineID != inPktHdr.from) {
      DEBUG('R', "Trying to signal cv on lock that doesn't belong to cv or machine. real: %d request: %d\n", temp_lock->machineID, inPktHdr.from);
      postOffice->Send(outPktHdr, outMailHdr, "0");
    } else {
      if (!temp_cv->waitQ.empty()) {
        DEBUG('R', "Releasing from waitQ and acquire lock\n");
        --temp_lock->numWaitingOnCV;
        Message m = temp_cv->waitQ.front();
        temp_cv->waitQ.pop_front();

        if (temp_lock->busy) {
          DEBUG('R', "lock is busy\n");
          temp_lock->addToWaitQ(m);
        } else {
          DEBUG('R', "lock is not busy\n");
          temp_lock->busy = true;
          temp_lock->machineID = inPktHdr.from;
          postOffice->Send(m.packetHdr, m.mailHdr, m.data);
        }

        if (temp_cv->waitQ.empty()) {
          temp_cv->lockID = -1;
          if (temp_cv->toBeDeleted) {
            cvs.erase(cvs.find(cvID));
          }
        }
        postOffice->Send(outPktHdr, outMailHdr, "1");
      }
    }
  } else {
    DEBUG('R', "Couldn't find cv or lock\n");
    postOffice->Send(outPktHdr, outMailHdr, "0");
  }
}

void broadcast_cv(PacketHeader outPktHdr, MailHeader outMailHdr, PacketHeader inPktHdr,
    std::map<int, ServerLock> & locks, std::map<int, ServerCV> & cvs, int cvID,
    int lockID) {
  DEBUG('R', "Broadcasting on cv on server starting\n");
  outMailHdr.length = 2;
  if (cvs.find(cvID) != cvs.end() && locks.find(lockID) != locks.end()) {
    ServerCV* temp_cv = &(cvs.find(cvID)->second);
    ServerLock* temp_lock = &(locks.find(lockID)->second);
    if ((temp_cv->lockID != -1 && temp_cv->lockID != lockID) 
      || temp_lock->machineID != inPktHdr.from) {
      DEBUG('R', "Trying to signal cv on lock that doesn't belong to cv or machine. real: %d request: %d\n", temp_lock->machineID, inPktHdr.from);
    postOffice->Send(outPktHdr, outMailHdr, "0");
    } else {
      while(!temp_cv->waitQ.empty()) {
        --temp_lock->numWaitingOnCV;
        Message m = temp_cv->waitQ.front();
        temp_cv->waitQ.pop_front();

        if (temp_lock->busy) {
          DEBUG('R', "lock is busy\n");
          temp_lock->addToWaitQ(m);
        } else {
          DEBUG('R', "lock is not busy\n");
          temp_lock->busy = true;
          temp_lock->machineID = inPktHdr.from;
          postOffice->Send(m.packetHdr, m.mailHdr, m.data);
        }
      } 
      temp_cv->lockID = -1;
      if (temp_cv->toBeDeleted) {
        cvs.erase(cvs.find(cvID));
      }
      postOffice->Send(outPktHdr, outMailHdr, "1");
    }
  } else {
    DEBUG('R', "Couldn't find cv or lock\n");
    postOffice->Send(outPktHdr, outMailHdr, "0");
  }
}

void destroy_cv(PacketHeader outPktHdr, MailHeader outMailHdr, 
    std::map<int, ServerCV> & cvs, int cvID) {
  DEBUG('R', "Destroying cv on server starting\n");
  outMailHdr.length = 2;
  if (cvs.find(cvID) != cvs.end()) {
    ServerCV* temp_cv = &(cvs.find(cvID)->second);
    if (!temp_cv->waitQ.empty()) {
      DEBUG('R', "Stuff on waitQ so marking for deletion\n");
      temp_cv->toBeDeleted = true;
      postOffice->Send(outPktHdr, outMailHdr, "1");
    } else {
      DEBUG('R', "Successfully deleted\n");
      cvs.erase(cvs.find(cvID));
      postOffice->Send(outPktHdr, outMailHdr, "1");
    }
  } else {
    DEBUG('R', "Couldn't find lock\n");
    postOffice->Send(outPktHdr, outMailHdr, "0");
  }
}

void create_mv(PacketHeader outPktHdr, MailHeader outMailHdr, int & currentMV,
    std::map<int, ServerMV> & mvs, int size, string mv_name) {
  DEBUG('R', "Create mv on server starting\n");
  int mvID;
  bool found = false;
  for (std::map<int, ServerMV>::iterator it = mvs.begin(); it != mvs.end(); ++it) {
    if (it->second.name == mv_name) {
      mvID = it->first;
      found = true;
      break;
    }
  } 
  if (!found) {
    mvID = currentMV;
    ServerMV mv(mv_name, size);
    mvs.insert(std::pair<int, ServerMV>(currentMV++, mv));
  }
  stringstream ss;
  ss << (mvID);
  outMailHdr.length = ss.str().length() + 1;
  char *cstr = new char[ss.str().length() + 1];
  strcpy(cstr, ss.str().c_str());
  DEBUG('R',"Create mv on server sending: %s\n", cstr);
  postOffice->Send(outPktHdr, outMailHdr, cstr);
}

void set_mv(PacketHeader outPktHdr, MailHeader outMailHdr, 
    std::map<int, ServerMV> & mvs, int mvID, int index, int value) {
  DEBUG('R', "Setting mv on server starting\n");
  outMailHdr.length = 2;
  if (mvs.find(mvID) != mvs.end() && mvs.find(mvID)->second.value.size() > index) {
    (mvs.find(mvID)->second).value[index] = value;
    postOffice->Send(outPktHdr, outMailHdr, "1");
  } else {
    DEBUG('R', "Couldn't find mv or index is out of range\n");
    postOffice->Send(outPktHdr, outMailHdr, "0");
  }
}

// First send whether or not success, if success then return value
void get_mv(PacketHeader outPktHdr, MailHeader outMailHdr,
    std::map<int, ServerMV> & mvs, int mvID, int index) {
  DEBUG('R', "Getting mv on server starting\n");
  if (mvs.find(mvID) != mvs.end() && mvs.find(mvID)->second.value.size() > index) {
    stringstream ss;
    ss << "1 " << (mvs.find(mvID)->second).value[index];
    char *cstr = new char[ss.str().length() + 1];
    strcpy(cstr, ss.str().c_str());
    outMailHdr.length = ss.str().length() + 1;
    postOffice->Send(outPktHdr, outMailHdr, cstr);
  } else {
    DEBUG('R', "Couldn't find mv or index is out of range\n");
    outMailHdr.length = 2;
    postOffice->Send(outPktHdr, outMailHdr, "0");
  }
}

void destroy_mv(PacketHeader outPktHdr, MailHeader outMailHdr, 
    std::map<int, ServerMV> & mvs, int mvID) {
  DEBUG('R', "Destroying mv on server starting\n");
  outMailHdr.length = 2;
  if (mvs.find(mvID) != mvs.end()) {
    mvs.erase(mvs.find(mvID));
    postOffice->Send(outPktHdr, outMailHdr, "1");
  } else {
    DEBUG('R', "Couldn't find lock\n");
    postOffice->Send(outPktHdr, outMailHdr, "0");
  }
}

void release_lock_helper(PacketHeader outPktHdr, MailHeader outMailHdr, 
    PacketHeader inPktHdr, std::map<int, ServerLock> & locks, int lockID, 
    ServerLock* temp_lock, bool send) {
  if (temp_lock->machineID != inPktHdr.from) {
    DEBUG('R', "Trying to release lock it doesn't have. real: %d request: %d\n", temp_lock->machineID, inPktHdr.from);
    postOffice->Send(outPktHdr, outMailHdr, "0");
  } else if (!temp_lock->waitQ.empty()) {
    DEBUG('R', "Releasing from waitQ\n");
    Message m = temp_lock->waitQ.front();
    temp_lock->waitQ.pop_front();
    temp_lock->machineID = m.packetHdr.to;
    DEBUG('R', "m.packetHdr.to: %d", m.packetHdr.to);
    postOffice->Send(m.packetHdr, m.mailHdr, m.data);
    if (send) {
      postOffice->Send(outPktHdr, outMailHdr, "1");
    }
  } else {
    DEBUG('R', "Releasing no waitQ\n");
    temp_lock->busy = false;
    temp_lock->machineID = -1;
    if (temp_lock->toBeDeleted && temp_lock->numWaitingOnCV == 0) {
      locks.erase(locks.find(lockID));
    }
    if (send) {
      postOffice->Send(outPktHdr, outMailHdr, "1");
    }
  }
}

// Test out message delivery, by doing the following:
//	1. send a message to the machine with ID "farAddr", at mail box #0
//	2. wait for the other machine's message to arrive (in our mailbox #0)
//	3. send an acknowledgment for the other machine's message
//	4. wait for an acknowledgement from the other machine to our 
//	    original message

void
MailTest(int farAddr)
{
  PacketHeader outPktHdr, inPktHdr;
  MailHeader outMailHdr, inMailHdr;
  char *data = "Hello there!";
  char *ack = "Got it!";
  char buffer[MaxMailSize];

    // construct packet, mail header for original message
    // To: destination machine, mailbox 0
    // From: our machine, reply to: mailbox 1
  outPktHdr.to = farAddr;		
  outMailHdr.to = 0;
  outMailHdr.from = 1;
  outMailHdr.length = strlen(data) + 1;

    // Send the first message
  bool success = postOffice->Send(outPktHdr, outMailHdr, data); 

  if ( !success ) {
    printf("The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
    interrupt->Halt();
  }

    // Wait for the first message from the other machine
  postOffice->Receive(0, &inPktHdr, &inMailHdr, buffer);
  printf("Got \"%s\" from %d, box %d\n",buffer,inPktHdr.from,inMailHdr.from);
  fflush(stdout);

    // Send acknowledgement to the other machine (using "reply to" mailbox
    // in the message that just arrived
  outPktHdr.to = inPktHdr.from;
  outMailHdr.to = inMailHdr.from;
  outMailHdr.length = strlen(ack) + 1;
  success = postOffice->Send(outPktHdr, outMailHdr, ack); 

  if ( !success ) {
    printf("The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
    interrupt->Halt();
  }

    // Wait for the ack from the other machine to the first message we sent.
  postOffice->Receive(1, &inPktHdr, &inMailHdr, buffer);
  printf("Got \"%s\" from %d, box %d\n",buffer,inPktHdr.from,inMailHdr.from);
  fflush(stdout);

    // Then we're done!
  interrupt->Halt();
}
