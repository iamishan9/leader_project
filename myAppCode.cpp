//
// Created by ishan, joshua, kwabena and setareh on 12/21/20.
//

#include "myAppCode.hpp"
#include <unistd.h>
#include <cstdlib>
#include "events/scheduler.h"
#include "events/events.h"

using namespace BlinkyBlocks;
int msg_count = 0;
int nb_l2 = 0;
#define LEADER_WAIT 1
#define COMPETE_WAIT 2
#define GET_RECRUITED_WAIT 3

bool leader_selection  = false;

MyAppCode::MyAppCode(BlinkyBlocksBlock *host) : BlinkyBlocksBlockCode(host) {

    if (not host) return;
    module = static_cast<BlinkyBlocksBlock *>(hostBlock);

    // Registers a callback to the message of given type
    addMessageEventFunc2(RECRUIT_MSG_ID,
                         std::bind(&MyAppCode::recruitMessage, this,
                                   std::placeholders::_1, std::placeholders::_2));

    addMessageEventFunc2(ACK_SUM_MSG_ID,
                         std::bind(&MyAppCode::acknowledgeSum, this,
                                   std::placeholders::_1, std::placeholders::_2));

    addMessageEventFunc2(ACK_SUM_LEAF_MSG_ID,
                         std::bind(&MyAppCode::acknowledgeSumLeaf, this,
                                   std::placeholders::_1, std::placeholders::_2));

    addMessageEventFunc2(LEAF_MSG_ID,
                         std::bind(&MyAppCode::updateTreeVal, this,
                                   std::placeholders::_1, std::placeholders::_2));
    addMessageEventFunc2(ACK_LEAF_MSG_ID,
                         std::bind(&MyAppCode::ackLeaf, this,
                                   std::placeholders::_1, std::placeholders::_2));

    addMessageEventFunc2(TO_COMPETE_MSG_ID,
                         std::bind(&MyAppCode::toCompete, this,
                                   std::placeholders::_1, std::placeholders::_2));

    addMessageEventFunc2(WIN_MSG_ID,
                         std::bind(&MyAppCode::acknowledgeWin, this,
                                   std::placeholders::_1, std::placeholders::_2));

    addMessageEventFunc2(LOSS_MSG_ID,
                         std::bind(&MyAppCode::acknowledgeLoss, this,
                                   std::placeholders::_1, std::placeholders::_2));

    addMessageEventFunc2(COMPETE_MSG_ID,
                         std::bind(&MyAppCode::compete, this,
                                   std::placeholders::_1, std::placeholders::_2));

    addMessageEventFunc2(ACK_COMPETE_MSG_ID,
                         std::bind(&MyAppCode::acknowledgeCompete, this,
                                   std::placeholders::_1, std::placeholders::_2));

    addMessageEventFunc2(DISMANTLE_MSG_ID,
                         std::bind(&MyAppCode::dismantle, this,
                                   std::placeholders::_1, std::placeholders::_2));

    addMessageEventFunc2(ACK_DISMANTLE_MSG_ID,
                         std::bind(&MyAppCode::ackDismantle, this,
                                   std::placeholders::_1, std::placeholders::_2));
}

/*
 * initial function which is called first when the program runs.
 */
