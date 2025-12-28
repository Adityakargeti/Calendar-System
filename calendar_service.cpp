#include "calendar_service.h"
#include <algorithm>
#include<bits/stdc++.h>
#include <mutex>
CalendarService::CalendarService() : next_event_id_(1) {
}

int CalendarService::getNextEventId() {
    return next_event_id_++;
}

int CalendarService::createEvent(const std::string& title, time_t start_utc, time_t end_utc) {
    // Validate: start must be before end
    if (start_utc >= end_utc) {
        return -1;
    }

    // Acquire lock for thread-safe operation
    std::lock_guard<std::mutex> lock(calendar_mutex_);

    // Check for conflicts
    if (hasConflict(start_utc, end_utc)) {
        return -1;  // Conflict detected
    }

    // Create and insert event
    int event_id = getNextEventId();
    Event new_event(event_id, title, start_utc, end_utc);
    events_.insert(new_event);

    return event_id;
}

bool CalendarService::deleteEvent(int event_id) {
    std::lock_guard<std::mutex> lock(calendar_mutex_);

    auto it = findEventById(event_id);
    if (it != events_.end()) {
        events_.erase(it);
        return true;
    }

    return false;
}

std::vector<Event> CalendarService::getWeeklyEvents(time_t week_start_utc, time_t week_end_utc) {
    std::lock_guard<std::mutex> lock(calendar_mutex_);

    std::vector<Event> result;

    // Find first event that could overlap with the week
    // An event overlaps if: event.start < week_end AND event.end > week_start
    Event search_start(0, "", week_start_utc, week_start_utc);
    
    // Find lower bound: first event with start >= week_start
    auto it = events_.lower_bound(search_start);

    // Also check events that start before week_start but end after week_start
    if (it != events_.begin()) {
        --it;
        // If this event ends after week_start, include it
        if (it->end_utc > week_start_utc) {
            result.push_back(*it);
        }
        ++it;
    }

    // Collect all events that start before week_end
    while (it != events_.end() && it->start_utc < week_end_utc) {
        result.push_back(*it);
        ++it;
    }

    return result;
}

std::vector<Event> CalendarService::getAllEvents() {
    std::lock_guard<std::mutex> lock(calendar_mutex_);

    std::vector<Event> result;
    for (const auto& event : events_) {
        result.push_back(event);
    }
    return result;
}

bool CalendarService::hasConflict(time_t start_utc, time_t end_utc) const {
    // Create a dummy event for searching
    Event search_event(0, "", start_utc, end_utc);

    // Find the position where this event would be inserted
    auto it = events_.lower_bound(search_event);

    // Check the event immediately after (if exists)
    if (it != events_.end()) {
        // Conflict if: new.start < existing.end AND new.end > existing.start
        if (start_utc < it->end_utc && end_utc > it->start_utc) {
            return true;
        }
    }

    // Check the event immediately before (if exists)
    if (it != events_.begin()) {
        --it;
        // Conflict if: new.start < existing.end AND new.end > existing.start
        if (start_utc < it->end_utc && end_utc > it->start_utc) {
            return true;
        }
    }

    return false;
}

std::set<Event, EventComparator>::iterator CalendarService::findEventById(int event_id) {
    // Linear search through the set (O(n))
    // Alternative: maintain a separate map<id, iterator> for O(log n) lookup
    // Chose simplicity for this implementation
    for (auto it = events_.begin(); it != events_.end(); ++it) {
        if (it->id == event_id) {
            return it;
        }
    }
    return events_.end();
}


