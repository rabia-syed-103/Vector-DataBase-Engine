#ifndef PROTOCOL_H
#define PROTOCOL_H
#include "persistence.h"
#include "VectorStore.h"
#include <string>
using namespace std;
string parse_and_dispatch(const std::string& line, VectorStore& store);
 
#endif
 
