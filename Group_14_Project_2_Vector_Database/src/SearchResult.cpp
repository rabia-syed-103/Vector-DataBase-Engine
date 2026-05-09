#include "SearchResult.h"

float distance_sq(const float* a, const float* b, int D) {
    float sum = 0.0f;
    for (int i = 0; i < D; i++) {
        float diff = a[i] - b[i];
        sum += diff * diff;
    }
    return sum;
}

vector<SearchResult> brute_search(VectorStore& store,const vector<float>& query,int k, int& scanned_out) {
    lock_guard<std::mutex> lock(store.mtx);

    using Pair = std::pair<float, int>; 
    std::priority_queue<Pair> heap;

    for (int i = 0; i < store.count; i++) {
        float d = distance_sq(query.data(), &store.vectors[i * store.dim],store.dim);
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

vector<SearchResult> ivf_search(VectorStore& store,const vector<float>& query,int k,int nprobe,int& scanned_out) {
    lock_guard<mutex> lock(store.mtx);
    IVFIndex& idx = store.index;

    using Pair = pair<float, int>;
    priority_queue<Pair> centroid_heap;
 
    for (int c = 0; c < idx.K; c++) {
        float d = distance_sq(query.data(),idx.centroids.data() + c * idx.dim,idx.dim);
        if ((int)centroid_heap.size() < nprobe) {
            centroid_heap.push({d, c});
        } else if (d < centroid_heap.top().first) {
            centroid_heap.pop();
            centroid_heap.push({d, c});
        }
    }
 
    vector<int> chosen;
    while (!centroid_heap.empty()) {
        chosen.push_back(centroid_heap.top().second);
        centroid_heap.pop();
    }

    priority_queue<Pair> result_heap;
    int scanned = 0;
 
    for (int c : chosen) {
        for (int internal : idx.cluster_lists[c]) {
            float d = distance_sq(query.data(),store.vectors.data() + internal * store.dim,store.dim);
            scanned++;
            if ((int)result_heap.size() < k) {
                result_heap.push({d, internal});
            } else if (d < result_heap.top().first) {
                result_heap.pop();
                result_heap.push({d, internal});
            }
        }
    }
    scanned_out = scanned;
 
    vector<SearchResult> results(result_heap.size());
    int pos = result_heap.size() - 1;
    while (!result_heap.empty()) {
        auto [d, internal] = result_heap.top();
        result_heap.pop();
        SearchResult r;
        r.id   = store.ids[internal];
        r.dist = d;
        r.vec  = vector<float>(store.vectors.begin() + internal * store.dim,store.vectors.begin() + internal * store.dim + store.dim);
        results[pos--] = r;
    }
    return results;
}