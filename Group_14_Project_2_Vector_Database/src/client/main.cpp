#include <iostream>
#include <string>
#include <cstdio>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

using namespace std;

int main(int argc, char* argv[]) {
    if (argc < 3) {
        cerr << "Usage: vdb-cli <host> <port>\n";
        return 1;
    }

    const char* host = argv[1];
    int port = stoi(argv[2]);

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(port);
    inet_pton(AF_INET, host, &addr.sin_addr);

    if (connect(sock, (sockaddr*)&addr, sizeof(addr)) < 0) {
        cerr << "ERROR: cannot connect to " << host << ":" << port << "\n";
        return 1;
    }

    cout << "connected to vdb at " << host << ":" << port << "\n";

    FILE* f = fdopen(sock, "r+");
    char line[65536];

    while (true) {
        cout << "> ";
        cout.flush();

        if (!fgets(line, sizeof(line), stdin)) break;

        fputs(line, f);
        fflush(f);

        string sent(line);
        if (!sent.empty() && sent.back() == '\n') sent.pop_back();

        while (fgets(line, sizeof(line), f)) {
            cout << line;
            cout.flush();
            string resp(line);
            if (!resp.empty() && resp.back() == '\n') resp.pop_back();
            if (resp == "OK" || resp == "BYE" ||
                (!resp.empty() && resp.front() == '(') ||
                (resp.size() >= 5 && resp.substr(0, 5) == "ERROR")) break;
        }

        if (sent == "QUIT") break;
    }

    fclose(f);
    return 0;
}