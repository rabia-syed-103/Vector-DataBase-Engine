#include <iostream>
#include <cassert>
#include <string>
#include "VectorStore.h"
#include "protocol.h"
#include "SearchResult.h"
using namespace std;

int main() {
    cout << "\n===distance_sq basic ===" << endl;
    {
        float a[] = {0.0f, 0.0f};
        float b[] = {3.0f, 4.0f};
        float result = distance_sq(a, b, 2);
        cout << "distance_sq([0,0], [3,4]) = " << result << " (expected 25)" << endl;
        assert(result == 25.0f);
        cout << "Passed." << endl;
    }
    VectorStore store;
    store_init(store, 4);

    cout << "=== TEST 1: ADD returns OK ===" << endl;
    string r1 = parse_and_dispatch("ADD 1 0.1 0.2 0.3 0.4", store);
    cout << "Response: " << r1;
    assert(r1 == "OK\n");

    string r2 = parse_and_dispatch("ADD 2 0.15 0.25 0.35 0.45", store);
    assert(r2 == "OK\n");

    string r3 = parse_and_dispatch("ADD 3 0.90 0.10 0.10 0.20", store);
    assert(r3 == "OK\n");

    string r4 = parse_and_dispatch("ADD 4 0.85 0.15 0.05 0.25", store);
    assert(r4 == "OK\n");

    string r5 = parse_and_dispatch("ADD 5 0.50 0.50 0.50 0.50", store);
    assert(r5 == "OK\n");

    cout << "All ADDs returned OK." << endl;

    cout << "\n=== TEST 2: ADD with wrong component count ===" << endl;
    string bad = parse_and_dispatch("ADD 99 0.1 0.2", store);
    cout << "Response: " << bad;
    assert(bad.find("ERROR") != string::npos);
    cout << "Correctly returned error." << endl;

    cout << "\n=== TEST 3: SEARCH BRUTE ===" << endl;
    string s1 = parse_and_dispatch("SEARCH 0.12 0.22 0.32 0.42 3 BRUTE", store);
    cout << s1;
    assert(s1.find("mode=BRUTE") != string::npos);
    assert(s1.find("scanned 5")  != string::npos);
    cout << "SEARCH BRUTE looks correct." << endl;

    cout << "\n=== TEST 4: STATS ===" << endl;
    string stats = parse_and_dispatch("STATS", store);
    cout << stats;
    assert(stats.find("dimension: 4")    != string::npos);
    assert(stats.find("total vectors: 5") != string::npos);
    assert(stats.find("index built: no")  != string::npos);
    cout << "STATS correct." << endl;

    cout << "\n=== TEST 5: QUIT ===" << endl;
    string q = parse_and_dispatch("QUIT", store);
    cout << "Response: " << q << endl;
    assert(q == "QUIT");
    cout << "QUIT correct." << endl;

    cout << "\n=== TEST 6: Unknown command ===" << endl;
    string unk = parse_and_dispatch("HELLO", store);
    cout << "Response: " << unk;
    assert(unk.find("ERROR") != string::npos);
    cout << "Unknown command handled correctly." << endl;

    cout << "\n=== TEST 7: Overwrite existing ID ===" << endl;
    string ow = parse_and_dispatch("ADD 1 9.9 9.9 9.9 9.9", store);
    assert(ow == "OK\n");
    string stats2 = parse_and_dispatch("STATS", store);
    assert(stats2.find("total vectors: 5") != string::npos);
    cout << "Overwrite kept count at 5. Correct." << endl;

    cout << "\n=== ALL TESTS PASSED ===" << endl;
    return 0;
}