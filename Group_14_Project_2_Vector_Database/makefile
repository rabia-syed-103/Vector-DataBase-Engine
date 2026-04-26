CXX      = g++
CXXFLAGS = -std=c++17 -Wall -pthread

all: vdb vdb-cli

vdb: src/main.cpp src/server.cpp src/VectorStore.cpp src/SearchResult.cpp src/protocol.cpp
	$(CXX) $(CXXFLAGS) $^ -o $@

vdb-cli: src/client/main.cpp
	$(CXX) $(CXXFLAGS) $^ -o $@

clean:
	rm -f vdb vdb-cli