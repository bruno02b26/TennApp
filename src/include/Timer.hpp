#pragma once
#include <chrono>

class Timer {
private:
    std::chrono::time_point<std::chrono::steady_clock> start_time_;
    bool is_running_;

public:
    Timer();

    void start();
    long long stop();
};