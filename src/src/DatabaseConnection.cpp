#include "DatabaseConnection.hpp"
#include <iostream>

DatabaseConnection& DatabaseConnection::getInstance() {
    static DatabaseConnection instance;
    return instance;
}

pqxx::connection* DatabaseConnection::getConnection() {
    return conn_.get();
}

DatabaseConnection::DatabaseConnection() {
    initializeConnection();
}

DatabaseConnection::~DatabaseConnection() {
    closeConnection();
}

void DatabaseConnection::initializeConnection() {
    try {
        conn_ = std::make_unique<pqxx::connection>("host=localhost dbname=postgres user=postgres password=password");
        if (!conn_->is_open()) {
            std::cerr << "Database connection failed.\n";
            throw std::runtime_error("Failed to open database connection.");
        }
    }
    catch (const std::exception& e) {
        handleException(e);
    }
}

void DatabaseConnection::handleException(const std::exception& e) {
    std::cerr << "Exception: " << e.what() << '\n';
}

void DatabaseConnection::closeConnection() const {
    if (conn_ && conn_->is_open()) {
        conn_->close();
        std::cout << "Database connection closed.\n";
    }
}
