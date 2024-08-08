#pragma once
#include <iostream>

class Tiebreak {
private:
	int points_player1;
	int points_player2;
	int max_points;
	bool is_player_one_serving;
	int set_num;

	static std::string getPointLabel(const int points) { return points == 1 ? " point: \t" : " points: \t"; }

public:
	Tiebreak() = default;
	Tiebreak(bool is_player_one_serving, int set_num, int max_points);
	Tiebreak(bool is_player_one_serving, int set_num, int max_points, int points_player1, int points_player2);

	int getPointsPlayerOne() const { return points_player1; }
	void setPointsPlayerOne(const int points) { points_player1 = points; }
	int getPointsPlayerTwo() const { return points_player2; }
	void setPointsPlayerTwo(const int points) { points_player2 = points; }
	bool getIsPlayerOneServing() const { return is_player_one_serving; }
	void setIsPlayerOneServing(const bool is_serving) { is_player_one_serving = is_serving; }

	void addPoint(int player, int match_id);
	void updateIsPlayerOneServing() { is_player_one_serving = !is_player_one_serving; }

	bool isTiebreakWon() const;
	int winner() const;
	int determineWinner() const;

	void saveToDatabase(int match_id) const;
	void updateInDatabase(int match_id) const;

	void printServingPlayer() const;
	void printScore() const {
		if (!winner()) {
			std::cout << "Score in tiebreak: " <<  points_player1 << " - " << points_player2 << '\n';
		}
	}
};
