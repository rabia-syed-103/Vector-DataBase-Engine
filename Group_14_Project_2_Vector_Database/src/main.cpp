#include <iostream>
#include <string>
#include "VectorStore.h"
#include "server.h"

using namespace std;

int main(int argc, char* argv[]) {
    int dim         = 128;
    int port        = 5556;
    string data_dir = "./vdata";

    for (int i = 1; i < argc; i++) {
        if (string(argv[i]) == "--dim"  && i + 1 < argc) dim      = stoi(argv[++i]);
        if (string(argv[i]) == "--port" && i + 1 < argc) port     = stoi(argv[++i]);
        if (string(argv[i]) == "--data" && i + 1 < argc) data_dir = argv[++i];
    }

    VectorStore store;
    store_init(store, dim);

    cout << "vdb started on port " << port
         << ", dimension " << dim
         << ", data directory " << data_dir << "\n";
    cout.flush();

    start_server(port, store);
    return 0;
}