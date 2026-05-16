#include "server.h"
#include "protocol.h"
#include <iostream>
#include <string>
#include <thread>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstdio>

using namespace std;

static void handle_client(int client_fd, VectorStore& store,string data_dir) {
    FILE* f = fdopen(client_fd, "r+");
    if (!f) { close(client_fd); return; }
    char line[65536];

    while (fgets(line, sizeof(line), f)) {
        string cmd(line);
        if (!cmd.empty() && cmd.back() == '\n') cmd.pop_back();
        if (!cmd.empty() && cmd.back() == '\r') cmd.pop_back();
        if (cmd.empty()) continue;

        string response = parse_and_dispatch(cmd, store,data_dir);

        if (response == "QUIT") {
            fputs("BYE\n", f);
            fflush(f);
            break;
        }

        fputs(response.c_str(), f);
        fflush(f);
    }

    fclose(f);
}

void start_server(int port, VectorStore& store,string& data_dir) {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(port);

    bind(server_fd, (sockaddr*)&addr, sizeof(addr));
    listen(server_fd, 10);

    cout << "Waiting for connections...\n";
    cout.flush();

    while (true) {
        int client_fd = accept(server_fd, nullptr, nullptr);
        if (client_fd < 0) continue;
       thread(handle_client, client_fd,ref(store), data_dir).detach();
    }
}