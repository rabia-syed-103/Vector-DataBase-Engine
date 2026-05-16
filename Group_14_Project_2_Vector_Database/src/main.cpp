#include "persistence.h"
#include "VectorStore.h"
#include "server.h"        // ← ADD THIS
#include <iostream>        // ← ADD THIS
#include <string>

int main(int argc, char* argv[]) {
    int         port     = 5556;
    int         dim      = 128;
    std::string data_dir = "./vdata";

    for (int i = 1; i < argc; i++) {
        if (std::string(argv[i]) == "--port") port     = std::stoi(argv[++i]);
        if (std::string(argv[i]) == "--dim")  dim      = std::stoi(argv[++i]);
        if (std::string(argv[i]) == "--data") data_dir = argv[++i];
    }

    std::string mkdir_cmd = "mkdir -p " + data_dir;
    system(mkdir_cmd.c_str());

    VectorStore store;
    store_init(store, dim);

    if (store_load(store, data_dir))
        std::cout << "auto-loaded snapshot from " << data_dir << "\n";

    std::cout << "vdb started on port " << port
              << ", dimension "         << dim
              << ", data directory "    << data_dir << "\n";

    start_server(port, store, data_dir);
    return 0;
}