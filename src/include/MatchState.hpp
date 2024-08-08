#pragma once

class Match;

class MatchState {
public:
    virtual void handle(Match* match) = 0;
    virtual ~MatchState() = default;
};

class PendingState : public MatchState {
public:
    void handle(Match* match) override;
};

class StartedState : public MatchState {
public:
    void handle(Match* match) override;
};

class SuspendedState : public MatchState {
public:
    void handle(Match* match) override;
};

class FinishedState : public MatchState {
public:
    void handle(Match* match) override;
};

class DelayedState : public MatchState {
public:
    void handle(Match* match) override;
};