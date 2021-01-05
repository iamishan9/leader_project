//
// Created by ishan, joshua, kwabena and setareh on 12/21/20.
//

#ifndef myAppCode_H_
#define myAppCode_H_

#include "robots/blinkyBlocks/blinkyBlocksWorld.h"
#include "robots/blinkyBlocks/blinkyBlocksBlockCode.h"

static const int RECRUIT_MSG_ID = 1001;
static const int ACK_MSG_ID = 1002;
static const int ACK_SUM_MSG_ID = 1004;
static const int ACK_SUM_LEAF_MSG_ID = 1005;

static const int LEAF_MSG_ID = 1006;
static const int ACK_LEAF_MSG_ID = 1007;

static const int TO_COMPETE_MSG_ID = 1008;
static const int WIN_MSG_ID = 1009;
static const int LOSS_MSG_ID = 1010;
static const int COMPETE_MSG_ID = 1011;
static const int ACK_COMPETE_MSG_ID = 1012;

static const int DISMANTLE_MSG_ID = 1013;
static const int ACK_DISMANTLE_MSG_ID = 1014;


using namespace BlinkyBlocks;

class MyAppCode : public BlinkyBlocksBlockCode {
private:
    int distance;
    int myLeader;
    int total = 0;
    int nbNode = 0;
    int nbLeaf = 0;
    int bchoice = 0;
    int winResponse = 0;
    int currentRound = 0;
    int interfaceStatus[6];
    int nbWaitedAnswers = 0;
    int totalConnectedInt = 0;

    bool isLeaf = false;
    bool isLeader = false;
    bool isRecruited = false;
    bool lossNews = false;
    bool competitor = false;
    bool readyToCompete = false;

    BlinkyBlocksBlock *module;

    P2PNetworkInterface *parent= nullptr;
    P2PNetworkInterface *waitingFor = nullptr;
public :
    MyAppCode(BlinkyBlocksBlock *host);
    ~MyAppCode() {};
    inline static size_t nMotions = 0;

    /**
     * This function is called on startup of the blockCode, it can be used to perform initial
     *  configuration of the host or this instance of the program.
     * @note this can be thought of as the main function of the module
     **/
    void startup() override;

    /**
     * @brief Handler for all events received by the host block
     * @param pev pointer to the received event
     */
    void processLocalEvent(EventPtr pev) override;

    /**
     * @brief Callback function executed whenever the module finishes a motion
     */
    void onMotionEnd() override;

    /**
     * @brief Sample message handler for this instance of the blockcode
     * @param _msg Pointer to the message received by the module, requires casting
     * @param sender Connector of the module that has received the message and that is connected to the sender */
    void handleSampleMessage(std::shared_ptr<Message> _msg, P2PNetworkInterface *sender);

    /// Advanced blockcode handlers below

    /**
     * @brief Provides the user with a pointer to the configuration file parser, which can be used to read additional user information from it.
     * @param config : pointer to the TiXmlDocument representing the configuration file, all information related to VisibleSim's core have already been parsed
     *
     * Called from BuildingBlock constructor, only once.
     */
    void parseUserElements(TiXmlDocument *config) override {}

    /**
     * @brief Provides the user with a pointer to the configuration file parser, which can be used to read additional user information from each block config. Has to be overriden in the child class.
     * @param config : pointer to the TiXmlElement representing the block configuration file, all information related to concerned block have already been parsed
     *
     */
    void parseUserBlockElements(TiXmlElement *config) override {}

    /**
     * User-implemented debug function that gets called when a module is selected in the GUI
     */
    void onBlockSelected() override;

    /**
     * User-implemented debug function that gets called when a VS_ASSERT is triggered
     * @note call is made from utils::assert_handler()
     */
    void onAssertTriggered() override;

    /**
     * User-implemented keyboard handler function that gets called when
     *  a key press event could not be caught by openglViewer
     * @param c key that was pressed (see openglViewer.cpp)
     * @param x location of the pointer on the x axis
     * @param y location of the pointer on the y axis
     * @note call is made from GlutContext::keyboardFunc (openglViewer.h)
     */
    void onUserKeyPressed(unsigned char c, int x, int y) override {}

    /**
     * Call by world during GL drawing phase, can be used by a user
     *  to draw custom Gl content into the simulated world
     * @note call is made from World::GlDraw
     */
    void onGlDraw() override {}

    /**
     * @brief This function is called when a module is tapped by the user. Prints a message to the console by default.
     Can be overloaded in the user blockCode
     * @param face face that has been tapped */
    void onTap(int face) override {}


    /**
     * User-implemented keyboard handler function that gets called when
     *  a key press event could not be caught by openglViewer
     * @note call is made from GlutContext::keyboardFunc (openglViewer.h)
     */
    bool parseUserCommandLineArgument(int& argc, char **argv[]) override;


    /*
     * function that sends the msg to recruit nodes
     */
    void recruitMessage(MessagePtr msg,P2PNetworkInterface *sender);

    /*
     * function that gets the sum from its tree
     */
    void acknowledgeSum(MessagePtr msg,P2PNetworkInterface *sender);

    /*
     * function to acknowledge sum to leaves
     */
    void acknowledgeSumLeaf(MessagePtr msg,P2PNetworkInterface *sender);

    /*
     * This function will msg the leaves with the total and in ack will msg leader
     * about the nb of nodes in the tree
     */
    void updateTreeVal(MessagePtr msg,P2PNetworkInterface *sender);

    /*
     * this function will send ack msg to leaves to compete
     */
    void ackLeaf(MessagePtr msg,P2PNetworkInterface *sender);


    /*
     * leader asks the leaves to compete and they reply back with whether they won or not.
     * leader will wait for all responses.
     * It will start recruiting again if the leaves reply with win msg else dismantles.
     */
    void toCompete(MessagePtr msg,P2PNetworkInterface *sender);

    /*
     * function which is called when a leader wins
     */
    void acknowledgeWin(MessagePtr msg,P2PNetworkInterface *sender);

    /*
     * function which is called when a leader loses
     */
    void acknowledgeLoss(MessagePtr msg,P2PNetworkInterface *sender);

    /*
     * the leaves of different trees compete with each other
     */
    void compete(MessagePtr msg,P2PNetworkInterface *sender);

    /*
     * the leaves of different trees reply to their parent with the result of the competition
     */
    void acknowledgeCompete(MessagePtr msg,P2PNetworkInterface *sender);


    /*
     * function to send dismantle msg
     */
    void dismantle(MessagePtr msg,P2PNetworkInterface *sender);

    /*
     * function to send ack of dismantle msg
     */
    void ackDismantle(MessagePtr msg,P2PNetworkInterface *sender);

    /*
     * clear data of winning tree
     */
    void winReset();

    /*
     * clear data of losing tree
     */
    void loseReset();

    /**
     * Called by openglviewer during interface drawing phase, can be used by a user
     *  to draw a custom Gl string onto the bottom-left corner of the GUI
     * @note call is made from OpenGlViewer::drawFunc
     * @return a string (can be multi-line with `\n`) to display on the GUI
     */
    string onInterfaceDraw() override;

/*****************************************************************************/
/** needed to associate code to module                                      **/
    static BlockCode *buildNewBlockCode(BuildingBlock *host) {
        return (new MyAppCode((BlinkyBlocksBlock*)host));
    };
/*****************************************************************************/
};

#endif /* myAppCode_H_ */
