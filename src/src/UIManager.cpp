#include "UIManager.hpp"
#include "Player.hpp"
#include "Match.hpp"
#include "validate.hpp"
#include <iostream>
#include <tabulate/table.hpp>

int getNumericInput(const std::string& prompt) {
	int input;
	while (true) {
		std::cout << prompt;
		std::cin >> input;
		if (std::cin.fail()) {
			std::cin.clear();
			std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
			std::cout << "Invalid input. Please enter a valid number.";
		}
		else {
			std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
			return input;
		}
	}
}

void UIManager::addPlayer() {
	std::string first_name, last_name;
	char confirmation;

	while (true) {
		std::cout << "Enter First Name: ";
		std::cin >> first_name;
		std::cout << "Enter Last Name: ";
		std::cin >> last_name;
		std::cout << "You have entered: First Name: " << first_name << ", Last Name: " << last_name << '\n';
		std::cout << "Do you want to save this data? (y/Y for yes): ";
		std::cin >> confirmation;

		if (confirmation == 'y' || confirmation == 'Y') {
			Player::addPlayer(first_name, last_name);
			break;
		}
		else {
			std::cout << "Let's try again.\n";
		}
	}
}

void UIManager::showPlayers() {
	const int player_id = getNumericInput("Enter Player ID (or -1 for all): ");
	Player::showPlayerStatistics(player_id);
}

void UIManager::addMatch() {
	const int player1_id = getNumericInput("Enter Player 1 ID: ");
	const int player2_id = getNumericInput("Enter Player 2 ID: ");

	if (player1_id == player2_id) {
		std::cerr << "Player IDs must be different.\n";
		return;
	}

	if (!(Player::exists(player1_id) && Player::exists(player2_id))) {
		std::cerr << "One or both player IDs do not exist.\n";
		return;
	}

	int no_sets = getNumericInput("Enter number of sets (max " + std::to_string(MAX_SETS_TO_WIN) + "): ");
	while (no_sets < 1 || no_sets > MAX_SETS_TO_WIN) {
		std::cerr << "Number of sets must be between 1 and " << MAX_SETS_TO_WIN << ".\n";
		no_sets = getNumericInput("Enter number of sets (max " + std::to_string(MAX_SETS_TO_WIN) + "): ");
	}

	std::tm date_tm = {}, time_tm = {};
	getDateAndTimeFromUser(date_tm, time_tm);
	date_tm.tm_isdst = -1;
	time_t match_time = mktime(&date_tm);
	if (match_time == -1) {
		std::cerr << "Failed to convert time.\n";
		return;
	}

	constexpr size_t buffer_size = 80;
	char buffer[buffer_size];

	if (strftime(buffer, buffer_size, "%Y-%m-%d %H:%M:%S", &date_tm) == 0) {
		std::cerr << "Failed to format date-time.\n";
		return;
	}

	const std::string predicted_start_time(buffer);

	try {
		Match new_match(player1_id, player2_id, no_sets, predicted_start_time);
		std::cout << "New match scheduled.\n";
	}
	catch (const std::exception& e) {
		std::cerr << "An error occurred while scheduling the match: " << e.what() << '\n';
	}
}

