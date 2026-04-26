#include "protocol.h"
#include "SearchResult.h"

#include <sstream>
#include <vector>
#include <string>
#include <cstdint>

using namespace std;


static vector<string> tokenize(const string& line) {
    vector<string> tokens;
    istringstream ss(line);
    string tok;
    while (ss >> tok) tokens.push_back(tok);
    return tokens;
}


static string handle_ADD(const vector<string>& tokens, VectorStore& store) {
    if ((int)tokens.size() != 2 + store.dim)
        return "ERROR wrong number of components\n";

    int64_t id = stoll(tokens[1]);

    vector<float> vec;
    for (int i = 0; i < store.dim; i++)
        vec.push_back(stof(tokens[2 + i]));

    store_add(store, id, vec);
    return "OK\n";
}


static string handle_SEARCH(const vector<string>& tokens, VectorStore& store) {

    if ((int)tokens.size() < store.dim + 3)
        return "ERROR too few arguments for SEARCH\n";

    vector<float> query;
    for (int i = 0; i < store.dim; i++)
        query.push_back(stof(tokens[1 + i]));

    int k         = stoi(tokens[1 + store.dim]);
    string mode   = tokens[2 + store.dim];

    if (mode == "BRUTE") {
        int scanned = 0;
        auto results = brute_search(store, query, k, scanned);

        ostringstream out;
        for (auto& r : results) {
            out << r.id << " " << r.dist;
            for (float f : r.vec) out << " " << f;
            out << "\n";
        }
        out << "(" << results.size()
            << " results, mode=BRUTE, scanned " << scanned << ")\n";
        return out.str();
    }

    if (mode == "IVF") {
        return "ERROR IVF index not built yet\n";
    }

    return "ERROR unknown mode\n";
}


static string handle_STATS(VectorStore& store) {
    ostringstream out;
    out << "dimension: "     << store.dim   << "\n";
    out << "total vectors: " << store.count << "\n";
    out << "index built: "   << (store.index_built ? "yes" : "no") << "\n";
    // Phase 2 adds cluster count and per-cluster sizes here
    return out.str();
}


string parse_and_dispatch(const string& line, VectorStore& store) {
    auto tokens = tokenize(line);
    if (tokens.empty()) return "";

    string cmd = tokens[0];

    if (cmd == "ADD")    return handle_ADD(tokens, store);
    if (cmd == "SEARCH") return handle_SEARCH(tokens, store);
    if (cmd == "STATS")  return handle_STATS(store);
    if (cmd == "QUIT")   return "QUIT";
    if (cmd == "BUILD")  return "ERROR not implemented yet\n";
    if (cmd == "SAVE")   return "ERROR not implemented yet\n";
    if (cmd == "LOAD")   return "ERROR not implemented yet\n";

    return "ERROR unknown command\n";
}