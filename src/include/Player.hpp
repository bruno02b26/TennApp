#pragma once
#include "DatabaseConnection.hpp"
#include <string>

class Player {
public:
    Player() = default;
    static bool exists(int player_id);
    static void addPlayer(const std::string& first_name, const std::string& last_name);
    static void showPlayerStatistics(int player_id);
    static void updateMatchResults(int winner_id, int loser_id);
};