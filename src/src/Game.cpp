#include "Game.hpp"
#include <iostream>
#include <string>
#include <DatabaseConnection.hpp>

Game::Game()
{
    this->points_player1 = 0;
    this->points_player2 = 0;
    this->game_num = 1;
    printGameInfo();
}

Game::Game(const int points_player1, const int points_player2, const int game_num)
{
    this->points_player1 = points_player1;
    this->points_player2 = points_player2;
    this->game_num = game_num;
    printGameInfo();
}

void Game::addPoint(const int player, const int match_id, const int set_num) {
    if (player == 1) {
        if (points_player1 >= 3 && points_player2 >= 3) {
            if (points_player1 == 4) {
                points_player1++;
            }
            else if (points_player1 == 3 && points_player2 == 4) {
                points_player2--;
            }
            else {
                points_player1++;
            }
        }
        else {
            points_player1++;
        }
        std::cout << "Point for player 1.\n";
    }
    else if (player == 2) {
        if (points_player1 >= 3 && points_player2 >= 3) { 
            if (points_player2 == 4) {
                points_player2++;
            }
            else if (points_player2 == 3 && points_player1 == 4) { 
                points_player1--;
            }
            else {
                points_player2++;
            }
        }
        else {
            points_player2++;
        }
        std::cout << "Point for player 2.\n";
    }
    else {
        std::cerr << "Invalid player number: " << player << '\n';
        return;
    }

    if (!isWonGame()) {
        updateGameRecord(match_id, set_num);
        printCurScore();
    }
}

int Game::determineWinner() const {
    if (points_player1 >= 4 && points_player1 >= points_player2 + 2) {
        std::cout << "Player 1 wins the " << game_num << " game with a score of " << getScoreString(points_player1 - 1)
            << " - " << getScoreString(points_player2) << '\n';
        return 1;
    }
    if (points_player2 >= 4 && points_player2 >= points_player1 + 2) {
        std::cout << "Player 2 wins the " << game_num << " game with a score of " << getScoreString(points_player1)
            << " - " << getScoreString(points_player2 - 1) << '\n';
        return 2;
    }
    return 0;
}


std::string Game::getScoreString(const int points) {
    switch (points) {
    case 0: return "0";
    case 1: return "15";
    case 2: return "30";
    case 3: return "40";
    case 4:
    case 5:
    	return "A";
    default: return std::to_string(points);
    }
}

void Game::saveGameRecordToDatabase(const bool is_new_record, const int match_id, const int set_num) const {
    DatabaseConnection& db = DatabaseConnection::getInstance();

    try {
        pqxx::work txn(*db.getConnection());
        std::string query;

        if (is_new_record) {
            query = "INSERT INTO game_points (match_id, set_number, game_number, player1_points, player2_points) VALUES (" +
                txn.quote(match_id) + ", " +
                txn.quote(set_num) + ", " +
                txn.quote(game_num) + ", " +
                txn.quote(getScoreString(points_player1)) + ", " +
                txn.quote(getScoreString(points_player2)) + ")";
        }
        else {
            query = "UPDATE game_points SET player1_points = " + txn.quote(getScoreString(points_player1)) + ", " +
                "player2_points = " + txn.quote(getScoreString(points_player2)) +
                " WHERE match_id = " + txn.quote(match_id) +
                " AND set_number = " + txn.quote(set_num) +
                " AND game_number = " + txn.quote(game_num);
        }
        txn.exec(query);
        txn.commit();
    }
    catch (const std::exception& e) {
        std::cerr << (is_new_record ? "Error saving game record to database: " : "Error updating game record in database: ") << e.what() << std::endl;
        throw std::runtime_error(is_new_record ? "Failed to save game record" : "Failed to update game record");
    }
}

void Game::saveGameRecord(const int match_id, const int set_num) const {
    saveGameRecordToDatabase(true, match_id, set_num);
}

void Game::updateGameRecord(const int match_id, const int set_num) const {
    saveGameRecordToDatabase(false, match_id, set_num);
}

void Game::resetGame(const int match_id, const int set_num) {
    points_player1 = 0;
    points_player2 = 0;
    game_num++;
    printGameInfo();
    checkServer();
    saveGameRecord(match_id, set_num);
    printCurScore();
}

void Game::checkServer() {
    is_player_one_serving = !is_player_one_serving;
    std::cout << "  Player " << (is_player_one_serving ? "1" : "2") << " is serving.\n";
}
