#ifndef CALENDAR_SERVICE_H
#define CALENDAR_SERVICE_H

#include "event.h"
#include <set>
#include<bits/stdc++.h>
#include <mutex>
#include <string>
#include <vector>

/**
 * CalendarService provides thread-safe calendar operations.
 * 
 * Design decisions:
 * - Single mutex protects all operations (simplicity over performance)
 * - Events stored in sorted set for efficient conflict detection
 * - All times stored in UTC internally
 * - Conflict detection only checks neighboring events (O(log n) complexity)
 */
class CalendarService {
public:
    CalendarService();
    ~CalendarService() = default;

    /**
     * Create a new event.
     * 
     * @param title Event title
     * @param start_utc Start time in UTC
     * @param end_utc End time in UTC
     * @return Event ID on success, -1 on failure (conflict or invalid times)
     */
    int createEvent(const std::string& title, time_t start_utc, time_t end_utc);

    /**
     * Delete an event by ID.
     * 
     * @param event_id Event ID to delete
     * @return true if deleted, false if not found
     */
    bool deleteEvent(int event_id);

    /**
     * Get all events in a week.
     * 
     * @param week_start_utc Start of week in UTC
     * @param week_end_utc End of week in UTC
     * @return Vector of events overlapping the week
     */
    std::vector<Event> getWeeklyEvents(time_t week_start_utc, time_t week_end_utc);

    /**
     * Get all events (for debugging/testing).
     */
    std::vector<Event> getAllEvents();

    /**
     * Get next available event ID.
     */
    int getNextEventId();

private:
    // Sorted set of events (ordered by start_utc)
    // Using std::set with custom comparator for efficient conflict detection
    std::set<Event, EventComparator> events_;
    
    // Mutex for thread-safe operations
    std::mutex calendar_mutex_;
    
    // Counter for event IDs
    int next_event_id_;

    /**
     * Check if a new event conflicts with existing events.
     * 
     * Conflict detection algorithm:
     * Two events overlap if: new.start < existing.end AND new.end > existing.start
     * 
     * Because events are sorted by start_utc, we only need to check:
     * - The event immediately before (if any)
     * - The event immediately after (if any)
     * 
     * @param start_utc Start time of new event
     * @param end_utc End time of new event
     * @return true if conflict exists, false otherwise
     */
    bool hasConflict(time_t start_utc, time_t end_utc) const;

    /**
     * Find event by ID (helper for deletion).
     */
    std::set<Event, EventComparator>::iterator findEventById(int event_id);
};

#endif // CALENDAR_SERVICE_H


