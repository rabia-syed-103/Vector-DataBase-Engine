#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <set>
#include <random>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

using namespace std;

FILE* connect_to_server(const char* host, int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(port);
    inet_pton(AF_INET, host, &addr.sin_addr);
    if (connect(sock, (sockaddr*)&addr, sizeof(addr)) < 0) {
        cerr << "ERROR: cannot connect to server at "
             << host << ":" << port << "\n";
        cerr << "Make sure the server is running first.\n";
        exit(1);
    }
    return fdopen(sock, "r+");
}

string send_cmd(FILE* f, const string& cmd) {
    fputs((cmd + "\n").c_str(), f);
    fflush(f);

    string result;
    char line[65536];
    while (fgets(line, sizeof(line), f)) {
        result += line;
        string l(line);
        while (!l.empty() && (l.back() == '\n' || l.back() == '\r'))
            l.pop_back();
        if (l == "OK"   || l == "BYE" ||
            (!l.empty() && l.front() == '(') ||
            (l.size() >= 5 && l.substr(0,5) == "ERROR"))
            break;
    }
    return result;
}

set<int64_t> parse_ids(const string& response) {
    set<int64_t> ids;
    istringstream ss(response);
    string line;
    while (getline(ss, line)) {
        if (line.empty() || line.front() == '(') continue;
        istringstream ls(line);
        int64_t id;
        if (ls >> id) ids.insert(id);
    }
    return ids;
}

int main() {
    const int   N         = 50000;   
    const int   D         = 64;      
    const int   K_QUERY   = 10;     
    const int   N_QUERIES = 100;    
    const int   SEED      = 42;      
    const char* HOST      = "127.0.0.1";
    const int   PORT      = 5556;

    vector<int> nprobes = {1, 5, 10, 25};

    cout << "========================================\n";
    cout << "   Vector DB Benchmark\n";
    cout << "========================================\n";
    cout << "N=" << N << " vectors, D=" << D
         << " dims, K=" << K_QUERY
         << ", queries=" << N_QUERIES
         << ", seed=" << SEED << "\n\n";

    mt19937 rng(SEED);
    normal_distribution<float> dist(0.0f, 1.0f);

    vector<vector<float>> data(N, vector<float>(D));
    for (auto& v : data)
        for (auto& x : v) x = dist(rng);

    mt19937 rng2(SEED + 1);
    vector<vector<float>> queries(N_QUERIES, vector<float>(D));
    for (auto& q : queries)
        for (auto& x : q) x = dist(rng2);

    cout << "Connecting to server at " << HOST << ":" << PORT << " ...\n";
    FILE* f = connect_to_server(HOST, PORT);
    cout << "Connected.\n\n";

    cout << "Inserting " << N << " vectors (D=" << D << ")...\n";
    auto t_insert_start = chrono::steady_clock::now();

    for (int i = 0; i < N; i++) {
        ostringstream cmd;
        cmd << "ADD " << (i + 1);
        for (float x : data[i]) cmd << " " << x;
        send_cmd(f, cmd.str());

        if ((i + 1) % 10000 == 0) {
            cout << "  " << (i + 1) << " / " << N << " inserted...\n";
            cout.flush();
        }
    }

    auto t_insert_end = chrono::steady_clock::now();
    double insert_sec = chrono::duration<double>(t_insert_end - t_insert_start).count();

    cout << "Insert complete: " << insert_sec << " s  ("<< (int)(N / insert_sec) << " inserts/sec)\n\n";

    cout << "Running BUILD...\n";
    auto t_build_start = chrono::steady_clock::now();
    string build_resp = send_cmd(f, "BUILD");
    auto t_build_end = chrono::steady_clock::now();
    double build_sec = chrono::duration<double>(t_build_end - t_build_start).count();
    cout << build_resp;
    cout << "Build wall time: " << build_sec << " s\n\n";
    cout << "Running " << N_QUERIES << " queries...\n\n";

    double brute_total_ms = 0;
    vector<double> ivf_total_ms(nprobes.size(), 0.0);
    vector<double> recall_total(nprobes.size(), 0.0);
    vector<double> scanned_total_brute(N_QUERIES, 0.0);
    vector<vector<double>> scanned_total_ivf(
        nprobes.size(), vector<double>(N_QUERIES, 0.0));

    vector<set<int64_t>> ground_truth(N_QUERIES);

    for (int qi = 0; qi < N_QUERIES; qi++) {
        ostringstream qbase;
        for (int d = 0; d < D; d++) {
            qbase << " " << queries[qi][d];
        }
        string qstr = qbase.str();
        string brute_cmd = "SEARCH" + qstr + " " + to_string(K_QUERY) + " BRUTE";
        auto tb0 = chrono::steady_clock::now();
        string brute_resp = send_cmd(f, brute_cmd);
        auto tb1 = chrono::steady_clock::now();
        brute_total_ms += chrono::duration<double>(tb1 - tb0).count() * 1000.0;
        ground_truth[qi] = parse_ids(brute_resp);


        for (int ni = 0; ni < (int)nprobes.size(); ni++) {
            string ivf_cmd = "SEARCH" + qstr + " "
                             + to_string(K_QUERY) + " IVF "
                             + to_string(nprobes[ni]);
            auto ti0 = chrono::steady_clock::now();
            string ivf_resp = send_cmd(f, ivf_cmd);
            auto ti1 = chrono::steady_clock::now();
            ivf_total_ms[ni] += chrono::duration<double>(
                                     ti1 - ti0).count() * 1000.0;

            set<int64_t> ivf_ids = parse_ids(ivf_resp);
            int hits = 0;
            for (int64_t id : ground_truth[qi])
                if (ivf_ids.count(id)) hits++;
            recall_total[ni] += (double)hits / (double)K_QUERY;
        }

        if ((qi + 1) % 20 == 0) {
            cout << "  " << (qi + 1) << " / " << N_QUERIES
                 << " queries done...\n";
            cout.flush();
        }
    }
    cout << "\n";
    cout << "========================================\n";
    cout << "   Benchmark Results\n";
    cout << "========================================\n";
    cout << "Parameters: N=" << N << ", D=" << D
         << ", K=" << K_QUERY << ", queries=" << N_QUERIES
         << ", seed=" << SEED << "\n\n";

    double avg_brute_ms = brute_total_ms / N_QUERIES;

    printf("%-8s %-8s %-16s %-12s %-10s\n",
           "Mode", "nprobe", "Avg time (ms)", "Recall@10", "Speedup");
    printf("%-8s %-8s %-16s %-12s %-10s\n",
           "----", "------", "-------------", "---------", "-------");
    printf("%-8s %-8s %-16.3f %-12s %-10s\n",
           "BRUTE", "-", avg_brute_ms, "1.000", "1.0x");

    for (int ni = 0; ni < (int)nprobes.size(); ni++) {
        double avg_ivf_ms = ivf_total_ms[ni] / N_QUERIES;
        double avg_recall = recall_total[ni]  / N_QUERIES;
        double speedup    = avg_brute_ms / avg_ivf_ms;

        printf("%-8s %-8d %-16.3f %-12.3f %-10.1fx\n",
               "IVF", nprobes[ni], avg_ivf_ms, avg_recall, speedup);
    }

    cout << "\n";
    send_cmd(f, "QUIT");
    fclose(f);
    return 0;
}