void MyAppCode::startup() {
    cout << "start \n";

    // choice generation and interface setup
    for (int i = 0; i < module->getNbInterfaces(); i++) {
        if (module->getInterface(i)->isConnected()) {
            interfaceStatus[i] = 1;
            totalConnectedInt++;
            bchoice += pow(2, i);
        } else {
            interfaceStatus[i] = 0;
        }
    }
    cout << "current bchoice: " << bchoice << "\n";

    srand(module->blockId);

    // if the block has only 1 neighbor connected
    if (totalConnectedInt == 1) {
        isLeader = true;
        module->setColor(WHITE);
        myLeader = module->blockId;
        isRecruited = true;
        leader_selection = true;
    }
    // if the block has 2 neighbors connected
    else if (totalConnectedInt == 2 && !leader_selection) {
        currentRound = 0;
        getScheduler()->schedule(
                new InterruptionEvent(getScheduler()->now() + (10000), module, LEADER_WAIT));
    }
    // if the block has 3 neighbors connected
    else if (totalConnectedInt == 3 && !leader_selection) {
        currentRound = 0;
        getScheduler()->schedule(
                new InterruptionEvent(getScheduler()->now() + (20000), module, LEADER_WAIT));
    }
    // if block is leader, then start recruiting
    if (isLeader) {
        currentRound = 1;
        int t = sendMessageToAllNeighbors("recruit msg", new MessageOf<pair<int, int>>(RECRUIT_MSG_ID,
                                        make_pair( module->blockId, currentRound)),
                                      1000, 100, 0);
        nbWaitedAnswers += t;
        msg_count += nbWaitedAnswers;
    } else {
        currentRound = 0;
    }
}

/*
 * function where the leader broadcasts the initial recruit msg to its neighbors
 */
void MyAppCode::recruitMessage(MessagePtr _msg, P2PNetworkInterface *sender) {
    MessageOf<pair<int, int>> *msg = static_cast<MessageOf<pair<int, int>> *>(_msg.get());
    pair<int, int> msgData = *msg->getData();

    // condition to wait if the block is waiting for ack message
    if (msgData.second > currentRound && currentRound > 1 && nbWaitedAnswers > 0) {
        waitingFor = sender;
        getScheduler()->schedule(
                new InterruptionEvent(getScheduler()->now() + 1000, module, GET_RECRUITED_WAIT));
    }
    // if not, then continue the recruitment process
    else {
        if ((msgData.second > currentRound && !isLeader && !isRecruited) ||
            (msgData.second > currentRound && !isLeader && myLeader == msgData.first && currentRound > 1)) {
            if (currentRound > 1) {
                winReset();
            }
            myLeader = msgData.first;
            currentRound = msgData.second;
            parent = sender;
            usleep(10000);
            module->setColor(int((myLeader * 27) % 10 + 1));
            isRecruited = true;
            interfaceStatus[module->getInterfaceBId(sender)] = 3;

            for (int i = 0; i < module->getNbInterfaces(); i++) {
                if (interfaceStatus[i] != 0 && interfaceStatus[i] != 3) {
                    sendMessage("leader found msg",
                                new MessageOf<pair<int, int>>(RECRUIT_MSG_ID, make_pair(myLeader, currentRound)),
                                module->getInterface(i), 1000, 100);
                    nbWaitedAnswers++;
                    msg_count++;
                }
            }

            if (nbWaitedAnswers == 0) {
                total += bchoice;
                sendMessage("ack2parent", new MessageOf<pair<int, int>>(ACK_SUM_MSG_ID, make_pair(total, myLeader)),
                            parent, 1000, 100);
                msg_count++;
            }
        } else {
            sendMessage("ack2sender", new MessageOf<pair<int, int>>(ACK_SUM_MSG_ID, make_pair(-1, myLeader)), sender,
                        1000, 100);
            msg_count++;
        }
    }
}

/*
 * function that gets the sum from its tree
 */
