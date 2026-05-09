#include <iostream>
#include <vector>
#include <cassert>
#include <cstdlib>
#include "../src/VectorStore.h"
#include "../src/SearchResult.h"
#include "../src/IVFIndex.h"

using namespace std;

#define TEST(name, condition) \
    do { \
        if (condition) cout << "PASS: " << name << "\n"; \
        else           cout << "FAIL: " << name << "\n"; \
    } while(0)

// ── Test 1: ivf_search returns same results as brute_search with high nprobe ─
void test_ivf_matches_brute() {
    VectorStore store;
    store_init(store, 4);

    store_add(store, 1, {0.10f, 0.20f, 0.30f, 0.40f});
    store_add(store, 2, {0.15f, 0.25f, 0.35f, 0.45f});
    store_add(store, 3, {0.90f, 0.10f, 0.10f, 0.20f});
    store_add(store, 4, {0.85f, 0.15f, 0.05f, 0.25f});
    store_add(store, 5, {0.50f, 0.50f, 0.50f, 0.50f});

    srand(42);
    // build index: K = floor(sqrt(5)) = 2
    ivf_build(store.index, store.vectors, store.count, store.dim);
    store.index_built = true;

    vector<float> query = {0.12f, 0.22f, 0.32f, 0.42f};

    int brute_scanned = 0;
    auto brute = brute_search(store, query, 3, brute_scanned);

    // nprobe = K means visit ALL clusters = same as brute force
    int ivf_scanned = 0;
    auto ivf = ivf_search(store, query, 3, store.index.K, ivf_scanned);

    TEST("ivf returns same number of results as brute",
         ivf.size() == brute.size());

    // check same ids in same order
    bool same = true;
    for (int i = 0; i < (int)brute.size(); i++)
        if (brute[i].id != ivf[i].id) same = false;
    TEST("ivf with nprobe=K returns same ids as brute", same);

    cout << "\n  Brute results:  ";
    for (auto& r : brute) cout << "id=" << r.id << " dist=" << r.dist << "  ";
    cout << "\n  IVF results:    ";
    for (auto& r : ivf)   cout << "id=" << r.id << " dist=" << r.dist << "  ";
    cout << "\n";
}

// ── Test 2: ivf_search scans fewer vectors than brute with nprobe=1 ──────────
void test_ivf_scans_fewer() {
    VectorStore store;
    store_init(store, 4);

    store_add(store, 1, {0.10f, 0.20f, 0.30f, 0.40f});
    store_add(store, 2, {0.15f, 0.25f, 0.35f, 0.45f});
    store_add(store, 3, {0.90f, 0.10f, 0.10f, 0.20f});
    store_add(store, 4, {0.85f, 0.15f, 0.05f, 0.25f});
    store_add(store, 5, {0.50f, 0.50f, 0.50f, 0.50f});

    srand(42);
    ivf_build(store.index, store.vectors, store.count, store.dim);
    store.index_built = true;

    vector<float> query = {0.12f, 0.22f, 0.32f, 0.42f};

    int brute_scanned = 0;
    brute_search(store, query, 3, brute_scanned);

    int ivf_scanned = 0;
    ivf_search(store, query, 3, 1, ivf_scanned);

    TEST("ivf with nprobe=1 scans fewer vectors than brute",
         ivf_scanned < brute_scanned);

    cout << "  Brute scanned: " << brute_scanned << "\n";
    cout << "  IVF scanned:   " << ivf_scanned   << "\n";
}

// ── Test 3: results are sorted ascending by distance ─────────────────────────
void test_results_sorted() {
    VectorStore store;
    store_init(store, 4);

    store_add(store, 1, {0.10f, 0.20f, 0.30f, 0.40f});
    store_add(store, 2, {0.50f, 0.50f, 0.50f, 0.50f});
    store_add(store, 3, {0.90f, 0.90f, 0.90f, 0.90f});
    store_add(store, 4, {0.12f, 0.22f, 0.32f, 0.42f});

    srand(42);
    ivf_build(store.index, store.vectors, store.count, store.dim);
    store.index_built = true;

    vector<float> query = {0.11f, 0.21f, 0.31f, 0.41f};
    int scanned = 0;
    auto results = ivf_search(store, query, 3, store.index.K, scanned);

    bool sorted = true;
    for (int i = 1; i < (int)results.size(); i++)
        if (results[i].dist < results[i-1].dist) sorted = false;

    TEST("ivf results sorted ascending by distance", sorted);
}

// ── Test 4: incremental insert after BUILD ────────────────────────────────────
void test_incremental_insert() {
    VectorStore store;
    store_init(store, 4);

    store_add(store, 1, {0.10f, 0.20f, 0.30f, 0.40f});
    store_add(store, 2, {0.15f, 0.25f, 0.35f, 0.45f});
    store_add(store, 3, {0.90f, 0.10f, 0.10f, 0.20f});
    store_add(store, 4, {0.85f, 0.15f, 0.05f, 0.25f});

    srand(42);
    ivf_build(store.index, store.vectors, store.count, store.dim);
    store.index_built = true;

    int total_before = 0;
    for (int c = 0; c < store.index.K; c++)
        total_before += store.index.cluster_lists[c].size();

    // add a new vector AFTER build — should go into nearest cluster
    store_add(store, 5, {0.11f, 0.21f, 0.31f, 0.41f});

    int total_after = 0;
    for (int c = 0; c < store.index.K; c++)
        total_after += store.index.cluster_lists[c].size();

    TEST("incremental insert increases cluster list size by 1",
         total_after == total_before + 1);
    TEST("store count increased to 5", store.count == 5);

    cout << "  Cluster sizes before insert: " << total_before << "\n";
    cout << "  Cluster sizes after insert:  " << total_after  << "\n";
}

// ── Test 5: ivf_search finds the exact nearest neighbor ──────────────────────
void test_nearest_found() {
    VectorStore store;
    store_init(store, 2);

    // 2 obvious groups
    store_add(store, 1, {0.10f, 0.10f});
    store_add(store, 2, {0.11f, 0.12f});
    store_add(store, 3, {0.90f, 0.90f});
    store_add(store, 4, {0.91f, 0.89f});

    srand(42);
    ivf_build(store.index, store.vectors, store.count, store.dim);
    store.index_built = true;

    // query very close to group near 0.1
    vector<float> query = {0.10f, 0.11f};
    int scanned = 0;
    // use nprobe=K to guarantee finding it
    auto results = ivf_search(store, query, 1, store.index.K, scanned);

    TEST("nearest neighbor found correctly",
         !results.empty() && (results[0].id == 1 || results[0].id == 2));

    if (!results.empty())
        cout << "  Nearest found: id=" << results[0].id
             << " dist=" << results[0].dist << "\n";
}

int main() {
    cout << "=== IVF Search Tests (Person 2 Phase 2) ===\n\n";

    cout << "-- Test 1: IVF matches brute with nprobe=K --\n";
    test_ivf_matches_brute();
    cout << "\n";

    cout << "-- Test 2: IVF scans fewer vectors with nprobe=1 --\n";
    test_ivf_scans_fewer();
    cout << "\n";

    cout << "-- Test 3: Results sorted ascending --\n";
    test_results_sorted();
    cout << "\n";

    cout << "-- Test 4: Incremental insert after BUILD --\n";
    test_incremental_insert();
    cout << "\n";

    cout << "-- Test 5: Nearest neighbor found --\n";
    test_nearest_found();
    cout << "\n";

    cout << "=== ALL TESTS DONE ===\n";
    return 0;
}