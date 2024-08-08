#include "Timer.hpp"

Timer::Timer() : is_running_(false) {}

void Timer::start() {
    if (!is_running_) {
        start_time_ = std::chrono::steady_clock::now();
        is_running_ = true;
    }
}

long long Timer::stop() {
    if (is_running_) {
        const auto end_time = std::chrono::steady_clock::now();
        is_running_ = false;
        const auto elapsed_seconds = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time_).count();
        return elapsed_seconds;
    }
    return 0;
}