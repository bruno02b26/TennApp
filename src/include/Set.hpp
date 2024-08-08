#pragma once
#include <chrono>
#include <iostream>
#include "Game.hpp"
#include "Timer.hpp"

class Set {
private:
    int match_id;
    int set_num;
    int no_sets;
    std::optional<Game> current_game;
    int games_player1;
    int games_player2;
    bool is_player_one_winner;
    bool is_player_one_serving;
    bool is_resumed_tiebreak = false;

    Timer timer;
    std::chrono::seconds duration;

	void updateMatchSetRecordWithoutServingPlayerId();
    std::pair<int, int> getTieBreakScores() const;
    static int convertScore(const std::string& db_score);
    void randomizeFirstServer() {
        std::srand(static_cast<unsigned int>(std::time(nullptr)));
        const bool is_player_one_starting = std::rand() % 2;
        this->is_player_one_serving = is_player_one_starting;
    }

    std::string getGameLabel() const { return games_player1 + games_player2 == 1 ? " game: \t" : " games: \t"; }
    
public:
    Set(int match_id, int no_sets, int set_num = 1);
    Set(int match_id, int no_sets, int set_num, int games_player1, int games_player2, bool is_first_player_serving);

    void setIsPlayerOneWinner(const bool winner) { is_player_one_winner = winner; }
    bool getIsPlayerOneWinner() const { return is_player_one_winner; }
    void setIsPlayerOneServing(const bool serve) { this->is_player_one_serving = serve; }
    bool getIsPlayerOneServing() const { return this->is_player_one_serving; }
    void setIsResumedTiebreak(const bool is_resumed_tiebreak_p) { this->is_resumed_tiebreak = is_resumed_tiebreak_p; }
    bool getIsResumedTiebreak() const { return is_resumed_tiebreak; }
    int getSetNum() const { return set_num; }
    void setSetNum(const int set_num_p) { this->set_num = set_num_p; }
    std::optional<Game>& getCurrentGame() { return current_game; }
    void setCurrentGame(const std::optional<Game>& game) { current_game = game; }
    void setGamesPlayerOne(const int games) { games_player1 = games; }
    void setGamesPlayerTwo(const int games) { games_player2 = games; }

    void updateTiebreakStatus() const;
    int getTiebreakPoints() const { return ((no_sets * 2 - 1) == set_num) ? 10 : 7; }
    bool isTieBreak() const;

    void resumeCurrentGame(bool is_serving);
    int addGameResult(int winning_player_id);
    int getNumberOfGames() const { return games_player1 + games_player2; }
    void changeGame()
	{
        if (current_game.has_value()) {
            current_game->resetGame(match_id, set_num);
        }
        else {
            std::cerr << "Error: No current game to reset." << '\n';
            std::exit(EXIT_FAILURE);
        }
    }

    void addMatchSetRecord();
    void updateMatchSetRecord();
    void saveSetRecordToDatabase(bool is_new_record) const;

    void printGameInfo() const
    {
        const int num_of_games = games_player1 + games_player2;
        std::cout << "Score after " << num_of_games << getGameLabel() << games_player1 << " - " << games_player2 << '\n';
    }

    bool isWonSet() const
	{
        return (games_player1 >= 6 && games_player1 >= games_player2 + 2) ||
            (games_player2 >= 6 && games_player2 >= games_player1 + 2);
    }

    int determineWinner() const
	{
        if (games_player1 >= 6 && games_player1 >= games_player2 + 2) return 1;
        if (games_player2 >= 6 && games_player2 >= games_player1 + 2) return 2;
        return 0;
    }

    void initializeCurrentGame(const bool is_serving)
	{
        if (!current_game.has_value()) {
            current_game = Game();
            current_game->setIsPlayerOneServing(is_serving);
            std::cout << "  Player " << (is_serving ? "1" : "2") << " is serving.\n";
            current_game->printCurScore();
            current_game->saveGameRecord(match_id, set_num);
        }
    }

    void printSetInfo() const { std::cout << "Set: " << set_num << '\n'; }
	void updateTime();
};
