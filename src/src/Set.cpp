#include "Set.hpp"
#include "DatabaseConnection.hpp"
#include <iostream>
#include "Tiebreak.hpp"
#include "UIManager.hpp"

Set::Set(const int match_id, const int no_sets, const int set_num)
{
	this->match_id = match_id;
	this->no_sets = no_sets;
	this->set_num = set_num;
	this->games_player1 = 0;
	this->games_player2 = 0;
	this->is_player_one_winner = false;
	this->duration = std::chrono::seconds::zero();
    timer.start();
	printSetInfo();
	std::cout << "Current score in " << set_num << " set: \t" << games_player1 << " - " << games_player2 << '\n';
	if (set_num == 1) { randomizeFirstServer(); }
}

Set::Set(const int match_id, const int no_sets, const int set_num, const int games_player1, const int games_player2, const bool is_first_player_serving)
{
	this->match_id = match_id;
	this->no_sets = no_sets;
	this->set_num = set_num;
	this->games_player1 = games_player1;
	this->games_player2 = games_player2;
	this->is_player_one_serving = is_first_player_serving;
	this->is_player_one_winner = false;
	timer.start();
	printSetInfo();
	std::cout << "Current score in " << set_num << " set: \t" << games_player1 << " - " << games_player2 << '\n';
}

int Set::addGameResult(const int winning_player_id) {
	updateTime();
	if (winning_player_id == 1) {
		games_player1++;
		updateMatchSetRecordWithoutServingPlayerId();
	}
	else if (winning_player_id == 2) {
		games_player2++;
		updateMatchSetRecordWithoutServingPlayerId();
	}
	else if (winning_player_id != 0) {
		std::cerr << "Invalid player ID: " << winning_player_id << '\n';
		return -1;
	}

	printGameInfo();

	if (isWonSet()) {
		int winner = (games_player1 > games_player2) ? 1 : 2;
		std::cout << "Set won by Player " << winner << "\n";
		return winner;
	}
	else if (isTieBreak()) {
		const int tiebreak_points = getTiebreakPoints();
		int no_point;
		Tiebreak tiebreak;

		bool is_serving_player_printed = false;
		if (!getIsResumedTiebreak()) {
			updateTiebreakStatus();
			tiebreak = Tiebreak(is_player_one_serving, set_num, tiebreak_points);
			tiebreak.saveToDatabase(match_id);
			no_point = 1;
			std::cout << "Score in tiebreak: \t0 - 0\n";
		}
		else
		{
			auto scores = getTieBreakScores();
			if (scores.first != -1 && scores.second != -1) {
				bool is_player_one_serving_var;
				no_point = scores.first + scores.second;

				if (no_point % 4 == 0 || no_point % 4 == 3) {
					is_player_one_serving_var = getIsPlayerOneServing();
				}
				else {
					is_player_one_serving_var = !getIsPlayerOneServing();
				}
					
				tiebreak = Tiebreak(is_player_one_serving_var, set_num, tiebreak_points, scores.first, scores.second);

				if (no_point == 0) {
					std::cout << "Score in tiebreak: \t0 - 0\n";
				}
				else {
					tiebreak.printScore();
					is_serving_player_printed = true;
					tiebreak.printServingPlayer();
				}

				no_point++;
			}
			else {
				std::cerr << "Failed to retrieve tie break scores.\n";
				return -1;
			}
		}
		while (!tiebreak.isTiebreakWon())
		{
			if (no_point == 1 || no_point % 2 == 0) {
				if (!is_serving_player_printed) {
					tiebreak.printServingPlayer();
				}
			}

			bool is_continuing = true;
			int return_value = 0;

			switch (const int choice = getNumericInput("===================================\n"
											  "Enter 1 for Player 1 point, 2 for Player 2 point, 3 to suspend, 4 to finish match: ")) {
			case 1:
				tiebreak.addPoint(1, match_id);
				break;
			case 2:
				tiebreak.addPoint(2, match_id);
				break;
			case 3:
				is_continuing = false;
				return_value = 10;
				break;
			case 4:
				is_continuing = false;
				return_value = 11;
				break;
			default:
				std::cerr << "Invalid choice: " << choice << '\n';
				break;
			}

			updateTime();

			if (!is_continuing) {
				return return_value;
			}

			tiebreak.printScore();

			if ((!(getIsResumedTiebreak() && is_serving_player_printed)) && (no_point % 2 == 1)) {
				tiebreak.updateIsPlayerOneServing();
			}
			else if (!getIsResumedTiebreak() && (no_point % 2 == 1)) {
				tiebreak.updateIsPlayerOneServing();
			}

			no_point++;
			is_serving_player_printed = false;
		}

		const int winner = tiebreak.determineWinner();

		if (winner == 1) {
			games_player1++;
		}
		else {
			games_player2++;
		}

		updateMatchSetRecordWithoutServingPlayerId();

		tiebreak.updateInDatabase(match_id);
		printGameInfo();
		return winner;
	}
	else
	{
		return 0;
	}
}

