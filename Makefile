# Makefile for custom shell

# Compiler
CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17

# Libraries
LIBS = -lreadline -lhistory

# Source files
SRC = shell.cpp

# Output executable
TARGET = shell

# Default target
all: $(TARGET)

# Build target
$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET) $(LIBS)

# Clean compiled files
clean:
	rm -f $(TARGET)

# Run the shell directly
run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