void UIManager::showMatches() {
	const int match_id = getNumericInput("Enter match ID (or -1 for all): ");
	DatabaseConnection& db = DatabaseConnection::getInstance();
	pqxx::connection* conn = db.getConnection();
	std::string query = R"(
	    SELECT m.id, ms.status, m.player_id1, m.player_id2, 
	           COALESCE(m.winner_id::text, 'N/A') AS winner_id,
	           m.predicted_start_time, 
               EXTRACT(HOUR FROM m.duration) || ':' || LPAD(EXTRACT(MINUTE FROM m.duration)::text, 2, '0') AS duration,
		       m.no_sets,
	           COALESCE(m.actual_start_time::text, 'N/A') AS actual_start_time
	    FROM public.matches m
	    JOIN public.match_status ms ON m.status_id = ms.id
	)";

	if (match_id != -1) {
		query += " WHERE m.id = $1";
	}

	query += " ORDER BY m.id ASC";

	try {
		pqxx::nontransaction nt(*conn);
		pqxx::result result = (match_id != -1) ? nt.exec_params(query, match_id) : nt.exec(query);

		if (result.empty()) {
			std::cout << (match_id == -1 ? "No matches found." : "No match with id " + std::to_string(match_id) + " found.") << '\n';
		}
		else {
			tabulate::Table table;
			table.add_row({ "ID", "Status", "Player ID1", "Player ID2", "Winner ID", "Predicted Start Time", "Duration", "No Sets", "Actual Start Time" });

			for (const auto& row : result) {
				table.add_row({
					row[0].as<std::string>(), row[1].as<std::string>(), row[2].as<std::string>(),
					row[3].as<std::string>(), row[4].as<std::string>(), row[5].as<std::string>(),
					row[6].as<std::string>(), row[7].as<std::string>(), row[8].as<std::string>()
					});
			}

			std::cout << table << '\n';
		}
	}
	catch (const pqxx::sql_error& e) {
		std::cerr << "SQL error: " << e.what() << " Query: " << e.query() << '\n';
	}
	catch (const std::exception& e) {
		std::cerr << "Database error: " << e.what() << '\n';
	}
}

void UIManager::startMatch() {
	while (true) {
		const int match_id = getNumericInput("Enter match ID (-1 for exit): ");

		if (match_id == -1) {
			break;
		}

		try {
			std::optional<Match> optional_match = Match::getMatchById(match_id, { "Pending", "Delayed" });

			if (optional_match) {
				Match& match = *optional_match;
				match.startMatch(match_id);

				bool is_continuing = true;

				while (match.getSetsPlayerOne() < match.getNoSets() && match.getSetsPlayerTwo() < match.getNoSets()) {
					switch (const int choice = getNumericInput("===================================\n"
						"Enter 1 for Player 1 point, 2 for Player 2 point, 3 to suspend, 4 to finish match: ")) {
					case 1:
						match.getCurrentSet()->getCurrentGame()->addPoint(1, match_id, match.getCurrentSet()->getSetNum());
						break;
					case 2:
						match.getCurrentSet()->getCurrentGame()->addPoint(2, match_id, match.getCurrentSet()->getSetNum());
						break;
					case 3:
						match.suspendMatch();
						is_continuing = false;
						break;
					case 4:
						handleMatchFinishing(match);
						is_continuing = false;
						break;
					default:
						std::cout << "Invalid choice. Please try again.\n";
						break;
					}

					if (!is_continuing) {
						return;
					}

					int is_finishing_or_suspending = processGameEnd(match);

					if (is_finishing_or_suspending == 10 || is_finishing_or_suspending == 11) {
						return;
					}
					if (is_finishing_or_suspending == 1 || is_finishing_or_suspending == 2) {
						updateMatchStatus(match);
					}

				}
			}
		}
		catch (const std::invalid_argument& e) {
			std::cerr << "Error: " << e.what() << '\n';
		}
		catch (const std::exception& e) {
			std::cerr << "Error: " << e.what() << '\n';
		}
	}
}

void UIManager::run() {
	while (true) {
		std::cout << "\n***************************************\n"
			<< "1. Add Player\n"
			<< "2. Show Players\n"
			<< "3. Show Player's Matches\n"
			<< "4. Add Match\n"
			<< "5. Show Full Matches Info\n"
			<< "6. Show Matches Info\n"
			<< "7. Show Match Details\n"
			<< "8. Start Match\n"
			<< "9. Resume Match\n"
			<< "0. Exit\n"
			<< "***************************************\n";

		switch (const int choice = getNumericInput("\nEnter choice: ")) {
		case 1:
			addPlayer();
			break;
		case 2:
			showPlayers();
			break;
		case 3:
			showMatchesResultsForPlayer();
			break;
		case 4:
			addMatch();
			break;
		case 5:
			showMatches();
			break;
		case 6:
			showAllMatches();
			break;
		case 7:
			showMatchDetails();
			break;
		case 8:
			startMatch();
			break;
		case 9:
			resumeMatch();
			break;
		case 0:
			std::cout << "Exiting program.\n";
			return;
		default:
			std::cout << "Invalid choice. Please try again.\n";
			break;
		}
	}
}

