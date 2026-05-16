#include <iostream>
#include <cassert>
#include <cstring>
#include <vector>
#include "../src/VectorStore.h"
#include "../src/persistence.h"
#include "../src/IVFIndex.h"
#include "../src/SearchResult.h"

#define TEST(name, cond) \
    do { \
        if (cond) std::cout << "PASS: " << name << "\n"; \
        else      std::cout << "FAIL: " << name << "\n"; \
    } while(0)


void test_save_load_no_index() {
    std::cout << "\n--- Test 1: Save/Load without IVF index ---\n";

    VectorStore store;
    store_init(store, 4);

    store_add(store, 1, {0.10f, 0.20f, 0.30f, 0.40f});
    store_add(store, 2, {0.15f, 0.25f, 0.35f, 0.45f});
    store_add(store, 3, {0.90f, 0.10f, 0.10f, 0.20f});
    store_add(store, 4, {0.85f, 0.15f, 0.05f, 0.25f});
    store_add(store, 5, {0.50f, 0.50f, 0.50f, 0.50f});

    bool saved = store_save(store, "/tmp");
    TEST("save returns true", saved);

    VectorStore loaded;
    store_init(loaded, 4);
    bool ok = store_load(loaded, "/tmp");
    TEST("load returns true", ok);

    TEST("count matches",     loaded.count == 5);
    TEST("dim matches",       loaded.dim   == 4);
    TEST("index not built",   loaded.index_built == false);

    TEST("id 1 in hashmap",   loaded.id_to_index.count(1) > 0);
    TEST("id 2 in hashmap",   loaded.id_to_index.count(2) > 0);
    TEST("id 5 in hashmap",   loaded.id_to_index.count(5) > 0);

    int idx = loaded.id_to_index[1];
    TEST("vector 1 component 0",
         std::abs(loaded.vectors[idx*4+0] - 0.10f) < 1e-6f);
    TEST("vector 1 component 1",
         std::abs(loaded.vectors[idx*4+1] - 0.20f) < 1e-6f);
    TEST("vector 1 component 2",
         std::abs(loaded.vectors[idx*4+2] - 0.30f) < 1e-6f);
    TEST("vector 1 component 3",
         std::abs(loaded.vectors[idx*4+3] - 0.40f) < 1e-6f);

    int idx3 = loaded.id_to_index[3];
    TEST("vector 3 component 0",
         std::abs(loaded.vectors[idx3*4+0] - 0.90f) < 1e-6f);
}

void test_save_load_with_index() {
    std::cout << "\n--- Test 2: Save/Load with IVF index ---\n";

    VectorStore store;
    store_init(store, 4);

    store_add(store, 1,  {0.10f, 0.10f, 0.10f, 0.10f});
    store_add(store, 2,  {0.12f, 0.11f, 0.10f, 0.10f});
    store_add(store, 3,  {0.11f, 0.12f, 0.10f, 0.10f});
    store_add(store, 4,  {0.90f, 0.90f, 0.90f, 0.90f});
    store_add(store, 5,  {0.91f, 0.89f, 0.90f, 0.90f});
    store_add(store, 6,  {0.89f, 0.91f, 0.90f, 0.90f});
    store_add(store, 7,  {0.50f, 0.50f, 0.50f, 0.50f});
    store_add(store, 8,  {0.51f, 0.49f, 0.50f, 0.50f});
    store_add(store, 9,  {0.49f, 0.51f, 0.50f, 0.50f});

    srand(42);
    ivf_build(store.index, store.vectors, store.count, store.dim);
    store.index_built = true;

    int original_K = store.index.K;

    int original_total = 0;
    for (int c = 0; c < store.index.K; c++)
        original_total += store.index.cluster_lists[c].size();

    bool saved = store_save(store, "/tmp");
    TEST("save with index returns true", saved);

    VectorStore loaded;
    store_init(loaded, 4);
    bool ok = store_load(loaded, "/tmp");
    TEST("load with index returns true", ok);

    TEST("count matches",       loaded.count == 9);
    TEST("index_built is true", loaded.index_built == true);
    TEST("K matches",           loaded.index.K == original_K);
    TEST("index dim matches",   loaded.index.dim == 4);

    TEST("centroids size = K*D",
         (int)loaded.index.centroids.size() == original_K * 4);

    int loaded_total = 0;
    for (int c = 0; c < loaded.index.K; c++)
        loaded_total += loaded.index.cluster_lists[c].size();
    TEST("cluster lists total = N",
         loaded_total == 9);
    TEST("cluster lists total matches original",
         loaded_total == original_total);

    bool centroids_match = true;
    for (int i = 0; i < original_K * 4; i++) {
        if (std::abs(store.index.centroids[i] -
                     loaded.index.centroids[i]) > 1e-6f) {
            centroids_match = false;
            break;
        }
    }
    TEST("centroid values match exactly", centroids_match);

    bool lists_match = true;
    for (int c = 0; c < original_K; c++) {
        if (store.index.cluster_lists[c].size() !=
            loaded.index.cluster_lists[c].size()) {
            lists_match = false;
            break;
        }
        for (int i = 0;
             i < (int)store.index.cluster_lists[c].size(); i++) {
            if (store.index.cluster_lists[c][i] !=
                loaded.index.cluster_lists[c][i]) {
                lists_match = false;
                break;
            }
        }
    }
    TEST("cluster list contents match exactly", lists_match);
}