void MyAppCode::acknowledgeSum(MessagePtr _msg, P2PNetworkInterface *sender) {
    nbWaitedAnswers--;

    MessageOf<pair<int, int>> *msg = static_cast<MessageOf<pair<int, int>> *>(_msg.get());
    pair<int, int> msgData = *msg->getData();

    if (msgData.first == -2) {
        sendMessage("recruit msg", new MessageOf<pair<int, int>>(RECRUIT_MSG_ID, make_pair(myLeader, currentRound)),
                    sender, 1000, 100);
        nbWaitedAnswers++;
        msg_count++;
        return;
    }
    if (msgData.first != -1) {
        total += msgData.first;
    }
    if (isLeader && msgData.first != -1) {
        interfaceStatus[module->getInterfaceBId(sender)] = 4;
    }

    if (msgData.first == -1 && msgData.second != myLeader) {
        isLeaf = true;
        competitor = true;
        interfaceStatus[module->getInterfaceBId(sender)] = 2;

    }
    if (nbWaitedAnswers == 0) {
        if (!isLeader) {
            total += bchoice;
            if (competitor) {
                sendMessage("ack2parent",
                            new MessageOf<pair<int, int>>(ACK_SUM_LEAF_MSG_ID, make_pair(total, myLeader)), parent,
                            1000, 100);
                msg_count++;
            } else {
                sendMessage("ack2parent", new MessageOf<pair<int, int>>(ACK_SUM_MSG_ID, make_pair(total, myLeader)),
                            parent, 1000, 100);
                msg_count++;
            }
        }
        // if the leader has competitors it continues to broadcast else program ends.
        else {
            total += bchoice;

            if (competitor) {
                currentRound++;

                for (int i = 0; i < module->getNbInterfaces(); i++) {
                    if (interfaceStatus[i] == 4) {
                        sendMessage("msg leaves",
                                    new MessageOf<pair<int, int>>(LEAF_MSG_ID, make_pair(total, currentRound)),
                                    module->getInterface(i), 1000, 100);
                        nbWaitedAnswers++;
                        msg_count++;
                    }
                }
                //if a leader doesn't have members it should dismantle
                if (nbWaitedAnswers == 0) {
                    loseReset();
                }
            } else {
                // do sth
            }
            return;
        }
    }
}

/*
 * function to acknowledge sum to leaves
 */
void MyAppCode::acknowledgeSumLeaf(MessagePtr _msg, P2PNetworkInterface *sender) {
    nbWaitedAnswers--;
    competitor = true;

    MessageOf<pair<int, int>> *msg = static_cast<MessageOf<pair<int, int>> *>(_msg.get());
    pair<int, int> msgData = *msg->getData();


    if (msgData.first != -1) {
        total += msgData.first;
    }

    if (msgData.first == -1 && msgData.second != myLeader) {
        isLeaf = true;
        interfaceStatus[module->getInterfaceBId(sender)] = 2;
    }
    if (isLeader && msgData.first != -1) {
        interfaceStatus[module->getInterfaceBId(sender)] = 4;
    }

    if (nbWaitedAnswers == 0) {
        if (!isLeader) {
            total += bchoice;
            sendMessage("ack2parent", new MessageOf<pair<int, int>>(ACK_SUM_LEAF_MSG_ID, make_pair(total, myLeader)),
                        parent, 1000, 100);
            msg_count++;
        }
        // if the leader has competitors it continues else ends
        else {
            total += bchoice;

            if (competitor) {
                currentRound++;
                for (int i = 0; i < module->getNbInterfaces(); i++) {
                    if (interfaceStatus[i] == 4) {
                        sendMessage("msg leaves",
                                    new MessageOf<pair<int, int>>(LEAF_MSG_ID, make_pair(total, currentRound)),
                                    module->getInterface(i), 1000, 100);
                        nbWaitedAnswers++;
                        msg_count++;
                    }
                }
                //if a leader doesn't have members it should dismantle
                if (nbWaitedAnswers == 0) {
                    loseReset();
                }
            } else {
                // do sth
            }
            return;
        }
    }
}

/*
 * This function will msg the leaves with the total and in ack will msg leader
 * about the nb of nodes in the tree
 */
