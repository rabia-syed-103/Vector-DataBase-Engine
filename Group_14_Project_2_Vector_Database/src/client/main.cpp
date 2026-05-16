#include "server.h"
#include "VectorStore.h"
#include "persistence.h"

using namespace std;

int main(int argc, char* argv[]) {
    int port = 5556;
    int dim = 128;
    string data_dir = "./vdata";

    for (int i = 1; i < argc; i++) {
        string arg = argv[i];

        if (arg == "--port" && i + 1 < argc) {
            port = stoi(argv[++i]);
        }
        else if (arg == "--dim" && i + 1 < argc) {
            dim = stoi(argv[++i]);
        }
        else if (arg == "--data" && i + 1 < argc) {
            data_dir = argv[++i];
        }
    }

    string mkdir_cmd = "mkdir -p " + data_dir;
    system(mkdir_cmd.c_str());

    VectorStore store(dim);

    if (loadSnapshot(store, data_dir)) {
        cout << "auto-loaded snapshot from "
             << data_dir << endl;
    }

    cout << "vdb started on port "
         << port
         << ", dimension "
         << store.dimension
         << ", data directory "
         << data_dir
         << endl;

    startServer(port, store, data_dir);

    return 0;
}