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
    int lockID;
    bool toBeDeleted;
    std::deque<Message> waitQ;
    ServerLock(int lock_num) {
        machineID = -1;
        lockID = lock_num;
        busy = false;
        toBeDeleted = false;
    }
    void addToWaitQ(Message m) {
        waitQ.push_back(m);
    }
};

struct ServerCV {
    int lock;
    std::deque<Message> waitQ;
    ServerCV(int lockID) {
        lock = lockID;
    }
};


// Server for Project 3 Part 3
void Server() {
    printf("In server function\n");
    int currentLock = 0;
    int currentCV = 0;
    std::map<std::string, ServerLock> locks;
    std::vector<ServerCV> cvs;

    for (;;) {
        // Receive a message
        PacketHeader outPktHdr, inPktHdr;
        MailHeader outMailHdr, inMailHdr;
        char buffer[MaxMailSize];
        postOffice->Receive(SERVER_MAILBOX, &inPktHdr, &inMailHdr, buffer);

        outPktHdr.to = inPktHdr.from;
        outMailHdr.to = inMailHdr.from;
        outMailHdr.from = SERVER_MAILBOX;

        printf("Received packet\n");

        // Parse the message
        int s;
        stringstream ss(buffer);
        std::cout << buffer << std::endl;
        ss >> s;

        // Process the message and Send a reply… maybe
        switch (s) {
            // Returns the lock ID for the given
            case CREATE_LOCK: {
                printf("Create lock on server starting\n");
                std::string lock_name = ss.str();
                int lockID;
                if (locks.find(lock_name) != locks.end()) {
                    lockID = locks.find(lock_name)->second.lockID;
                } else {
                    lockID = currentLock;
                    ServerLock lock(currentLock++);
                    locks.insert(std::pair<std::string, ServerLock>(lock_name, lock));
                }
                ss.str("");
                ss.clear();
                ss << (lockID);
                outMailHdr.length = ss.str().length() + 1;
                char *cstr = new char[ss.str().length() + 1];
                strcpy(cstr, ss.str().c_str());
                printf("Create lock on server sending: %s\n", cstr);
                postOffice->Send(outPktHdr, outMailHdr, cstr);
                break;
            }
            // Returns 0 if lock doesn't exist, 1 if it does and is acquired.
            case ACQUIRE_LOCK: {
                printf("Acquiring lock on server starting\n");
                int lockID;
                ss >> lockID;
                bool isValid = false;
                outMailHdr.length = 2;
                for (std::map<std::string, ServerLock>::iterator it = locks.begin();
                        it != locks.end(); ++it) {
                    if (it->second.lockID == lockID) {
                        ServerLock *temp_lock = &(it->second);
                        isValid = true;
                        if (temp_lock->busy) {
                            printf("Found lock and busy\n");
                            Message m(outPktHdr, outMailHdr, "1");
                            temp_lock->addToWaitQ(m);
                        } else {
                            printf("Found lock and not busy\n");
                            temp_lock->busy = true;
                            temp_lock->machineID = inPktHdr.from;
                            postOffice->Send(outPktHdr, outMailHdr, "1");
                        }
                        break;
                    }
                }
                if (!isValid) {
                    printf("Couldn't find lock\n");
                    postOffice->Send(outPktHdr, outMailHdr, "0");
                }
                break;
            }
            // Returns 0 if lock doesn't exist, 1 if it does and is acquired.
            case RELEASE_LOCK: {
                printf("Releasing lock on server starting\n");
                int lockID;
                ss >> lockID;
                bool isValid = false;
                outMailHdr.length = 2;
                for (std::map<std::string, ServerLock>::iterator it = locks.begin();
                        it != locks.end(); ++it) {
                    if (it->second.lockID == lockID) {
                        ServerLock* temp_lock = &(it->second);
                        if (temp_lock->machineID != inPktHdr.from) {
                            printf("Trying to release lock it doesn't have. real: %d request: %d\n", temp_lock->machineID, inPktHdr.from);
                            break;
                        }
                        isValid = true;
                        if (!temp_lock->waitQ.empty()) {
                            printf("Releasing from waitQ\n");
                            Message m = temp_lock->waitQ.front();
                            temp_lock->waitQ.pop_front();
                            temp_lock->machineID = m.packetHdr.to;
                            postOffice->Send(outPktHdr, outMailHdr, "1");
                            postOffice->Send(m.packetHdr, m.mailHdr, m.data);
                        } else {
                            printf("Releasing no waitQ\n");
                            temp_lock->busy = false;
                            temp_lock->machineID = -1;
                            if (temp_lock->toBeDeleted) {
                                locks.erase(it);
                            }
                            postOffice->Send(outPktHdr, outMailHdr, "1");
                        }
                        break;
                    }
                }
                if (!isValid) {
                    printf("Couldn't find lock\n");
                    postOffice->Send(outPktHdr, outMailHdr, "0");
                }
                break;
            }
            // Returns 0 if lock doesn't exist, 1 if it does and is acquired.
            case DESTROY_LOCK: {
                printf("Destroying lock on server starting\n");
                int lockID;
                ss >> lockID;
                bool isValid = false;
                outMailHdr.length = 2;
                for (std::map<std::string, ServerLock>::iterator it = locks.begin();
                        it != locks.end(); ++it) {
                    if (it->second.lockID == lockID) {
                        ServerLock* temp_lock = &(it->second);
                        isValid = true;
                        if (temp_lock->busy) {
                            printf("In use so, marking for deletion\n");
                            temp_lock->toBeDeleted = true;
                            postOffice->Send(outPktHdr, outMailHdr, "1");
                        } else {
                            printf("Successfully deleted\n");
                            locks.erase(it);
                            postOffice->Send(outPktHdr, outMailHdr, "1");
                        }
                        break;
                    }
                }
                if (!isValid) {
                    printf("Couldn't find lock\n");
                    postOffice->Send(outPktHdr, outMailHdr, "0");
                }
                break;
            }

            // case 3:
            // case CREATE_CV:
            //     ServerCV cv(mailHdr.from);
            //     cvs.insert(std::pair<int, ServerCV>(currentCV++, cv));

            //     ss.clear();
            //     ss << (currentCV -1);
            //     outMailHdr.length = ss.str().length() + 1;
            //     postOffice->Send(outPktHdr, outMailHdr, ss.str().c_str());
            //     break;
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
