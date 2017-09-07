#include <iostream>
#include <fstream>
#include <thread>
#include "ConcurrentHashMap.hpp"

using namespace std;

struct test_s {
    test_s(ConcurrentHashMap &map, const string &key, atomic_bool &ok) : map(map), key(key), ok(ok) {}
    ConcurrentHashMap& map;
    const std::string& key;
    std::atomic_bool& ok;
};

void* test_always_member(void* arg) {
    test_s* params = (test_s*)arg;
    params->map.addAndInc(params->key);
    for (int i = 0; i < 10; ++i) {
        if (!params->map.member(params->key)) {
            std::cerr << "Wait, where did it go? Key: " << params->key << std::endl;
            params->ok.store(false);
            break;
        }
    }
    pthread_exit(NULL);
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

    std::list<pthread_t> threads;

    std::vector<test_s*> params;

    for (auto it = lines.begin(); it != lines.end(); ++it) {
        test_s* param = new test_s(map, *it, ok);
        params.push_back(param);
        pthread_t t;
        pthread_create(&t, NULL, test_always_member, param);
        threads.push_back(t);
    }
    for (auto it = threads.begin(); it != threads.end(); ++it) {
        pthread_join(*it, NULL);
    }
    for (auto it = params.begin(); it != params.end(); ++it) {
        delete *it;
    }
    params.clear();
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

