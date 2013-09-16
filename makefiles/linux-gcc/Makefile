CXX = g++
SDL_INCLUDE = -I/usr/local/include/SDL2/
SDL_LIB = -L/usr/local/lib/ -lSDL2
CXXFLAGS = -std=c++11 -c -Wall -Wextra -pedantic -g $(SDL_INCLUDE)
LDFLAGS = -lGL -lGLEW $(SDL_LIB)
# Generate listing of all our source files and object  files
SRC = src
BUILD = build
OUT = bin
SOURCES = $(wildcard $(SRC)/*.cpp)
OBJS = $(addprefix $(BUILD)/, $(notdir $(SOURCES:.cpp=.o)))
TARGET = $(OUT)/render.exe

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) $(LDFLAGS) -o $@

$(BUILD)/%.o: $(SRC)/%.cpp | $(BUILD)
	$(CXX) $(CXXFLAGS) $< -o $@

# Make sure the build directory exists
$(BUILD):
	mkdir $(BUILD)
	mkdir $(OUT)

# Note that it's $(OUT)\*.o here to avoid some weird windows issue
.PHONY: clean
clean:
	rm $(BUILD)/*.o && rm $(TARGET)
