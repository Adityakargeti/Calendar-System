# Calendar Management System

A minimal, thread-safe calendar management system in C++ focused on correctness, time modeling, and concurrency.

---

## At a Glance

- Thread-safe event store using UTC as the single source of truth.
- Efficient conflict detection via a sorted set of events.
- Simple CLI for creating, listing, deleting events and a concurrency demo.

---

## Architecture

┌──────────────────────────────┐
│             CLI              │
│          (main.cpp)          │
│  - Command parsing           │
│  - User interaction          │
└───────────────┬──────────────┘
                │
                ▼
┌──────────────────────────────┐
│        CalendarService        │
│  (calendar_service.h/.cpp)   │
│  - Thread safety (mutex)     │
│  - Business logic            │
│  - Conflict detection        │
└───────────────┬──────────────┘
                │
                ▼
┌────────────────────────────────────────────┐
│              EventStore                    │
│  std::set<Event, EventComparator>          │
│  - Ordered by start_utc                    │
│  - Efficient neighbor-based checks         │
│  (event.h)                                 │
└───────────────┬────────────────────────────┘
                │
                ▼
┌──────────────────────────────┐
│       Timezone Utilities     │
│     (timezone.h/.cpp)        │
│  - Local → UTC conversion    │
│  - UTC → Local conversion    │
└──────────────────────────────┘

### Components

1. **CLI (`main.cpp`)**: Command-line interface responsible for:
   - Parsing user commands
   - Calling service methods
   - Displaying results

2. **CalendarService (`calendar_service.h/cpp`)**: Thread-safe service layer that:
   - Validates event data
   - Performs conflict detection
   - Manages concurrent access via mutex
   - Stores events in sorted order

3. **EventStore**: Implemented as `std::set<Event, EventComparator>` for:
   - Automatic sorting by start time
   - Efficient conflict detection (O(log n) insertion, O(1) neighbor checks)

4. **Timezone Utilities (`timezone.h/cpp`)**: Handles conversion between:
   - Local time (user input) → UTC (internal storage)
   - UTC (internal storage) → Local time (display)

## Why UTC-Only Storage?

All events are stored internally in UTC to avoid:

- **Daylight Saving Time bugs**: DST transitions can cause ambiguous or invalid times
- **Timezone overlap errors**: Events created in different timezones can be correctly compared
- **Consistency**: All conflict detection operates on a single time reference

The conversion happens at the boundaries:
- **Input**: User provides local time → immediately converted to UTC
- **Storage**: All events stored in UTC
- **Output**: UTC converted back to user's timezone for display

## Conflict Detection Logic

Two events overlap if and only if:

```
new.start < existing.end AND new.end > existing.start
```

### Efficient Implementation

Because events are stored in a sorted container (ordered by `start_utc`), conflict detection only needs to check:

1. **The event immediately before** (if any)
2. **The event immediately after** (if any)

This gives us **O(log n)** complexity for insertion and **O(1)** for conflict checking (only 2 neighbors to check), compared to **O(n)** if we scanned the entire collection.

### Algorithm

```cpp
bool hasConflict(time_t start_utc, time_t end_utc) {
    // Find insertion point in sorted set
    auto it = events_.lower_bound(search_event);
    
    // Check next event
    if (it != events_.end()) {
        if (start_utc < it->end_utc && end_utc > it->start_utc) {
            return true;  // Conflict!
        }
    }
    
    // Check previous event
    if (it != events_.begin()) {
        --it;
        if (start_utc < it->end_utc && end_utc > it->start_utc) {
            return true;  // Conflict!
        }
    }
    
    return false;  // No conflict
}
```

## Concurrency Design

### Thread Safety Strategy

The system uses a **single mutex** (`std::mutex`) to protect all calendar operations:

```cpp
std::mutex calendar_mutex_;
```

### Protected Operations

All modifications are protected with `std::lock_guard<std::mutex>`:

- **Event creation**: Lock → Check conflict → Insert → Unlock
- **Event deletion**: Lock → Find → Erase → Unlock
- **Event queries**: Lock → Read → Unlock (for consistency)

### Design Rationale

**Why a single mutex?**
- **Simplicity**: Easier to reason about, less prone to deadlocks
- **Correctness**: Guarantees atomic operations
- **Trade-off**: Slightly lower concurrency, but acceptable for this use case

**Why not lock-free structures?**
- More complex to implement correctly
- Higher risk of subtle bugs
- Single mutex is sufficient for correctness

### Concurrency Demonstration

