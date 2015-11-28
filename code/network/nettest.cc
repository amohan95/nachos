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

#include "../threads/system.h"
#include "network.h"
#include "network_utility.h"
#include "string.h"
#include "deque.h"
#include "vector.h"

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

struct Request {
  int requestorMID;
  int requestorMB;
  int requestType;
  bool yesResponse;
  int noCount;
  string info;
  Request(int mid, int mb, int type, string info_string) {
    requestorMID = mid;
    requestorMB = mb;
    requestType = type;
    info = info_string;
    yesResponse = false;
    noCount = 0;
  }
};

int find_lock_by_name(std::map<int, ServerLock> locks, string lock_name);

bool find_lock_by_id(std::map<int, ServerLock>& locks, int id);

void create_new_lock_and_send(PacketHeader outPktHdr, MailHeader outMailHdr, 
    int & currentLock, map<int, ServerLock> & locks, string lock_name);

void acquire_lock(PacketHeader outPktHdr, MailHeader outMailHdr, int pkt,
    std::map<int, ServerLock> & locks, int lockID);

void release_lock(PacketHeader outPktHdr, MailHeader outMailHdr, int pkt, 
    std::map<int, ServerLock> & locks, int lockID, ServerLock* temp_lock,
    bool send);

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

void setup_message_and_send(PacketHeader outPktHdr, MailHeader outMailHdr, string s) {
  outMailHdr.length = s.length() + 1;
  char *cstr = new char[s.length() + 1];
  strcpy(cstr, s.c_str());
  postOffice->Send(outPktHdr, outMailHdr, cstr);
}

string get_rest_of_stream(std::stringstream& ss) {
  string full;
  ss >> full;
  string s;
  while(ss >> s) {
    full += " " + s;
  }
  return full;
}

void create_request_and_send_servers(PacketHeader inPktHdr, 
    MailHeader inMailHdr, PacketHeader outPktHdr, 
    MailHeader outMailHdr, map<int, Request> & pending_requests, 
    int & currentRequest, int type, string info) {
  pending_requests.insert(std::pair<int, Request>(currentRequest++, 
      Request(inPktHdr.from, inMailHdr.from, type, info)));
  outMailHdr.to = SERVER_MAILBOX;
  for (int i = 0; i < numServers; ++i) {
    if (i != machineId) {
      DEBUG('R', "Sending find request to server: %d for %s\n", i, info.c_str());
      outPktHdr.to = i;
      stringstream ss;
      ss << SERVER_REQUEST << " " << currentRequest - 1  << " " << type 
          << " " << inPktHdr.from << " " << inMailHdr.from << " " 
          << info;
      setup_message_and_send(outPktHdr, outMailHdr, ss.str());
    }
  }
}

void sendResponse(PacketHeader outPktHdr, MailHeader outMailHdr, int requestId, 
    bool yes) {
  stringstream ss;
  int response = yes ? YES : NO;
  ss << SERVER_RESPONSE << " " << requestId << " " << response;
  setup_message_and_send(outPktHdr, outMailHdr, ss.str());
}

