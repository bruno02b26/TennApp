#include "Tiebreak.hpp"
#include "DatabaseConnection.hpp"
#include <iostream>

Tiebreak::Tiebreak(const bool is_player_one_serving, const int set_num, const int max_points)
{
    this->is_player_one_serving = is_player_one_serving;
    this->set_num = set_num;
    this->max_points = max_points;
	this->points_player1 = 0;
    this->points_player2 = 0;
}

Tiebreak::Tiebreak(const bool is_player_one_serving, const int set_num, const int max_points, const int points_player1, const int points_player2)
{
    this->is_player_one_serving = is_player_one_serving;
    this->set_num = set_num;
    this->max_points = max_points;
    this->points_player1 = points_player1;
    this->points_player2 = points_player2;
}

void Tiebreak::addPoint(const int player, const int match_id)
{
    if (player == 1) {
        points_player1++;
        std::cout << "Point for player 1.\n";
    }
    else if (player == 2) {
        points_player2++;
        std::cout << "Point for player 2.\n";
    }
    else {
        std::cerr << "Invalid player number: " << player << '\n';
        return;
    }

	updateInDatabase(match_id);
}

bool Tiebreak::isTiebreakWon() const
{
    return (points_player1 >= max_points && (points_player1 - points_player2) >= 2) ||
        (points_player2 >= max_points && (points_player2 - points_player1) >= 2);
}

int Tiebreak::winner() const
{
    if (points_player1 >= max_points && (points_player1 - points_player2) >= 2) {
        return 1;
    }
    else if (points_player2 >= max_points && (points_player2 - points_player1) >= 2) {
        return 2;
    }
    return 0;
}

int Tiebreak::determineWinner() const
{
    const int winner_id = winner();
    if (winner_id == 1) {
        std::cout << "Tiebreak won by player 1 with result: " << points_player1 << " - " << points_player2 << '\n';
    }
    else if (winner_id == 2) {
        std::cout << "Tiebreak won by player 2 with result: " << points_player1 << " - " << points_player2 << '\n';
    }
    return winner_id;
}

void Tiebreak::saveToDatabase(const int match_id) const
{
    DatabaseConnection& db = DatabaseConnection::getInstance();
    try {
        pqxx::work txn(*db.getConnection());
        const std::string type_query = "SELECT type_id FROM tie_break_type WHERE min_points = " + txn.quote(max_points);
        const pqxx::result result = txn.exec(type_query);

        if (result.empty()) {
            throw std::runtime_error("No matching tie_break_type found for maxPoints: " + std::to_string(max_points));
        }

        const int tiebreak_type_id = result[0][0].as<int>();
        const std::string insert_query = "INSERT INTO tie_breaks (match_id, set_number, player1_score, player2_score, tie_break_type) VALUES (" +
            txn.quote(match_id) + ", " +
            txn.quote(set_num) + ", " +
            txn.quote(points_player1) + ", " +
            txn.quote(points_player2) + ", " +
            txn.quote(tiebreak_type_id) + ")";
        txn.exec(insert_query);
        txn.commit();
    }
    catch (const pqxx::sql_error& e) {
        std::cerr << "SQL Error: " << e.what() << '\n';
        std::cerr << "Query was: " << e.query() << '\n';
        throw;
    }
    catch (const std::exception& e) {
        std::cerr << "Error saving tiebreak to database: " << e.what() << '\n';
        throw;
    }
}

void Tiebreak::updateInDatabase(const int match_id) const
{
    DatabaseConnection& db = DatabaseConnection::getInstance();
    try {
        pqxx::work txn(*db.getConnection());
        const std::string update_query = "UPDATE tie_breaks SET player1_score = " + txn.quote(points_player1) + ", " +
            "player2_score = " + txn.quote(points_player2) +
            " WHERE match_id = " + txn.quote(match_id) +
            " AND set_number = " + txn.quote(set_num);
        txn.exec(update_query);
        txn.commit();
    }
    catch (const std::exception& e) {
        std::cerr << "Error updating tiebreak in database: " << e.what() << '\n';
        throw;
    }
}

void Tiebreak::printServingPlayer() const
{
    if (getIsPlayerOneServing()) {
        std::cout << "  Player 1 is serving\n";
    }
    else {
        std::cout << "  Player 2 is serving\n";
    }
}