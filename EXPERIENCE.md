# Development Experience

## What Was Challenging

### 1. Timezone Conversion Correctness

The most challenging aspect was implementing correct timezone conversion without relying on external libraries or system-specific functions.

**Problem**: `mktime()` interprets `struct tm` as local system time, not UTC. This means converting "2025-01-10 14:00 IST" to UTC requires careful handling.

**Solution**: Implemented a manual calculation of days since epoch, accounting for leap years, then applying timezone offsets. This ensures correctness regardless of the system's local timezone.

**Key Insight**: Always convert to UTC immediately upon input, and only convert back to local time for display. This single source of truth (UTC) prevents timezone-related bugs.

### 2. Efficient Conflict Detection

Ensuring conflict detection is both correct and efficient required careful design.

**Problem**: Naive approach would check every event (O(n)), which doesn't scale.

**Solution**: Use a sorted container (`std::set`) ordered by start time. This allows:
- O(log n) insertion
- O(1) conflict checking (only check 2 neighbors)

**Key Insight**: The data structure choice directly enables the efficient algorithm. The sorted order is not just for display—it's fundamental to the conflict detection logic.

### 3. Thread Safety Without Over-Engineering

Balancing correctness with simplicity in concurrent code.

**Problem**: Multiple mutexes can lead to deadlocks. Lock-free structures are complex and error-prone.

**Solution**: Single mutex protecting all operations. Simple, correct, and easy to reason about.

**Key Insight**: For this use case, a single mutex is sufficient. The slight performance trade-off is worth the correctness and simplicity gains.

### 4. Week Boundary Calculation

Calculating week start (Monday) and end (Sunday) correctly across timezones.

**Problem**: Week boundaries depend on the user's timezone, but we need to query in UTC.

**Solution**: Convert the user's date to UTC, calculate week boundaries in UTC, then query events. Display converts back to user's timezone.

**Key Insight**: Always think in UTC internally, convert at boundaries.

## Trade-offs Made

### 1. Single Mutex vs. Fine-Grained Locking

**Chosen**: Single mutex  
**Trade-off**: Lower concurrency, but simpler and less error-prone  
**Rationale**: Correctness > Performance for this educational project

### 2. Sorted Set vs. Hash Map + Sorted List

**Chosen**: `std::set<Event, EventComparator>`  
**Trade-off**: O(n) lookup by ID, but O(log n) insertion and O(1) conflict checking  
**Rationale**: Conflict detection is the critical path; ID lookup is less frequent

### 3. Manual Timezone Calculation vs. Library

**Chosen**: Manual calculation with fixed offsets  
**Trade-off**: No DST support, limited timezones, but no external dependencies  
**Rationale**: Educational focus on understanding time handling; production would use a library

### 4. In-Memory Storage vs. Persistence

**Chosen**: In-memory only  
**Trade-off**: Data lost on exit, but simpler implementation  
**Rationale**: Focus on core algorithms, not persistence layer

### 5. Basic Date Validation vs. Comprehensive

**Chosen**: Basic validation (format, ranges)  
**Trade-off**: Doesn't catch all invalid dates (e.g., Feb 30), but simpler code  
**Rationale**: Sufficient for demonstration; production would use proper date library

## What I'd Improve With More Time

### 1. Timezone Library Integration

Replace manual timezone calculation with a proper library (e.g., Howard Hinnant's date library or ICU). This would:
- Support DST transitions
- Support all IANA timezones
- Handle historical timezone changes

### 2. Event Lookup Optimization

Add a `std::unordered_map<int, std::set<Event>::iterator>` for O(1) ID lookup while maintaining sorted storage. Currently, deletion is O(n) due to linear search.

### 3. More Comprehensive Date Validation

Add validation for:
- Invalid dates (Feb 30, etc.)
- Leap year handling
- Month/day boundaries

### 4. Better Error Messages

Provide more specific error messages:
- "Event conflicts with 'Team Sync' (ID: 2) from 14:00 to 15:00"
- "Invalid date: February only has 28 days in 2025"

### 5. Unit Tests

Add comprehensive unit tests for:
- Timezone conversion edge cases
- Conflict detection correctness
- Thread safety under stress
- Week boundary calculations

### 6. Persistence Layer

Add file-based persistence (JSON or binary format) so events survive program restarts.

### 7. More CLI Features

- List all events
- List events for a date range
- Update existing events
- Better formatting for weekly view

### 8. Performance Profiling

Profile the system under load to identify bottlenecks. The single mutex might become a bottleneck with many concurrent writers.

## Key Learnings

1. **UTC as Single Source of Truth**: Storing everything in UTC eliminates a whole class of timezone bugs.

2. **Data Structure Choice Matters**: The sorted set isn't just for ordering—it enables efficient conflict detection.

3. **Simplicity in Concurrency**: A single mutex is often sufficient and much easier to reason about than fine-grained locking.

4. **Boundary Conversions**: Convert at system boundaries (input/output), keep internals in UTC.

5. **Time is Hard**: Even "simple" time operations have edge cases (DST, leap years, timezone offsets). This is why libraries exist.

## Code Philosophy

The code follows these principles:

- **Readability over cleverness**: Prefer clear code that's easy to understand
- **Comments explain why**: Not just what the code does, but why it does it
- **Correctness first**: Performance optimizations come after correctness
- **Minimal dependencies**: Use standard library only
- **Interview-ready**: Every line should be explainable in a technical interview

This project demonstrates systems thinking: choosing the right data structures, handling concurrency correctly, and modeling time accurately. These are the skills that matter in production systems.