// Server for Project 3 Part 3
void Server() {
  DEBUG('R', "In server function\n");
  int currentLock = 0;
  int currentCV = 0;
  int currentMV = 0;
  int currentRequest = 0;
  std::map<int, ServerLock> locks;
  std::map<int, ServerCV> cvs;
  std::map<int, ServerMV> mvs;
  std::map<int, Request> pending_requests;

  for (;;) {
    // Receive a message.
    PacketHeader outPktHdr, inPktHdr;
    MailHeader outMailHdr, inMailHdr;
    char buffer[MaxMailSize];
    postOffice->Receive(SERVER_MAILBOX, &inPktHdr, &inMailHdr, buffer);

    outPktHdr.to = inPktHdr.from;
    outMailHdr.to = inMailHdr.from;
    outMailHdr.from = SERVER_MAILBOX;

    DEBUG('R', "Received packet\n");

    // Parse the message.
    int s;
    stringstream ss(buffer);
    DEBUG('R', "MESSAGE: ");
    DEBUG('R', buffer);
    DEBUG('R', "\n");
    ss >> s;

    if (inPktHdr.from >= numServers) { // Client message.
      switch (s) {
        case CREATE_LOCK: {
          string lock_name = get_rest_of_stream(ss);
          int lockID = find_lock_by_name(locks, lock_name);
          if (lockID != -1) { // If on this server, send response.
            ss.str("");
            ss.clear();
            ss << lockID;
            DEBUG('R',"Lock found on server sending: %d\n", lockID);
            setup_message_and_send(outPktHdr, outMailHdr, ss.str());
          } else if (numServers == 1) { // If only server, create lock and send response.
            create_new_lock_and_send(outPktHdr, outMailHdr, currentLock, locks, lock_name);
          } else { // See if on another server.
            create_request_and_send_servers(inPktHdr, inMailHdr, outPktHdr, outMailHdr,
                pending_requests, currentRequest, s, lock_name);
          }
          break;
        }
        case ACQUIRE_LOCK: {
          int lockID;
          ss >> lockID;
          if (find_lock_by_id(locks, lockID)) {  // If on this server, send response.
            acquire_lock(outPktHdr, outMailHdr, inPktHdr.from, locks, lockID);
          } else if (numServers == 1) { // If only server, send error.
            DEBUG('R', "Couldn't find lock\n");
            setup_message_and_send(outPktHdr, outMailHdr, "0");
          } else { // See if on another server.
            ss.str("");
            ss.clear();
            ss << lockID;
            create_request_and_send_servers(inPktHdr, inMailHdr, outPktHdr, outMailHdr,
                pending_requests, currentRequest, s, ss.str());
          }
          break;
        }
        case RELEASE_LOCK: {
          int lockID;
          ss >> lockID;
          if (find_lock_by_id(locks, lockID)) {  // If on this server, send response.
            DEBUG('R', "Found lock and releasing: %d\n", lockID);
            release_lock(outPktHdr, outMailHdr, inPktHdr.from, locks, lockID,
                &(locks.find(lockID)->second), true);
          } else if (numServers == 1) { // If only server, send error.
            DEBUG('R', "Couldn't find lock\n");
            setup_message_and_send(outPktHdr, outMailHdr, "0");
          } else { // See if on another server.
            ss.str("");
            ss.clear();
            ss << lockID;
            create_request_and_send_servers(inPktHdr, inMailHdr, outPktHdr, outMailHdr,
                pending_requests, currentRequest, s, ss.str());
          }
          break;
        }
        case DESTROY_LOCK: {
          int lockID;
          ss >> lockID;
          destroy_lock(outPktHdr, outMailHdr, locks, lockID);
          break;
        }
        case CREATE_CV: {
          string temp = get_rest_of_stream(ss);
          create_cv(outPktHdr, outMailHdr, currentCV, cvs, temp);
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
          string mv_name = get_rest_of_stream(ss);
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
    } else { // Server message.
      int requestId;
      ss >> requestId;
      if (s == SERVER_REQUEST) { // Server request.
        ss >> s;
        int pkt, mail;
        ss >> pkt;
        ss >> mail;
        switch (s) {
          case CREATE_LOCK: {
            string lock_name = get_rest_of_stream(ss);
            int lockID = find_lock_by_name(locks, lock_name);

            // If on this server, send response to client and yes to requesting server.
            if (lockID != -1) { 
              DEBUG('R', "Found lock and sending to client: %s\n", lock_name.c_str());
              sendResponse(outPktHdr, outMailHdr, requestId, YES);
              
              ss.str("");
              ss.clear();
              ss << lockID;
              outPktHdr.to = pkt;
              outMailHdr.to = mail;
              setup_message_and_send(outPktHdr, outMailHdr, ss.str());
            } else { // Send no.
              DEBUG('R', "Can't Find lock: %s\n", lock_name.c_str());
              sendResponse(outPktHdr, outMailHdr, requestId, NO);
            }
            break;
          }
          case ACQUIRE_LOCK: {
            int lockID;
            ss >> lockID;
            // If on this server, send response to client and yes to requesting server.
            if (find_lock_by_id(locks, lockID)) {  // If on this server, send response.
              DEBUG('R', "Found lock and handling acquire of lockid: %d\n", lockID);
              sendResponse(outPktHdr, outMailHdr, requestId, YES);

              outPktHdr.to = pkt;
              outMailHdr.to = mail;
              acquire_lock(outPktHdr, outMailHdr, pkt, locks, lockID);
            } else { // Send no.
              DEBUG('R', "Can't Find lock id: %d\n", lockID);
              sendResponse(outPktHdr, outMailHdr, requestId, NO);
            }
            break;
          }
          case RELEASE_LOCK: {
            int lockID;
            ss >> lockID;
            // If on this server, send response to client and yes to requesting server.
            if (find_lock_by_id(locks, lockID)) {  // If on this server, send response.
              DEBUG('R', "Found lock and handling release of lockid: %d\n", lockID);
              sendResponse(outPktHdr, outMailHdr, requestId, YES);

              outPktHdr.to = pkt;
              outMailHdr.to = mail;
              release_lock(outPktHdr, outMailHdr, pkt, locks, lockID, 
                  &(locks.find(lockID)->second), true);
            } else { // Send no.
              DEBUG('R', "Can't Find lock id: %d\n", lockID);
              sendResponse(outPktHdr, outMailHdr, requestId, NO);
            }
          }
        }
      } else { // Server response.
        bool yes;
        ss >> yes;
        map<int, Request>::iterator it = pending_requests.find(requestId);
        if (yes) {
          DEBUG('R', "Received a yes server response\n");
          it->second.yesResponse = true;
          if (numServers == 2) {
            pending_requests.erase(it);
          }
        } else if (it->second.yesResponse  && it->second.noCount == numServers - 3) {
          DEBUG('R', "Received a no server response and erasing\n");
          pending_requests.erase(it);
        } else if (it->second.noCount == numServers - 2){
          DEBUG('R', "Sending client response since all nos\n");
          outPktHdr.to = it->second.requestorMID;
          outMailHdr.to = it->second.requestorMB;
          switch(it->second.requestType) {
            case CREATE_LOCK: {
              create_new_lock_and_send(outPktHdr, outMailHdr, currentLock, locks, it->second.info);
              break;
            }
            case ACQUIRE_LOCK: {
              setup_message_and_send(outPktHdr, outMailHdr, "-1");
              break;
            }
            case RELEASE_LOCK: {
              setup_message_and_send(outPktHdr, outMailHdr, "-1");
              break;
            }
          }
        } else {
          DEBUG('R', "Received a no server response\n");
          it->second.noCount++;
        }
      }
    }
  }

  // Then we're done!
  interrupt->Halt();
}

// Returns the lock ID for the given lock or -1 if not on this server.
int find_lock_by_name(std::map<int, ServerLock> locks, string lock_name) {
  for (std::map<int, ServerLock>::iterator it = locks.begin(); 
      it != locks.end(); ++it) {
    if (it->second.name == lock_name) {
      return it->first;
    }
  }
  return -1;
}

// Returns true if it is on this server, false otherwise.
bool find_lock_by_id(std::map<int, ServerLock>& locks, int id) {
  std::map<int, ServerLock>::iterator it = locks.find(id);
  return it != locks.end();
}


// Returns the lock ID for the new lock, or -1 if not possible.
void create_new_lock_and_send(PacketHeader outPktHdr, MailHeader outMailHdr, 
    int & currentLock, map<int, ServerLock> & locks, string lock_name) {
  if (locks.size() >= NUM_SYSTEM_LOCKS) {
    setup_message_and_send(outPktHdr, outMailHdr, "-1");
  } else {
    ServerLock lock(lock_name);
    locks.insert(std::pair<int, ServerLock>(currentLock++, lock));
    stringstream ss;
    ss << currentLock-1;
    setup_message_and_send(outPktHdr, outMailHdr, ss.str());
  }
}

// Returns 0 if lock doesn't exist, 1 if it does and is acquired.
void acquire_lock(PacketHeader outPktHdr, MailHeader outMailHdr, 
    int pkt, std::map<int, ServerLock> & locks, int lockID) {
  DEBUG('R', "Acquiring lock on server starting\n");
  ServerLock *temp_lock = &(locks.find(lockID)->second);
  if (temp_lock->busy && temp_lock->machineID != pkt) {
    DEBUG('R', "Found lock and busy\n");
    Message m(outPktHdr, outMailHdr, "1", 2);
    temp_lock->addToWaitQ(m);
  } else if (temp_lock->toBeDeleted){
    DEBUG('R', "Lock requested is marked for deletion\n");
    setup_message_and_send(outPktHdr, outMailHdr, "0");
  } else {
    DEBUG('R', "Found lock and not busy or trying to acquire lock it already has\n");
    temp_lock->busy = true;
    temp_lock->machineID = pkt;
    setup_message_and_send(outPktHdr, outMailHdr, "1");
  }
}

// Returns 0 if lock doesn't exist, 1 if it does and is acquired.
void release_lock(PacketHeader outPktHdr, MailHeader outMailHdr, int pkt, 
    std::map<int, ServerLock> & locks, int lockID, ServerLock* temp_lock,
    bool send) {
  if (temp_lock->machineID != pkt) {
    DEBUG('R', "Trying to release lock it doesn't have. real: %d request: %d\n", temp_lock->machineID, pkt);
    setup_message_and_send(outPktHdr, outMailHdr, "0");
  } else if (!temp_lock->waitQ.empty()) {
    DEBUG('R', "Releasing from waitQ\n");
    Message m = temp_lock->waitQ.front();
    temp_lock->waitQ.pop_front();
    temp_lock->machineID = m.packetHdr.to;
    DEBUG('R', "m.packetHdr.to: %d", m.packetHdr.to);
    setup_message_and_send(m.packetHdr, m.mailHdr, m.data);
    if (send) {
      setup_message_and_send(outPktHdr, outMailHdr, "1");
    }
  } else {
    DEBUG('R', "Releasing no waitQ\n");
    temp_lock->busy = false;
    temp_lock->machineID = -1;
    if (temp_lock->toBeDeleted && temp_lock->numWaitingOnCV == 0) {
      locks.erase(locks.find(lockID));
    }
    if (send) {
      setup_message_and_send(outPktHdr, outMailHdr, "1");
    }
  }
}

// Returns 0 if lock doesn't exist, 1 if it does and is acquired.
void destroy_lock(PacketHeader outPktHdr, MailHeader outMailHdr, 
    std::map<int, ServerLock> & locks, int lockID) {
  DEBUG('R', "Destroying lock on server starting\n");
  if (locks.find(lockID) != locks.end()) {
    ServerLock* temp_lock = &(locks.find(lockID)->second);
    if (temp_lock->busy || temp_lock->numWaitingOnCV > 0) {
      DEBUG('R', "In use so, marking for deletion\n");
      temp_lock->toBeDeleted = true;
      setup_message_and_send(outPktHdr, outMailHdr, "1");
    } else {
      DEBUG('R', "Successfully deleted\n");
      locks.erase(locks.find(lockID));
      setup_message_and_send(outPktHdr, outMailHdr, "1");
    }
  } else {
    DEBUG('R', "Couldn't find lock\n");
    setup_message_and_send(outPktHdr, outMailHdr, "0");
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
      setup_message_and_send(outPktHdr, outMailHdr, "-1");
    }
    cvID = currentCV;
    ServerCV cv(cv_name);
    cvs.insert(std::pair<int, ServerCV>(currentCV++, cv));
  }
  stringstream ss;
  ss << (cvID);
  DEBUG('R', "Create cv on server sending: %s\n", ss.str().c_str());
  setup_message_and_send(outPktHdr, outMailHdr, ss.str());
}

void wait_cv(PacketHeader outPktHdr, MailHeader outMailHdr, PacketHeader inPktHdr,
    std::map<int, ServerLock> & locks, std::map<int, ServerCV> & cvs, int cvID,
    int lockID) {
  DEBUG('R', "Waiting on cv on server starting\n");
  if (cvs.find(cvID) != cvs.end() && locks.find(lockID) != locks.end()) {
    ServerCV* temp_cv = &(cvs.find(cvID)->second);
    ServerLock* temp_lock = &(locks.find(lockID)->second);
    if ((temp_cv->lockID != -1 && temp_cv->lockID != lockID) 
        || temp_lock->machineID != inPktHdr.from) {
      DEBUG('R', "Trying to wait on lock that doesn't belong to cv or machine. real: %d request: %d\n", temp_lock->machineID, inPktHdr.from);
      setup_message_and_send(outPktHdr, outMailHdr, "0");
    } else {
      DEBUG('R', "Adding to CV waitQ\n");
      ++temp_lock->numWaitingOnCV;
      Message m(outPktHdr, outMailHdr, "1", 2);
      temp_cv->addToWaitQ(m);
      temp_cv->lockID = lockID;

      // Release lock
      release_lock(outPktHdr, outMailHdr, inPktHdr.from, locks, lockID, temp_lock, false);
    }
  } else {
    DEBUG('R', "Couldn't find cv or lock\n");
    setup_message_and_send(outPktHdr, outMailHdr, "0");
  }
}

void signal_cv(PacketHeader outPktHdr, MailHeader outMailHdr, PacketHeader inPktHdr,
    std::map<int, ServerLock> & locks, std::map<int, ServerCV> & cvs, int cvID,
    int lockID) {
  DEBUG('R', "Signalling on cv on server starting\n");
  if (cvs.find(cvID) != cvs.end() && locks.find(lockID) != locks.end()) {
    ServerCV* temp_cv = &(cvs.find(cvID)->second);
    ServerLock* temp_lock = &(locks.find(lockID)->second);
    if ((temp_cv->lockID != -1 && temp_cv->lockID != lockID) 
        || temp_lock->machineID != inPktHdr.from) {
      DEBUG('R', "Trying to signal cv on lock that doesn't belong to cv or machine. real: %d request: %d\n", temp_lock->machineID, inPktHdr.from);
      setup_message_and_send(outPktHdr, outMailHdr, "0");
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
          setup_message_and_send(m.packetHdr, m.mailHdr, m.data);
        }

        if (temp_cv->waitQ.empty()) {
          temp_cv->lockID = -1;
          if (temp_cv->toBeDeleted) {
            cvs.erase(cvs.find(cvID));
          }
        }
        setup_message_and_send(outPktHdr, outMailHdr, "1");
      }
    }
  } else {
    DEBUG('R', "Couldn't find cv or lock\n");
    setup_message_and_send(outPktHdr, outMailHdr, "0");
  }
}

