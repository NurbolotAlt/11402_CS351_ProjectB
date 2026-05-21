CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2
TARGET   := csv_db
SRCS     := main.cpp csv_parser.cpp indexer.cpp fuzzy.cpp \
            query_parser.cpp query_engine.cpp
OBJS     := $(SRCS:.cpp=.o)

.PHONY: all clean run
all: $(TARGET)
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<
clean:
	rm -f $(OBJS) $(TARGET)
run: $(TARGET)
	./$(TARGET)