void MyAppCode::updateTreeVal(MessagePtr _msg, P2PNetworkInterface *sender) {
    MessageOf<pair<int, int>> *msg = static_cast<MessageOf<pair<int, int>> *>(_msg.get());
    pair<int, int> msgData = *msg->getData();

    if (parent == nullptr || (msgData.second > currentRound && sender == parent)) {
        if (isLeaf) {
            total = msgData.first;

        }
        currentRound = msgData.second;
        for (int i = 0; i < module->getNbInterfaces(); i++) {
            if (interfaceStatus[i] != 0 && interfaceStatus[i] != 3 && interfaceStatus[i] != 2) {

                sendMessage("Inform total",
                            new MessageOf<pair<int, int>>(LEAF_MSG_ID, make_pair(msgData.first, currentRound)),
                            module->getInterface(i), 1000, 100);
                nbWaitedAnswers++;
                msg_count++;
            }
        }

        if (nbWaitedAnswers == 0) {
            nbNode = 1;
            nbLeaf = 0;
            if (isLeaf) {
                nbLeaf++;
            }

            sendMessage("ack2parent", new MessageOf<pair<int, int>>(ACK_LEAF_MSG_ID, make_pair(nbNode, nbLeaf)), parent,
                        1000, 100);
            msg_count++;
        }
    } else {
        sendMessage("ack2sender", new MessageOf<pair<int, int>>(ACK_LEAF_MSG_ID, make_pair(0, 0)), sender, 1000, 100);
        msg_count++;
    }
}

/*
 * this function will send ack msg to leaves to compete
 */
void MyAppCode::ackLeaf(MessagePtr _msg, P2PNetworkInterface *sender) {

    nbWaitedAnswers--;

    MessageOf<pair<int, int>> *msg = static_cast<MessageOf<pair<int, int>> *>(_msg.get());
    pair<int, int> msgData = *msg->getData();

    nbNode += msgData.first;
    nbLeaf += msgData.second;

    if (nbWaitedAnswers == 0) {
        nbNode++;
        if (!isLeader) {
            if (isLeaf) {
                nbLeaf++;
            }

            sendMessage("ack2parent", new MessageOf<pair<int, int>>(ACK_LEAF_MSG_ID, make_pair(nbNode, nbLeaf)), parent,
                        1000, 100);
            msg_count++;
        } else {

            currentRound++;

            for (int i = 0; i < module->getNbInterfaces(); i++) {
                if (interfaceStatus[i] == 4) {
                    sendMessage("ask leaves to compete",
                                new MessageOf<pair<int, int>>(TO_COMPETE_MSG_ID, make_pair(nbNode, currentRound)),
                                module->getInterface(i), 1000, 100);
                    nbWaitedAnswers++;
                    msg_count++;
                }
            }

            return;
        }
    }
}

/*
 * leader asks the leaves to compete and they reply back with whether they won or not.
 * leader will wait for all responses.
 * It will start recruiting again if the leaves reply with win msg else dismantles.
 */
void MyAppCode::toCompete(MessagePtr _msg, P2PNetworkInterface *sender) {
    MessageOf<pair<int, int>> *msg = static_cast<MessageOf<pair<int, int>> *>(_msg.get());
    pair<int, int> msgData = *msg->getData();

    if (parent == nullptr || (msgData.second > currentRound && sender == parent)) {
        if (isLeaf) {
            nbNode = msgData.first;
            readyToCompete = true;
        }
        currentRound = msgData.second;


        for (int i = 0; i < module->getNbInterfaces(); i++) {
            if (interfaceStatus[i] != 0 && interfaceStatus[i] != 3 && interfaceStatus[i] != 2) {
                sendMessage("Inform Leafs of number of nodes",
                            new MessageOf<pair<int, int>>(TO_COMPETE_MSG_ID, make_pair(msgData.first, currentRound)),
                            module->getInterface(i), 1000, 100);
                nbWaitedAnswers++;
                msg_count++;
            }
            if (isLeaf && interfaceStatus[i] == 2) {
                sendMessage("Let's Compete", new MessageOf<pair<int, int>>(COMPETE_MSG_ID, make_pair(total, nbNode)),
                            module->getInterface(i), 1000, 100);
                nbWaitedAnswers++;
                msg_count++;
            }
        }


        if (nbWaitedAnswers == 0) {
            nbNode = 1;
            nbLeaf = 0;
            if (isLeaf) {

                nbLeaf++;
            }

            sendMessage("ack2parent", new MessageOf<int>(WIN_MSG_ID, 0), parent, 1000, 100);
            msg_count++;
        }
    } else {
        sendMessage("ack2sender", new MessageOf<int>(WIN_MSG_ID, 0), sender, 1000, 100);
        msg_count++;
    }
}


