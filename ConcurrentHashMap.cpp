#include <mutex>
#include <fstream>
#include <thread>
#include <condition_variable>
#include "ConcurrentHashMap.hpp"


ConcurrentHashMap::ConcurrentHashMap() : mod_counter(0) {
    pthread_mutex_init(&mod_counter_lock, NULL);
    pthread_cond_init(&mod_counter_condition, NULL);
    for (int i = 0; i < 26; ++i) {
        tabla[i] = std::make_shared<Lista<entrada>>();
        pthread_mutex_init(&add_locks[i], NULL);
    }
}

ConcurrentHashMap::~ConcurrentHashMap() {
    pthread_mutex_destroy(&mod_counter_lock);
    pthread_cond_destroy(&mod_counter_condition);
    for (int i = 0; i < 26; ++i) {
        pthread_mutex_destroy(&add_locks[i]);
    }
}

void ConcurrentHashMap::addAndInc(std::string key) {
    // check for other ops running
    pthread_mutex_lock(&mod_counter_lock);
    // if maximum is running, wait until they're finished
    while(mod_counter < 0) {
        pthread_cond_wait(&mod_counter_condition, &mod_counter_lock);
    }
    // register this instance
    ++mod_counter;
    pthread_mutex_unlock(&mod_counter_lock);

    // lock the list for that hash
    int h = hash(key);
    pthread_mutex_lock(&add_locks[h]);
    // do the actual adding
    auto it = tabla[h]->CrearIt();
    for (; it.HaySiguiente(); it.Avanzar()) {
        if (it.Siguiente().first == key) {
            ++it.Siguiente().second;
            break;
        }
    }
    if (!it.HaySiguiente()) {
        tabla[h]->push_front(entrada(key, 1));
    }
    pthread_mutex_unlock(&add_locks[h]);

    // deregister this instance
    pthread_mutex_lock(&mod_counter_lock);
    --mod_counter;
    if (mod_counter == 0) {
        // no more concurrent adding, maximum can run now
        pthread_cond_broadcast(&mod_counter_condition);
    }
    pthread_mutex_unlock(&mod_counter_lock);
}

bool ConcurrentHashMap::member(std::string key) {
    std::shared_ptr<Lista<entrada>> l = tabla[hash(key)];
    for (auto it = l->CrearIt(); it.HaySiguiente(); it.Avanzar()) {
        if(it.Siguiente().first == key) {
            return true;
        }
    }
    return false;
}

void maximum_internal(ConcurrentHashMap& map,
                      std::atomic<entrada*>* res,
                      std::atomic_uint& next) {
    uint cur;
    while((cur = next++) < 26) {
        std::shared_ptr<Lista<entrada>> l = map.tabla[cur];
        auto max = l->CrearIt();
        if(max.HaySiguiente()) {
            for (auto it = max; it.HaySiguiente(); it.Avanzar()) {
                if (it.Siguiente().second > max.Siguiente().second) {
                    max = it;
                }
            }

            entrada* old = res->load();
            while((old == NULL || old->second < max.Siguiente().second)
                  && !std::atomic_compare_exchange_weak(res, &old, &max.Siguiente()));
        }
    }
}

entrada ConcurrentHashMap::maximum(uint nt) {
    // check for other ops running
    pthread_mutex_lock(&mod_counter_lock);
    // if addAndInc is running, wait until they're finished
    while(mod_counter > 0) {
        pthread_cond_wait(&mod_counter_condition, &mod_counter_lock);
    }
    // register this instance
    --mod_counter;
    pthread_mutex_unlock(&mod_counter_lock);

    // set up threads
    std::atomic<entrada*> e(NULL);
    std::atomic_uint next(0);
    // start threads
    std::vector< std::thread > ts;
    for (uint i = 0; i < nt; ++i) {
        ts.push_back(std::thread(maximum_internal,
                                 std::ref(*this),
                                 &e,
                                 std::ref(next)));
    }
    // sync with all threads
    for (uint i = 0; i < nt; ++i) {
        ts[i].join();
    }

    // deregister this instance
    pthread_mutex_lock(&mod_counter_lock);
    ++mod_counter;
    if (mod_counter == 0) {
        // no more concurrent adding, maximum can run now
        pthread_cond_broadcast(&mod_counter_condition);
    }
    entrada res(*e);
    pthread_mutex_unlock(&mod_counter_lock);
    return res;
}

