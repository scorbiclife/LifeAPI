.PHONY: test

all: test

CXXFLAGS = -std=c++20 -Wall -Wextra -pedantic

GTEST_CFLAGS = `pkg-config --cflags gtest_main`
GTEST_LIBS = `pkg-config --libs gtest_main`

TEST_SOURCES = $(wildcard tests/*.cpp)
TEST_OBJECTS = $(TEST_SOURCES:.cpp=.o)

tests/%.o: tests/%.cpp *.hpp
	$(CXX) $(CXXFLAGS) $(GTEST_CFLAGS) -c -o $@ $<

testapp: $(TEST_OBJECTS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ $(GTEST_LIBS)

test: testapp
	./testapp
