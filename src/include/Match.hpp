#pragma once
#include "Set.hpp"
#include "MatchState.hpp"
#include "Timer.hpp"
#include <optional>
#include <string>
#include <chrono>

class Match {
private:
	int id;
	int player_id1;
	int player_id2;
	std::optional<int> winner_id;

	int sets_player1 = 0;
	int sets_player2 = 0;

	int no_sets;
	std::optional<Set> current_set;

	MatchState* current_state;
	int status_id;

	Timer timer;
	std::chrono::seconds duration;

	static bool playerExists(int player_id);
	static std::string getCurrentTime();
	static bool isLaterThanNow(const std::string& date_time_str);

	void saveToDatabase(const std::string& predicted_start_time);
	void updateStatusInDatabase() const;

	std::string getGameLabel() const { return sets_player1 + sets_player2 == 1 ? " set: \t" : " sets: \t"; }

public:

	Match() = default;
	Match(int player1_id, int player2_id, int no_sets, const std::string& predicted_start_time);
	Match(int id, int player1_id, int player2_id, int no_sets);
	Match(int id, int player1_id, int player2_id, int no_sets, std::chrono::seconds duration = std::chrono::seconds(0));
	~Match();

	void startMatch(int match_id);
	void endMatch(int winning_player_id);
	void suspendMatch();
	void changeState(MatchState* state);
	static std::optional<Match> getMatchById(int id, const std::vector<std::string>& statuses);
	static int getStatusId(const std::string& status_name);
	static std::string getStatusById(int status_id);

	int getId() const { return id; }
	int getStatusId() const { return status_id; }
	void setStatusId(const int status) { this->status_id = status; }
	int getIdPlayerOne() const { return player_id1; }
	int getIdPlayerTwo() const { return player_id2; }

	int getSetsPlayerOne() const { return sets_player1; }
	void updateSetsPlayerOne() { sets_player1++; }

	int getSetsPlayerTwo() const { return sets_player2; }
	void updateSetsPlayerTwo() { sets_player2++; }

	int getNoSets() const { return no_sets; }

	static bool isPlayerOneServing(const int total_games, const bool is_first_player_starting) {
		return (total_games % 2 == 0) ? is_first_player_starting : !is_first_player_starting;
	}

	std::optional<Set>& getCurrentSet() { return current_set; }
	void setCurrentSet(const std::optional<Set>& set) { current_set = set; }

	void initializeCurrentSet(int match_id);
	void resumeCurrentSet(int match_id);
	void updateCurrentSet(int match_id);

	bool isMatchWinner() const { return sets_player1 == no_sets || sets_player2 == no_sets;  }

	void finishMatch();
	void updateWinnerInDatabase() const;
	void setWinner(int player_id) { winner_id = player_id; }
	void resumeMatch(int match_id);

	void updateMatchInDatabase();

	void printScoreInfo() const
	{
		std::cout << "Score in sets: \t"
			<< getSetsPlayerOne() << " - "
			<< getSetsPlayerTwo() << '\n';
	}

	void displayPlayerInfo() const;
};