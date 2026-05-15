#ifndef PERSISTENCE_H
#define PERSISTENCE_H
#include "VectorStore.h"
#include <string>
using namespace std;
bool store_save(const VectorStore& store, const string& data_dir);
bool store_load(VectorStore& store, const string& data_dir);
#endif