void UIManager::resumeMatch()
{
	bool exit_outer_loop = false;

	while (!exit_outer_loop) {
		const int match_id = getNumericInput("Enter match ID (-1 for exit): ");

		if (match_id == -1) {
			break;
		}

		std::optional<Match> match_opt;
		try {
			match_opt = Match::getMatchById(match_id, { "Suspended" });

			if (match_opt) {
				Match& match = match_opt.value();
				match.resumeMatch(match_id);

				bool is_ended = false;

				if (match.getCurrentSet()->getIsResumedTiebreak()) {
					switch (const int game_status = match.getCurrentSet()->addGameResult(0)) {
					case 10:
						match.suspendMatch();
						exit_outer_loop = true;
						break;
					case 11:
						handleMatchFinishing(match);
						exit_outer_loop = true;
						break;
					case -1:
						exit_outer_loop = true;
						break;
					default:
						handleSetContinuation(match, game_status);
						break;
					}
				}

				updateMatchStatus(match);
			}
			else {
				std::cout << "Please try again.\n";
				continue;
			}
		}
		catch (const std::invalid_argument& e) {
			std::cerr << "Error: " << e.what() << '\n';
			std::cout << "Please try again.\n";
			continue;
		}
		catch (const std::exception& e) {
			std::cerr << "Error: " << e.what() << '\n';
			std::cout << "Please try again.\n";
			continue;
		}

		if (exit_outer_loop) {
			break;
		}

		bool is_continuing = true;
		Match& match = match_opt.value();

		while (match.getSetsPlayerOne() < match.getNoSets() && match.getSetsPlayerTwo() < match.getNoSets()) {
			switch (const int choice = getNumericInput("===================================\n"
				"Enter 1 for Player 1 point, 2 for Player 2 point, 3 to suspend, 4 to finish match: ")) {
			case 1:
				match.getCurrentSet()->getCurrentGame()->addPoint(1, match_id, match.getCurrentSet()->getSetNum());
				break;

			case 2:
				match.getCurrentSet()->getCurrentGame()->addPoint(2, match_id, match.getCurrentSet()->getSetNum());
				break;

			case 3:
				match.suspendMatch();
				exit_outer_loop = true;
				is_continuing = false;
				break;

			case 4:
				handleMatchFinishing(match);
				exit_outer_loop = true;
				is_continuing = false;
				break;

			default:
				std::cout << "Invalid choice. Please try again.\n";
				break;
			}

			if (!is_continuing) {
				break;
			}

			int is_finishing_or_suspending = processGameEnd(match);

			if (is_finishing_or_suspending == 10 || is_finishing_or_suspending == 11) {
				return;
			}
			if (is_finishing_or_suspending == 1 || is_finishing_or_suspending == 2) {
				updateMatchStatus(match);
			}
		}
	}
}

void UIManager::handleMatchFinishing(Match& match) {
	int winner_id;
	do {
		winner_id = getNumericInput("Enter the winner of the match (1 for Player 1 / 2 for Player 2): ");
		if (winner_id != 1 && winner_id != 2) {
			std::cerr << "Invalid choice. Please enter 1 or 2.\n";
		}
	} while (winner_id != 1 && winner_id != 2);

	const int actual_winner_id = (winner_id == 1) ? match.getIdPlayerOne() : match.getIdPlayerTwo();
	match.setWinner(actual_winner_id);
	match.getCurrentSet()->updateTime();
	match.finishMatch();
}

void UIManager::updateMatchStatus(Match& match) {
	const int sets_player_one = match.getSetsPlayerOne();
	const int sets_player_two = match.getSetsPlayerTwo();
	const int no_sets = match.getNoSets();

	if (sets_player_one >= no_sets || sets_player_two >= no_sets) {
		const int winner_id = (sets_player_one > sets_player_two) ? match.getIdPlayerOne() : match.getIdPlayerTwo();
		match.endMatch(winner_id);
	}
}

