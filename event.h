#ifndef EVENT_H
#define EVENT_H
#include <mutex>
#include <string>
#include <ctime>

/**
 * Event represents a calendar event with a start and end time.
 * All times are stored internally in UTC (time_t).
 */
struct Event {
    int id;
    std::string title;
    time_t start_utc;  // Start time in UTC
    time_t end_utc;    // End time in UTC

    // Default constructor
    Event() : id(0), start_utc(0), end_utc(0) {}

    // Parameterized constructor
    Event(int event_id, const std::string& event_title, time_t start, time_t end)
        : id(event_id), title(event_title), start_utc(start), end_utc(end) {}
};

/**
 * Comparator for Event to enable sorted storage in std::set.
 * Events are ordered by start_utc (ascending), then by id for tie-breaking.
 * 
 * Why sorted? Enables efficient conflict detection by only checking
 * neighboring events instead of scanning the entire collection.
 */
struct EventComparator {
    bool operator()(const Event& a, const Event& b) const {
        if (a.start_utc != b.start_utc) {
            return a.start_utc < b.start_utc;
        }
        return a.id < b.id;  // Tie-breaker for events starting at same time
    }
};

#endif // EVENT_H

