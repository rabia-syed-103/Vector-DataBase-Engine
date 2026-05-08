#ifndef IVF_INDEX_H
#define IVF_INDEX_H

#include <vector>

using namespace std;

struct IVFIndex {
    int K   = 0;    
    int dim = 0;   


    vector<float> centroids;
    vector<vector<int>> cluster_lists;
};

int ivf_build(IVFIndex& idx,const vector<float>& vectors,int N, int D);
int nearest_centroid(const IVFIndex& idx, const float* vec);


#endif 