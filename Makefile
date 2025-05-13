CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pedantic

TARGET = nfa_to_dfa
SRC = main.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)

.PHONY: all clean 