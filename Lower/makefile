CXX = g++
CXXFLAGS = -std=c++17 -Wall   -g -O0

SRCS = lower.cpp
OBJS = $(SRCS:.cpp=.o)
TARGET = lower

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)