CXX ?= g++
CXXFLAGS = -std=c++14 -O2 -Wall
TARGET = metro
SRC = main.cpp
DATA_DIR = data

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)
	@echo "Build complete: ./$(TARGET)"

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET) *.o
