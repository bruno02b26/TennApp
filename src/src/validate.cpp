#include "validate.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cctype>
#include <chrono>
#include <algorithm>
#include <regex>
#include <ctime>

bool isAlpha(const std::string& str) {
    return std::all_of(str.begin(), str.end(), [](const char c) {
        return std::isalpha(static_cast<unsigned char>(c));
    });
}

std::string formatName(const std::string& str) {
    if (str.empty()) return str;

    std::string formattedName = str;
    formattedName.erase(0, formattedName.find_first_not_of(' '));
    formattedName.erase(formattedName.find_last_not_of(' ') + 1);

    formattedName[0] = std::toupper(static_cast<unsigned char>(formattedName[0]));
    std::transform(formattedName.begin() + 1, formattedName.end(), formattedName.begin() + 1, [](const unsigned char c) {
        return std::tolower(c);
    });

    return formattedName;
}

bool validateAndParseDate(const std::string& date_str, std::tm& tm) {
    std::string formats[] = { "%d-%m-%Y", "%d/%m/%Y", "%d-%m", "%d/%m" };
    const bool year_provided = (date_str.find('-') != std::string::npos && date_str.find('-') != date_str.rfind('-')) ||
        (date_str.find('/') != std::string::npos && date_str.find('/') != date_str.rfind('/'));

    std::string cleaned_date_str = date_str;
    cleaned_date_str.erase(remove_if(cleaned_date_str.begin(), cleaned_date_str.end(), ::isspace), cleaned_date_str.end());

    for (const auto& format : formats) {
        std::istringstream date_stream(cleaned_date_str);
        date_stream >> std::get_time(&tm, format.c_str());
        if (!date_stream.fail()) {
            if (!year_provided) {
                const auto now = std::chrono::system_clock::now();
                const auto in_time_t = std::chrono::system_clock::to_time_t(now);
                std::tm current_tm;
                localtime_s(&current_tm, &in_time_t);
                tm.tm_year = current_tm.tm_year;
            }
            tm.tm_isdst = -1;
            return true;
        }
    }
    return false;
}

bool validateAndParseTime(const std::string& time_str, std::tm& tm) {
    std::string formats[] = { "%H:%M", "%H" };
    std::istringstream time_stream(time_str);

    if (time_str.empty()) {
        const auto now = std::chrono::system_clock::now();
        const auto in_time_t = std::chrono::system_clock::to_time_t(now);
        localtime_s(&tm, &in_time_t);
        tm.tm_hour += 1;
        tm.tm_min = 0;
        tm.tm_sec = 0;
        mktime(&tm);
        return true;
    }

    for (const auto& format : formats) {
        time_stream.clear();
        time_stream.str(time_str);
        time_stream >> std::get_time(&tm, format.c_str());
        if (!time_stream.fail()) {
            if (format == "%H") {
                tm.tm_min = 0;
            }
            return true;
        }
    }
    return false;
}

void getDateAndTimeFromUser(std::tm& date_tm, std::tm& time_tm) {
    std::string date_str, time_str;
    bool confirmed = false;

    while (!confirmed) {
        std::cout << "Enter the date (DD-MM-YYYY, DD/MM/YYYY, DD-MM, DD/MM): ";
        std::getline(std::cin, date_str);
        while (date_str.empty() || !validateAndParseDate(date_str, date_tm)) {
            std::cout << "Invalid date format. Please enter a valid date (DD-MM-YYYY, DD/MM/YYYY, DD-MM, DD/MM): ";
            std::getline(std::cin, date_str);
        }

        std::cout << "Enter the time (HH, HH:MM): ";
        std::getline(std::cin, time_str);
        while (time_str.empty() || !validateAndParseTime(time_str, time_tm)) {
            std::cout << "Invalid time format. Please enter a valid time (HH, HH:MM): ";
            std::getline(std::cin, time_str);
        }

        char confirmation;
        std::cout << "You entered date: " << std::put_time(&date_tm, "%d-%m-%Y") << " and time: " << std::put_time(&time_tm, "%H:%M") << "\n";
        std::cout << "Is this correct? (y/n): ";
        std::cin >> confirmation;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        confirmed = (confirmation == 'y' || confirmation == 'Y');
    }

    date_tm.tm_hour = time_tm.tm_hour;
    date_tm.tm_min = time_tm.tm_min;
    date_tm.tm_sec = time_tm.tm_sec;
}

bool parseDate(const std::string& date_input, std::tm& date_tm) {
    const std::regex date_regex(R"((\d{1,2})[-/: ](\d{1,2})([-/: ](\d{4}))?)");

	if (std::smatch matches; std::regex_match(date_input, matches, date_regex)) {
        int year;
        if (matches[4].matched) {
            year = std::stoi(matches[4]);
        }
        else {
            const time_t now = time(nullptr);
            tm ltm = {};
            localtime_s(&ltm, &now);
            year = 1900 + ltm.tm_year;
        }
        const int month = std::stoi(matches[1]);
        const int day = std::stoi(matches[2]);

        if (!isValidDate(year, month, day)) return false;

        date_tm.tm_year = year - 1900;
        date_tm.tm_mon = month - 1;
        date_tm.tm_mday = day;
        return true;
    }
    return false;
}

bool parseTime(const std::string& time_input, std::tm& date_tm) {
    const std::regex time_regex(R"((\d{1,2})([:\s](\d{2}))?)");

    if (std::smatch matches; std::regex_match(time_input, matches, time_regex)) {
        date_tm.tm_hour = std::stoi(matches[1]);
        date_tm.tm_min = (matches[2].matched) ? std::stoi(matches[3]) : 0;
        date_tm.tm_sec = 0;
        return true;
    }
    return false;
}

bool isValidDate(const int year, const int month, const int day) {
    const bool is_leap_year = (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
    const int days_in_month[] = { 31, 28 + is_leap_year, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

    if (month < 1 || month > 12) return false;
    if (day < 1 || day > days_in_month[month - 1]) return false;

    return true;
}