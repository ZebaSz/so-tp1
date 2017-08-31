#include <mutex>
#include <fstream>
#include <thread>
#include <condition_variable>
#include "ConcurrentHashMap.hpp"


ConcurrentHashMap::ConcurrentHashMap() : mod_counter(0) {
    for (int i = 0; i < 26; ++i) {
        tabla[i] = new Lista<entrada>();
    }
}

ConcurrentHashMap::~ConcurrentHashMap() {
    for (int i = 0; i < 26; ++i) {
        delete tabla[i];
    }
}

void ConcurrentHashMap::addAndInc(std::string key) {
    // check for other ops running
    {
        std::unique_lock<std::mutex> unique_lock(mod_counter_lock);
        // if maximum is running, wait until they're finished
        mod_counter_condition.wait(unique_lock, [&] { return mod_counter >= 0; });
        // register this instance
        ++mod_counter;
    }
    {
        // lock the list for that hash
        int h = hash(key);
        std::lock_guard<std::mutex> lock_guard(add_locks[h]);
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
    }
    {
        // deregister this instance
        std::lock_guard<std::mutex> lock_guard(mod_counter_lock);
        --mod_counter;
        if (mod_counter == 0) {
            // no more concurrent adding, maximum can run now
            mod_counter_condition.notify_all();
        }
    }
}

bool ConcurrentHashMap::member(std::string key) {
    // FIXME: this needs to be wait-free, can we do this with mutexes?
    Lista<entrada>* l = tabla[hash(key)];
    for (auto it = l->CrearIt(); it.HaySiguiente(); it.Avanzar()) {
        if(it.Siguiente().first == key) {
            return true;
        }
    }
    return false;
}

void maximum_internal(ConcurrentHashMap& map,
                      std::mutex& compare_lock,
                      entrada** res,
                      std::atomic_uint& next) {
    uint cur;
    while((cur = next++) < 26) {
        Lista<entrada>* l = map.tabla[cur];
        auto max = l->CrearIt();
        if(max.HaySiguiente()) {
            for (auto it = max; it.HaySiguiente(); it.Avanzar()) {
                if (it.Siguiente().second > max.Siguiente().second) {
                    max = it;
                }
            }
            {
                std::lock_guard<std::mutex> lock_guard(compare_lock);
                if(*res == nullptr || (*res)->second < max.Siguiente().second) {
                    *res = &max.Siguiente();
                }
            }
        }
    }
}

entrada ConcurrentHashMap::maximum(uint nt) {
    // check for other ops running
    {
        std::unique_lock<std::mutex> unique_lock(mod_counter_lock);
        // if addAndInc is running, wait until they're finished
        mod_counter_condition.wait(unique_lock, [&]{return mod_counter <= 0;});
        // register this instance
        ++mod_counter;
    }

    // set up threads
    entrada* e = nullptr;
    std::mutex compare_lock;
    std::atomic_uint next(0);
    // start threads
    std::vector< std::thread > ts;
    for (uint i = 0; i < nt; ++i) {
        ts.push_back(std::thread(maximum_internal,
                                 std::ref(*this),
                                 std::ref(compare_lock),
                                 &e,
                                 std::ref(next)));
    }
    // sync with all threads
    for (uint i = 0; i < nt; ++i) {
        ts[i].join();
    }
    {
        // deregister this instance
        std::lock_guard<std::mutex> lock_guard(mod_counter_lock);
        ++mod_counter;
        if (mod_counter == 0) {
            // no more concurrent maximums, addAndInc can run now
            mod_counter_condition.notify_all();
        }
        return *e;
    }
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

entrada ConcurrentHashMap::maximum(unsigned int p_archivos,
                                   unsigned int p_maximos,
                                   std::list<std::string> archs) {
    // FIXME: this is the same as ConcurrentHashMap::count_words,
    // but it was forbidden to use said function
    ConcurrentHashMap map;
    std::vector<std::string> files(archs.begin(), archs.end());
    std::atomic_uint next(0);
    // start exactly n threads
    std::list< std::thread > ts;
    for (uint i = 0; i < p_archivos; ++i) {
        ts.push_back(std::thread(add_words_multiple, std::ref(map), std::ref(files), std::ref(next)));
    }
    // sync with all threads
    for (auto it = ts.begin(); it != ts.end(); ++it) {
        it->join();
    }
    return map.maximum(p_maximos);
}

unsigned char ConcurrentHashMap::hash(const std::string &key) {
    return (unsigned char)key[0] - 'a';
}

// TODO: these are required due to mutex deleted constructors/assignments
// see if there is a better way of doing this

// Move constructor
ConcurrentHashMap::ConcurrentHashMap(ConcurrentHashMap&& other) : ConcurrentHashMap() {
    /* TODO: see if we should lock on move
    {
        std::unique_lock<std::mutex> this_lock(mod_counter_lock);
        std::unique_lock<std::mutex> other_lock(other.mod_counter_lock);
        mod_counter_condition.wait(this_lock, [&] { return mod_counter == 0; });
        other.mod_counter_condition.wait(other_lock, [&other] { return other.mod_counter == 0; });
    */
    for (int i = 0; i < 26; ++i) {
        tabla[i] = other.tabla[i];
        other.tabla[i] = nullptr;
    }
    //}
}

// Move assignment
ConcurrentHashMap& ConcurrentHashMap::operator=(ConcurrentHashMap&& other) {
    /* TODO: see if we should lock on move
    {
        std::unique_lock<std::mutex> this_lock(mod_counter_lock);
        std::unique_lock<std::mutex> other_lock(other.mod_counter_lock);
        mod_counter_condition.wait(this_lock, [&] { return mod_counter == 0; });
        other.mod_counter_condition.wait(other_lock, [&other] { return other.mod_counter == 0; });
    */
    for (int i = 0; i < 26; ++i) {
        delete tabla[i];
        tabla[i] = other.tabla[i];
        other.tabla[i] = nullptr;
    }
    //}
    return *this;
}
