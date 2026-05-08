#ifndef SEARCHRESULT_H
#define SEARCHRESULT_H
#include "VectorStore.h"
#include <vector>
#include <queue>
using namespace std;
struct SearchResult {
    int64_t id;
    float dist;
    vector<float> vec;
};

float distance_sq(const float* a, const float* b, int D);

vector<SearchResult> brute_search(VectorStore& store,const vector<float>& query,int k,int& scanned_out);

#endif