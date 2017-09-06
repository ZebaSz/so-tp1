#include <time.h>
#include <iostream>
#include "ConcurrentHashMap.hpp"

#define REPETITIONS 50

int main(int argc, char **argv) {
    if (argc != 3) {
        std::cerr << "uso: " << argv[0] << " #tarchivos #tmaximum" << std::endl;
        return 1;
    }
    uint tarch = (uint)atoi(argv[1]);
    uint tmax = (uint)atoi(argv[2]);
    timespec start;
    timespec end;
    unsigned long best;
    std::list<std::string> l = { "corpus-0", "corpus-1", "corpus-2", "corpus-3", "corpus-4" };

    std::cout << "tarch = " << tarch << ", tmax = "  << tmax << std::endl;

    FILE* f = fopen("time-data.csv" ,"a");

    for (int i = 0; i < REPETITIONS; ++i) {
        clock_gettime(CLOCK_REALTIME, &start);
        ConcurrentHashMap::maximum(tarch, tmax, l);
        clock_gettime(CLOCK_REALTIME, &end);
        unsigned long diff =(unsigned long)(end.tv_nsec - start.tv_nsec) ;
        fprintf(f,"%u,%u,5,%lu\n", tarch, tmax, diff);
        if(best > diff) {
            best = diff;
        }
    }

    for (int i = 0; i < REPETITIONS; ++i) {
        clock_gettime(CLOCK_REALTIME, &start);
        ConcurrentHashMap::maximum_6(tarch, tmax, l);
        clock_gettime(CLOCK_REALTIME, &end);
        unsigned long diff =(unsigned long)(end.tv_nsec - start.tv_nsec) ;
        fprintf(f,"%u,%u,6,%lu\n", tarch, tmax, diff);
        if(best > diff) {
            best = diff;
        }
    }
    fclose(f);
}