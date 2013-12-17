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
const int VAO = 0;
const int VBO = 1;

const GLfloat triangle[] = {
	0.f, 0.f, 0.f,
	1.f, 0.f, 0.f,
	1.f, 1.f, 0.f
};
//Positions of vertices for a quad
const GLfloat quadVerts[18] = {
	//positions
	-1, -1, 0,
	1, -1, 0,
	1, 1, 0,
	-1, -1, 0,
	1, 1, 0,
	-1, 1, 0
};
//Cube-map texcoords for the quad, for each face of the cubemap
//face names are in gl order: pos_x, neg_x, pos_y, neg_y, pos_z, neg_z
const GLfloat cubeMapUV[6][18] = {
	{ 1, 1, 1, 1, 1, -1, 1, -1, -1, 1, 1, 1, 1, -1, -1, 1, -1, 1 },
	{ -1, 1, -1, -1, 1, 1, -1, -1, 1, -1, 1, -1, -1, -1, 1, -1, -1, -1 },
	{ -1, 1, -1, 1, 1, -1, 1, 1, 1, -1, 1, -1, 1, 1, 1, -1, 1, 1 },
	{ -1, -1, 1, 1, -1, 1, 1, -1, -1, -1, -1, 1, 1, -1, -1, -1, -1, -1 },
	{ -1, 1, 1, 1, 1, 1, 1, -1, 1, -1, 1, 1, 1, -1, 1, -1, -1, 1 },
	{ 1, 1, -1, -1, 1, -1, -1, -1, -1, 1, 1, -1, -1, -1, -1, 1, -1, -1 }
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
	glClearDepth(1.f);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

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
	//Load and setup the program with layered view matrices and a single projection matrix
	glm::mat4 shadowProj = glm::perspective<GLfloat>(90.f, 1.f, 1.f, 100.f);
	glm::vec3 viewDirs[6] = { glm::vec3(1.f, 0.f, 0.f), glm::vec3(-1.f, 0.f, 0.f),
		glm::vec3(0.f, 1.f, 0.f), glm::vec3(0.f, -1.f, 0.f), glm::vec3(0.f, 0.f, 1.f),
		glm::vec3(0.f, 0.f, -1.f)
	};
	glm::mat4 views[6];
	//Viewing matrices for each face of the cube looking out
	views[0] = glm::lookAt<GLfloat>(glm::vec3(0.f, 0.f, 0.f),
		viewDirs[0], glm::vec3(0.f, 1.f, 0.f));
	views[1] = glm::lookAt<GLfloat>(glm::vec3(0.f, 0.f, 0.f),
		 viewDirs[1], glm::vec3(0.f, 1.f, 0.f));
	views[2] = glm::lookAt<GLfloat>(glm::vec3(0.f, 0.f, 0.f),
		viewDirs[2], glm::vec3(0.f, 0.f, -1.f));
	views[3] = glm::lookAt<GLfloat>(glm::vec3(0.f, 0.f, 0.f),
		viewDirs[3], glm::vec3(0.f, 0.f, 1.f));
	views[4] = glm::lookAt<GLfloat>(glm::vec3(0.f, 0.f, 0.f),
		viewDirs[4], glm::vec3(0.f, 1.f, 0.f));
	views[5] = glm::lookAt<GLfloat>(glm::vec3(0.f, 0.f, 0.f),
		viewDirs[5], glm::vec3(0.f, 1.f, 0.f));
	//Setup the shadow program to render to the cube map
	GLuint shadowProg = util::loadProgram("res/vlayered_instanced.glsl", "res/fshadow.glsl", "res/glayered_test.glsl");
	glUseProgram(shadowProg);
	GLuint projUnif = glGetUniformLocation(shadowProg, "proj");
	GLuint viewUnif = glGetUniformLocation(shadowProg, "view");
	glUniformMatrix4fv(projUnif, 1, GL_FALSE, glm::value_ptr(shadowProj));
	//Better way to do this properly? UBO? Unpacking the matrices would be a real pain and
	//i think glm is supposed to interop transparently w/GL
	glUniformMatrix4fv(viewUnif, 6, GL_FALSE, (GLfloat*)(views));
	if (util::logGLError("Set up shadow program")){
		return 1;
	}
	
	//The scene's view and projection matrices
	glm::mat4 sceneView = glm::lookAt<GLfloat>(glm::vec3(5.f, 5.f, 5.f), glm::vec3(0.f, 0.f, 0.f),
		glm::vec3(-1.f, 1.f, -1.f));
	glm::mat4 sceneProj = glm::perspective(75.f, static_cast<float>(WIN_WIDTH) / WIN_HEIGHT, 1.f, 100.f);
	//Setup a forward rendering program to simply draw the models w/ a point light at the origin
	GLuint program = util::loadProgram("res/vinstanced.glsl", "res/fshader.glsl");
	glUseProgram(program);
	projUnif = glGetUniformLocation(program, "proj");
	viewUnif = glGetUniformLocation(program, "view");
	glUniformMatrix4fv(projUnif, 1, GL_FALSE, glm::value_ptr(sceneProj));
	glUniformMatrix4fv(viewUnif, 1, GL_FALSE, glm::value_ptr(sceneView));

	Model model("res/suzanne.obj", program, shadowProg);
	//Setup some instances of the model surrounding the origin such that an instance
	//shows up on each face
	const int numInstances = 6;
	glm::mat4 modelMats[numInstances];
	for (int i = 0; i < numInstances; ++i){
		modelMats[i] = glm::translate<GLfloat>(3.f * viewDirs[i]);
	}
	model.bind();
	GLuint modelMatBuf;
	glGenBuffers(1, &modelMatBuf);
	glBindBuffer(GL_ARRAY_BUFFER, modelMatBuf);
	glBufferData(GL_ARRAY_BUFFER, numInstances * sizeof(glm::mat4), modelMats, GL_STATIC_DRAW);
	//Enable each column attribute of the matrix
	for (int j = 0; j < 4; ++j){
		glEnableVertexAttribArray(3 + j);
		glVertexAttribPointer(3 + j, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4),
			(void*)(sizeof(glm::vec4) * j));
		glVertexAttribDivisor(3 + j, 1);
	}

	if (util::logGLError("Setup model")){
		return 1;
	}

	//Setup the cube map rendering target for color and depth
	GLuint tex[2];
	glGenTextures(2, tex);
	for (int i = 0; i < 2; ++i){
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_CUBE_MAP, tex[i]);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, 0);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}
	glActiveTexture(GL_TEXTURE0);
	for (int i = 0; i < 6; ++i){
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA, 512, 512, 0,
			GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	}
	glActiveTexture(GL_TEXTURE1);
	for (int i = 0;  i < 6; ++i){
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT32, 512, 512, 0,
			GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	}
	if (util::logGLError("setup texture")){
		return 1;
	}

	GLuint fbo;
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, tex[0], 0);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, tex[1], 0);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	if (!checkFrameBuffer(fbo) || util::logGLError("setup fbo")){
		std::cerr << "FBO error!\n";
		return 1;
	}
	//Query if it's really a layered attachmnet
	GLint isLayered = GL_FALSE;
	glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
		GL_FRAMEBUFFER_ATTACHMENT_LAYERED, &isLayered);
	if (isLayered == GL_FALSE){
		std::cout << "Attachment is not layered?\n";
		return 1;
	}

	//Setup a debug output quad
	GLuint quad[2];
	glGenVertexArrays(1, &quad[VAO]);
	glGenBuffers(1, &quad[VBO]);
	glBindVertexArray(quad[VAO]);
	glBindBuffer(GL_ARRAY_BUFFER, quad[VBO]);
	//Room for the quads positions and cube map tex coords
	glBufferData(GL_ARRAY_BUFFER, 36 * sizeof(GLfloat), NULL, GL_DYNAMIC_DRAW);
	//Write the position data
	glBufferSubData(GL_ARRAY_BUFFER, 0, 18 * sizeof(GLfloat), quadVerts);
	//Initially we'll draw +X face
	glBufferSubData(GL_ARRAY_BUFFER, 18 * sizeof(GLfloat), 18 * sizeof(GLfloat), cubeMapUV[0]);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), 0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (void*)(18 * sizeof(GLfloat)));
	GLuint quadProg = util::loadProgram("res/vdbg.glsl", "res/fdbg.glsl");
	glUseProgram(quadProg);
	GLuint texsUnif = glGetUniformLocation(quadProg, "cube_maps");
	GLuint mapUnif = glGetUniformLocation(quadProg, "map");
	GLint texs[2] = { 0, 1 };
	glUniform1iv(texsUnif, 2, texs);
	glUniform1i(mapUnif, 0);

	//For tracking which cube face to render
	int face = 0;
	bool changeFace = false;
	//If we want to draw the scene or the cubemap
	bool drawScene = false;

	SDL_Event e;
	bool quit = false;
	while (!quit){
		while (SDL_PollEvent(&e)){
			if (e.type == SDL_KEYDOWN){
				switch (e.key.keysym.sym){
				case SDLK_ESCAPE:
					quit = true;
					break;
				case SDLK_1:
					face = 0;
					changeFace = true;
					break;
				case SDLK_2:
					face = 1;
					changeFace = true;
					break;
				case SDLK_3:
					face = 2;
					changeFace = true;
					break;
				case SDLK_4:
					face = 3;
					changeFace = true;
					break;
				case SDLK_5:
					face = 4;
					changeFace = true;
					break;
				case SDLK_6:
					face = 5;
					changeFace = true;
					break;
				case SDLK_d:
					glUseProgram(quadProg);
					glUniform1i(mapUnif, 1);
					break;
				case SDLK_c:
					glUseProgram(quadProg);
					glUniform1i(mapUnif, 0);
					break;
				case SDLK_s:
					drawScene = !drawScene;
					break;
				default:
					break;
				}
			}
		}
		if (changeFace){
			glBindBuffer(GL_ARRAY_BUFFER, quad[VBO]);
			glBufferSubData(GL_ARRAY_BUFFER, 18 * sizeof(GLfloat), 18 * sizeof(GLfloat), cubeMapUV[face]);
		}

		glViewport(0, 0, 512, 512);
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		model.bindShadow();
		glDrawElementsInstanced(GL_TRIANGLES, model.elems(), GL_UNSIGNED_SHORT, NULL, numInstances);

		glViewport(0, 0, 640, 480);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		if (drawScene){
			model.bind();
			glDrawElementsInstanced(GL_TRIANGLES, model.elems(), GL_UNSIGNED_SHORT, NULL, numInstances);
		}
		else {
			glUseProgram(quadProg);
			glBindVertexArray(quad[VAO]);
			glDrawArrays(GL_TRIANGLES, 0, 6);
		}

		if (util::logGLError("Post-draw")){
				return 1;
		}
		SDL_GL_SwapWindow(win);
	}
	glDeleteVertexArrays(1, &quad[VAO]);
	glDeleteBuffers(1, &quad[VBO]);
	glDeleteProgram(quadProg);
	glDeleteFramebuffers(1, &fbo);
	glDeleteTextures(2, tex);
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

