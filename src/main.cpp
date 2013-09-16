#include <iostream>
#include <string>
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include "util.h"

const int WIN_WIDTH = 640;
const int WIN_HEIGHT = 480;

GLuint makeProgram();

int main(int argc, char **argv){
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0){
		std::cout << "Failed to init: " << SDL_GetError() << std::endl;
		return 1;
	}
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_Window *win = SDL_CreateWindow("Deferred Renderer",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIN_WIDTH, WIN_HEIGHT,
		SDL_WINDOW_OPENGL);

	SDL_GLContext context = SDL_GL_CreateContext(win);

	GLenum err = glewInit();
	if (err != GLEW_OK){
		std::cout << "GLEW init error: " << err << std::endl;
		return 2;
	}
	glEnable(GL_DEPTH_TEST);

	GLuint program = makeProgram();
	glUseProgram(program);
	GLint posAttrib = glGetAttribLocation(program, "position");
	GLint normAttrib = glGetAttribLocation(program, "normal");
	GLint modelLoc = glGetUniformLocation(program, "model");
	GLint projLoc = glGetUniformLocation(program, "proj");

	glm::mat4 model = glm::rotate<GLfloat>(0, 1, 1, 0);
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	
	glm::mat4 projection = glm::perspective(75.f, WIN_WIDTH / static_cast<float>(WIN_HEIGHT), 0.1f, 100.f);
	projection = projection * glm::lookAt(glm::vec3(0.f, 0.f, -5.f), glm::vec3(0.f, 0.f, 0.f),
		glm::vec3(0.f, 1.f, 0.f));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	GLuint obj[2];
	glGenBuffers(2, obj);
	size_t nElems = 0;
	//Note: Resource paths are relative to the top level directory as that's where it's expected
	//you're running the program
	if (!util::loadOBJ("res/polyhedron.obj", obj[0], obj[1], nElems)){
		std::cout << "obj loading failed" << std::endl;
	}
	glEnableVertexAttribArray(posAttrib);
	glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(GLfloat), 0);
	glEnableVertexAttribArray(normAttrib);
	glVertexAttribPointer(normAttrib, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(GL_FLOAT),
		(void*)(3 * sizeof(GL_FLOAT)));
	
	err = glGetError();
	if (err != GL_NO_ERROR){
		std::cout << "gl error: " << std::hex 
			<< err << std::dec << std::endl;
		return err;
	}

	SDL_Event e;
	bool quit = false;
	while (!quit){
		while (SDL_PollEvent(&e)){
			if (e.type == SDL_QUIT || (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)){
				quit = true;
			}
		}
		glClearColor(0.5f, 0.5f, 0.5f, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glDrawElements(GL_TRIANGLES, nElems, GL_UNSIGNED_SHORT, 0);

		SDL_GL_SwapWindow(win);
		//We're not doing anything interactive, just testing some rendering
		//so don't use so much cpu
		SDL_Delay(10);
	}

	glDeleteBuffers(2, obj);
	glDeleteVertexArrays(1, &vao);
	glDeleteProgram(program);
	SDL_GL_DeleteContext(context);
	SDL_DestroyWindow(win);

	return 0;
}
//Issue note: shaders/programs not deleted in case of error
GLuint makeProgram(){
	GLuint fShader = glCreateShader(GL_FRAGMENT_SHADER);
	GLuint vShader = glCreateShader(GL_VERTEX_SHADER);
	std::string fShaderCode = util::readFile("res/fshader.glsl");
	std::string vShaderCode = util::readFile("res/vshader.glsl");
	const char *fsrc = fShaderCode.c_str();
	const char *vsrc = vShaderCode.c_str();

	glShaderSource(fShader, 1, &fsrc, 0);
	glCompileShader(fShader);
	GLint status = 0;
	glGetShaderiv(fShader, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE){
		std::cout << "fShader compilation failed" << std::endl;
		GLint logLen = 0;
		glGetShaderiv(fShader, GL_INFO_LOG_LENGTH, &logLen);
		
		char *log = new char[logLen];
		glGetShaderInfoLog(fShader, logLen, 0, log);
		std::cout << "fshader log: " << log << std::endl;

		delete[] log;
		glDeleteShader(fShader);
		glDeleteShader(vShader);
		return -1;
	}

	glShaderSource(vShader, 1, &vsrc, 0);
	glCompileShader(vShader);
	glGetShaderiv(vShader, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE){
		std::cout << "vShader compilation failed" << std::endl;
		GLint logLen = 0;
		glGetShaderiv(vShader, GL_INFO_LOG_LENGTH, &logLen);
		
		char *log = new char[logLen];
		glGetShaderInfoLog(vShader, logLen, 0, log);
		std::cout << "vshader log: " << log << std::endl;

		delete[] log;
		glDeleteShader(fShader);
		glDeleteShader(vShader);
		return -1;
	}

	GLuint program = glCreateProgram();
	glAttachShader(program, vShader);
	glAttachShader(program, fShader);
	glLinkProgram(program);
	glGetProgramiv(program, GL_LINK_STATUS, &status);
	if (status == GL_FALSE){
		std::cout << "Linking failed" << std::endl;
		glDeleteShader(fShader);
		glDeleteShader(vShader);
		glDeleteProgram(program);
		return -1;
	}
	glDetachShader(program, fShader);
	glDetachShader(program, vShader);
	glDeleteShader(fShader);
	glDeleteShader(vShader);
	return program;
}
