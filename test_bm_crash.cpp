#include <iostream>
#include <thread>
#include <chrono>
#include "src/core/bm.hpp"

int main() {
    std::cout << "Starting BM test..." << std::endl;
    bool res = BM::getInstance().start("lo", 5002);
    std::cout << "BM Started: " << res << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << "Stopping BM..." << std::endl;
    BM::getInstance().stop();
    std::cout << "BM Stopped." << std::endl;
    std::cout << "Starting BM again..." << std::endl;
    res = BM::getInstance().start("lo", 5002);
    std::cout << "BM Started: " << res << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << "Stopping BM..." << std::endl;
    BM::getInstance().stop();
    std::cout << "BM Test Complete." << std::endl;
    return 0;
}
