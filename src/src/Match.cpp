#include "Match.hpp"
#include "MatchState.hpp"
#include "Player.hpp"
#include <tabulate/table.hpp>
#include <iostream>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <pqxx/pqxx>
#include <random>
#include <optional>

Match::Match(const int player1_id, const int player2_id, const int no_sets, const std::string& predicted_start_time)
    : player_id1(player1_id), player_id2(player2_id), no_sets(no_sets), current_state(nullptr) {

	if (!playerExists(player_id1) || !playerExists(player_id2)) {
        throw std::invalid_argument("One or both players do not exist.");
    }

    changeState(new PendingState());
    current_state->handle(this);

    if (!isLaterThanNow(predicted_start_time)) {
        changeState(new DelayedState());
        current_state->handle(this);
    }

    saveToDatabase(predicted_start_time);
}

Match::Match(const int id, const int player1_id, const int player2_id, const int no_sets, const std::chrono::seconds duration) :
	id(id), player_id1(player1_id), player_id2(player2_id), no_sets(no_sets), duration(duration), current_state(nullptr), status_id(0) {}

Match::~Match() {
    delete current_state;
}

bool Match::playerExists(const int player_id) {
    DatabaseConnection& db = DatabaseConnection::getInstance();
    pqxx::nontransaction nt(*db.getConnection());
    
    try {
        const std::string query = "SELECT COUNT(*) FROM public.players WHERE id = " + nt.quote(player_id) + ";";
        pqxx::result r = nt.exec(query);

        if (r.size() == 1 && r[0].size() == 1) {
            return r[0][0].as<int>() > 0;
        }
        else {
            std::cerr << "Unexpected query result size in playerExists.\n";
            return false;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Exception in playerExists: " << e.what() << '\n';
        return false;
    }
}

void Match::startMatch(const int match_id) {
    timer.start();
    const std::string actual_start_time = getCurrentTime();
	changeState(new StartedState());
    current_state->handle(this);

    if (id == -1) {
        std::cerr << "Match ID not set. Cannot update match.\n";
        return;
    }

    DatabaseConnection& db = DatabaseConnection::getInstance();
    try {
        pqxx::work w(*db.getConnection());
        const std::string match_query = "UPDATE public.matches SET actual_start_time = '" + actual_start_time +
            "', status_id = " + std::to_string(status_id) + " WHERE id = " + std::to_string(id) + ";";
        w.exec(match_query);
        w.commit();
        std::cout << "Match with ID " << id << " is starting.\n";
    }
    catch (const pqxx::sql_error& e) {
        std::cerr << "SQL error in startMatch: " << e.what() << '\n';
    }
    catch (const std::exception& e) {
        std::cerr << "Exception in startMatch: " << e.what() << '\n';
    }

    displayPlayerInfo();
    initializeCurrentSet(match_id);
}

void Match::endMatch(int winning_player_id) {
    std::cout << "Winner is player with ID " << winning_player_id << '\n';
    const long long elapsed_minutes = timer.stop();
    duration += std::chrono::minutes(elapsed_minutes);

    winner_id = winning_player_id;
    changeState(new FinishedState());
    current_state->handle(this);

    if (id == -1) {
        std::cerr << "Match ID not set. Cannot update match.\n";
        return;
    }

    DatabaseConnection& db = DatabaseConnection::getInstance();
    try {
        pqxx::work w(*db.getConnection());

        const int duration_minutes = std::chrono::duration_cast<std::chrono::minutes>(duration).count();

        const std::string match_query = "UPDATE public.matches SET "
            "winner_id = " + w.quote(winner_id) + ", "
            "status_id = 3, "
            "duration = duration + interval '" + std::to_string(duration_minutes) + " minutes' "
            "WHERE id = " + w.quote(id) + ";";

        w.exec(match_query);
        w.commit();
        std::cout << "Match with ID " << id << " is finishing.\n";
    }
    catch (const pqxx::sql_error& e) {
        std::cerr << "SQL error in endMatch: " << e.what() << '\n';
    }
    catch (const std::exception& e) {
        std::cerr << "Exception in endMatch: " << e.what() << '\n';
    }

    if (winner_id == player_id1) {
        Player::updateMatchResults(player_id1, player_id2);
    }
    else {
        Player::updateMatchResults(player_id2, player_id1);
    }
}

void Match::suspendMatch() {
    changeState(new SuspendedState());
    current_state->handle(this);
    updateStatusInDatabase();
    getCurrentSet()->updateTime();
    std::cout << "Match is suspended.\n";
}

void Match::saveToDatabase(const std::string& predicted_start_time) {
    DatabaseConnection& db = DatabaseConnection::getInstance();
    try {
        pqxx::work w(*db.getConnection());
        const int duration_minutes = std::chrono::duration_cast<std::chrono::minutes>(duration).count();
        const std::string query = "INSERT INTO public.matches (status_id, player_id1, player_id2, predicted_start_time, duration, no_sets) "
            "VALUES ($1, $2, $3, $4, interval '" + std::to_string(duration_minutes) + " minutes', $5) RETURNING id;";
        const pqxx::result r = w.exec_params(query, status_id, player_id1, player_id2, predicted_start_time, no_sets);
        w.commit();

        if (!r.empty()) {
            id = r[0][0].as<int>();
            std::cout << "Inserted match with ID " << id << '\n';
        }
        else {
            throw std::runtime_error("Failed to retrieve the inserted match ID.");
        }
    }
    catch (const pqxx::sql_error& e) {
        std::cerr << "SQL error in saveToDatabase: " << e.what() << '\n';
    }
    catch (const std::exception& e) {
        std::cerr << "Exception in saveToDatabase: " << e.what() << '\n';
    }
}

void Match::updateStatusInDatabase() const {
    if (id == -1) {
        std::cerr << "Match ID not set. Cannot update match.\n";
        return;
    }

    DatabaseConnection& db = DatabaseConnection::getInstance();
    try {
        pqxx::work w(*db.getConnection());
        const std::string match_query = "UPDATE public.matches SET status_id = $1 WHERE id = $2;";
        w.exec_params(match_query, status_id, id);
        w.commit();
    }
    catch (const pqxx::sql_error& e) {
        std::cerr << "SQL error in updateStatusInDatabase: " << e.what() << '\n';
    }
    catch (const std::exception& e) {
        std::cerr << "Exception in updateStatusInDatabase: " << e.what() << '\n';
    }
}

std::string Match::getCurrentTime() {
    const auto now = std::chrono::system_clock::now();
    const auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm tm_buf;

    if (localtime_s(&tm_buf, &in_time_t) != 0) {
        throw std::runtime_error("Failed to get local time");
    }

    std::stringstream ss;
    ss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

bool Match::isLaterThanNow(const std::string& date_time_str) {
    std::tm tm = {};
    std::istringstream ss(date_time_str);
    ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");

    if (ss.fail()) {
        std::cerr << "Failed to parse dateTimeStr: " << date_time_str << '\n';
        return false;
    }

    const auto tp = std::chrono::system_clock::from_time_t(std::mktime(&tm));
    const auto now = std::chrono::system_clock::now();

    return tp > now;
}

int Match::getStatusId(const std::string& status_name) {
    DatabaseConnection& db = DatabaseConnection::getInstance();
    try {
        pqxx::nontransaction nt(*db.getConnection());

        const std::string query = "SELECT id FROM public.match_status WHERE status = $1;";
        const pqxx::result r = nt.exec_params(query, status_name);

        if (r.empty()) {
            throw std::runtime_error("Status not found.");
        }

        return r[0][0].as<int>();
    }
    catch (const pqxx::sql_error& e) {
        std::cerr << "SQL error in getStatusId: " << e.what() << '\n';
        throw;
    }
    catch (const std::exception& e) {
        std::cerr << "Exception in getStatusId: " << e.what() << '\n';
        throw;
    }
}

std::optional<Match> Match::getMatchById(const int id, const std::vector<std::string>& statuses) {
    DatabaseConnection& db = DatabaseConnection::getInstance();
    std::vector<int> status_ids;

    try {
        for (const auto& status : statuses) {
            status_ids.push_back(getStatusId(status));
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error while getting status IDs: " << e.what() << '\n';
        return std::nullopt;
    }

    std::stringstream status_condition;
    for (size_t i = 0; i < status_ids.size(); ++i) {
        if (i > 0) {
            status_condition << ", ";
        }
        status_condition << status_ids[i];
    }

    pqxx::result r;
    try {
        pqxx::nontransaction nt(*db.getConnection());
        const std::string query =
            "SELECT id, player_id1, player_id2, no_sets, status_id, EXTRACT(EPOCH FROM duration) AS duration_seconds "
            "FROM public.matches "
            "WHERE id = " + nt.quote(id) + ";";

        r = nt.exec(query);
    }
    catch (const pqxx::sql_error& e) {
        std::cerr << "SQL error: " << e.what() << '\n';
        return std::nullopt;
    }
    catch (const std::exception& e) {
        std::cerr << "Error executing query: " << e.what() << '\n';
        return std::nullopt;
    }

    if (r.empty()) {
        std::cerr << "Match with ID " << id << " not found.\n";
        return std::nullopt;
    }

    const int found_status_id = r[0]["status_id"].as<int>();
    if (std::find(status_ids.begin(), status_ids.end(), found_status_id) == status_ids.end()) {
        std::cerr << "Match found but status does not match. Status: " << getStatusById(found_status_id) << '\n';
        return std::nullopt;
    }

    try {
        const int player1_id = r[0]["player_id1"].as<int>();
        const int player2_id = r[0]["player_id2"].as<int>();
        const int no_sets = r[0]["no_sets"].as<int>();
        std::chrono::seconds duration(0);
        const double duration_seconds = r[0]["duration_seconds"].as<double>();
        duration = std::chrono::seconds(static_cast<int>(duration_seconds));

        return Match(id, player1_id, player2_id, no_sets, duration);
    }
    catch (const std::exception& e) {
        std::cerr << "Error processing query result: " << e.what() << '\n';
        return std::nullopt;
    }
}

void Match::changeState(MatchState* state) {
    if (current_state) {
        delete current_state;
    }
    current_state = state;
}

void Match::initializeCurrentSet(const int match_id) {
    if (!current_set.has_value()) {
        current_set = Set(match_id, no_sets);
        current_set->addMatchSetRecord();
        current_set->initializeCurrentGame(current_set->getIsPlayerOneServing());
    }
}

void Match::resumeCurrentSet(const int match_id) {
    if (!current_set.has_value()) {
        DatabaseConnection& db = DatabaseConnection::getInstance();
        try {
            pqxx::work txn(*db.getConnection());
            const std::string query = "SELECT match_id, set_number, games_won_player1, games_won_player2, "
                "is_tie_break, is_first_player_serving "
                "FROM matches_sets WHERE match_id = " + txn.quote(match_id) + " "
                "ORDER BY set_number DESC LIMIT 1;";
            pqxx::result r = txn.exec(query);

            if (r.size() == 1) {
                const int set_number = r[0]["set_number"].as<int>();
                const int games_player1 = r[0]["games_won_player1"].as<int>();
                const int games_player2 = r[0]["games_won_player2"].as<int>();
                const bool is_first_player_serving = r[0]["is_first_player_serving"].as<bool>();
                const bool is_tiebreak = r[0]["is_tie_break"].as<bool>();
	            txn.commit();
                current_set = Set(match_id, no_sets, set_number, games_player1, games_player2, is_first_player_serving);

                if (is_tiebreak)
                {
                    current_set->setIsResumedTiebreak(true);
                }
                else
                {
                    current_set->setIsResumedTiebreak(false);
                	current_set->setIsPlayerOneServing(is_first_player_serving);

                    if ((games_player1 + games_player2) % 2 == 0) {
                        current_set->resumeCurrentGame(is_first_player_serving);
                    }
                    else
                    {
                        current_set->resumeCurrentGame(!is_first_player_serving);
                    }
                }
                txn.commit();
            }
            else {
                std::cerr << "No sets found for match_id: " << match_id << '\n';
                std::cerr << "You have to start the match.\n";
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Exception in resumeCurSet: " << e.what() << '\n';
        }
    }
}

void Match::updateCurrentSet(const int match_id) {
    const int number_of_games = current_set->getNumberOfGames();
    const bool is_player_one_serving = isPlayerOneServing(number_of_games, current_set->getIsPlayerOneServing());
    current_set = Set(match_id, no_sets, current_set->getSetNum() + 1);
    current_set->setIsPlayerOneServing(is_player_one_serving);
    current_set->addMatchSetRecord();
    current_set->initializeCurrentGame(current_set->getIsPlayerOneServing());
}


void Match::finishMatch() {
    changeState(new FinishedState());
    current_state->handle(this);
    updateStatusInDatabase();
    updateWinnerInDatabase();
    if (winner_id == player_id1) {
        Player::updateMatchResults(player_id1, player_id2);
    }
    else {
        Player::updateMatchResults(player_id2, player_id1);
    }
    std::cout << "Match is finished.\n";
}

void Match::updateWinnerInDatabase() const {
    if (id == -1) {
        std::cerr << "Match ID not set. Cannot update match.\n";
        return;
    }

    DatabaseConnection& db = DatabaseConnection::getInstance();
    try {
        pqxx::work w(*db.getConnection());
        const std::string update_query = "UPDATE public.matches SET "
            "winner_id = " + (winner_id.has_value() ? std::to_string(winner_id.value()) : "NULL") +
            " WHERE id = " + std::to_string(id) + ";";
        w.exec(update_query);
        w.commit();
    }
    catch (const pqxx::sql_error& e) {
        std::cerr << "SQL error in updateWinnerInDatabase: " << e.what() << '\n';
    }
    catch (const std::exception& e) {
        std::cerr << "Exception in updateWinnerInDatabase: " << e.what() << '\n';
    }
}

void Match::resumeMatch(const int match_id) {
    timer.start();
    changeState(new StartedState());
    current_state->handle(this);

    if (id == -1) {
        std::cerr << "Match ID not set. Cannot update match.\n";
        return;
    }

    DatabaseConnection& db = DatabaseConnection::getInstance();
    try {
        pqxx::work w(*db.getConnection());
        const std::string match_query = "UPDATE public.matches SET status_id = " + std::to_string(status_id) + " WHERE id = " + std::to_string(id) + ";";
        w.exec(match_query);
        w.commit();
        std::cout << "Match with ID " << id << " is resuming.\n";
    }
    catch (const pqxx::sql_error& e) {
        std::cerr << "SQL error in resumeMatch: " << e.what() << '\n';
    }
    catch (const std::exception& e) {
        std::cerr << "Exception in resumeMatch: " << e.what() << '\n';
    }

    displayPlayerInfo();
    resumeCurrentSet(match_id);
}

void Match::updateMatchInDatabase() {
    if (id == -1) {
        std::cerr << "Match ID not set. Cannot update match.\n";
        return;
    }

    const long long elapsed_seconds = timer.stop();
    duration += std::chrono::seconds(elapsed_seconds);
    timer.start();

    DatabaseConnection& db = DatabaseConnection::getInstance();
    try {
        pqxx::work w(*db.getConnection());
        const int duration_minutes = std::chrono::duration_cast<std::chrono::minutes>(duration).count();
        const std::string update_query = "UPDATE public.matches SET "
            "duration = duration + interval '" + std::to_string(duration_minutes) + " minutes', "
            "status_id = " + std::to_string(status_id) + ", "
            "winner_id = " + (winner_id.has_value() ? std::to_string(winner_id.value()) : "NULL") +
            " WHERE id = " + std::to_string(id) + ";";

        w.exec(update_query);
        w.commit();
    }
    catch (const pqxx::sql_error& e) {
        std::cerr << "SQL error in updateMatchInDatabase: " << e.what() << '\n';
    }
    catch (const std::exception& e) {
        std::cerr << "Exception in updateMatchInDatabase: " << e.what() << '\n';
    }
}

std::string Match::getStatusById(const int status_id) {
    DatabaseConnection& db = DatabaseConnection::getInstance();
    pqxx::result r;

    try {
        pqxx::nontransaction nt(*db.getConnection());
        const std::string query = "SELECT status FROM match_status WHERE id = " + nt.quote(status_id) + ";";
        r = nt.exec(query);

        if (!r.empty()) {
            return r[0]["status"].as<std::string>();
        }
        else {
            std::cerr << "Status ID " << status_id << " not found.\n";
            return "Unknown Status";
        }
    }
    catch (const pqxx::sql_error& e) {
        std::cerr << "SQL error while retrieving status name: " << e.what() << '\n';
        return "Unknown Status";
    }
    catch (const std::exception& e) {
        std::cerr << "Error while retrieving status name: " << e.what() << '\n';
        return "Unknown Status";
    }
}

void Match::displayPlayerInfo() const {
    DatabaseConnection& db = DatabaseConnection::getInstance();
    try {
        pqxx::work w(*db.getConnection());

        const std::string player_query = "SELECT id, first_name, last_name, matches_won, matches_lost "
            "FROM public.players WHERE id IN (" + std::to_string(player_id1) + ", " + std::to_string(player_id2) + ");";
        pqxx::result player_result = w.exec(player_query);

        tabulate::Table table;
        table.add_row({ "ID", "First Name", "Last Name", "Matches Won", "Matches Lost" });

        for (const auto& row : player_result) {
            table.add_row({
                row["id"].c_str(),
                row["first_name"].c_str(),
                row["last_name"].c_str(),
                std::to_string(row["matches_won"].as<int>()),
                std::to_string(row["matches_lost"].as<int>())
                });
        }

        std::cout << table << '\n';
    }
    catch (const pqxx::sql_error& e) {
        std::cerr << "SQL error in displayPlayerInfo: " << e.what() << '\n';
    }
    catch (const std::exception& e) {
        std::cerr << "Exception in displayPlayerInfo: " << e.what() << '\n';
    }
}