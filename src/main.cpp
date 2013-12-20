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

const int WIN_WIDTH = 1280;
const int WIN_HEIGHT = 720;
const int CUBE_MAP_DIM = 512;
//Indices for the quad's data
const int VAO = 0;
const int VBO = 1;
const int NORMALS = 2;
const int MODEL = 3;

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
const GLfloat quadNormals[18] = {
	0, 0, 1,
	0, 0, 1,
	0, 0, 1,
	0, 0, 1,
	0, 0, 1,
	0, 0, 1,
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
		viewDirs[2], glm::vec3(0.f, 0.f, 1.f));
	views[3] = glm::lookAt<GLfloat>(glm::vec3(0.f, 0.f, 0.f),
		viewDirs[3], glm::vec3(0.f, 0.f, -1.f));
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
		glm::vec3(0.f, 1.f, 0.f));
	glm::mat4 sceneProj = glm::perspective(75.f, static_cast<float>(WIN_WIDTH) / WIN_HEIGHT, 1.f, 100.f);
	//Setup a forward rendering program to simply draw the models w/ a point light at the origin
	GLuint program = util::loadProgram("res/vinstanced.glsl", "res/fshader.glsl");
	glUseProgram(program);
	projUnif = glGetUniformLocation(program, "proj");
	viewUnif = glGetUniformLocation(program, "view");
	GLuint lightViewUnif = glGetUniformLocation(program, "light_view");
	GLuint lightProjUnif = glGetUniformLocation(program, "light_proj");
	GLuint shadowMapUnif = glGetUniformLocation(program, "shadow_map");
	glUniformMatrix4fv(projUnif, 1, GL_FALSE, glm::value_ptr(sceneProj));
	glUniformMatrix4fv(viewUnif, 1, GL_FALSE, glm::value_ptr(sceneView));
	//Shadow map is texture unit 1
	glUniform1i(shadowMapUnif, 1);
	glUniformMatrix4fv(lightViewUnif, 6, GL_FALSE, (GLfloat*)(views));
	glUniformMatrix4fv(lightProjUnif, 1, GL_FALSE, glm::value_ptr(shadowProj));

	Model model("res/polyhedron.obj", program, shadowProg);
	//Setup some instances of the model surrounding the origin such that an instance
	//shows up on each face
	const int numInstances = 6;
	glm::mat4 modelMats[numInstances];
	for (int i = 0; i < numInstances; ++i){
		modelMats[i] = glm::translate<GLfloat>(3.f * viewDirs[i]);// * glm::rotate<GLfloat>(45.f, 0.f, 1.f, 1.f);
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
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}
	glActiveTexture(GL_TEXTURE0);
	for (int i = 0; i < 6; ++i){
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA, CUBE_MAP_DIM, CUBE_MAP_DIM, 0,
			GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	}
	glActiveTexture(GL_TEXTURE1);
	//How can i make use of these for the cube map? there's no textureProj for it. Or was textureProj
	//unrelated to the texture type? I'm not sure
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
	for (int i = 0;  i < 6; ++i){
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT32F, CUBE_MAP_DIM, CUBE_MAP_DIM, 0,
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

	//Setup a debug output quad, will also have buffers for normals and model mats so
	//that it can also be drawn in the scene to recieve shadows
	GLuint quad[4];
	glGenVertexArrays(1, &quad[VAO]);
	glGenBuffers(3, &quad[VBO]);
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
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (void*)(18 * sizeof(GLfloat)));
	GLuint quadCubeProg = util::loadProgram("res/vdbg.glsl", "res/fdbg.glsl");
	glUseProgram(quadCubeProg);
	GLuint texsUnif = glGetUniformLocation(quadCubeProg, "cube_maps");
	GLuint mapUnif = glGetUniformLocation(quadCubeProg, "map");
	GLint texs[2] = { 0, 1 };
	glUniform1iv(texsUnif, 2, texs);
	glUniform1i(mapUnif, 0);
	
	//Also want to draw the quads in the scene to recieve shadows
	glBindBuffer(GL_ARRAY_BUFFER, quad[NORMALS]);
	glBufferData(GL_ARRAY_BUFFER, 18 * sizeof(GLfloat), quadNormals, GL_STATIC_DRAW);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), 0);

	glm::mat4 quadModelMats[numInstances];
	quadModelMats[0] = glm::translate<GLfloat>(0.f, -4.f, 0.f) * glm::rotate<GLfloat>(-90.f, 1.f, 0.f, 0.f) *
		glm::scale<GLfloat>(6.f, 6.f, 1.f);
	glBindBuffer(GL_ARRAY_BUFFER, quad[MODEL]);
	glBufferData(GL_ARRAY_BUFFER, 1 * sizeof(glm::mat4), quadModelMats, GL_STATIC_DRAW);
	//Enable each column attribute of the matrix
	for (int j = 0; j < 4; ++j){
		glEnableVertexAttribArray(3 + j);
		glVertexAttribPointer(3 + j, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4),
			(void*)(sizeof(glm::vec4) * j));
		glVertexAttribDivisor(3 + j, 1);
	}

	//For tracking which cube face to render
	int face = 0;
	bool changeFace = false;
	//If we want to draw the scene or the cubemap
	bool drawScene = true;
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
					drawScene = false;
					break;
				case SDLK_2:
					face = 1;
					changeFace = true;
					drawScene = false;
					break;
				case SDLK_3:
					face = 2;
					changeFace = true;
					drawScene = false;
					break;
				case SDLK_4:
					face = 3;
					changeFace = true;
					drawScene = false;
					break;
				case SDLK_5:
					face = 4;
					changeFace = true;
					drawScene = false;
					break;
				case SDLK_6:
					face = 5;
					changeFace = true;
					drawScene = false;
					break;
				case SDLK_d:
					glUseProgram(quadCubeProg);
					glUniform1i(mapUnif, 1);
					drawScene = false;
					break;
				case SDLK_c:
					glUseProgram(quadCubeProg);
					glUniform1i(mapUnif, 0);
					drawScene = false;
					break;
				case SDLK_s:
					drawScene = true;
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

		glViewport(0, 0, CUBE_MAP_DIM, CUBE_MAP_DIM);
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glEnable(GL_POLYGON_OFFSET_FILL);
		glPolygonOffset(2.f, 4.f);
		model.bindShadow();
		glDrawElementsInstanced(GL_TRIANGLES, model.elems(), GL_UNSIGNED_SHORT, NULL, numInstances);

		glBindVertexArray(quad[VAO]);
		glDrawArraysInstanced(GL_TRIANGLES, 0, 6, 1);
		glDisable(GL_POLYGON_OFFSET_FILL);

		glViewport(0, 0, WIN_WIDTH, WIN_HEIGHT);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		if (drawScene){
			model.bind();
			glDrawElementsInstanced(GL_TRIANGLES, model.elems(), GL_UNSIGNED_SHORT, NULL, numInstances);

			glBindVertexArray(quad[VAO]);
			glDrawArraysInstanced(GL_TRIANGLES, 0, 6, 1);
		}
		else {
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_COMPARE_MODE, GL_NONE);
			glUseProgram(quadCubeProg);
			glBindVertexArray(quad[VAO]);
			glDrawArrays(GL_TRIANGLES, 0, 6);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
		}

		if (util::logGLError("Post-draw")){
				return 1;
		}
		SDL_GL_SwapWindow(win);
	}
	glDeleteVertexArrays(1, &quad[VAO]);
	glDeleteBuffers(3, &quad[VBO]);
	glDeleteBuffers(1, &modelMatBuf);
	glDeleteProgram(quadCubeProg);
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

