#include <time.h>
#include <iostream>
#include <climits>
#include "ConcurrentHashMap.hpp"

#define REPETITIONS 500
#define NS_IN_SEC 1000000000
#define TO_NS(t) (unsigned long)(t.tv_nsec + t.tv_sec * NS_IN_SEC)

int main(int argc, char **argv) {
	if (argc != 2) {
		std::cerr << "uso: " << argv[0] << " #threads" << std::endl;
		return 1;
	}
    uint tarch = (uint)atoi(argv[1]);
    ulong best = ULONG_MAX;
    std::list<std::string> l = { "corpus-0", "corpus-1", "corpus-2", "corpus-3", "corpus-4" };

    std::cout << "tarch = " << tarch << std::endl;

    FILE* f = fopen("time-2-data.csv" ,"a");
    
	best = ULONG_MAX;
	double acum = 0;
    for (int i = 0; i < REPETITIONS; ++i) {
        timespec start;
        timespec end;
        clock_gettime(CLOCK_REALTIME, &start);
        ConcurrentHashMap::count_words(tarch, l);
        clock_gettime(CLOCK_REALTIME, &end);
        ulong diff = TO_NS(end) - TO_NS(start) ;
        fprintf(f,"%u,No,%lu\n", tarch, diff);
        if(best > diff) {
            best = diff;
        }
        acum += diff;
    }

	std::cout << "THREADS: " << tarch << " PROMEDIO: " << (ulong)(acum/REPETITIONS) << std::endl;
    fclose(f);
}