/*
 * the leaves of different trees compete with each other
 */
void MyAppCode::compete(MessagePtr _msg, P2PNetworkInterface *sender) {

    MessageOf<pair<int, int>> *msg = static_cast<MessageOf<pair<int, int>> *>(_msg.get());
    pair<int, int> msgData = *msg->getData();


    if (msgData.first > total) {
        sendMessage("ack2sender", new MessageOf<int>(ACK_COMPETE_MSG_ID, 1), sender, 1000, 100);
        msg_count++;

    } else if (msgData.first < total) {
        sendMessage("ack2sender", new MessageOf<int>(ACK_COMPETE_MSG_ID, 0), sender, 1000, 100);
        msg_count++;
    } else if (msgData.first == total && readyToCompete) {
        if (msgData.second >= nbNode) {
            sendMessage("ack2sender", new MessageOf<int>(ACK_COMPETE_MSG_ID, 1), sender, 1000, 100);
            msg_count++;

        } else {
            sendMessage("ack2sender", new MessageOf<int>(ACK_COMPETE_MSG_ID, 0), sender, 1000, 100);
            msg_count++;

        }
    } else if (msgData.first == total && !readyToCompete) {
        sendMessage("ack2sender", new MessageOf<int>(ACK_COMPETE_MSG_ID, -1), sender, 1000, 100);
        msg_count++;
    }

}

/*
 * the leaves of different trees reply to their parent with the result of the competition
 */
void MyAppCode::acknowledgeCompete(MessagePtr _msg, P2PNetworkInterface *sender) {
    nbWaitedAnswers--;

    MessageOf<int> *msg = static_cast<MessageOf<int> *>(_msg.get());
    int msgData = *msg->getData();
    if (msgData == 1) {

        if (nbWaitedAnswers == 0) {
            if (lossNews) {
                sendMessage("ack2parent", new Message(LOSS_MSG_ID), parent, 1000, 100);
                msg_count++;
            } else {
                sendMessage("ack2parent", new MessageOf<int>(WIN_MSG_ID, 1), parent, 1000, 100);
                msg_count++;
            }
        }
    } else if (msgData == 0) {

        lossNews = true;
        if (nbWaitedAnswers == 0) {
            sendMessage("ack2parent", new Message(LOSS_MSG_ID), parent, 1000, 100);
            msg_count++;
        }
    } else {
        waitingFor = sender;
        getScheduler()->schedule(
                new InterruptionEvent(getScheduler()->now() + 100, module, COMPETE_WAIT));
    }
}

/*
 * function called when a tree wins
 */
void MyAppCode::acknowledgeWin(MessagePtr _msg, P2PNetworkInterface *sender) {

    nbWaitedAnswers--;
    MessageOf<int> *msg = static_cast<MessageOf<int> *>(_msg.get());
    int msgData = *msg->getData();

    winResponse += msgData;
    if (nbWaitedAnswers == 0) {
        if (!isLeader) {
            if (!lossNews) {
                sendMessage("ack2parent", new MessageOf<int>(WIN_MSG_ID, winResponse), parent, 1000, 100);
                msg_count++;
            } else {
                sendMessage("ack2parent", new Message(LOSS_MSG_ID), parent, 1000, 100);
                msg_count++;
            }

        } else {

            if (!lossNews) {

                currentRound += 2;
                nbLeaf = 0;
                nbNode = 0;
                nbLeaf = 0;
                total = 0;
                competitor = false;
                readyToCompete = false;
                winResponse = 0;
                lossNews = false;
                for (int i = 0; i < module->getNbInterfaces(); i++) {
                    if (interfaceStatus[i] == 4) {
                        sendMessage("recruiting",
                                    new MessageOf<pair<int, int>>(RECRUIT_MSG_ID, make_pair(myLeader, currentRound)),
                                    module->getInterface(i), 1000, 100);
                        nbWaitedAnswers++;
                        msg_count++;
                    }
                }

                return;

            } else {

                currentRound++;

                for (int i = 0; i < module->getNbInterfaces(); i++) {
                    if (interfaceStatus[i] == 4) {
                        sendMessage("dismantle msg",
                                    new MessageOf<int>(DISMANTLE_MSG_ID, currentRound), module->getInterface(i), 1000,
                                    100);
                        nbWaitedAnswers++;
                        msg_count++;
                    }
                }
                loseReset();
                return;
            }
        }
    }
}


