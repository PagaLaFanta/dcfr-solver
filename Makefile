# DCFR Solver -- build
#   make            -> build ./solver
#   make run        -> build and launch the web UI
#   make console    -> build and open the text console
#   make test       -> evaluator unit tests
#   make bench      -> engine check across flop / turn / river
#   make clean

CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O3 -march=native -Wall -Wextra -Isrc
ifeq ($(OS),Windows_NT)
  # Other tools on PATH (Git for Windows, Qt...) ship an older, ABI-incompatible
  # libstdc++-6.dll, so link the runtime in. ws2_32 is the web UI socket.
  LDFLAGS ?= -static -lws2_32
else
  LDFLAGS ?= -pthread
endif

TARGET := solver
SRC    := src/main.cpp
HDRS   := $(wildcard src/*.hpp)

all: $(TARGET)

$(TARGET): $(SRC) $(HDRS)
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

run: $(TARGET)
	./$(TARGET) --gui

console: $(TARGET)
	./$(TARGET)

test_eval: src/test_eval.cpp $(HDRS)
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

test_solve: src/test_solve.cpp $(HDRS)
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

test: test_eval
	./test_eval

bench: test_solve
	./test_solve turn

clean:
	rm -f $(TARGET) $(TARGET).exe test_eval test_eval.exe test_solve test_solve.exe 	      bench_deal bench_deal.exe report*.txt *_out.txt

.PHONY: all run console test bench clean
