#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>
#include "../src/IVFIndex.h"
#include "../src/SearchResult.h"

int main() {

    // -------------------------------------------------------
    // We create 12 vectors in 2D space that form 3 obvious
    // groups that a human can see:
    //
    //   Group A (bottom-left):  near [0.1, 0.1]
    //   Group B (top-right):    near [0.9, 0.9]
    //   Group C (middle):       near [0.5, 0.5]
    //
    // After k-means with K = floor(sqrt(12)) = 3,
    // we expect one centroid near each group.
    // -------------------------------------------------------

    int D = 2;
    int N = 12; // K will be floor(sqrt(12)) = 3

    std::vector<float> vectors = {
        // Group A — bottom left
        0.10f, 0.10f,
        0.12f, 0.11f,
        0.09f, 0.13f,
        0.11f, 0.09f,

        // Group B — top right
        0.90f, 0.90f,
        0.92f, 0.88f,
        0.89f, 0.91f,
        0.91f, 0.92f,

        // Group C — middle
        0.50f, 0.50f,
        0.52f, 0.49f,
        0.48f, 0.51f,
        0.51f, 0.48f,
    };

    srand(42);
    IVFIndex idx;
    int iters = ivf_build(idx, vectors, N, D);

    // -------------------------------------------------------
    // Print what k-means produced
    // -------------------------------------------------------
    std::cout << "==============================\n";
    std::cout << "  k-means Visual Result\n";
    std::cout << "==============================\n\n";

    std::cout << "Input: " << N << " vectors in "
              << D << "D space\n";
    std::cout << "K = floor(sqrt(" << N << ")) = "
              << idx.K << " clusters\n";
    std::cout << "Converged in " << iters << " iterations\n\n";

    // -------------------------------------------------------
    // Print each cluster: its centroid and which vectors
    // ended up in it, with their actual coordinates
    // -------------------------------------------------------
    std::cout << "--- Clusters ---\n\n";

    for (int c = 0; c < idx.K; c++) {
        // print centroid
        std::cout << "Cluster " << c << ":\n";
        std::cout << "  Centroid = [";
        for (int d = 0; d < D; d++) {
            std::cout << idx.centroids[c * D + d];
            if (d < D-1) std::cout << ", ";
        }
        std::cout << "]\n";

        // print which vectors are in this cluster
        std::cout << "  Contains " 
                  << idx.cluster_lists[c].size() 
                  << " vectors:\n";

        for (int internal : idx.cluster_lists[c]) {
            std::cout << "    vector[" << internal << "] = [";
            for (int d = 0; d < D; d++) {
                std::cout << vectors[internal * D + d];
                if (d < D-1) std::cout << ", ";
            }
            std::cout << "]\n";
        }
        std::cout << "\n";
    }

    // -------------------------------------------------------
    // Print nearest_centroid results for 3 test points
    // We know exactly which cluster each should land in
    // -------------------------------------------------------
    std::cout << "--- nearest_centroid() results ---\n\n";

    struct TestPoint { float x, y; const char* label; };
    TestPoint points[] = {
        {0.10f, 0.10f, "near Group A (bottom-left)"},
        {0.90f, 0.90f, "near Group B (top-right)"},
        {0.50f, 0.50f, "near Group C (middle)"},
    };

    for (auto& p : points) {
        float vec[] = {p.x, p.y};
        int c = nearest_centroid(idx, vec);
        std::cout << "Query [" << p.x << ", " << p.y << "]"
                  << " (" << p.label << ")\n";
        std::cout << "  → assigned to Cluster " << c
                  << " with centroid ["
                  << idx.centroids[c * D]     << ", "
                  << idx.centroids[c * D + 1] << "]\n\n";
    }

    // -------------------------------------------------------
    // Show the speed/accuracy idea visually
    // 
    // With nprobe=1: only search 1 cluster (~4 vectors)
    // With nprobe=3: search all clusters (all 12 vectors)
    // This is the whole point of IVF
    // -------------------------------------------------------
    std::cout << "--- Speed vs Accuracy Preview ---\n\n";
    std::cout << "Total vectors: " << N << "\n";
    std::cout << "Average cluster size: " << N / idx.K << "\n\n";

    std::cout << "nprobe=1 → scans ~" << N/idx.K 
              << " vectors (1 cluster)\n";
    std::cout << "nprobe=2 → scans ~" << 2*(N/idx.K) 
              << " vectors (2 clusters)\n";
    std::cout << "nprobe=3 → scans ~" << N 
              << " vectors (all clusters = same as BRUTE)\n\n";

    std::cout << "Speedup with nprobe=1 vs BRUTE: ~" 
              << idx.K << "x\n";

    return 0;
}