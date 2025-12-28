# Makefile for Calendar Management System

CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -pthread
TARGET = calendar
SOURCES = main.cpp calendar_service.cpp timezone.cpp
OBJECTS = $(SOURCES:.cpp=.o)

# Default target
all: $(TARGET)

# Build the executable
$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJECTS)

# Compile source files to object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -f $(OBJECTS) $(TARGET) $(TARGET).exe

# Run the program
run: $(TARGET)
	./$(TARGET)

# Phony targets
.PHONY: all clean run