int UIManager::processGameEnd(Match& match)
{
	const int is_game_ended = match.getCurrentSet()->getCurrentGame()->determineWinner();

	if (is_game_ended == 0) {
		return 0;
	}

	int return_value = 0;

	switch (const int game_status = match.getCurrentSet()->addGameResult(is_game_ended)) {
	case 10:
		handleMatchSuspension(match);
		return_value = 10;
		break;
	case 11:
		handleMatchFinishing(match);
		return_value = 11;
		break;
	case 0:
		match.getCurrentSet()->changeGame();
		break;
	case -1:
		break;
	default:
		return_value = game_status;
		if (game_status == 1) {
			match.updateSetsPlayerOne();
		}
		else if (game_status == 2) {
			match.updateSetsPlayerTwo();
		}

		std::cout << "Score in sets: \t" << match.getSetsPlayerOne() << " - " << match.getSetsPlayerTwo() << std::endl;
		
		match.updateMatchInDatabase();

		if (!match.isMatchWinner()) {
			match.updateCurrentSet(match.getId());
		}
		break;
	}

	return return_value;
}

void UIManager::handleSetContinuation(Match& match, const int game_status) {
	if (game_status == 1) {
		match.updateSetsPlayerOne();
	}
	else if (game_status == 2) {
		match.updateSetsPlayerTwo();
	}

	const int num_of_sets = match.getSetsPlayerOne() + match.getSetsPlayerTwo();
	match.printScoreInfo();
	match.updateMatchInDatabase();

	if (!match.isMatchWinner()) {
		match.updateCurrentSet(match.getId());
	}
}

void UIManager::handleMatchSuspension(Match& match) {
	match.suspendMatch();
}

void UIManager::showAllMatches() {
	DatabaseConnection& db = DatabaseConnection::getInstance();
	pqxx::nontransaction nt(*db.getConnection());

	try {
		const std::string query = R"(
        SELECT 
            m.id, ms.status, 
            p1.first_name || ' ' || p1.last_name AS player1_name, 
            p2.first_name || ' ' || p2.last_name AS player2_name, 
            COALESCE(w.first_name || ' ' || w.last_name, 'N/A') AS winner_name, 
            m.no_sets AS no_sets, 
            EXTRACT(HOUR FROM m.duration) || ':' || LPAD(EXTRACT(MINUTE FROM m.duration)::text, 2, '0') AS duration,
			SUM(CASE WHEN (mse.games_won_player1 >= 6 AND mse.games_won_player1 >= mse.games_won_player2 + 2) OR mse.games_won_player1 = 7 THEN 1 ELSE 0 END) AS player1_sets_won,
            SUM(CASE WHEN (mse.games_won_player2 >= 6 AND mse.games_won_player2 >= mse.games_won_player1 + 2) OR mse.games_won_player2 = 7 THEN 1 ELSE 0 END) AS player2_sets_won
		FROM 
            public.matches m
        JOIN 
            public.players p1 ON m.player_id1 = p1.id
        JOIN 
            public.players p2 ON m.player_id2 = p2.id
        LEFT JOIN 
            public.players w ON m.winner_id = w.id
        LEFT JOIN 
            public.match_status ms ON m.status_id = ms.id
        LEFT JOIN 
            public.matches_sets mse ON m.id = mse.match_id
        GROUP BY 
		    m.id, ms.status, p1.first_name, p1.last_name, p2.first_name, p2.last_name, w.first_name, w.last_name, m.no_sets, m.duration
		ORDER BY 
			m.id ASC;
    )";

		pqxx::result r = nt.exec(query);

		if (r.empty()) {
			std::cout << "No matches found.\n";
		}
		else {
			tabulate::Table table;
			table.add_row({ "ID", "Status", "Player 1", "Player 2", "Winner", "Score in Sets", "Duration" });

			for (const auto& row : r) {
				std::string sets_score = row["player1_sets_won"].as<std::string>() + ":" + row["player2_sets_won"].as<std::string>();

				table.add_row({
					row["id"].as<std::string>(),
					row["status"].as<std::string>(),
					row["player1_name"].as<std::string>(),
					row["player2_name"].as<std::string>(),
					row["winner_name"].as<std::string>(),
					sets_score,
					row["duration"].as<std::string>()
					});
			}

			std::cout << table << '\n';
		}
	}
	catch (const std::exception& e) {
		std::cerr << "Exception: " << e.what() << '\n';
	}
}

