#include <iostream>
#include <sstream>
#include <iomanip>
#include <string>
#include <vector>
#include <mutex>
#include <thread>
#include <atomic>
#include <ctime>
#include "calendar_service.h"
#include "timezone.h"

/**
 * CLI interface for the Calendar Management System.
 * 
 * Commands:
 *   create "Title" YYYY-MM-DD HH:MM HH:MM TZ
 *   list week YYYY-MM-DD TZ
 *   delete ID
 *   demo (concurrency demonstration)
 *   exit
 */

class CLI {
private:
    CalendarService calendar_service_;

    /**
     * Split a string by whitespace, handling quoted strings.
     */
    std::vector<std::string> parseCommand(const std::string& line) {
        std::vector<std::string> tokens;
        std::istringstream iss(line);
        std::string token;
        bool in_quotes = false;
        std::string quoted_token;

        while (iss >> token) {
            if (token.front() == '"') {
                in_quotes = true;
                quoted_token = token.substr(1);  // Remove leading quote
                if (token.back() == '"' && token.length() > 1) {
                    // Single token in quotes
                    quoted_token = quoted_token.substr(0, quoted_token.length() - 1);
                    tokens.push_back(quoted_token);
                    in_quotes = false;
                }
            } else if (in_quotes) {
                quoted_token += " " + token;
                if (token.back() == '"') {
                    quoted_token = quoted_token.substr(0, quoted_token.length() - 1);
                    tokens.push_back(quoted_token);
                    in_quotes = false;
                }
            } else {
                tokens.push_back(token);
            }
        }

        return tokens;
    }

    /**
     * Calculate week start (Monday) and end (Sunday) for a given date.
     */
    void calculateWeekBounds(const std::string& date_str, const std::string& tz_str,
                             time_t& week_start_utc, time_t& week_end_utc) {
        // Convert date at midnight (00:00) to UTC
        time_t date_utc = TimezoneUtils::localToUTC(date_str, "00:00", tz_str);
        if (date_utc == -1) {
            week_start_utc = -1;
            week_end_utc = -1;
            return;
        }

        // Get day of week (0=Sunday, 1=Monday, ..., 6=Saturday)
        struct tm* tm_utc = gmtime(&date_utc);
        if (!tm_utc) {
            week_start_utc = -1;
            week_end_utc = -1;
            return;
        }
        int day_of_week = tm_utc->tm_wday;
        
        // Calculate days to Monday (if Sunday, go back 6 days; otherwise go back (day-1) days)
        int days_to_monday = (day_of_week == 0) ? 6 : (day_of_week - 1);
        
        // Week start: Monday 00:00:00 UTC
        week_start_utc = date_utc - (days_to_monday * 24LL * 3600LL);
        
        // Week end: Monday 00:00:00 UTC of next week
        week_end_utc = week_start_utc + (7LL * 24LL * 3600LL);
    }

    void handleCreate(const std::vector<std::string>& tokens) {
        if (tokens.size() != 6) {
            std::cout << "Error: Invalid create command. Usage: create \"Title\" YYYY-MM-DD HH:MM HH:MM TZ\n";
            return;
        }

        std::string title = tokens[1];
        std::string date_str = tokens[2];
        std::string start_time_str = tokens[3];
        std::string end_time_str = tokens[4];
        std::string tz_str = tokens[5];

        if (!TimezoneUtils::isValidTimezone(tz_str)) {
            std::cout << "Error: Invalid timezone. Supported: UTC, IST, PST\n";
            return;
        }

        // Convert to UTC
        time_t start_utc = TimezoneUtils::localToUTC(date_str, start_time_str, tz_str);
        time_t end_utc = TimezoneUtils::localToUTC(date_str, end_time_str, tz_str);

        if (start_utc == -1 || end_utc == -1) {
            std::cout << "Error: Invalid date or time format. Use YYYY-MM-DD and HH:MM\n";
            return;
        }

        // Handle case where end time is on next day
        if (end_utc <= start_utc) {
            // Try next day
            struct tm* tm_start = gmtime(&start_utc);
            struct tm tm_end = *tm_start;
            tm_end.tm_mday += 1;
            time_t next_day = mktime(&tm_end);
            if (next_day != -1) {
                // Recalculate end_utc with next day
                std::ostringstream oss;
                oss << std::setfill('0')
                    << std::setw(4) << (tm_end.tm_year + 1900) << "-"
                    << std::setw(2) << (tm_end.tm_mon + 1) << "-"
                    << std::setw(2) << tm_end.tm_mday;
                std::string next_date_str = oss.str();
                end_utc = TimezoneUtils::localToUTC(next_date_str, end_time_str, tz_str);
            }
        }

        int event_id = calendar_service_.createEvent(title, start_utc, end_utc);

        if (event_id == -1) {
            std::cout << "Error: Failed to create event. Possible reasons:\n";
            std::cout << "  - End time must be after start time\n";
            std::cout << "  - Event conflicts with existing event\n";
        } else {
            std::cout << "Event created successfully. ID: " << event_id << "\n";
        }
    }

