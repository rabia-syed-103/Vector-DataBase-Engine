#ifndef PROTOCOL_H
#define PROTOCOL_H
 
#include "VectorStore.h"
#include <string>

std::string parse_and_dispatch(const std::string& line, VectorStore& store);
 
#endif
 
