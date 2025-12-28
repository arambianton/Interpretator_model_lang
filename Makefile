CXX      := g++
CXXFLAGS := -std=c++17 -Wall -O2
TARGET   := interpreter         
SRC      := interpretator.cpp
OBJ      := $(SRC:.cpp=.o)

TXTFILES := $(wildcard *.txt)

.PHONY: all clean $(TXTFILES)

all: $(TARGET)

# сборка бинарника
$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(TXTFILES): $(TARGET)
	@echo "─── running $(TARGET) with $@ ───"
	@./$(TARGET) $@

clean:
	$(RM) $(OBJ) $(TARGET)
