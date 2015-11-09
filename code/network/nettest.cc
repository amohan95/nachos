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
#include "vector.h"

#include <sstream>
#include <iostream>

struct Message {
    PacketHeader packetHdr;
    MailHeader mailHdr;
    char * data;
    Message(PacketHeader p, MailHeader m, char * d) {
        packetHdr = p;
        mailHdr = m;
        data = d;
    }
};

struct ServerLock {
    bool busy;
    int machineID;
    std::string name;
    bool toBeDeleted;
    std::deque<Message> waitQ;
    ServerLock(std::string n) {
        machineID = -1;
        name = n;
        busy = false;
        toBeDeleted = false;
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


// Server for Project 3 Part 3
void Server() {
    DEBUG('Z', "In server function\n");
    int currentLock = 0;
    int currentCV = 0;
    std::map<int, ServerLock> locks;
    std::map<int, ServerCV> cvs;

    for (;;) {
        // Receive a message
        PacketHeader outPktHdr, inPktHdr;
        MailHeader outMailHdr, inMailHdr;
        char buffer[MaxMailSize];
        postOffice->Receive(SERVER_MAILBOX, &inPktHdr, &inMailHdr, buffer);

        outPktHdr.to = inPktHdr.from;
        outMailHdr.to = inMailHdr.from;
        outMailHdr.from = SERVER_MAILBOX;

        DEBUG('Z', "Received packet\n");

        // Parse the message
        int s;
        stringstream ss(buffer);
        std::cout << buffer << std::endl;
        ss >> s;

        // Process the message and Send a reply… maybe
        switch (s) {
            // Returns the lock ID for the given
            case CREATE_LOCK: {
                DEBUG('Z', "Create lock on server starting\n");
                std::string lock_name = ss.str();
                int lockID;
                bool found = false;
                for (std::map<int, ServerLock>::iterator it = locks.begin(); it != locks.end(); ++it) {
                    if (it->second.name == lock_name) {
                        lockID = it->first;
                        found = true;
                        break;
                    }
                } 
                if (!found) {
                    lockID = currentLock;
                    ServerLock lock(lock_name);
                    locks.insert(std::pair<int, ServerLock>(currentLock++, lock));
                }
                ss.str("");
                ss.clear();
                ss << (lockID);
                outMailHdr.length = ss.str().length() + 1;
                char *cstr = new char[ss.str().length() + 1];
                strcpy(cstr, ss.str().c_str());
                DEBUG('Z',"Create lock on server sending: %s\n", cstr);
                postOffice->Send(outPktHdr, outMailHdr, cstr);
                break;
            }
            // Returns 0 if lock doesn't exist, 1 if it does and is acquired.
            case ACQUIRE_LOCK: {
                DEBUG('Z', "Acquiring lock on server starting\n");
                int lockID;
                ss >> lockID;
                outMailHdr.length = 2;
                if (locks.find(lockID) != locks.end()) {
                    ServerLock *temp_lock = &(locks.find(lockID)->second);
                    if (temp_lock->busy) {
                        DEBUG('Z', "Found lock and busy\n");
                        Message m(outPktHdr, outMailHdr, "1");
                        temp_lock->addToWaitQ(m);
                    } else {
                        DEBUG('Z', "Found lock and not busy\n");
                        temp_lock->busy = true;
                        temp_lock->machineID = inPktHdr.from;
                        postOffice->Send(outPktHdr, outMailHdr, "1");
                    }
                    break;
                } else {
                    DEBUG('Z', "Couldn't find lock\n");
                    postOffice->Send(outPktHdr, outMailHdr, "0");
                }
                break;
            }
            // Returns 0 if lock doesn't exist, 1 if it does and is acquired.
            case RELEASE_LOCK: {
                DEBUG('Z', "Releasing lock on server starting\n");
                int lockID;
                ss >> lockID;
                bool isValid = false;
                outMailHdr.length = 2;
                if (locks.find(lockID) != locks.end()) {
                    ServerLock* temp_lock = &(locks.find(lockID)->second);
                    if (temp_lock->machineID != inPktHdr.from) {
                        DEBUG('Z', "Trying to release lock it doesn't have. real: %d request: %d\n", temp_lock->machineID, inPktHdr.from);
                        postOffice->Send(outPktHdr, outMailHdr, "0");
                    } else if (!temp_lock->waitQ.empty()) {
                        DEBUG('Z', "Releasing from waitQ\n");
                        Message m = temp_lock->waitQ.front();
                        temp_lock->waitQ.pop_front();
                        temp_lock->machineID = m.packetHdr.to;
                        postOffice->Send(m.packetHdr, m.mailHdr, m.data);
                        postOffice->Send(outPktHdr, outMailHdr, "1");
                    } else {
                        DEBUG('Z', "Releasing no waitQ\n");
                        temp_lock->busy = false;
                        temp_lock->machineID = -1;
                        if (temp_lock->toBeDeleted) {
                            locks.erase(locks.find(lockID));
                        }
                        postOffice->Send(outPktHdr, outMailHdr, "1");
                    }
                } else {
                    DEBUG('Z', "Couldn't find lock\n");
                    postOffice->Send(outPktHdr, outMailHdr, "0");
                }
                break;
            }
            // Returns 0 if lock doesn't exist, 1 if it does and is acquired.
            case DESTROY_LOCK: {
                DEBUG('Z', "Destroying lock on server starting\n");
                int lockID;
                ss >> lockID;
                bool isValid = false;
                outMailHdr.length = 2;
                if (locks.find(lockID) != locks.end()) {
                    ServerLock* temp_lock = &(locks.find(lockID)->second);
                    if (temp_lock->busy) {
                        DEBUG('Z', "In use so, marking for deletion\n");
                        temp_lock->toBeDeleted = true;
                        postOffice->Send(outPktHdr, outMailHdr, "1");
                    } else {
                        DEBUG('Z', "Successfully deleted\n");
                        locks.erase(locks.find(lockID));
                        postOffice->Send(outPktHdr, outMailHdr, "1");
                    }
                } else {
                    DEBUG('Z', "Couldn't find lock\n");
                    postOffice->Send(outPktHdr, outMailHdr, "0");
                }
                break;
            }
            // Returns the cv ID for the given CV.
            case CREATE_CV: {
                DEBUG('Z', "Create cv on server starting\n");
                std::string cv_name = ss.str();
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
                    cvID = currentCV;
                    ServerCV cv(cv_name);
                    cvs.insert(std::pair<int, ServerCV>(currentCV++, cv));
                }
                ss.str("");
                ss.clear();
                ss << (cvID);
                outMailHdr.length = ss.str().length() + 1;
                char *cstr = new char[ss.str().length() + 1];
                strcpy(cstr, ss.str().c_str());
                DEBUG('Z', "Create cv on server sending: %s\n", cstr);
                postOffice->Send(outPktHdr, outMailHdr, cstr);
                break;
            }
            case WAIT_CV: {
                DEBUG('Z', "Waiting on cv on server starting\n");
                int cvID, lockID;
                ss >> cvID;
                ss >> lockID;
                bool isValid = false;
                outMailHdr.length = 2;
                if (cvs.find(cvID) != cvs.end() && locks.find(lockID) != locks.end()) {
                    ServerCV* temp_cv = &(cvs.find(cvID)->second);
                    ServerLock* temp_lock = &(locks.find(lockID)->second);
                    if ((temp_cv->lockID != -1 && temp_cv->lockID != lockID) 
                            || temp_lock->machineID != inPktHdr.from) {
                        DEBUG('Z', "Trying to wait on lock that doesn't belong to cv or machine. real: %d request: %d\n", temp_lock->machineID, inPktHdr.from);
                        postOffice->Send(outPktHdr, outMailHdr, "0");
                        break;
                    } else {
                        DEBUG('Z', "Adding to CV waitQ\n");
                        Message m(outPktHdr, outMailHdr, "1");
                        temp_cv->addToWaitQ(m);
                        temp_cv->lockID = lockID;

                        // Release lock
                        if (!temp_lock->waitQ.empty()) {
                            DEBUG('Z', "Releasing from waitQ\n");
                            Message m2 = temp_lock->waitQ.front();
                            temp_lock->waitQ.pop_front();
                            temp_lock->machineID = m2.packetHdr.to;
                            postOffice->Send(m2.packetHdr, m2.mailHdr, m2.data);
                        } else {
                            DEBUG('Z', "Releasing no waitQ\n");
                            temp_lock->busy = false;
                            temp_lock->machineID = -1;
                            if (temp_lock->toBeDeleted) {
                                locks.erase(locks.find(lockID));
                            }
                        }
                    }
                } else {
                    DEBUG('Z', "Couldn't find cv or lock\n");
                    postOffice->Send(outPktHdr, outMailHdr, "0");
                }
                break;
            }
            case SIGNAL_CV: {
                DEBUG('Z', "Signalling on cv on server starting\n");
                int cvID, lockID;
                ss >> cvID;
                ss >> lockID;
                bool isValid = false;
                outMailHdr.length = 2;
                if (cvs.find(cvID) != cvs.end() && locks.find(lockID) != locks.end()) {
                    ServerCV* temp_cv = &(cvs.find(cvID)->second);
                    ServerLock* temp_lock = &(locks.find(lockID)->second);
                    if ((temp_cv->lockID != -1 && temp_cv->lockID != lockID) 
                            || temp_lock->machineID != inPktHdr.from) {
                        DEBUG('Z', "Trying to signal cv on lock that doesn't belong to cv or machine. real: %d request: %d\n", temp_lock->machineID, inPktHdr.from);
                        postOffice->Send(outPktHdr, outMailHdr, "0");
                        break;
                    } else {
                        if (!temp_cv->waitQ.empty()) {
                            DEBUG('Z', "Releasing from waitQ and acquire lock\n");
                            Message m = temp_cv->waitQ.front();
                            temp_cv->waitQ.pop_front();
                            
                            if (temp_lock->busy) {
                                DEBUG('Z', "lock is busy\n");
                                temp_lock->addToWaitQ(m);
                            } else {
                                DEBUG('Z', "lock is not busy\n");
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
                    DEBUG('Z', "Couldn't find cv or lock\n");
                    postOffice->Send(outPktHdr, outMailHdr, "0");
                }
                break;
            }
            case BROADCAST_CV: {
                DEBUG('Z', "Broadcasting on cv on server starting\n");
                int cvID, lockID;
                ss >> cvID;
                ss >> lockID;
                bool isValid = false;
                outMailHdr.length = 2;
                if (cvs.find(cvID) != cvs.end() && locks.find(lockID) != locks.end()) {
                    ServerCV* temp_cv = &(cvs.find(cvID)->second);
                    ServerLock* temp_lock = &(locks.find(lockID)->second);
                    if ((temp_cv->lockID != -1 && temp_cv->lockID != lockID) 
                            || temp_lock->machineID != inPktHdr.from) {
                        DEBUG('Z', "Trying to signal cv on lock that doesn't belong to cv or machine. real: %d request: %d\n", temp_lock->machineID, inPktHdr.from);
                        postOffice->Send(outPktHdr, outMailHdr, "0");
                        break;
                    } else {
                        while(!temp_cv->waitQ.empty()) {
                            Message m = temp_cv->waitQ.front();
                            temp_cv->waitQ.pop_front();
                            
                            if (temp_lock->busy) {
                                DEBUG('Z', "lock is busy\n");
                                temp_lock->addToWaitQ(m);
                            } else {
                                DEBUG('Z', "lock is not busy\n");
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
                    DEBUG('Z', "Couldn't find cv or lock\n");
                    postOffice->Send(outPktHdr, outMailHdr, "0");
                }
                break;
            }
            case DESTROY_CV: {
                DEBUG('Z', "Destroying cv on server starting\n");
                int cvID;
                ss >> cvID;
                bool isValid = false;
                outMailHdr.length = 2;
                if (cvs.find(cvID) != cvs.end()) {
                    ServerCV* temp_cv = &(cvs.find(cvID)->second);
                    if (!temp_cv->waitQ.empty()) {
                        DEBUG('Z', "Stuff on waitQ so marking for deletion\n");
                        temp_cv->toBeDeleted = true;
                        postOffice->Send(outPktHdr, outMailHdr, "1");
                    } else {
                        DEBUG('Z', "Successfully deleted\n");
                        cvs.erase(cvs.find(cvID));
                        postOffice->Send(outPktHdr, outMailHdr, "1");
                    }
                } else {
                    DEBUG('Z', "Couldn't find lock\n");
                    postOffice->Send(outPktHdr, outMailHdr, "0");
                }
                break;
            }
        }
    }

    // Then we're done!
    interrupt->Halt();
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
