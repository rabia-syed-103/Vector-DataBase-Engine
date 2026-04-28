# Vector Database Engine
---

## Overview

VDB is a TCP-based vector database server that stores fixed-dimension floating-point vectors and answers nearest-neighbor queries against them. The system consists of two binaries: `vdb` (the server) and `vdb-cli` (the command-line client).

---

## Building

```bash
make
```

This produces two binaries in the project root:
- `vdb` — the server
- `vdb-cli` — the interactive client

To clean build artifacts:
```bash
make clean
```
if you pull the repositories and `vdb`,`vdb-cli` are already there it might not match with your version of gcc so make it again using the following commands:

```bash
 rm vdb vdb-cl
 make clean
```
---

## Running the Server

```bash
./vdb --dim <D> --port <PORT> --data <DATA_DIR>
```

| Flag | Default | Description |
|------|---------|-------------|
| `--dim` | `128` | Dimension of all stored vectors |
| `--port` | `5556` | TCP port to listen on |
| `--data` | `./vdata` | Directory for snapshot files (Phase 3) |

**Example:**
```bash
./vdb --data ./vdata --dim 4 --port 5556
```

Expected output:
```
vdb started on port 5556, dimension 4, data directory ./vdata
Waiting for connections...
```

---

## Running the Client

In a separate terminal:
```bash
./vdb-cli <host> <port>
```

**Example:**
```bash
./vdb-cli localhost 5556
```

Expected output:
```
connected to vdb at localhost:5556
>
```
---

## Supported Commands (Phase 1)

### ADD
```
ADD <id> <v1> <v2> ... <vD>
```
Inserts a vector with the given 64-bit integer ID. If the ID already exists, the vector is overwritten. Returns `OK` or an error.

**Example:**
```
> ADD 1 0.10 0.20 0.30 0.40
OK
```

### SEARCH BRUTE
```
SEARCH <v1> <v2> ... <vD> <k> BRUTE
```
Returns the `k` nearest stored vectors to the query using brute-force scan. Results are ordered by increasing squared Euclidean distance.

**Example:**
```
> SEARCH 0.12 0.22 0.32 0.42 3 BRUTE
1 0.0038 0.10 0.20 0.30 0.40
2 0.0138 0.15 0.25 0.35 0.45
5 0.4904 0.50 0.50 0.50 0.50
(3 results, mode=BRUTE, scanned 5)
```

### STATS
```
STATS
```
Prints server state information.

**Example:**
```
> STATS
dimension: 4
total vectors: 5
index built: no
```

### QUIT
```
QUIT
```
Disconnects the client. The server continues running.

---

## Commands Implemented in Later Phases

| Command | Phase | Description |
|---------|-------|-------------|
| `SEARCH ... IVF <nprobe>` | Phase 2 | IVF index-based nearest-neighbor search |
| `BUILD` | Phase 2 | Run k-means and build the IVF index |
| `SAVE` | Phase 3 | Persist all vectors and index to disk |
| `LOAD` | Phase 3 | Restore state from snapshot file |

---

## Running Tests

```bash
g++ -std=c++17 \
    src/tests/test_main.cpp \
    src/VectorStore.cpp \
    src/protocol.cpp \
    src/SearchResult.cpp \
    -I src/ \
    -o src/tests/test_main && ./src/tests/test_main
```

---

## Known Limitations (Phase 1)

- `SEARCH IVF` returns an error. IVF index not yet implemented (Phase 2).
- `BUILD`, `SAVE`, and `LOAD` return errors. Not yet implemented (Phases 2–3).
- No snapshot persistence. All data is lost when the server stops (Phase 3).
- Distances printed in results are **squared Euclidean distance** (not the square root). This is intentional and consistent throughout the system.
- The server does not validate that `--data` directory exists at startup; create it manually if needed.

---

## Project Structure

```
mod@mod-ThinkBook-14-G6-IRL:~/Vector-DataBase-Engine/Group_14_Project_2_Vector_Database$ tree
.
├── documentation
│   └── Design.pdf
├── makefile
├── README.md
├── src
│   ├── client
│   │   └── main.cpp
│   ├── main.cpp
│   ├── protocol.cpp
│   ├── protocol.h
│   ├── SearchResult.cpp
│   ├── SearchResult.h
│   ├── server.cpp
│   ├── server.h
│   ├── tests
│   │   ├── test
│   │   ├── test_main
│   │   └── test_main.cpp
│   ├── VectorStore.cpp
│   └── VectorStore.h
├── vdb
└── vdb-cli

4 directories, 18 files
```