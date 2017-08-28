#ifndef SO_TP1_CONCURRENTHASHMAP_H
#define SO_TP1_CONCURRENTHASHMAP_H

#include <list>
#include <string>
#include <vector>
#include "ListaAtomica.hpp"

typedef unsigned int uint;

class ConcurrentHashMap {
public:

    static ConcurrentHashMap count_words(std::string arch);

    static ConcurrentHashMap count_words(std::list<std::string> archs);

    static ConcurrentHashMap count_words(uint n, std::list<std::string> archs);

    static std::pair<std::string, uint>maximum(uint p_archivos,
                                               uint p_maximos,
                                               std::list<std::string> archs);

    ConcurrentHashMap();

    void addAndInc(std::string key);

    bool member(std::string key);

    std::pair<std::string, uint> maximum(uint nt);

    std::vector< Lista< std::pair<std::string, uint> >* > tabla;
};

#endif //SO_TP1_CONCURRENTHASHMAP_H