void broadcast_cv(PacketHeader outPktHdr, MailHeader outMailHdr, PacketHeader inPktHdr,
    std::map<int, ServerLock> & locks, std::map<int, ServerCV> & cvs, int cvID,
    int lockID) {
  DEBUG('R', "Broadcasting on cv on server starting\n");
  if (cvs.find(cvID) != cvs.end() && locks.find(lockID) != locks.end()) {
    ServerCV* temp_cv = &(cvs.find(cvID)->second);
    ServerLock* temp_lock = &(locks.find(lockID)->second);
    if ((temp_cv->lockID != -1 && temp_cv->lockID != lockID) 
      || temp_lock->machineID != inPktHdr.from) {
      DEBUG('R', "Trying to signal cv on lock that doesn't belong to cv or machine. real: %d request: %d\n", temp_lock->machineID, inPktHdr.from);
    setup_message_and_send(outPktHdr, outMailHdr, "0");
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
          setup_message_and_send(m.packetHdr, m.mailHdr, m.data);
        }
      } 
      temp_cv->lockID = -1;
      if (temp_cv->toBeDeleted) {
        cvs.erase(cvs.find(cvID));
      }
      setup_message_and_send(outPktHdr, outMailHdr, "1");
    }
  } else {
    DEBUG('R', "Couldn't find cv or lock\n");
    setup_message_and_send(outPktHdr, outMailHdr, "0");
  }
}