void UIManager::showMatchDetails() {
	const int match_id = getNumericInput("Enter match ID: ");
	DatabaseConnection& db = DatabaseConnection::getInstance();
	pqxx::nontransaction nt(*db.getConnection());

	std::string match_query = R"(
	    SELECT 
	        m.id, 
	        ms.status, 
	        p1.first_name || ' ' || p1.last_name AS player1_name, 
	        p2.first_name || ' ' || p2.last_name AS player2_name, 
	        COALESCE(w.first_name || ' ' || w.last_name, 'N/A') AS winner_name, 
	        m.no_sets AS score_in_sets, 
	        SUM(CASE WHEN (mse.games_won_player1 >= 6 AND mse.games_won_player1 >= mse.games_won_player2 + 2) OR mse.games_won_player1 = 7 THEN 1 ELSE 0 END) AS player1_sets_won,
            SUM(CASE WHEN (mse.games_won_player2 >= 6 AND mse.games_won_player2 >= mse.games_won_player1 + 2) OR mse.games_won_player2 = 7 THEN 1 ELSE 0 END) AS player2_sets_won,
	        EXTRACT(HOUR FROM m.duration) || ':' || LPAD(EXTRACT(MINUTE FROM m.duration)::text, 2, '0') AS duration
	    FROM 
	        public.matches m
	    JOIN 
	        public.players p1 ON m.player_id1 = p1.id
	    JOIN 
	        public.players p2 ON m.player_id2 = p2.id
	    LEFT JOIN 
	        public.players w ON m.winner_id = w.id
	    LEFT JOIN 
	        public.match_status ms ON m.status_id = ms.id
	    LEFT JOIN 
	        public.matches_sets mse ON m.id = mse.match_id
	    WHERE 
	        m.id = )" + nt.quote(match_id) + R"(
	    GROUP BY 
	        m.id, ms.status, p1.first_name, p1.last_name, p2.first_name, p2.last_name, w.first_name, w.last_name, m.no_sets, m.duration
	)";

	pqxx::result match_result = nt.exec(match_query);

	if (match_result.empty()) {
		std::cout << "No match found with ID " << match_id << ".\n";
		return;
	}

	const auto& match_row = match_result[0];

	std::string sets_score = match_row["player1_sets_won"].as<std::string>() + ":" + match_row["player2_sets_won"].as<std::string>();

	tabulate::Table match_table;
	match_table.add_row({ "ID", "Status", "Player 1", "Player 2", "Winner", "Score in Sets", "Duration" });
	match_table.add_row({
		match_row["id"].as<std::string>(),
		match_row["status"].as<std::string>(),
		match_row["player1_name"].as<std::string>(),
		match_row["player2_name"].as<std::string>(),
		match_row["winner_name"].as<std::string>(),
		sets_score,
		match_row["duration"].as<std::string>()
		});

	std::cout << match_table << '\n';

	std::string sets_query = R"(
	    SELECT 
	        set_number, 
	        games_won_player1, 
	        games_won_player2
	    FROM 
	        public.matches_sets
	    WHERE 
	        match_id = )" + nt.quote(match_id) + R"( 
	    ORDER BY 
	        set_number ASC
	)";

	pqxx::result sets_result = nt.exec(sets_query);

	if (!sets_result.empty()) {
		tabulate::Table sets_table;
		sets_table.add_row({ "Set Number", "Games Won Player 1", "Games Won Player 2" });

		for (const auto& row : sets_result) {
			sets_table.add_row({
				row["set_number"].as<std::string>(),
				row["games_won_player1"].as<std::string>(),
				row["games_won_player2"].as<std::string>()
				});
		}

		std::cout << sets_table << '\n';
	}

	std::string games_query = R"(
        SELECT 
            set_number, game_number, player1_points, player2_points
        FROM 
            public.game_points
        WHERE 
            match_id = )" + nt.quote(match_id) + R"( 
        ORDER BY 
            set_number ASC, game_number ASC;
    )";

	pqxx::result games_result = nt.exec(games_query);

	if (!games_result.empty()) {
		tabulate::Table games_table;
		games_table.add_row({ "Set Number", "Game Number", "Player 1 Points", "Player 2 Points" });

		for (const auto& row : games_result) {
			games_table.add_row({
				row["set_number"].as<std::string>(),
				row["game_number"].as<std::string>(),
				row["player1_points"].as<std::string>(),
				row["player2_points"].as<std::string>()
				});
		}
		std::cout << games_table << '\n';
	}

	std::string tiebreaks_query = R"(
        SELECT 
            set_number, player1_score, player2_score
        FROM 
            public.tie_breaks
        WHERE 
            match_id = )" + nt.quote(match_id) + R"( 
        ORDER BY 
            set_number ASC;
    )";

	pqxx::result tiebreaks_result = nt.exec(tiebreaks_query);

	if (!tiebreaks_result.empty()) {
		tabulate::Table tiebreaks_table;
		tiebreaks_table.add_row({ "Set Number", "Player 1 Score", "Player 2 Score" });

		for (const auto& row : tiebreaks_result) {
			tiebreaks_table.add_row({
				row["set_number"].as<std::string>(),
				row["player1_score"].as<std::string>(),
				row["player2_score"].as<std::string>()
				});
		}
		std::cout << tiebreaks_table << '\n';
	}
}

