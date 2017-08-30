#ifndef SO_TP1_CONCURRENTHASHMAP_H
#define SO_TP1_CONCURRENTHASHMAP_H

#include <list>
#include <string>
#include <vector>
#include <mutex>
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

    ConcurrentHashMap();

    ~ConcurrentHashMap();

    void addAndInc(std::string key);

    bool member(std::string key);

    entrada maximum(uint nt);

    Lista< entrada >* tabla[26];

    ConcurrentHashMap& operator=(ConcurrentHashMap&& other);

private:

    ConcurrentHashMap(ConcurrentHashMap&& other);

    unsigned char hash(const std::string& key);

    std::mutex add_locks[26];

    int mod_counter;

    std::mutex mod_counter_lock;

    std::mutex notify_lock;
};

#endif //SO_TP1_CONCURRENTHASHMAP_H
