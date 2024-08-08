#pragma once
#include <pqxx/pqxx>

class IDatabaseConnection {
public:
    virtual ~IDatabaseConnection() = default;
    virtual pqxx::connection* getConnection() = 0;
};