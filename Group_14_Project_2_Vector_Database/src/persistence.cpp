#include "persistence.h"
#include "IVFIndex.h"
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
using namespace std;

static bool write_bytes(FILE* f, const void* data, size_t n) {
    if (n == 0) return true;
    return fwrite(data, 1, n, f) == n;
}

static bool read_bytes(FILE* f, void* data, size_t n) {
    if (n == 0) return true;
    return fread(data, 1, n, f) == n;
}

bool store_save(const VectorStore& store, const string& data_dir) {

    string tmp_path   = data_dir + "/snapshot.vdb.tmp";
    string final_path = data_dir + "/snapshot.vdb";
    long   file_size  = 0;

    const char magic[4] = {'V', 'D', 'B', '1'};
    uint32_t version = 1;
    uint32_t dim     = (uint32_t)store.dim;
    uint32_t N       = (uint32_t)store.count;
    uint32_t K       = store.index_built ? (uint32_t)store.index.K : 0;

    FILE* f = fopen(tmp_path.c_str(), "wb");
    if (!f) {
        cerr << "ERROR: cannot open " << tmp_path << " for writing\n";
        return false;
    }

    if (!write_bytes(f, magic,    4)) goto fail;
    if (!write_bytes(f, &version, 4)) goto fail;
    if (!write_bytes(f, &dim,     4)) goto fail;
    if (!write_bytes(f, &N,       4)) goto fail;
    if (!write_bytes(f, &K,       4)) goto fail;

    if (N > 0)
        if (!write_bytes(f, store.ids.data(), N * sizeof(int64_t)))
            goto fail;

    if (N > 0)
        if (!write_bytes(f, store.vectors.data(),
                         N * store.dim * sizeof(float)))
            goto fail;

    if (store.index_built && K > 0) {
        if (!write_bytes(f, store.index.centroids.data(),
                         K * store.dim * sizeof(float)))
            goto fail;

        for (int c = 0; c < store.index.K; c++) {
            uint32_t sz = (uint32_t)store.index.cluster_lists[c].size();
            if (!write_bytes(f, &sz, 4)) goto fail;

            for (int idx : store.index.cluster_lists[c]) {
                uint32_t u = (uint32_t)idx;
                if (!write_bytes(f, &u, 4)) goto fail;
            }
        }
    }

    file_size = ftell(f);
    fclose(f);

    if (rename(tmp_path.c_str(), final_path.c_str()) != 0) {
        cerr << "ERROR: rename failed\n";
        return false;
    }

    cout << "Saved " << N << " vectors";
    if (store.index_built) cout << " and IVF index";
    cout << " to " << final_path
         << " (" << file_size << " bytes).\n";
    return true;

fail:
    fclose(f);
    cerr << "ERROR: write failed during SAVE\n";
    return false;
}

bool store_load(VectorStore& store, const string& data_dir) {

    string path = data_dir + "/snapshot.vdb";

    char     magic[4];
    uint32_t version = 0;
    uint32_t dim     = 0;
    uint32_t N       = 0;
    uint32_t K       = 0;

    FILE* f = fopen(path.c_str(), "rb");
    if (!f) return false;  

    if (!read_bytes(f, magic, 4)) goto fail;
    if (magic[0] != 'V' || magic[1] != 'D' ||
        magic[2] != 'B' || magic[3] != '1') {
        cerr << "ERROR: invalid magic bytes in snapshot\n";
        goto fail;
    }

    if (!read_bytes(f, &version, 4)) goto fail;
    if (version != 1) {
        cerr << "ERROR: unsupported version: " << version << "\n";
        goto fail;
    }

    if (!read_bytes(f, &dim, 4)) goto fail;
    if (!read_bytes(f, &N,   4)) goto fail;
    if (!read_bytes(f, &K,   4)) goto fail;

    store.vectors.clear();
    store.ids.clear();
    store.id_to_index.clear();
    store.count       = 0;
    store.dim         = (int)dim;
    store.index_built = false;
    store.index       = IVFIndex();

    if (N > 0) {
        store.ids.resize(N);
        if (!read_bytes(f, store.ids.data(), N * sizeof(int64_t)))
            goto fail;
    }

    if (N > 0) {
        store.vectors.resize(N * dim);
        if (!read_bytes(f, store.vectors.data(),
                        N * dim * sizeof(float)))
            goto fail;
    }

    for (uint32_t i = 0; i < N; i++)
        store.id_to_index[store.ids[i]] = (int)i;
    store.count = (int)N;

    if (K > 0) {
        store.index.K   = (int)K;
        store.index.dim = (int)dim;

        store.index.centroids.resize(K * dim);
        if (!read_bytes(f, store.index.centroids.data(),
                        K * dim * sizeof(float)))
            goto fail;

        store.index.cluster_lists.resize(K);
        for (uint32_t c = 0; c < K; c++) {
            uint32_t sz = 0;
            if (!read_bytes(f, &sz, 4)) goto fail;

            store.index.cluster_lists[c].resize(sz);
            for (uint32_t i = 0; i < sz; i++) {
                uint32_t u = 0;
                if (!read_bytes(f, &u, 4)) goto fail;
                store.index.cluster_lists[c][i] = (int)u;
            }
        }

        store.index_built = true;
    }

    fclose(f);

    cout << "Loaded " << N << " vectors";
    if (K > 0) cout << " and IVF index (" << K << " clusters)";
    cout << " from " << path << "\n";
    return true;

fail:
    fclose(f);
    cerr << "ERROR: read failed during LOAD\n";
    return false;
}