bool Set::isTieBreak() const
{
	if (games_player1 == 6 && games_player2 == 6) {
		if ((set_num * 2 - 1) == no_sets) {
			std::cout << "Super tiebreak in set " << set_num << '\n';
		}
		else {
			std::cout << "Tiebreak in set " << set_num << '\n';
		}
		return true;
	}
	return false;
}

void Set::addMatchSetRecord() {
	const long long elapsed_seconds = timer.stop();
	duration += std::chrono::seconds(elapsed_seconds);
	timer.start();
	saveSetRecordToDatabase(true);
}

void Set::updateMatchSetRecord() {
	const long long elapsed_seconds = timer.stop();
	duration += std::chrono::seconds(elapsed_seconds);
	timer.start();
	saveSetRecordToDatabase(false);
}

void Set::saveSetRecordToDatabase(const bool is_new_record) const {
	DatabaseConnection& db = DatabaseConnection::getInstance();
	try {
		pqxx::work txn(*db.getConnection());
		std::string query;
		if (is_new_record) {
			query = "INSERT INTO matches_sets (match_id, set_number, games_won_player1, games_won_player2, is_first_player_serving) VALUES (" +
				txn.quote(match_id) + ", " +
				txn.quote(set_num) + ", " +
				txn.quote(games_player1) + ", " +
				txn.quote(games_player2) + ", " +
				txn.quote(is_player_one_serving) + ")";
		}
		else {
			query = "UPDATE matches_sets SET games_won_player1 = " + txn.quote(games_player1) + ", " +
				"games_won_player2 = " + txn.quote(games_player2) + ", " +
				"is_first_player_serving = " + txn.quote(is_player_one_serving) + ", " +
				" WHERE match_id = " + txn.quote(match_id) + " AND set_number = " + txn.quote(set_num);
		}
		txn.exec(query);
		txn.commit();
	}
	catch (const std::exception& e) {
		std::cerr << "Error saving set record to database: " << e.what() << '\n';
		throw;
	}
}

void Set::updateTiebreakStatus() const {
	DatabaseConnection& db = DatabaseConnection::getInstance();
	try {
		pqxx::work txn(*db.getConnection());
		const std::string query = "UPDATE matches_sets SET is_tie_break = TRUE WHERE match_id = " +
			txn.quote(match_id) + " AND set_number = " + txn.quote(set_num);
		txn.exec(query);
		txn.commit();
	}
	catch (const std::exception& e) {
		std::cerr << "Error marking set as tiebreak in database: " << e.what() << '\n';
		throw;
	}
}

void Set::updateMatchSetRecordWithoutServingPlayerId() {
	const long long elapsed_seconds = timer.stop();
	duration += std::chrono::seconds(elapsed_seconds);
	timer.start();
	DatabaseConnection& db = DatabaseConnection::getInstance();
	try {
		pqxx::work txn(*db.getConnection());
		const std::string query = "UPDATE matches_sets SET " +
			std::string("games_won_player1 = ") + txn.quote(games_player1) + ", " +
			"games_won_player2 = " + txn.quote(games_player2) +
			" WHERE match_id = " + txn.quote(match_id) + " AND set_number = " + txn.quote(set_num);
		txn.exec(query);
		txn.commit();
	}
	catch (const std::exception& e) {
		std::cerr << "Error updating set record in database: " << e.what() << '\n';
		throw;
	}
}

