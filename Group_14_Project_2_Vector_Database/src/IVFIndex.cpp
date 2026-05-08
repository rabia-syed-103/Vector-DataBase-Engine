#include "IVFIndex.h"
#include "SearchResult.h"  
#include <cmath>
#include <cstdlib>
#include <numeric>
#include <algorithm>
#include <limits>


int nearest_centroid(const IVFIndex& idx, const float* vec) {
    int   best_c    = 0;
    float best_dist = std::numeric_limits<float>::max();

    for (int c = 0; c < idx.K; c++) {
        float d = distance_sq(vec,idx.centroids.data() + c * idx.dim,idx.dim);
        if (d < best_dist) {
            best_dist = d;
            best_c    = c;
        }
    }
    return best_c;
}

int ivf_build(IVFIndex& idx,const std::vector<float>& vectors,int N, int D) {

    idx.dim = D;
    idx.K   = (int)std::floor(std::sqrt((double)N));

    vector<int> indices(N);
    std::iota(indices.begin(), indices.end(), 0);

    for (int i = 0; i < idx.K; i++) {
        int j = i + rand() % (N - i);
        std::swap(indices[i], indices[j]);
    }

    idx.centroids.resize(idx.K * D);
    for (int c = 0; c < idx.K; c++) {
        int src_vector = indices[c];
        for (int d = 0; d < D; d++)
            idx.centroids[c * D + d] = vectors[src_vector * D + d];
    }

    vector<int> assignment(N, -1);

    int iterations = 0;

    for (int iter = 0; iter < 50; iter++) {
        iterations++;
        bool changed = false;
        for (int i = 0; i < N; i++) {
            int c = nearest_centroid(idx, vectors.data() + i * D);
            if (c != assignment[i]) {
                assignment[i] = c;
                changed = true;
            }
        }
        std::fill(idx.centroids.begin(), idx.centroids.end(), 0.0f);
        std::vector<int> cnt(idx.K, 0);

        for (int i = 0; i < N; i++) {
            int c = assignment[i];
            cnt[c]++;
            for (int d = 0; d < D; d++)
                idx.centroids[c * D + d] += vectors[i * D + d];
        }

        for (int c = 0; c < idx.K; c++) {
            if (cnt[c] > 0) {
                for (int d = 0; d < D; d++)
                    idx.centroids[c * D + d] /= (float)cnt[c];
            }
           
        }
        if (!changed) break;
    }
    idx.cluster_lists.assign(idx.K, std::vector<int>());
    for (int i = 0; i < N; i++)
        idx.cluster_lists[assignment[i]].push_back(i);

    return iterations;
}