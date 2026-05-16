#include "protocol.h"
#include "SearchResult.h"
#include "IVFIndex.h"
#include <sstream>
#include <vector>
#include <string>
#include <cstdint>
#include <chrono>
#include "persistence.h"

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

    int64_t id;
    try {
        id = std::stoll(tokens[1]);
    } catch (...) {
        return "ERROR invalid id, must be a 64-bit integer\n";
    }
    vector<float> vec;
    for (int i = 0; i < store.dim; i++)
        vec.push_back(stof(tokens[2 + i]));
    store_add(store, id, vec);
    return "OK\n";
}

static string handle_SEARCH(const vector<string>& tokens, VectorStore& store) {
    if ((int)tokens.size() < store.dim + 3)
        return "ERROR too few arguments for SEARCH\n";

    std::vector<float> query;
    try {
        for (int i = 0; i < store.dim; i++)
            query.push_back(std::stof(tokens[1 + i]));
    } catch (...) {
        return "ERROR invalid float in query vector\n";
    }

    int k;
    try {
        k = stoi(tokens[1 + store.dim]);
    } catch (...) {
        return "ERROR invalid k value\n";
    }
    string mode = tokens[2 + store.dim];

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
        if (!store.index_built)
            return "ERROR index not built, run BUILD first\n";

        if ((int)tokens.size() < store.dim + 4)
            return "ERROR missing nprobe argument\n";

        int nprobe  = stoi(tokens[3 + store.dim]);
        int scanned = 0;
        auto results = ivf_search(store, query, k, nprobe, scanned);

        ostringstream out;
        for (auto& r : results) {
            out << r.id << " " << r.dist;
            for (float f : r.vec) out << " " << f;
            out << "\n";
        }
        out << "(" << results.size()
            << " results, mode=IVF, nprobe=" << nprobe
            << ", scanned " << scanned << ")\n";
        return out.str();
    }

    return "ERROR unknown mode\n";
}

static string handle_BUILD(VectorStore& store) {
    if (store.count == 0)
        return "ERROR no vectors stored\n";

    auto t0 = chrono::high_resolution_clock::now();

    int iters;
    {
        lock_guard<mutex> lock(store.mtx);
        iters = ivf_build(store.index, store.vectors, store.count, store.dim);
        store.index_built = true;
    }

    auto t1 = chrono::high_resolution_clock::now();
    double secs = chrono::duration<double>(t1 - t0).count();

    ostringstream out;
    out << "Building IVF index...\n";
    out << "vectors: "    << store.count   << "\n";
    out << "clusters: "   << store.index.K << "\n";
    out << "iterations: " << iters         << "\n";
    out << "done in "     << secs          << " s.\n";
    out << "OK\n";
    return out.str();
}

static string handle_STATS(VectorStore& store) {
    ostringstream out;
    out << "dimension: "     << store.dim   << "\n";
    out << "total vectors: " << store.count << "\n";
    out << "index built: "   << (store.index_built ? "yes" : "no") << "\n";

    if (store.index_built) {
        out << "clusters: " << store.index.K << "\n";
        out << "cluster sizes: ";
        for (int c = 0; c < store.index.K; c++) {
            out << store.index.cluster_lists[c].size();
            if (c < store.index.K - 1) out << ", ";
        }
        out << "\n";
    }

    out << "\n";
    return out.str();
}

static string handle_SAVE(VectorStore& store,const string& data_dir) {
    std::lock_guard<std::mutex> lock(store.mtx);
    bool ok = store_save(store, data_dir);
    if (ok) return "OK\n";
    return "ERROR save failed\n";
}

static string handle_LOAD(VectorStore& store,const string& data_dir) {
    std::lock_guard<std::mutex> lock(store.mtx);
    bool ok = store_load(store, data_dir);
    if (ok) return "OK\n";
    return "ERROR load failed or no snapshot found\n";
}

string parse_and_dispatch(const string& line, VectorStore& store,const string& data_dir) {
    auto tokens = tokenize(line);
    if (tokens.empty()) return "";

    string cmd = tokens[0];

    if (cmd == "ADD")    return handle_ADD(tokens, store);
    if (cmd == "SEARCH") return handle_SEARCH(tokens, store);
    if (cmd == "STATS")  return handle_STATS(store);
    if (cmd == "QUIT")   return "QUIT";
    if (cmd == "BUILD")  return handle_BUILD(store);
    if (cmd == "SAVE") return handle_SAVE(store, data_dir);
    if (cmd == "LOAD") return handle_LOAD(store, data_dir);

    return "ERROR unknown command\n";
}

