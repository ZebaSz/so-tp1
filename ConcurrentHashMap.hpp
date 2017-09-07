#ifndef SO_TP1_CONCURRENTHASHMAP_H
#define SO_TP1_CONCURRENTHASHMAP_H

#include <memory>
#include <list>
#include <string>
#include <vector>
#include <mutex>
#include <condition_variable>
#include "ListaAtomica.hpp"

typedef unsigned int uint;
typedef std::pair<std::string, uint> entrada;

class ConcurrentHashMap {
public:

    static ConcurrentHashMap count_words(std::string arch);

    static ConcurrentHashMap count_words(std::list<std::string> archs);

    static ConcurrentHashMap count_words(uint n, std::list<std::string> archs);

    static entrada maximum(uint p_archivos,
                           uint p_maximos,
                           std::list<std::string> archs);

    static entrada maximum_6(uint p_archivos,
                           uint p_maximos,
                           std::list<std::string> archs);

    ConcurrentHashMap();

    ~ConcurrentHashMap();

    void addAndInc(std::string key);

    bool member(std::string key);

    entrada maximum(uint nt);

    std::shared_ptr<Lista< entrada >> tabla[26];

private:

    unsigned char hash(const std::string& key);

    pthread_mutex_t add_locks[26];

    int mod_counter;

    pthread_mutex_t mod_counter_lock;

    pthread_cond_t mod_counter_condition;
};

#endif //SO_TP1_CONCURRENTHASHMAP_H
