#include <iostream>
#include <vector>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include "../src/IVFIndex.h"
#include "../src/SearchResult.h"


#define TEST(name, condition) \
    do { \
        if (condition) std::cout << "PASS: " << name << "\n"; \
        else           std::cout << "FAIL: " << name << "\n"; \
    } while(0)


void test_K_value() {
    int D = 2;
    int N = 100;
    std::vector<float> vectors(N * D);
    srand(42);
    for (float& f : vectors) f = (float)rand() / RAND_MAX;

    IVFIndex idx;
    ivf_build(idx, vectors, N, D);

    TEST("K = floor(sqrt(100)) = 10", idx.K == 10);
    TEST("dim stored correctly",       idx.dim == D);
}
void test_all_vectors_assigned() {
    int D = 4, N = 50;
    std::vector<float> vectors(N * D);
    srand(42);
    for (float& f : vectors) f = (float)rand() / RAND_MAX;

    IVFIndex idx;
    ivf_build(idx, vectors, N, D);

    int total = 0;
    for (int c = 0; c < idx.K; c++)
        total += idx.cluster_lists[c].size();

    TEST("all vectors assigned to a cluster", total == N);
    TEST("correct number of clusters created",
         (int)idx.cluster_lists.size() == idx.K);
}


void test_centroids_size() {
    int D = 4, N = 36;  // K = floor(sqrt(36)) = 6
    std::vector<float> vectors(N * D);
    srand(1);
    for (float& f : vectors) f = (float)rand() / RAND_MAX;

    IVFIndex idx;
    ivf_build(idx, vectors, N, D);

    TEST("K = floor(sqrt(36)) = 6", idx.K == 6);
    TEST("centroids array size = K*D",
         (int)idx.centroids.size() == idx.K * D);
}


void test_iteration_cap() {
    int D = 8, N = 200;
    std::vector<float> vectors(N * D);
    srand(99);
    for (float& f : vectors) f = (float)rand() / RAND_MAX;

    IVFIndex idx;
    int iters = ivf_build(idx, vectors, N, D);

    TEST("iterations <= 50", iters <= 50);
    TEST("iterations >= 1",  iters >= 1);
}


void test_obvious_clusters() {
    int D = 2;

    int N = 4;
    std::vector<float> vectors = {
        0.10f, 0.10f,   
        0.12f, 0.11f,   
        0.90f, 0.90f,   
        0.91f, 0.89f, 
    };

    srand(42);
    IVFIndex idx;
    ivf_build(idx, vectors, N, D);

    TEST("K=2 for N=4", idx.K == 2);

    bool c0_near_A = (idx.centroids[0] < 0.5f);
    bool c0_near_B = (idx.centroids[0] > 0.5f);
    bool c1_near_A = (idx.centroids[2] < 0.5f);
    bool c1_near_B = (idx.centroids[2] > 0.5f);

    bool separated = (c0_near_A && c1_near_B) ||
                     (c0_near_B && c1_near_A);
    TEST("two obvious groups land in separate clusters", separated);

    int a0 = -1, a1 = -1, a2 = -1, a3 = -1;
    for (int c = 0; c < idx.K; c++) {
        for (int idx_v : idx.cluster_lists[c]) {
            if (idx_v == 0) a0 = c;
            if (idx_v == 1) a1 = c;
            if (idx_v == 2) a2 = c;
            if (idx_v == 3) a3 = c;
        }
    }
    TEST("group A vectors in same cluster", a0 == a1);
    TEST("group B vectors in same cluster", a2 == a3);
    TEST("group A and B in different clusters", a0 != a2);
}

void test_nearest_centroid() {
    int D = 2;
    IVFIndex idx;
    idx.K   = 2;
    idx.dim = D;
    idx.centroids = {
        0.1f, 0.1f,   
        0.9f, 0.9f   
    };
    idx.cluster_lists.resize(2);

    float near_zero[]  = {0.15f, 0.12f};
    float near_one[]   = {0.88f, 0.91f};
    float exactly_mid[] = {0.5f, 0.5f};

    TEST("vector near 0 → centroid 0",
         nearest_centroid(idx, near_zero) == 0);

    TEST("vector near 1 → centroid 1",
         nearest_centroid(idx, near_one) == 1);

    int mid_result = nearest_centroid(idx, exactly_mid);
    TEST("midpoint returns valid centroid index",
         mid_result == 0 || mid_result == 1);
}


void test_no_duplicate_assignment() {
    int D = 4, N = 100;
    std::vector<float> vectors(N * D);
    srand(7);
    for (float& f : vectors) f = (float)rand() / RAND_MAX;

    IVFIndex idx;
    ivf_build(idx, vectors, N, D);

    std::vector<int> seen(N, 0);
    bool duplicate = false;
    for (int c = 0; c < idx.K; c++) {
        for (int v : idx.cluster_lists[c]) {
            if (seen[v]++ > 0) duplicate = true;
        }
    }
    TEST("no vector assigned to two clusters", !duplicate);
}


void test_centroid_range() {
    int D = 4, N = 64;
    std::vector<float> vectors(N * D);
    srand(13);
    for (float& f : vectors) f = (float)rand() / RAND_MAX;

    IVFIndex idx;
    ivf_build(idx, vectors, N, D);

    bool all_in_range = true;
    for (float f : idx.centroids) {
        if (f < 0.0f || f > 1.0f) {
            all_in_range = false;
            break;
        }
    }
    TEST("all centroid values within data range [0,1]", all_in_range);
}


int main() {
    std::cout << "=== k-means / IVFIndex Tests ===\n\n";

    test_K_value();
    std::cout << "\n";

    test_all_vectors_assigned();
    std::cout << "\n";

    test_centroids_size();
    std::cout << "\n";

    test_iteration_cap();
    std::cout << "\n";

    test_obvious_clusters();
    std::cout << "\n";

    test_nearest_centroid();
    std::cout << "\n";

    test_no_duplicate_assignment();
    std::cout << "\n";

    test_centroid_range();
    std::cout << "\n";

    std::cout << "=== Done ===\n";
    return 0;
}