void destroy_cv(PacketHeader outPktHdr, MailHeader outMailHdr, 
    std::map<int, ServerCV> & cvs, int cvID) {
  DEBUG('R', "Destroying cv on server starting\n");
  if (cvs.find(cvID) != cvs.end()) {
    ServerCV* temp_cv = &(cvs.find(cvID)->second);
    if (!temp_cv->waitQ.empty()) {
      DEBUG('R', "Stuff on waitQ so marking for deletion\n");
      temp_cv->toBeDeleted = true;
      setup_message_and_send(outPktHdr, outMailHdr, "1");
    } else {
      DEBUG('R', "Successfully deleted\n");
      cvs.erase(cvs.find(cvID));
      setup_message_and_send(outPktHdr, outMailHdr, "1");
    }
  } else {
    DEBUG('R', "Couldn't find lock\n");
    setup_message_and_send(outPktHdr, outMailHdr, "0");
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
  DEBUG('R',"Create mv on server sending: %s\n", ss.str().c_str());
  setup_message_and_send(outPktHdr, outMailHdr, ss.str());
}

void set_mv(PacketHeader outPktHdr, MailHeader outMailHdr, 
    std::map<int, ServerMV> & mvs, int mvID, int index, int value) {
  DEBUG('R', "Setting mv on server starting\n");
  if (mvs.find(mvID) != mvs.end() && mvs.find(mvID)->second.value.size() > index) {
    (mvs.find(mvID)->second).value[index] = value;
    setup_message_and_send(outPktHdr, outMailHdr, "1");
  } else {
    DEBUG('R', "Couldn't find mv or index is out of range\n");
    setup_message_and_send(outPktHdr, outMailHdr, "0");
  }
}

// First send whether or not success, if success then return value
void get_mv(PacketHeader outPktHdr, MailHeader outMailHdr,
    std::map<int, ServerMV> & mvs, int mvID, int index) {
  DEBUG('R', "Getting mv on server starting\n");
  if (mvs.find(mvID) != mvs.end() && mvs.find(mvID)->second.value.size() > index) {
    stringstream ss;
    ss << "1 " << (mvs.find(mvID)->second).value[index];
    setup_message_and_send(outPktHdr, outMailHdr, ss.str());
  } else {
    DEBUG('R', "Couldn't find mv or index is out of range\n");
    setup_message_and_send(outPktHdr, outMailHdr, "0");
  }
}

void destroy_mv(PacketHeader outPktHdr, MailHeader outMailHdr, 
    std::map<int, ServerMV> & mvs, int mvID) {
  DEBUG('R', "Destroying mv on server starting\n");
  if (mvs.find(mvID) != mvs.end()) {
    mvs.erase(mvs.find(mvID));
    setup_message_and_send(outPktHdr, outMailHdr, "1");
  } else {
    DEBUG('R', "Couldn't find lock\n");
    setup_message_and_send(outPktHdr, outMailHdr, "0");
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
