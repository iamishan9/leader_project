#include <iostream>
#include "myAppCode.hpp"
#include "robots/blinkyBlocks/blinkyBlocksSimulator.h"

using namespace std;
using namespace BlinkyBlocks;


int main(int argc, char **argv) {
    try {
        createSimulator(argc, argv, MyAppCode::buildNewBlockCode);
        getSimulator()->printInfo();
        BaseSimulator::getWorld()->printInfo();
        deleteSimulator();
    } catch(std::exception const& e) {
        cerr << "Uncaught exception: " << e.what();
    }

    return 0;
}