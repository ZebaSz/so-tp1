#include <iostream>
#include <fstream>
#include <thread>
#include "ConcurrentHashMap.hpp"

using namespace std;

void test_always_member(ConcurrentHashMap& map, const std::string& key, std::atomic_bool& ok) {
    map.addAndInc(key);
    for (int i = 0; i < 10; ++i) {
        if (!map.member(key)) {
            std::cerr << "Wait, where did it go? Key: " << key << std::endl;
            ok.store(false);
            break;
        }
    }
}


int main(void) {
    pair<string, unsigned int> p;
    ConcurrentHashMap map;
    std::atomic_bool ok(true);

    std::ifstream file("corpus");
    std::list<std::string> lines;
    std::string line;
    while (std::getline(file, line)) {
        lines.push_back(line);
    }

    std::list<std::thread> threads;

    for (auto it = lines.begin(); it != lines.end(); ++it) {
        threads.push_back(std::thread(test_always_member, std::ref(map),
                                      std::cref(*it), std::ref(ok)));
    }
    for (auto it = threads.begin(); it != threads.end(); ++it) {
        it->join();
    }
    if(ok.load()) {
        for (auto it = lines.begin(); it != lines.end(); ++it) {
            if (!map.member(*it)) {
                std::cerr << "Oh shit, we lost something! Key: " << *it << std::endl;
                ok.store(false);
                break;
            }
        }
    }
    if(ok.load()) {
        std::cout << "Everything looks a-ok from here!" << std::endl;

        std::cout << "Used " << threads.size() << " threads" << std::endl;
        return 0;
    }
    return 1;
}