void Set::resumeCurrentGame(const bool is_serving) {
	if (!current_game.has_value()) {
		DatabaseConnection& db = DatabaseConnection::getInstance();
		try {
			pqxx::nontransaction nt(*db.getConnection());
			const std::string query =
				"SELECT match_id, set_number, game_number, player1_points, player2_points "
				"FROM public.game_points "
				"WHERE match_id = " + std::to_string(match_id) + " "
				"ORDER BY set_number DESC, game_number DESC "
				"LIMIT 1;";
			pqxx::result r = nt.exec(query);

			if (r.size() == 1) {
				const int game_number = r[0]["game_number"].as<int>();
				const std::string player1_points_str = r[0]["player1_points"].c_str();
				const std::string player2_points_str = r[0]["player2_points"].c_str();
				const int player1_points = convertScore(player1_points_str);
				const int player2_points = convertScore(player2_points_str);

				if (player1_points == -1 || player2_points == -1) {
					std::cerr << "Error converting scores." << '\n';
					return;
				}

				current_game = Game(player1_points, player2_points, game_number);
				current_game->setIsPlayerOneServing(is_serving);
				std::cout << "  Player " << (is_serving ? "1" : "2") << " is serving.\n";
				current_game->printCurScore();
			}
			else {
				std::cerr << "No game points found for match_id: " << match_id << " and set_number: " << set_num << '\n';
			}
		}
		catch (const std::exception& e) {
			std::cerr << "Exception in resumeCurGame: " << e.what() << '\n';
		}
	}
}

int Set::convertScore(const std::string& db_score) {
	if (db_score == "0") return 0;
	if (db_score == "15") return 1;
	if (db_score == "30") return 2;
	if (db_score == "40") return 3;
	if (db_score == "A") return 4;
	std::cerr << "Unknown score: " << db_score << '\n';
	return -1;
}

std::pair<int, int> Set::getTieBreakScores() const {
	DatabaseConnection& db = DatabaseConnection::getInstance();
	try {
		pqxx::nontransaction nt(*db.getConnection());
		const std::string query =
			"SELECT player1_score, player2_score "
			"FROM public.tie_breaks "
			"WHERE match_id = " + nt.quote(match_id) + " AND set_number = " + nt.quote(set_num) +
			" ORDER BY tie_break_id DESC LIMIT 1;";
		pqxx::result r = nt.exec(query);

		if (r.size() == 1) {
			int player1_score = r[0]["player1_score"].as<int>();
			int player2_score = r[0]["player2_score"].as<int>();
			return std::make_pair(player1_score, player2_score);
		}
		else {
			std::cerr << "Tie break not found for match_id: " << match_id << " and set_number: " << set_num << '\n';
			return std::make_pair(-1, -1);
		}
	}
	catch (const std::exception& e) {
		std::cerr << "Exception in getTieBreakScores: " << e.what() << '\n';
		throw;
	}
}

void Set::updateTime() {
	const long long elapsed_seconds = timer.stop();
	duration = std::chrono::seconds(elapsed_seconds);
	timer.start();

	DatabaseConnection& db = DatabaseConnection::getInstance();
	try {
		pqxx::work w(*db.getConnection());

		const int duration_seconds = std::chrono::duration_cast<std::chrono::seconds>(duration).count();
		const int hours = duration_seconds / 3600;
		const int minutes = (duration_seconds % 3600) / 60;
		const int seconds = duration_seconds % 60;

		const std::string interval_string = std::to_string(hours) + " hours " +
			std::to_string(minutes) + " minutes " +
			std::to_string(seconds) + " seconds";

		const std::string match_query = "UPDATE public.matches SET duration = duration + interval '" +
			interval_string + "' WHERE id = " + w.quote(match_id) + ";";

		w.exec(match_query);
		w.commit();
	}
	catch (const pqxx::sql_error& e) {
		std::cerr << "SQL error in updateTime: " << e.what() << '\n';
	}
	catch (const std::exception& e) {
		std::cerr << "Exception in updateTime: " << e.what() << '\n';
	}
}