Run the `demo` command to see two threads attempting to create overlapping events. Only one will succeed, demonstrating thread-safe conflict detection.

## Supported Timezones

- **UTC**: Coordinated Universal Time (offset: 0)
- **IST**: Indian Standard Time (offset: +5:30)
- **PST**: Pacific Standard Time (offset: -8:00)

### Limitations

- Fixed offsets (does not account for DST changes)
- Only 3 timezones supported
- For production use, integrate a proper timezone library (e.g., ICU, date library)

## Building

### Requirements

- C++11 or later compiler (g++, clang++, MSVC)
- Standard C++ library

### Build Commands

**Linux/macOS:**
```bash
g++ -std=c++11 -pthread -o calendar main.cpp calendar_service.cpp timezone.cpp
```

**Windows (MinGW):**
```bash
g++ -std=c++11 -o calendar.exe main.cpp calendar_service.cpp timezone.cpp
```

**Windows (MSVC):**
```cmd
cl /EHsc /std:c++11 main.cpp calendar_service.cpp timezone.cpp /Fe:calendar.exe
```

## Usage

### Commands

1. **Create Event**
   ```
   create "Event Title" YYYY-MM-DD HH:MM HH:MM TZ
   ```
   Example:
   ```
   create "Team Sync" 2025-01-10 14:00 15:00 IST
   ```

2. **List Weekly Events**
   ```
   list week YYYY-MM-DD TZ
   ```
   Example:
   ```
   list week 2025-01-08 IST
   ```

3. **Delete Event**
   ```
   delete ID
   ```
   Example:
   ```
   delete 2
   ```

4. **Concurrency Demo**
   ```
   demo
   ```

5. **Exit**
   ```
   exit
   ```

### Example Session

```
=== Calendar Management System ===
> create "Morning Standup" 2025-01-10 09:00 09:30 IST
Event created successfully. ID: 1

> create "Team Sync" 2025-01-10 14:00 15:00 IST
Event created successfully. ID: 2

> create "Lunch Meeting" 2025-01-10 14:30 15:30 IST
Error: Failed to create event. Possible reasons:
  - End time must be after start time
  - Event conflicts with existing event

> list week 2025-01-08 IST

Weekly Events:
----------------------------------------
ID: 1
Title: Morning Standup
Start: 2025-01-10 09:00 IST
End: 2025-01-10 09:30 IST
----------------------------------------
ID: 2
Title: Team Sync
Start: 2025-01-10 14:00 IST
End: 2025-01-10 15:00 IST
----------------------------------------

> delete 1
Event 1 deleted successfully.

> exit
Goodbye!
```

## Known Limitations

1. **Timezone Support**: Fixed offsets only, no DST handling
2. **Event Lookup**: Linear search by ID (O(n)); could be optimized with a map
3. **Date Validation**: Basic validation only (doesn't check for invalid dates like Feb 30)
4. **Error Handling**: Simple error messages; could be more detailed
5. **Persistence**: Events are lost on program exit (in-memory only)

## Code Quality

- Clear naming conventions
- Comments explaining *why*, not just *what*
- Logical file separation
- No overengineering
- Easy to explain line-by-line in an interview

## License

This is an educational project. Use freely for learning purposes.

---

## Core Concepts

- All times stored internally in UTC (time_t).
- Convert user local input → UTC on input; convert UTC → local for display.
- Conflict detection formula (KaTeX):

Inline: $new.start < existing.end \land new.end > existing.start$

Block:
$$
\text{conflict} \iff \text{new.start} < \text{existing.end} \land \text{new.end} > \text{existing.start}
$$

---

## Key Files

- [main.cpp](main.cpp) — CLI and entrypoint.
- [calendar_service.h](calendar_service.h) / [calendar_service.cpp](calendar_service.cpp) — service logic, concurrency, conflict detection.
- [event.h](event.h) — Event model and comparator.
- [timezone.h](timezone.h) / [timezone.cpp](timezone.cpp) — timezone conversion utilities.
- [Makefile](Makefile) — build commands.

---

## Important Symbols

- [`TimezoneUtils::localToUTC`](timezone.h) — convert local date/time to UTC.
- [`TimezoneUtils::utcToLocal`](timezone.h) — convert UTC to local display string.
- [`CalendarService::createEvent`](calendar_service.h) — create an event (thread-safe).
- [`Event`](event.h) — event structure stored in the calendar.

---

## Build

Linux/macOS:
```sh
g++ -std=c++11 -pthread -o calendar
```


