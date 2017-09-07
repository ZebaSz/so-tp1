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

struct maximum_s {
    ConcurrentHashMap& map;
    std::atomic<entrada*>* res;
    std::atomic_uint& next;
};

void* maximum_internal(void* arg) {
    maximum_s* params = (maximum_s*)arg;
    uint cur;
    while((cur = params->next++) < 26) {
        std::shared_ptr<Lista<entrada>> l = params->map.tabla[cur];
        auto max = l->CrearIt();
        if(max.HaySiguiente()) {
            for (auto it = max; it.HaySiguiente(); it.Avanzar()) {
                if (it.Siguiente().second > max.Siguiente().second) {
                    max = it;
                }
            }

            entrada* old = params->res->load();
            while((old == NULL || old->second < max.Siguiente().second)
                  && !std::atomic_compare_exchange_weak(params->res, &old, &max.Siguiente()));
        }
    }
    pthread_exit(NULL);
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
    std::vector< pthread_t > ts;
    maximum_s params = {*this, &e, next};

    for (uint i = 0; i < nt; ++i) {
        pthread_t t;
        pthread_create(&t, NULL, maximum_internal, &params);
        ts.push_back(t);
    }
    // sync with all threads
    for (uint i = 0; i < nt; ++i) {
        pthread_join(ts[i], NULL);
    }

    // deregister this instance
    pthread_mutex_lock(&mod_counter_lock);
    ++mod_counter;
    if (mod_counter == 0) {
        // no more concurrent adding, maximum can run now
        pthread_cond_broadcast(&mod_counter_condition);
    }
    entrada res(*e.load());
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

struct add_words_s {
    add_words_s(ConcurrentHashMap &map, const std::string &arch) : map(map), arch(arch) {}
    ConcurrentHashMap& map;
    const std::string& arch;
};

void add_words(add_words_s* params) {
    std::ifstream file(params->arch);
    std::string line;
    while (std::getline(file, line)) {
        params->map.addAndInc(line);
    }
}

void* add_words_single(void* arg) {
    add_words_s* params = (add_words_s*)arg;
    add_words(params);
    pthread_exit(NULL);
}

ConcurrentHashMap ConcurrentHashMap::count_words(std::list<std::string> archs) {
    ConcurrentHashMap map;
    // start a thread per file
    std::list< pthread_t > ts;
    std::list< add_words_s* > params;
    for (auto it = archs.begin(); it != archs.end(); ++it) {
        add_words_s* param = new add_words_s(map, *it);
        params.push_back(param);
        pthread_t t;
        pthread_create(&t, NULL, add_words_single, param);
        ts.push_back(t);
    }
    // sync with all threads
    for (auto it = ts.begin(); it != ts.end(); ++it) {
        pthread_join(*it, NULL);
    }
    for (auto it = params.begin(); it != params.end(); ++it) {
        delete *it;
    }
    return map;
}

struct add_words_multiple_s {
    ConcurrentHashMap& map;
    const std::vector<std::string> archs;
    std::atomic_uint& next;
};

void* add_words_multiple(void* arg) {
    add_words_multiple_s* params = (add_words_multiple_s*) arg;
    uint cur;
    while((cur = params->next++) < params->archs.size()) {
        add_words_s param = {params->map, params->archs[cur]};
        add_words(&param);
    }
    pthread_exit(NULL);
}

ConcurrentHashMap ConcurrentHashMap::count_words(unsigned int n, std::list<std::string> archs) {
    ConcurrentHashMap map;
    std::vector<std::string> files(archs.begin(), archs.end());
    std::atomic_uint next(0);
    add_words_multiple_s params = {map, files, next};
    // start exactly n threads
    std::list< pthread_t > ts;
    for (uint i = 0; i < n; ++i) {
        pthread_t t;
        pthread_create(&t, NULL, add_words_multiple, &params);
        ts.push_back(t);
    }
    // sync with all threads
    for (auto it = ts.begin(); it != ts.end(); ++it) {
        pthread_join(*it, NULL);
    }
    return map;
}

struct count_words_s {
    Lista<ConcurrentHashMap>& maps;
    const std::vector<std::string>& archs;
    std::atomic_uint& next;
};

void* count_words_single(void* arg) {
    count_words_s* params = (count_words_s*)arg;
    uint cur;
    while((cur = params->next++) < params->archs.size()) {
        params->maps.push_front(ConcurrentHashMap::count_words(params->archs[cur]));
    }
    pthread_exit(NULL);
}

struct merge_maps_s {
    const Lista<ConcurrentHashMap>& maps;
    ConcurrentHashMap& unifiedMap;
    std::atomic_uint& next;
    size_t size;
};

void* merge_maps(void* arg) {
    merge_maps_s* params = (merge_maps_s*)arg;
    uint cur;
    while((cur = params->next++) < params->size) {
        const ConcurrentHashMap& map = params->maps.iesimo(cur);
        for (int i = 0; i < 26; ++i) {
            for (auto lista = map.tabla[i]->CrearIt(); lista.HaySiguiente(); lista.Avanzar()) {
                for (uint j = 0; j < lista.Siguiente().second; ++j) {
                    params->unifiedMap.addAndInc(lista.Siguiente().first);
                }
            }
        }
    }
    pthread_exit(NULL);
}

// ugly not-so-concurrent version
entrada ConcurrentHashMap::maximum(unsigned int p_archivos,
                                   unsigned int p_maximos,
                                   std::list<std::string> archs) {
    Lista<ConcurrentHashMap> maps;
    std::vector<std::string> files(archs.begin(), archs.end());
    std::atomic_uint next(0);
    // start exactly n threads
    std::list< pthread_t > ts;

    count_words_s cw_params = {maps, files, next};
    for (uint i = 0; i < p_archivos; ++i) {
        pthread_t t;
        pthread_create(&t, NULL, count_words_single, &cw_params);
        ts.push_back(t);
    }
    // sync with all threads
    for (auto it = ts.begin(); it != ts.end(); ++it) {
        pthread_join(*it, NULL);
    }

    ts.clear();
    ConcurrentHashMap unifiedMap;
    next.store(0);
    merge_maps_s mm_params = {maps, unifiedMap, next, archs.size()};
    // start exactly n threads
    for (uint i = 0; i < p_maximos; ++i) {
        pthread_t t;
        pthread_create(&t, NULL, merge_maps, &mm_params);
        ts.push_back(t);
    }
    // sync with all threads
    for (auto it = ts.begin(); it != ts.end(); ++it) {
        pthread_join(*it, NULL);
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