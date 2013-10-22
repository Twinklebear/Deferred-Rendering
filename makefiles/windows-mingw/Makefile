CXX = g++
SDL2 = C:/SDL2-2.0.0-mingw
SDL_INCLUDE = -I $(SDL2)/i686-w64-mingw32/include
SDL_LIB = -L $(SDL2)/i686-w64-mingw32/lib -lSDL2main -lSDL2
GLEW = C:\glew-1.10.0-mingw
GLEW_INCLUDE = -I $(GLEW)/include
GLEW_LIB = -L $(GLEW)/lib -lglew32
GLM = C:/glm-0.9.4.1
GLM_LIB = -L $(GLM)/bin
CXXFLAGS = -std=c++11 -c -Wall -Wextra -pedantic -g -I $(GLM) $(GLEW_INCLUDE)  $(SDL_INCLUDE)
LDFLAGS = -lmingw32 -mwindows -mconsole -lglu32 -lOpenGL32 $(GLEW_LIB) $(GLM_LIB) $(SDL_LIB)
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
