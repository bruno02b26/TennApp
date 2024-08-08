#pragma once
#include "DatabaseConnection.hpp"
#include <string>

class Match;

int getNumericInput(const std::string& prompt);

class UIManager {
private:
    static constexpr int MAX_SETS_TO_WIN = 3;

    static void addPlayer();
    static void showPlayers();
    static void addMatch();
    static void showMatches();
    static void showAllMatches();
    static void showMatchDetails();
    static void showMatchesResultsForPlayer();
    static void handleMatchSuspension(Match& match);
    static void handleMatchFinishing(Match& match);
    static void handleSetContinuation(Match& match, int game_status);
    static void updateMatchStatus(Match& match);

    void startMatch();
    void resumeMatch();
    static int processGameEnd(Match& match);

public:
    UIManager() = default;
    void run();
};