#ifndef TIMEZONE_H
#define TIMEZONE_H

#include <string>
#include <ctime>
#include <mutex>


/**
 * Timezone utilities for converting between local time and UTC.
 * 
 * Limitations:
 * - Uses fixed offsets (does not account for DST)
 * - Only supports UTC, IST, and PST
 * - For production, use a proper timezone library (e.g., ICU, date library)
 */
class TimezoneUtils {
public:
    /**
     * Convert a local time string to UTC time_t.
     * 
     * @param date_str Date in format "YYYY-MM-DD"
     * @param time_str Time in format "HH:MM"
     * @param tz_str Timezone string ("UTC", "IST", or "PST")
     * @return time_t representing UTC time, or -1 on error
     */
    static time_t localToUTC(const std::string& date_str, 
                            const std::string& time_str, 
                            const std::string& tz_str);

    /**
     * Convert UTC time_t to local time string.
     * 
     * @param utc_time UTC time as time_t
     * @param tz_str Target timezone string
     * @return Formatted string "YYYY-MM-DD HH:MM" in local timezone
     */
    static std::string utcToLocal(time_t utc_time, const std::string& tz_str);

    /**
     * Get timezone offset in seconds from UTC.
     * 
     * @param tz_str Timezone string
     * @return Offset in seconds (positive for east of UTC, negative for west)
     */
    static int getOffsetSeconds(const std::string& tz_str);

    /**
     * Validate timezone string.
     * 
     * @param tz_str Timezone string to validate
     * @return true if valid, false otherwise
     */
    static bool isValidTimezone(const std::string& tz_str);

    /**
     * Parse date string "YYYY-MM-DD" into tm structure.
     * Public for use in week boundary calculations.
     */
    static bool parseDate(const std::string& date_str, struct tm& tm_out);

private:
    /**
     * Parse time string "HH:MM" into tm structure.
     */
    static bool parseTime(const std::string& time_str, struct tm& tm_out);
};

#endif // TIMEZONE_H