ConcurrentHashMap ConcurrentHashMap::count_words(std::string arch) {
    std::ifstream file(arch);
    ConcurrentHashMap map;
    std::string line;
    while (std::getline(file, line)) {
        map.addAndInc(line);
    }
    return map;
}

void add_words(ConcurrentHashMap& map, std::string arch) {
    std::ifstream file(arch);
    std::string line;
    while (std::getline(file, line)) {
        map.addAndInc(line);
    }
}

ConcurrentHashMap ConcurrentHashMap::count_words(std::list<std::string> archs) {
    ConcurrentHashMap map;
    // start a thread per file
    std::list< std::thread > ts;
    for (auto it = archs.begin(); it != archs.end(); ++it) {
        ts.push_back(std::thread(add_words, std::ref(map), *it));
    }
    // sync with all threads
    for (auto it = ts.begin(); it != ts.end(); ++it) {
        it->join();
    }
    return map;
}

void add_words_multiple(ConcurrentHashMap& map,
                        const std::vector<std::string> archs,
                        std::atomic_uint& next) {
    uint cur;
    while((cur = next++) < archs.size()) {
        std::string arch = archs[cur];
        add_words(map, arch);
    }
}

ConcurrentHashMap ConcurrentHashMap::count_words(unsigned int n, std::list<std::string> archs) {
    ConcurrentHashMap map;
    std::vector<std::string> files(archs.begin(), archs.end());
    std::atomic_uint next(0);
    // start exactly n threads
    std::list< std::thread > ts;
    for (uint i = 0; i < n; ++i) {
        ts.push_back(std::thread(add_words_multiple, std::ref(map), std::ref(files), std::ref(next)));
    }
    // sync with all threads
    for (auto it = ts.begin(); it != ts.end(); ++it) {
        it->join();
    }
    return map;
}

void count_words_single(Lista<ConcurrentHashMap>& maps,
                        const std::vector<std::string>& archs,
                        std::atomic_uint& next) {
    uint cur;
    while((cur = next++) < archs.size()) {
        maps.push_front(ConcurrentHashMap::count_words(archs[cur]));
    }
}

void merge_maps(const Lista<ConcurrentHashMap>& maps,
                ConcurrentHashMap& unifiedMap,
                std::atomic_uint& next,
                size_t size) {
    uint cur;
    while((cur = next++) < size) {
        const ConcurrentHashMap& map = maps.iesimo(cur);
        for (int i = 0; i < 26; ++i) {
            for (auto lista = map.tabla[i]->CrearIt(); lista.HaySiguiente(); lista.Avanzar()) {
                for (uint j = 0; j < lista.Siguiente().second; ++j) {
                    unifiedMap.addAndInc(lista.Siguiente().first);
                }
            }
        }
    }
}

// ugly not-so-concurrent version
entrada ConcurrentHashMap::maximum(unsigned int p_archivos,
                                   unsigned int p_maximos,
                                   std::list<std::string> archs) {
    Lista<ConcurrentHashMap> maps;
    std::vector<std::string> files(archs.begin(), archs.end());
    std::atomic_uint next(0);
    // start exactly n threads
    std::list< std::thread > ts;
    for (uint i = 0; i < p_archivos; ++i) {
        ts.push_back(std::thread(count_words_single, std::ref(maps),
                                 std::cref(files), std::ref(next)));
    }
    // sync with all threads
    for (auto it = ts.begin(); it != ts.end(); ++it) {
        it->join();
    }

    ts.clear();
    ConcurrentHashMap unifiedMap;
    next.store(0);
    // start exactly n threads
    for (uint i = 0; i < p_maximos; ++i) {
        ts.push_back(std::thread(merge_maps, std::cref(maps),
                                 std::ref(unifiedMap), std::ref(next), archs.size()));
    }
    // sync with all threads
    for (auto it = ts.begin(); it != ts.end(); ++it) {
        it->join();
    }

    return unifiedMap.maximum(p_maximos);
}

// pretty concurrent version
entrada ConcurrentHashMap::maximum_6(unsigned int p_archivos,
                                     unsigned int p_maximos,
                                     std::list<std::string> archs) {
    return ConcurrentHashMap::count_words(p_archivos, archs).maximum(p_maximos);
}

unsigned char ConcurrentHashMap::hash(const std::string &key) {
    return (unsigned char)key[0] - 'a';
}