#pragma once
#include <iostream>
#include <string>

class Game {
private:
    int points_player1;
    int points_player2;
    int game_num;
    bool is_player_one_serving;

    static std::string getScoreString(int points);

public:
    Game();
    Game(int points_player1, int points_player2, int game_num);

    int getGameNum() const { return game_num; }
    void setGameNum(const int game_num) { this->game_num = game_num; }
    void setIsPlayerOneServing(const bool is_serving) { is_player_one_serving = is_serving; }

	void addPoint(int player, int match_id, int set_num);

    void checkServer();
    int determineWinner() const;
    void resetGame(int match_id, int set_num);
    bool isWonGame() const {
        return (points_player1 >= 4 && points_player1 >= points_player2 + 2) ||
            (points_player2 >= 4 && points_player2 >= points_player1 + 2);
    }

    void saveGameRecordToDatabase(bool is_new_record, int match_id, int set_num) const;
    void saveGameRecord(int match_id, int set_num) const;
    void updateGameRecord(int match_id, int set_num) const;

    void printCurScore() const { std::cout << "Current score in " << game_num << " game: \t" << getScoreString(points_player1)
    	<< " - " << getScoreString(points_player2) << '\n';
    }
    void printGameInfo() const { std::cout << "Game: " << game_num << '\n'; }
};