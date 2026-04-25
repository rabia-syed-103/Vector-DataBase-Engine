#include "SearchResult.h"

float distance_sq(const float* a, const float* b, int D) {
    float sum = 0.0f;
    for (int i = 0; i < D; i++) {
        float diff = a[i] - b[i];
        sum += diff * diff;
    }
    return sum;
}

std::vector<SearchResult> brute_search(VectorStore& store,const vector<float>& query,int k,
                                        int& scanned_out) {
    std::lock_guard<std::mutex> lock(store.mtx);

    using Pair = std::pair<float, int>; 
    std::priority_queue<Pair> heap;

    for (int i = 0; i < store.count; i++) {
        float d = distance_sq(query.data(),
                              &store.vectors[i * store.dim],
                              store.dim);
        if ((int)heap.size() < k) {
            heap.push({d, i});
        } else if (d < heap.top().first) {
            heap.pop();
            heap.push({d, i});
        }
    }
    scanned_out = store.count;

    vector<SearchResult> results(heap.size());
    int pos = heap.size() - 1;
    while (!heap.empty()) {
        auto [d, idx] = heap.top(); 
        heap.pop();
        SearchResult r;
        r.id   = store.ids[idx];
        r.dist = d;
        r.vec  = vector<float>(
                    store.vectors.begin() + idx * store.dim,
                    store.vectors.begin() + idx * store.dim + store.dim);
        results[pos--] = r;
    }
    return results;
}