/*
 * function called when a tree loses
 */
void MyAppCode::acknowledgeLoss(MessagePtr _msg, P2PNetworkInterface *sender) {

    nbWaitedAnswers--;
    competitor = false;
    lossNews = true;
    if (nbWaitedAnswers == 0) {
        if (!isLeader) {
            sendMessage("ack2parent", new Message(LOSS_MSG_ID), parent, 1000, 100);
            msg_count++;
        } else {

            currentRound++;
            for (int i = 0; i < module->getNbInterfaces(); i++) {
                if (interfaceStatus[i] == 4) {
                    sendMessage("dismantle msg", new MessageOf<int>(DISMANTLE_MSG_ID, currentRound),
                                module->getInterface(i), 1000, 100);
                    nbWaitedAnswers++;
                    msg_count++;
                }
            }
            return;
        }
    }
}


/*
 * function called when a tree needs to dismantle its nodes
 */
void MyAppCode::dismantle(MessagePtr _msg, P2PNetworkInterface *sender) {
    MessageOf<int> *msg = static_cast<MessageOf<int> *>(_msg.get());
    int msgData = *msg->getData();

    if (parent == nullptr || (msgData > currentRound && sender == parent)) {
        currentRound = msgData;
        for (int i = 0; i < module->getNbInterfaces(); i++) {
            if (interfaceStatus[i] != 0 && interfaceStatus[i] != 3 && interfaceStatus[i] != 2) {
                sendMessage("dismantle msg", new MessageOf<int>(DISMANTLE_MSG_ID, currentRound),
                            module->getInterface(i), 1000, 100);
                nbWaitedAnswers++;
                msg_count++;
            }
        }

        if (nbWaitedAnswers == 0) {
            sendMessage("ack2parent", new Message(ACK_DISMANTLE_MSG_ID), parent, 1000, 100);
            msg_count++;
            loseReset();

        }
    } else {
        sendMessage("ack2sender", new Message(ACK_DISMANTLE_MSG_ID), sender, 1000, 100);
        msg_count++;
    }
}

/*
 * function which acknowledges the dismantle msg and reduces the number of waited answers
 */
void MyAppCode::ackDismantle(MessagePtr _msg, P2PNetworkInterface *sender) {
    nbWaitedAnswers--;
    if (nbWaitedAnswers == 0) {


        if (parent != nullptr) {
            sendMessage("ack2parent", new Message(ACK_DISMANTLE_MSG_ID), parent, 1000, 100);
            msg_count++;
        }
        loseReset();
    }
}

/*
 * function called to clear data of winning tree
 */
void MyAppCode::winReset() {

    total = 0;
    nbNode = 0;
    nbLeaf = 0;
    winResponse = 0;
    //nbWaitedAnswers = 0;

    isLeaf = false;
    isLeader = false;
    lossNews = false;
    competitor = false;
    readyToCompete = false;

    for (int i = 0; i < module->getNbInterfaces(); i++) {
        if (module->getInterface(i)->isConnected()) {
            interfaceStatus[i] = 1;
        }
    }
}

/*
 * function called to clear the data of losing tree.
 */
