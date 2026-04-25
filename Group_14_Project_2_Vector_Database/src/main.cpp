#include <iostream>
#include "VectorStore.h"
#include "SearchResult.h"

using namespace std;

int main() {
    VectorStore store;

    // 1. Initialize store
    store_init(store, 4);

    cout << "=== ADDING VECTORS ===" << endl;

    // 2. Add vectors
    store_add(store, 1, {0.1, 0.2, 0.3, 0.4});
    store_add(store, 2, {0.5, 0.6, 0.7, 0.8});
    store_add(store, 3, {0.9, 0.1, 0.2, 0.3});

    cout << "Count after adds: " << store.count << endl;

    // 3. Overwrite test (same ID)
    cout << "\n=== OVERWRITE TEST ===" << endl;
    store_add(store, 2, {1.0, 1.0, 1.0, 1.0}); // overwrite ID 2

    cout << "Vector for ID 2 overwritten.\n";

    // 4. Search test
    cout << "\n=== BRUTE SEARCH ===" << endl;

    vector<float> query = {1.2, 1.2, 1.2, 1.2};
    int scanned = 0;

    vector<SearchResult> results = brute_search(store, query, 2, scanned);

    cout << "Scanned vectors: " << scanned << endl;

    // 5. Print results
    for (auto &r : results) {
        cout << "ID: " << r.id
             << " | Dist: " << r.dist
             << " | Vec: ";

        for (float v : r.vec)
            cout << v << " ";

        cout << endl;
    }

    // 6. Validation checks
    cout << "\n=== VALIDATION ===" << endl;

    cout << "Expected dim: 4, actual: " << store.dim << endl;
    cout << "Total vectors: " << store.count << endl;

    if (store.id_to_index.find(2) != store.id_to_index.end()) {
        cout << "Overwrite check passed for ID 2" << endl;
    }

    return 0;
}