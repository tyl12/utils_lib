// main.cpp

#include "cpu.h"

int main() {
    CPU* cpu = new CPU("Intel CPU");
    VideoCard* videocard = new VideoCard("XXX video card");
    MainBoard* mainboard = new MainBoard("HUAWEI mainboard");
    Computer* myComputer = new Computer(cpu, videocard, mainboard);

    CircuitDetector* Dan = new CircuitDetector("CircuitDetector Dan");
    FunctionDetector* Tom = new FunctionDetector("FunctionDetector Tom");

    std::cout << "\nStep 1: Dan is checking computer's circuits." << std::endl;
    myComputer->Accept(Dan);
    std::cout << "\nStep 2: Tom is checking computer's functions." << std::endl;
    myComputer->Accept(Tom);

    return 0;
}
