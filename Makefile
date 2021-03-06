.POSIX:
.SUFFIXES:
.SUFFIXES: .o .cpp

CXX = g++
CXXFLAGS = -std=c++11
LDLIBS = -lpthread -lrt

.cpp.o:
	$(CXX) $(CXXFLAGS) -c $<

BIN = test-2 test-3 test-5 test-extra
OBJ = ConcurrentHashMap.o

all: $(BIN)

$(BIN): ListaAtomica.hpp

ConcurrentHashMap.o: ConcurrentHashMap.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ ConcurrentHashMap.cpp

test-2: $(OBJ) test-2.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ test-2.cpp $(OBJ) $(LDLIBS)
	
test-2-run: test-2
	awk -f corpus.awk corpus | sort >corpus-post
	./test-2 | sort | diff -u - corpus-post
	rm -f corpus-post

test-3: $(OBJ) test-3.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ test-3.cpp $(OBJ) $(LDLIBS)

test-3-run: test-3
	awk -f corpus.awk corpus | sort >corpus-post
	for i in 0 1 2 3 4; do sed -n "$$((i * 500 + 1)),$$(((i + 1) * 500))p" corpus >corpus-"$$i"; done
	for i in 0 1 2 3 4; do ./test-3 $$((i + 1)) | sort | diff -u - corpus-post; done
	rm -f corpus-post corpus-[0-4]

test-5: $(OBJ) test-5.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ test-5.cpp $(OBJ) $(LDLIBS)

test-5-run: test-5
	awk -f corpus.awk corpus | sort -nk 2 | tail -n 1 >corpus-max
	cat corpus-max
	for i in 0 1 2 3 4; do sed -n "$$((i * 500 + 1)),$$(((i + 1) * 500))p" corpus >corpus-"$$i"; done
	for i in 0 1 2 3 4; do for j in 0 1 2 3 4; do \
		./test-5 $$((i + 1)) $$((j + 1)) | diff -u - corpus-max; \
	done; done
	rm -f corpus-max corpus-[0-4]

test-extra: $(OBJ) test-extra.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ test-extra.cpp $(OBJ) $(LDLIBS)
	
test-extra-run: test-extra
	./test-extra

test-time: $(OBJ) test-time.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ test-time.cpp $(OBJ) $(LDLIBS)

test-time-run: test-time
	rm -f time-data.csv
	echo "tarch,tmax,count_words concurrente,Tiempo en nanosegundos" > time-data.csv
	for i in 0 1 2 3 4; do sed -n "$$((i * 500 + 1)),$$(((i + 1) * 500))p" corpus >corpus-"$$i"; done
	for i in 0 1 2 3 4; do for j in 0 1 2 3 4; do \
		./test-time $$((i + 1)) $$((j + 1)); \
	done; done
	rm -f corpus-[0-4]

test-time-2: $(OBJ) test-time-2.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ test-time-2.cpp $(OBJ) $(LDLIBS)
	
test-time-2-run: test-time-2
	rm -f time-2-data.csv
	echo "tmax,Tiempo en nanosegundos" > time-2-data.csv
	for i in 0 1 2 3 4; do sed -n "$$((i * 500 + 1)),$$(((i + 1) * 500))p" corpus >corpus-"$$i"; done
	for i in 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20; do \
		./test-time-2 $$((i + 1)); \
	done
	rm -f corpus-[0-4]

clean:
	rm -f $(BIN) $(OBJ)
	rm -f corpus-*
