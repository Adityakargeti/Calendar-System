#include "timezone.h"
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <mutex>


// Fixed timezone offsets in seconds
// UTC: 0
// IST: +5:30 = 19800 seconds
// PST: -8:00 = -28800 seconds

int TimezoneUtils::getOffsetSeconds(const std::string& tz_str) {
    if (tz_str == "UTC") {
        return 0;
    } else if (tz_str == "IST") {
        return 19800;  // +5:30
    } else if (tz_str == "PST") {
        return -28800;  // -8:00
    }
    throw std::invalid_argument("Invalid timezone: " + tz_str);
}

bool TimezoneUtils::isValidTimezone(const std::string& tz_str) {
    return tz_str == "UTC" || tz_str == "IST" || tz_str == "PST";
}

bool TimezoneUtils::parseDate(const std::string& date_str, struct tm& tm_out) {
    // Format: YYYY-MM-DD
    if (date_str.length() != 10) {
        return false;
    }
    
    std::istringstream iss(date_str);
    char dash1, dash2;
    int year, month, day;
    
    if (!(iss >> year >> dash1 >> month >> dash2 >> day)) {
        return false;
    }
    
    if (dash1 != '-' || dash2 != '-') {
        return false;
    }
    
    if (year < 1970 || month < 1 || month > 12 || day < 1 || day > 31) {
        return false;
    }
    
    tm_out.tm_year = year - 1900;  // tm_year is years since 1900
    tm_out.tm_mon = month - 1;      // tm_mon is 0-11
    tm_out.tm_mday = day;
    tm_out.tm_hour = 0;
    tm_out.tm_min = 0;
    tm_out.tm_sec = 0;
    tm_out.tm_isdst = -1;  // Let system determine DST
    
    return true;
}

bool TimezoneUtils::parseTime(const std::string& time_str, struct tm& tm_out) {
    // Format: HH:MM
    if (time_str.length() != 5) {
        return false;
    }
    
    std::istringstream iss(time_str);
    char colon;
    int hour, minute;
    
    if (!(iss >> hour >> colon >> minute)) {
        return false;
    }
    
    if (colon != ':') {
        return false;
    }
    
    if (hour < 0 || hour > 23 || minute < 0 || minute > 59) {
        return false;
    }
    
    tm_out.tm_hour = hour;
    tm_out.tm_min = minute;
    tm_out.tm_sec = 0;
    
    return true;
}

// Helper function to calculate days since epoch for a given date
static int daysSinceEpoch(int year, int month, int day) {
    // Algorithm to calculate days since 1970-01-01
    // Accounts for leap years
    int days = 0;
    
    // Days from 1970 to year
    for (int y = 1970; y < year; y++) {
        days += 365;
        if ((y % 4 == 0 && y % 100 != 0) || (y % 400 == 0)) {
            days++;  // Leap year
        }
    }
    
    // Days from Jan 1 to month in the given year
    int days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    bool is_leap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
    if (is_leap) {
        days_in_month[1] = 29;
    }
    
    for (int m = 0; m < month - 1; m++) {
        days += days_in_month[m];
    }
    
    days += (day - 1);
    return days;
}

time_t TimezoneUtils::localToUTC(const std::string& date_str, 
                                 const std::string& time_str, 
                                 const std::string& tz_str) {
    if (!isValidTimezone(tz_str)) {
        return -1;
    }
    
    struct tm tm_local = {0};
    
    // Parse date
    if (!parseDate(date_str, tm_local)) {
        return -1;
    }
    
    // Parse time
    if (!parseTime(time_str, tm_local)) {
        return -1;
    }
    
    // Manually calculate UTC time_t
    // This avoids issues with mktime interpreting tm as local time
    int year = tm_local.tm_year + 1900;
    int month = tm_local.tm_mon + 1;
    int day = tm_local.tm_mday;
    int hour = tm_local.tm_hour;
    int minute = tm_local.tm_min;
    
    // Calculate days since epoch (1970-01-01)
    int days = daysSinceEpoch(year, month, day);
    
    // Calculate total seconds
    time_t utc_time = days * 24LL * 3600LL + hour * 3600LL + minute * 60LL;
    
    // Get timezone offset
    int offset_seconds = getOffsetSeconds(tz_str);
    
    // Convert to UTC: subtract the offset (because local time = UTC + offset)
    utc_time = utc_time - offset_seconds;
    
    return utc_time;
}

std::string TimezoneUtils::utcToLocal(time_t utc_time, const std::string& tz_str) {
    if (!isValidTimezone(tz_str)) {
        return "INVALID_TZ";
    }
    
    // Get timezone offset
    int offset_seconds = getOffsetSeconds(tz_str);
    
    // Convert UTC to local: add the offset
    time_t local_time = utc_time + offset_seconds;
    
    // Convert to tm structure
    struct tm* tm_local = gmtime(&local_time);
    if (!tm_local) {
        return "INVALID_TIME";
    }
    
    // Format as "YYYY-MM-DD HH:MM"
    std::ostringstream oss;
    oss << std::setfill('0')
        << std::setw(4) << (tm_local->tm_year + 1900) << "-"
        << std::setw(2) << (tm_local->tm_mon + 1) << "-"
        << std::setw(2) << tm_local->tm_mday << " "
        << std::setw(2) << tm_local->tm_hour << ":"
        << std::setw(2) << tm_local->tm_min;
    
    return oss.str();
}

