#include "MatchState.hpp"
#include "Match.hpp"

void PendingState::handle(Match* match) {
    match->setStatusId(Match::getStatusId("Pending"));
}

void StartedState::handle(Match* match) {
    match->setStatusId(Match::getStatusId("Started"));
}

void SuspendedState::handle(Match* match) {
    match->setStatusId(Match::getStatusId("Suspended"));
}

void FinishedState::handle(Match* match) {
    match->setStatusId(Match::getStatusId("Finished"));
}

void DelayedState::handle(Match* match) {
    match->setStatusId(Match::getStatusId("Delayed"));
}