void MyAppCode::loseReset() {
    isLeaf = false;
    isLeader = false;
    isRecruited = false;

    nbNode = 0;
    nbLeaf = 0;
    total = 0;
    competitor = false;
    readyToCompete = false;
    winResponse = 0;
    lossNews = false;
    setColor(0);

    for (int i = 0; i < module->getNbInterfaces(); i++) {
        if (module->getInterface(i)->isConnected()) {
            interfaceStatus[i] = 1;
        }
    }
}

//others
void MyAppCode::onMotionEnd() {
    cout << " on motion end" << "\n";

}

// Exception handling function
void MyAppCode::processLocalEvent(EventPtr pev) {
    MessagePtr message;
    stringstream info;

    // Do not remove line below
    BlinkyBlocksBlockCode::processLocalEvent(pev);

    switch (pev->eventType) {
        case EVENT_ADD_NEIGHBOR: {
            break;
        }

        case EVENT_REMOVE_NEIGHBOR: {
            break;
        }

        case EVENT_INTERRUPTION: {
            std::shared_ptr<InterruptionEvent> itev =
                    std::static_pointer_cast<InterruptionEvent>(pev);

            if (itev->mode == 1) {
                if (!isRecruited && currentRound < 1) {
                    nb_l2++;
                    isLeader = true;
                    module->setColor(WHITE);
                    myLeader = module->blockId;
                    isRecruited = true;
                }

                if (isLeader) {
                    currentRound = 1;
                    cout << module->blockId << " recruiting" << "\n";


                    int t = sendMessageToAllNeighbors("recruit msg",
                                                      new MessageOf<pair<int, int>>(RECRUIT_MSG_ID,
                                                                                    make_pair(module->blockId,
                                                                                              currentRound)), 1000, 100,
                                                      0);
                    nbWaitedAnswers += t;
                    msg_count += nbWaitedAnswers;
                }
            } else if (itev->mode == 2) {
                sendMessage("compete msg", new MessageOf<pair<int, int>>(COMPETE_MSG_ID, make_pair(total, nbNode)),
                            waitingFor, 1000, 100);
                nbWaitedAnswers++;
                msg_count++;

            } else if (itev->mode == 3) {

                sendMessage("Try to recruit me again",
                            new MessageOf<pair<int, int>>(ACK_SUM_MSG_ID, make_pair(-2, myLeader)), waitingFor, 1000,
                            100);
                msg_count++;
            }
            break;

        }
    }
}

// Number of messages output
string MyAppCode::onInterfaceDraw() {
    return "number of msg exchanged in the network: " + to_string(msg_count);
}

/// ADVANCED BLOCKCODE FUNCTIONS BELOW

void MyAppCode::onBlockSelected() {

    cerr << endl << "--- PRINT MODULE " << *module << "---" << endl;
}

void MyAppCode::onAssertTriggered() {
    cout << " has triggered an assert" << "\n";

}

bool MyAppCode::parseUserCommandLineArgument(int &argc, char **argv[]) {
    /* Reading the command line */
    if ((argc > 0) && ((*argv)[0][0] == '-')) {
        switch ((*argv)[0][1]) {

            // Single character example: -b
            case 'b': {
                cout << "-b option provided" << "\n";
                return true;
            }
                break;

                // Composite argument example: --foo 13
            case '-': {
                string varg = string((*argv)[0] + 2); // argv[0] without "--"
                if (varg == string("foo")) { //
                    int fooArg;
                    try {
                        fooArg = stoi((*argv)[1]);
                        argc--;
                        (*argv)++;
                    } catch (std::logic_error &) {
                        stringstream err;
                        err << "foo must be an integer. Found foo = " << argv[1] << endl;
                        throw CLIParsingError(err.str());
                    }

                    cout << "--foo option provided with value: " << fooArg << "\n";
                } else return false;

                return true;
            }

            default:
                cerr << "Unrecognized command line argument: " << (*argv)[0] << endl;
        }
    }

    return false;
}