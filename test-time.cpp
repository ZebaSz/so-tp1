#include <time.h>
#include <iostream>
#include <climits>
#include "ConcurrentHashMap.hpp"

#define REPETITIONS 50
#define NS_IN_SEC 1000000000
#define TO_NS(t) (unsigned long)(t.tv_nsec + t.tv_sec * NS_IN_SEC)

int main(int argc, char **argv) {
    if (argc != 3) {
        std::cerr << "uso: " << argv[0] << " #tarchivos #tmaximum" << std::endl;
        return 1;
    }
    uint tarch = (uint)atoi(argv[1]);
    uint tmax = (uint)atoi(argv[2]);
    ulong best = ULONG_MAX;
    std::list<std::string> l = { "corpus-0", "corpus-1", "corpus-2", "corpus-3", "corpus-4" };

    std::cout << "tarch = " << tarch << ", tmax = "  << tmax << std::endl;

    FILE* f = fopen("time-data.csv" ,"a");

    for (int i = 0; i < REPETITIONS; ++i) {
        timespec start;
        timespec end;
        clock_gettime(CLOCK_REALTIME, &start);
        ConcurrentHashMap::maximum(tarch, tmax, l);
        clock_gettime(CLOCK_REALTIME, &end);
        ulong diff = TO_NS(end) - TO_NS(start) ;
        fprintf(f,"%u,%u,No,%lu\n", tarch, tmax, diff);
        if(best > diff) {
            best = diff;
        }
    }

    best = ULONG_MAX;

    for (int i = 0; i < REPETITIONS; ++i) {
        timespec start;
        timespec end;
        clock_gettime(CLOCK_REALTIME, &start);
        ConcurrentHashMap::maximum_6(tarch, tmax, l);
        clock_gettime(CLOCK_REALTIME, &end);
        ulong diff = TO_NS(end) - TO_NS(start) ;
        fprintf(f,"%u,%u,Sí,%lu\n", tarch, tmax, diff);
        if(best > diff) {
            best = diff;
        }
    }
    fclose(f);
}