# Makefile for the lex project

# Compiler and compiler flags
CXX = g++
CXXFLAGS = -Wall -g

# Define the target executable
TARGET = lex

# Define the source files
SRC = lex.cpp

# Define the object files
OBJ = $(SRC:.cpp=.o)

# Default target
all: $(TARGET)

# Rule to link the object files into the executable
$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Rule to compile the source files into object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $<

# Clean up
clean:
	rm -f $(TARGET) $(OBJ)

# Phony targets
.PHONY: all clean
