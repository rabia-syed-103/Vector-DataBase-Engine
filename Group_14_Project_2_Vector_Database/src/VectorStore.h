#ifndef VECTOR_STORE_H
#define VECTOR_STORE_H
#include <vector>
#include <unordered_map>
#include <mutex>
#include <cstdint>
using namespace std;
struct VectorStore {
    vector<float> vectors;      
    vector<int64_t> ids;         
    unordered_map<int64_t, int> id_to_index;  
    mutex mtx;
    int dim = 0;
    int count = 0;
    bool index_built = false; 
};

void store_init(VectorStore& store, int dim);
void store_add (VectorStore& store, int64_t id, const vector<float>& vec);

#endif 