void UIManager::showMatchesResultsForPlayer() {
	const int player_id = getNumericInput("Enter Player ID: ");
	DatabaseConnection& db = DatabaseConnection::getInstance();

	try {
		std::string query_ = "SELECT * FROM public.players WHERE id = " + std::to_string(player_id);
		pqxx::nontransaction nt1(*db.getConnection());
		pqxx::result r_ = nt1.exec(query_);
		nt1.commit();

		if (r_.empty()) {
			std::cout << "No user found.\n";
			return;
		}

		const std::string query = R"(
		    SELECT 
		        m.id, 
		        p1.first_name || ' ' || p1.last_name AS player1_name, 
		        p2.first_name || ' ' || p2.last_name AS player2_name, 
		        COALESCE(w.first_name || ' ' || w.last_name, 'N/A') AS winner_name, 
		        m.no_sets AS score_in_sets, 
		        EXTRACT(HOUR FROM m.duration) || ':' || LPAD(EXTRACT(MINUTE FROM m.duration)::text, 2, '0') AS duration,
		        SUM(CASE WHEN (ms.games_won_player1 >= 6 AND ms.games_won_player1 >= ms.games_won_player2 + 2) OR ms.games_won_player1 = 7 THEN 1 ELSE 0 END) AS player1_sets_won,
				SUM(CASE WHEN (ms.games_won_player2 >= 6 AND ms.games_won_player2 >= ms.games_won_player1 + 2) OR ms.games_won_player2 = 7 THEN 1 ELSE 0 END) AS player2_sets_won
			FROM 
		        public.matches m
		    JOIN 
		        public.players p1 ON m.player_id1 = p1.id
		    JOIN 
		        public.players p2 ON m.player_id2 = p2.id
		    LEFT JOIN 
		        public.players w ON m.winner_id = w.id
		    LEFT JOIN 
		        public.matches_sets ms ON m.id = ms.match_id
		    WHERE 
		        m.player_id1 = )" + nt1.quote(player_id) + R"( OR m.player_id2 = )" + nt1.quote(player_id) + R"(
		    GROUP BY 
		        m.id, p1.first_name, p1.last_name, p2.first_name, p2.last_name, w.first_name, w.last_name, m.no_sets, m.duration
		    ORDER BY 
		        m.id ASC
		)";

		pqxx::nontransaction nt2(*db.getConnection());
		pqxx::result r = nt2.exec(query);

		if (r.empty()) {
			std::cout << "No matches found for player with ID " << player_id << ".\n";
		}
		else {
			tabulate::Table table;
			table.add_row({ "ID", "Player 1", "Player 2", "Winner", "Score in Sets", "Duration" });

			for (const auto& row : r) {
				std::string sets_score = row["player1_sets_won"].as<std::string>() + ":" + row["player2_sets_won"].as<std::string>();

				table.add_row({
					row["id"].as<std::string>(),
					row["player1_name"].as<std::string>(),
					row["player2_name"].as<std::string>(),
					row["winner_name"].as<std::string>(),
					sets_score,
					row["duration"].as<std::string>()
					});
			}

			std::cout << table << '\n';
		}
	}
	catch (const std::exception& e) {
		std::cerr << "Exception: " << e.what() << '\n';
	}
}