#pragma once
#include "IDatabaseConnection.hpp"
#include <pqxx/pqxx>
#include <memory>

class DatabaseConnection : public IDatabaseConnection {
public:
	static DatabaseConnection& getInstance();
	pqxx::connection* getConnection() override;

	DatabaseConnection(const DatabaseConnection&) = delete;
	DatabaseConnection& operator=(const DatabaseConnection&) = delete;

	~DatabaseConnection() override;

private:
	std::unique_ptr<pqxx::connection> conn_;
	DatabaseConnection();
	void initializeConnection();
	static void handleException(const std::exception& e);
	void closeConnection() const;
};