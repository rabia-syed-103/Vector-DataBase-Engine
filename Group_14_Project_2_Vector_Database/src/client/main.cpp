#include <iostream>
#include <string>
#include <cstdio>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>        // ← ADD for gethostbyname
#include <unistd.h>
using namespace std;

int main(int argc, char* argv[]) {
    if (argc < 3) {
        cerr << "Usage: vdb-cli <host> <port>\n";
        return 1;
    }

    const char* host = argv[1];
    int port = stoi(argv[2]);

    // create socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        cerr << "ERROR: cannot create socket\n";
        return 1;
    }

    // resolve hostname → works for both "localhost" and "127.0.0.1"
    struct hostent* he = gethostbyname(host);
    if (!he) {
        cerr << "ERROR: cannot resolve host " << host << "\n";
        return 1;
    }

    // connect
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(port);
    addr.sin_addr   = *(struct in_addr*)he->h_addr;

    if (connect(sock, (sockaddr*)&addr, sizeof(addr)) < 0) {
        cerr << "ERROR: cannot connect to "
             << host << ":" << port << "\n";
        return 1;
    }

    cout << "connected to vdb at " << host << ":" << port << "\n";

    FILE* f = fdopen(sock, "r+");
    if (!f) {
        cerr << "ERROR: fdopen failed\n";
        return 1;
    }

    char line[65536];

    while (true) {
        cout << "> ";
        cout.flush();

        if (!fgets(line, sizeof(line), stdin)) break;

        fputs(line, f);
        fflush(f);

        string sent(line);
        if (!sent.empty() && sent.back() == '\n')
            sent.pop_back();

        if (sent == "QUIT") {
            if (fgets(line, sizeof(line), f))
                cout << line;
            break;
        }

        while (fgets(line, sizeof(line), f)) {
            string resp(line);
            if (!resp.empty() && resp.back() == '\n')
                resp.pop_back();

            cout << resp << "\n";
            cout.flush();

            if (resp.empty()                      ||
                resp == "OK"                      ||
                resp == "BYE"                     ||
                (!resp.empty() && resp[0] == '(') ||
                (resp.size() >= 5 &&
                 resp.substr(0, 5) == "ERROR")) break;
        }
    }

    fclose(f);
    return 0;
}