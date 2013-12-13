#include <iostream>
#include <string>
#include <vector>
#include <GL/glew.h>
#include <glm/ext.hpp>
#include <glm/glm.hpp>

#ifdef __linux__
#include <SDL2/SDL.h>
#elif defined(_WIN32)
#include <SDL.h>
#endif

#include "model.h"
#include "util.h"

const int WIN_WIDTH = 640;
const int WIN_HEIGHT = 480;

const GLfloat triangle[] = {
	0.f, 0.f, 0.f,
	1.f, 0.f, 0.f,
	1.f, 1.f, 0.f
};

//Check a framebuffer's status, returns true if ok
bool checkFrameBuffer(GLuint fbo);

/*
 * Some basic layered rendering tests, should be run within gDEBugger as nothing is
 * output to the screen, just to the layered render targets
 */
int main(int argc, char **argv){
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0){
		std::cout << "Failed to init: " << SDL_GetError() << std::endl;
		return 1;
	}
	
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
#ifdef DEBUG
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
#endif

	SDL_Window *win = SDL_CreateWindow("Layered Renderering",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIN_WIDTH, WIN_HEIGHT,
		SDL_WINDOW_OPENGL);
	
	SDL_GLContext context = SDL_GL_CreateContext(win);

	util::logGLError("Post SDL init");

	glewExperimental = GL_TRUE;
	GLenum err = glewInit();
	if (err != GLEW_OK){
		std::cout << "GLEW init error: " << err << std::endl;
		return 1;
	}
	//Note: If we see an invalid enumerant error that's a result of glewExperimental
	//and it sounds like it can be safely ignored
	util::logGLError("Post GLEW init");

	glClearColor(0.f, 0.f, 0.f, 1.f);
	glDisable(GL_DEPTH_TEST);

	std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << "\n"
		<< "OpenGL Vendor: " << glGetString(GL_VENDOR) << "\n"
		<< "OpenGL Renderer: " << glGetString(GL_RENDERER) << "\n"
		<< "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << "\n";

#ifdef DEBUG
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
	glDebugMessageCallbackARB(util::glDebugCallback, NULL);
	glDebugMessageControlARB(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE,
		0, NULL, GL_TRUE);
#endif
	GLuint vao, vbo;
	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, 9 * sizeof(GLfloat), triangle, GL_STATIC_DRAW);
	//All we care about is position for this test, since doing flat shading
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

	//Load and setup the program with the model, layered view matrices and project matrix
	glm::mat4 modelMat = glm::translate<GLfloat>(-1.5f, 0.f, 0.f) * glm::scale<GLfloat>(3.f, 3.f, 1.f);
	glm::mat4 proj = glm::perspective<GLfloat>(90.f, 1.f, 1.f, 100.f);
	glm::mat4 views[2];
	//Right side up and up-side down viewing matrices
	views[0] = glm::lookAt<GLfloat>(glm::vec3(0.f, 0.f, 4.f),
		glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, 1.f, 0.f));
	views[1] = glm::lookAt<GLfloat>(glm::vec3(0.f, 0.f, 4.f),
		glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, -1.f, 0.f));

	GLuint program = util::loadProgram("res/vlayered_test.glsl", "res/fshader.glsl", "res/glayered_test.glsl");
	glUseProgram(program);
	GLuint modelUnif = glGetUniformLocation(program, "model");
	GLuint projUnif = glGetUniformLocation(program, "proj");
	GLuint viewsUnif = glGetUniformLocation(program, "view");
	glUniformMatrix4fv(modelUnif, 1, GL_FALSE, glm::value_ptr(modelMat));
	glUniformMatrix4fv(projUnif, 1, GL_FALSE, glm::value_ptr(proj));
	//Better way to do this properly? UBO? Unpacking the matrices would be a real pain and
	//i think glm is supposed to interop transparently w/GL
	glUniformMatrix4fv(viewsUnif, 2, GL_FALSE, (GLfloat*)(views));
	if (util::logGLError("Set uniforms")){
		return 1;
	}

	//Setup the layered rendering target w/ 2 layers
	GLuint tex;
	glGenTextures(1, &tex);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D_ARRAY, tex);
	glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA, 512, 512, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	if (util::logGLError("setup texture array")){
		return 1;
	}

	GLuint fbo;
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, tex, 0);
	const GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, drawBuffers);
	if (!checkFrameBuffer(fbo) || util::logGLError("setup fbo")){
		std::cerr << "FBO error!\n";
		return 1;
	}
	//Query if it's really a layered attachmnet
	GLint isLayered = GL_FALSE;
	glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
		GL_FRAMEBUFFER_ATTACHMENT_LAYERED, &isLayered);
	if (isLayered == GL_FALSE){
		std::cout << "Attachment is not layered somehow?\n";
		return 1;
	}

	glViewport(0, 0, 512, 512);
	SDL_Event e;
	bool quit = false;
	while (!quit){
		while (SDL_PollEvent(&e)){
			if (e.type == SDL_QUIT || (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)){
				quit = true;
			}
		}
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glClear(GL_COLOR_BUFFER_BIT);
		glDrawArrays(GL_TRIANGLES, 0, 3);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		if (util::logGLError("Post-draw")){
				return 1;
		}

		SDL_GL_SwapWindow(win);
	}
	glDeleteVertexArrays(1, &vao);
	glDeleteBuffers(1, &vbo);
	glDeleteProgram(program);
	glDeleteFramebuffers(1, &fbo);
	glDeleteTextures(1, &tex);
	SDL_GL_DeleteContext(context);
	SDL_DestroyWindow(win);

	return 0;
}
bool checkFrameBuffer(GLuint fbo){
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	GLenum fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	switch (fboStatus){
	case GL_FRAMEBUFFER_UNDEFINED:
		std::cout << "FBO incomplete: undefined\n";
		return false;
	case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
		std::cout << "FBO incomplete: attachment\n";
		return false;
	case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
		std::cout << "FBO incomplete: missing attachment\n";
		return false;
	case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
		std::cout << "FBO incomplete: draw buffer\n";
		return false;
	case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
		std::cout << "FBO incomplete: read buffer\n";
		return false;
	case GL_FRAMEBUFFER_UNSUPPORTED:
		std::cout << "FBO incomplete: unsupported\n";
		return false;
	case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
		std::cout << "FBO incomplete: multisample\n";
		return false;
	case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
		std::cout << "FBO incomplete: layer targets\n";
		return false;
	case GL_FRAMEBUFFER_INCOMPLETE_LAYER_COUNT_ARB:
		std::cout << "FBO incomplete: layer count\n";
		return false;
	default:
			break;
	}
	return true;
}

