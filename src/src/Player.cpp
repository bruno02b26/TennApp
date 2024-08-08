#include "Player.hpp"
#include "validate.hpp"
#include <iostream>
#include <tabulate/table.hpp>

bool Player::exists(const int player_id) {
    DatabaseConnection& db = DatabaseConnection::getInstance();
    pqxx::nontransaction nt(*db.getConnection());
    const pqxx::result r = nt.exec("SELECT 1 FROM public.players WHERE id = " + std::to_string(player_id));
    return !r.empty();
}

void Player::addPlayer(const std::string& first_name, const std::string& last_name) {
    if (!isAlpha(first_name) || !isAlpha(last_name)) {
        std::cout << "Names should contain only letters." << '\n';
        return;
    }
    if (first_name.length() > 25 || last_name.length() > 25) {
        std::cout << "Names should have maximum 25 letters." << '\n';
        return;
    }

    const std::string formatted_first_name = formatName(first_name);
    const std::string formatted_last_name = formatName(last_name);
    DatabaseConnection& db = DatabaseConnection::getInstance();

    try {
        std::string query = "INSERT INTO public.players (first_name, last_name) VALUES ('" + formatted_first_name + "', '" + formatted_last_name + "');";
        pqxx::work w(*db.getConnection());
        w.exec(query);
        w.commit();
        std::cout << "Player added successfully.\n";
    } catch (const pqxx::sql_error& e) {
        std::cerr << "SQL error: " << e.what() << '\n';
        std::cerr << "Query was: " << e.query() << '\n';
    } catch (const std::exception& e) {
        std::cerr << "Exception in addPlayer: " << e.what() << '\n';
    }
}

void Player::showPlayerStatistics(const int player_id) {
    DatabaseConnection& db = DatabaseConnection::getInstance();
    std::string query = "SELECT * FROM public.players";

    if (player_id != -1) {
        query += " WHERE id = " + std::to_string(player_id);
    }

    query += " ORDER BY id ASC";
    pqxx::nontransaction nt(*db.getConnection());
    pqxx::result r = nt.exec(query);

    if (r.empty()) {
        std::cout << "No users found.\n";
    }
    else {
        tabulate::Table table;
        table.add_row({ "Player ID", "First Name", "Last Name", "Matches Won", "Matches Lost" });

        for (const auto& row : r) {
            table.add_row({
                row[0].as<std::string>(),
                row[1].as<std::string>(),
                row[2].as<std::string>(),
                std::to_string(row["matches_won"].as<int>()),
	            std::to_string(row["matches_lost"].as<int>())
            });
        }

        std::cout << table << '\n';
    }
}

void Player::updateMatchResults(const int winner_id, const int loser_id) {
    DatabaseConnection& db = DatabaseConnection::getInstance();

    try {
        pqxx::work w(*db.getConnection());
        std::string queryWinner = "UPDATE public.players SET matches_won = matches_won + 1 WHERE id = " + std::to_string(winner_id) + ";";
        w.exec(queryWinner);
        std::string queryLoser = "UPDATE public.players SET matches_lost = matches_lost + 1 WHERE id = " + std::to_string(loser_id) + ";";
        w.exec(queryLoser);
        w.commit();
    } catch (const pqxx::sql_error& e) {
        std::cerr << "SQL error: " << e.what() << '\n';
        std::cerr << "Query was: " << e.query() << '\n';
    } catch (const std::exception& e) {
        std::cerr << "Exception in updateMatchResults: " << e.what() << '\n';
    }
}