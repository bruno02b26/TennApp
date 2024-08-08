#include "DatabaseConnection.hpp"
#include "UIManager.hpp"
#include <iostream>

int main() {
    try {
        DatabaseConnection& db = DatabaseConnection::getInstance();

        if (!db.getConnection()) {
            std::cerr << "Database connection failed.\n";
            return 1;
        }

        std::cout << "Database connection successful.\n";
        UIManager ui_manager;
        ui_manager.run();
    }
    catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << '\n';
        return 1;
    }

	return 0;
}