void test_search_after_roundtrip() {
    std::cout << "\n--- Test 3: Search results identical after round-trip ---\n";

    VectorStore store;
    store_init(store, 4);

    store_add(store, 1, {0.10f, 0.20f, 0.30f, 0.40f});
    store_add(store, 2, {0.15f, 0.25f, 0.35f, 0.45f});
    store_add(store, 3, {0.90f, 0.10f, 0.10f, 0.20f});
    store_add(store, 4, {0.85f, 0.15f, 0.05f, 0.25f});
    store_add(store, 5, {0.50f, 0.50f, 0.50f, 0.50f});

    std::vector<float> query = {0.12f, 0.22f, 0.32f, 0.42f};
    int scanned1 = 0;
    auto results1 = brute_search(store, query, 3, scanned1);

    store_save(store, "/tmp");
    VectorStore loaded;
    store_init(loaded, 4);
    store_load(loaded, "/tmp");

    int scanned2 = 0;
    auto results2 = brute_search(loaded, query, 3, scanned2);

    TEST("same number of results",
         results1.size() == results2.size());

    bool ids_match = true;
    bool dists_match = true;
    for (int i = 0; i < (int)results1.size(); i++) {
        if (results1[i].id != results2[i].id)
            ids_match = false;
        if (std::abs(results1[i].dist - results2[i].dist) > 1e-5f)
            dists_match = false;
    }
    TEST("result ids match after round-trip",   ids_match);
    TEST("result dists match after round-trip", dists_match);
}

void test_corrupt_file_rejected() {
    std::cout << "\n--- Test 4: Corrupt file rejected ---\n";

    FILE* f = fopen("/tmp/snapshot.vdb", "wb");
    const char bad[4] = {'X','X','X','X'};
    fwrite(bad, 1, 4, f);
    fclose(f);

    VectorStore store;
    store_init(store, 4);
    bool ok = store_load(store, "/tmp");
    TEST("corrupt magic bytes rejected", !ok);
}

void test_empty_store() {
    std::cout << "\n--- Test 5: Empty store round-trip ---\n";

    VectorStore store;
    store_init(store, 4);

    bool saved = store_save(store, "/tmp");
    TEST("empty store saves", saved);

    VectorStore loaded;
    store_init(loaded, 4);
    bool ok = store_load(loaded, "/tmp");
    TEST("empty store loads",          ok);
    TEST("count is 0 after load",      loaded.count == 0);
    TEST("index not built after load", !loaded.index_built);
}

int main() {
    std::cout << "=== Persistence Round-Trip Tests ===\n";

    test_save_load_no_index();
    test_save_load_with_index();
    test_search_after_roundtrip();
    test_corrupt_file_rejected();
    test_empty_store();

    std::cout << "\n=== Done ===\n";
    return 0;
}