#include "VectorStore.h"

void store_init(VectorStore& store, int dim) {
    store.dim = dim;
    store.count = 0;
    store.vectors.reserve(1000 * dim); 
    store.ids.reserve(1000);
}

void store_add(VectorStore& store, int64_t id, const vector<float>& vec) {
    lock_guard<mutex> lock(store.mtx);

    auto it = store.id_to_index.find(id);
    if (it != store.id_to_index.end()) {
        int idx = it->second;
        for (int i = 0; i < store.dim; i++)
            store.vectors[idx * store.dim + i] = vec[i];
    } else {
        for (float f : vec)
            store.vectors.push_back(f);
        store.ids.push_back(id);
        store.id_to_index[id] = store.count;
        store.count++;
    }
    if (store.index_built) 
    {
        int internal = store.id_to_index[id];
        int c = nearest_centroid(store.index, store.vectors.data() + internal * store.dim);
        store.index.cluster_lists[c].push_back(internal);
    }
}