CXX      := g++
CXXFLAGS := -Wall -Wextra -std=c++17 -O2
LDLIBS   := -luring

SRCS := main.cpp IoUringManager.cpp
TARGET := reactor-demo

all: $(TARGET)

$(TARGET): $(SRCS) EventHandler.h IoUringManager.h FileReader.h
	$(CXX) $(CXXFLAGS) -o $@ $(SRCS) $(LDLIBS)

run: $(TARGET)
	./$(TARGET) main.cpp

clean:
	rm -f $(TARGET)

.PHONY: all run clean
