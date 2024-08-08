#pragma once
#include <string>
#include <ctime>

std::string formatName(const std::string& str);
bool isAlpha(const std::string& str);

void getDateAndTimeFromUser(std::tm& date_tm, std::tm& time_tm);
bool isValidDate(int year, int month, int day);

inline bool validateAndParseDate(const std::string& date_str, std::tm& tm);
inline bool validateAndParseTime(const std::string& time_str, std::tm& tm);