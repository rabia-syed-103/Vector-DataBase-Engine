CXX      = g++
CXXFLAGS = -std=c++17 -Wall -pthread

all: vdb vdb-cli bench

vdb: src/main.cpp src/server.cpp src/VectorStore.cpp src/SearchResult.cpp src/protocol.cpp src/IVFIndex.cpp src/persistence.cpp
	$(CXX) $(CXXFLAGS) $^ -o $@

vdb-cli: src/client/main.cpp
	$(CXX) $(CXXFLAGS) $^ -o $@

bench: benchmark/benchmark.cpp
	$(CXX) $(CXXFLAGS) $^ -o benchmark/bench

clean:
	rm -f vdb vdb-cli benchmark/bench