    void handleListWeek(const std::vector<std::string>& tokens) {
        if (tokens.size() != 4) {
            std::cout << "Error: Invalid list command. Usage: list week YYYY-MM-DD TZ\n";
            return;
        }

        std::string date_str = tokens[2];
        std::string tz_str = tokens[3];

        if (!TimezoneUtils::isValidTimezone(tz_str)) {
            std::cout << "Error: Invalid timezone. Supported: UTC, IST, PST\n";
            return;
        }

        time_t week_start_utc, week_end_utc;
        calculateWeekBounds(date_str, tz_str, week_start_utc, week_end_utc);

        if (week_start_utc == -1) {
            std::cout << "Error: Invalid date format. Use YYYY-MM-DD\n";
            return;
        }

        std::vector<Event> events = calendar_service_.getWeeklyEvents(week_start_utc, week_end_utc);

        if (events.empty()) {
            std::cout << "No events found for this week.\n";
        } else {
            std::cout << "\nWeekly Events:\n";
            std::cout << "----------------------------------------\n";
            for (const auto& event : events) {
                std::string start_local = TimezoneUtils::utcToLocal(event.start_utc, tz_str);
                std::string end_local = TimezoneUtils::utcToLocal(event.end_utc, tz_str);
                std::cout << "ID: " << event.id << "\n";
                std::cout << "Title: " << event.title << "\n";
                std::cout << "Start: " << start_local << " " << tz_str << "\n";
                std::cout << "End: " << end_local << " " << tz_str << "\n";
                std::cout << "----------------------------------------\n";
            }
        }
    }

    void handleDelete(const std::vector<std::string>& tokens) {
        if (tokens.size() != 2) {
            std::cout << "Error: Invalid delete command. Usage: delete ID\n";
            return;
        }

        int event_id = std::stoi(tokens[1]);
        bool deleted = calendar_service_.deleteEvent(event_id);

        if (deleted) {
            std::cout << "Event " << event_id << " deleted successfully.\n";
        } else {
            std::cout << "Error: Event " << event_id << " not found.\n";
        }
    }

    /**
     * Concurrency demonstration: spawn two threads attempting to create overlapping events.
     * Only one should succeed.
     */
    void handleDemo() {
        std::cout << "\n=== Concurrency Demonstration ===\n";
        std::cout << "Creating two threads that attempt to create overlapping events...\n\n";

        std::atomic<int> success_count(0);
        std::atomic<int> failure_count(0);

        auto createOverlappingEvent = [&](int thread_id) {
            // Both threads try to create an event at the same time
            time_t now = time(nullptr);
            time_t start = now + 3600;  // 1 hour from now
            time_t end = start + 1800;  // 30 minutes duration

            std::string title = "Thread " + std::to_string(thread_id) + " Event";
            int event_id = calendar_service_.createEvent(title, start, end);

            if (event_id != -1) {
                success_count++;
                std::cout << "Thread " << thread_id << ": Successfully created event ID " << event_id << "\n";
            } else {
                failure_count++;
                std::cout << "Thread " << thread_id << ": Failed to create event (conflict detected)\n";
            }
        };

        std::thread t1(createOverlappingEvent, 1);
        std::thread t2(createOverlappingEvent, 2);

        t1.join();
        t2.join();

        std::cout << "\nResult: " << success_count << " succeeded, " << failure_count << " failed\n";
        std::cout << "This demonstrates that the calendar is thread-safe.\n";
        std::cout << "Only one overlapping event can be created.\n\n";
    }

public:
    void run() {
        std::cout << "=== Calendar Management System ===\n";
        std::cout << "Commands:\n";
        std::cout << "  create \"Title\" YYYY-MM-DD HH:MM HH:MM TZ\n";
        std::cout << "  list week YYYY-MM-DD TZ\n";
        std::cout << "  delete ID\n";
        std::cout << "  demo (concurrency demonstration)\n";
        std::cout << "  exit\n\n";

        std::string line;
        while (true) {
            std::cout << "> ";
            std::getline(std::cin, line);

            if (line.empty()) {
                continue;
            }

            std::vector<std::string> tokens = parseCommand(line);

            if (tokens.empty()) {
                continue;
            }

            std::string command = tokens[0];

            if (command == "exit") {
                std::cout << "Goodbye!\n";
                break;
            } else if (command == "create") {
                handleCreate(tokens);
            } else if (command == "list") {
                if (tokens.size() > 1 && tokens[1] == "week") {
                    handleListWeek(tokens);
                } else {
                    std::cout << "Error: Invalid list command. Use 'list week YYYY-MM-DD TZ'\n";
                }
            } else if (command == "delete") {
                handleDelete(tokens);
            } else if (command == "demo") {
                handleDemo();
            } else {
                std::cout << "Error: Unknown command. Type 'exit' to quit.\n";
            }
        }
    }
};

int main() {
    CLI cli;
    cli.run();
    return 0;
}

