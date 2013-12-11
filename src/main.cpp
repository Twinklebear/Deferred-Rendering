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

/*
 * Load up the models being drawn in the scene and return them in the vector passed
 * Will set the models to use the uniform block containing the view and projection
 * matrices to the uniform buffer passed in. Also setup to use the passed in ubo
 * to provide the shadow shaders with the light's model/proj/view matrices. Super hacked in atm
 * TODO: Setup proper ref-counting for GL objects so I don't need to return a pointer
 * The view and proj matrices are the viewing and projection matrices for the scene
 * Texture units 0-2 are reserved for the deferred pass and 3 is used by the shadow map
 * but 4+ will be used by model textures
 * As a side note, the handles to the textures created here are lost and leaked
 * not a big deal now, but should make a wrapper around Texture2D that can be
 * associated with a Model or something
 */
std::vector<Model*> setupModels(const GLuint vpUBO, const GLuint lightUbo);
/*
 * Setup the depth buffer for the shadow map pass and return the texture
 * and framebuffer in the params passed. The texture will be active in
 * GL_TEXTURE3
 */
void setupShadowMap(GLuint &fbo, GLuint &tex);
/*
 * Perform the shadow map rendering pass
 */
void renderShadowMap(GLuint &fbo, const std::vector<Model*> &models, const int numInstances);

int main(int argc, char **argv){
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0){
		std::cout << "Failed to init: " << SDL_GetError() << std::endl;
		return 1;
	}
	
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
#ifdef DEBUG
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
#endif

	SDL_Window *win = SDL_CreateWindow("Deferred Renderer",
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
	//Get our projection and view matrices setup along with the inverses and the UBO
	//View is @ 0, proj is @ 1, the inverses are at idx + 2
	glm::mat4 viewProjMats[4];
	glm::vec4 viewPos(0.f, 6.f, 6.f, 1.f);
	viewProjMats[0] = glm::lookAt(glm::vec3(viewPos), glm::vec3(0.f, 0.f, 0.f),
		glm::vec3(0.f, 1.f, 0.f));
	viewProjMats[1] = glm::perspective(75.f, WIN_WIDTH / static_cast<float>(WIN_HEIGHT), 1.f, 100.f);
	viewProjMats[2] = glm::inverse(viewProjMats[0]);
	viewProjMats[3] = glm::inverse(viewProjMats[1]);

	GLuint viewProjUBO;
	glGenBuffers(1, &viewProjUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, viewProjUBO);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(glm::mat4) * 4, &viewProjMats[0], GL_STATIC_DRAW);

	//Center the light at the origin
	glm::vec3 lightPos(0.f, 1.f, -0.5);
	//Since the light is at the origin the matrix to translate it there is identity heh
	glm::mat4 lightMats[8];
	lightMats[0] = glm::translate<GLfloat>(-lightPos);
	lightMats[1] = glm::perspective<GLfloat>(90.f, 1.f, 1.f, 100.f);
	//Setup the different view matrices for each cube face
	glm::vec3 origin(0.f, 0.f, 0.f);
	//Why do the up directions get flipped?
	lightMats[2] = glm::lookAt<GLfloat>(origin, glm::vec3(1.f, 0.f, 0.f), glm::vec3(0.f, -1.f, 0.f));
	lightMats[3] = glm::lookAt<GLfloat>(origin, glm::vec3(-1.f, 0.f, 0.f), glm::vec3(0.f, -1.f, 0.f));
	lightMats[4] = glm::lookAt<GLfloat>(origin, glm::vec3(0.f, 1.f, 0.f), glm::vec3(0.f, 0.f, 1.f));
	lightMats[5] = glm::lookAt<GLfloat>(origin, glm::vec3(0.f, -1.f, 0.f), glm::vec3(0.f, 0.f, 1.f));
	lightMats[6] = glm::lookAt<GLfloat>(origin, glm::vec3(0.f, 0.f, 1.f), glm::vec3(0.f, 1.f, 0.f));
	lightMats[7] = glm::lookAt<GLfloat>(origin, glm::vec3(0.f, 0.f, 1.f), glm::vec3(0.f, 1.f, 0.f));

	//Setup buffer to hold the PointLight information
	GLuint pointLightUbo;
	glGenBuffers(1, &pointLightUbo);
	glBufferData(GL_UNIFORM_BUFFER, 8 * sizeof(glm::mat4), lightMats, GL_STATIC_DRAW);

	std::vector<Model*> models = setupModels(viewProjUBO, pointLightUbo);

	//Setup our render targets
	GLuint fbo;
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	util::logGLError("Made fbo");

	//0: diffuse, 1: normals, 2: depth
	GLuint texBuffers[3];
	glGenTextures(3, texBuffers);
	for (int i = 0; i < 2; ++i){
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, texBuffers[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, WIN_WIDTH, WIN_HEIGHT, 0, GL_RGB,
			GL_UNSIGNED_BYTE, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D,
			texBuffers[i], 0);
	}
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, texBuffers[2]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, WIN_WIDTH, WIN_HEIGHT,
		0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
		texBuffers[2], 0);

	GLenum drawBuffers[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
	glDrawBuffers(2, drawBuffers);

	util::logGLError("made & attached render targets");

	//Need another shader program for the second pass
	GLint progStatus = util::loadProgram("res/vsecondpass.glsl", "res/fsecondpass.glsl");
	if (progStatus == -1){
		return 1;
	}
	GLuint quadProg = progStatus;
	glUseProgram(quadProg);
	GLuint diffuseUnif = glGetUniformLocation(quadProg, "diffuse");
	GLuint normalUnif = glGetUniformLocation(quadProg, "normal");
	GLuint depthUnif = glGetUniformLocation(quadProg, "depth");
	glUniform1i(diffuseUnif, 0);
	glUniform1i(normalUnif, 1);
	glUniform1i(depthUnif, 2);

	//Pass in view proj uniform block
	GLuint vpBlockIdx = glGetUniformBlockIndex(quadProg, "ViewProj");
	glBindBufferBase(GL_UNIFORM_BUFFER, vpBlockIdx, viewProjUBO);

	//Pass them to the first pass shader for a forward lighting test
/*
	GLuint lightDirUnif = glGetUniformLocation(quadProg, "light_dir");
	GLuint viewPosUnif = glGetUniformLocation(quadProg, "view_pos");
	glUniform4fv(lightDirUnif, 1, glm::value_ptr(lightDir));
	glUniform4fv(viewPosUnif, 1, glm::value_ptr(viewPos));
*/
	//Shadow map is bound to texture unit 3
	GLuint shadowMapUnif = glGetUniformLocation(quadProg, "shadow_map");
	glUniform1i(shadowMapUnif, 3);
	//GLuint lightVPUnif = glGetUniformLocation(quadProg, "light_vp");
	//glUniformMatrix4fv(lightVPUnif, 1, GL_FALSE, glm::value_ptr(lightVP));

	//We render the second pass onto a quad drawn to the NDC
	Model quad("res/quad.obj", quadProg);

	//Setup the shadow map
	GLuint shadowTex, shadowFbo;
	setupShadowMap(shadowFbo, shadowTex);

	//For instanced rendering: Bind the cube and slip our model matrix buffer into the VAO as
	//attribute 3
	const int numInstances = 6;
	//Location of the first instance
	glm::vec3 firstPos(-4.f, 1.f, -2.f);
	glm::mat4 modelMats[numInstances];
	//Build up a few model matrices and give them ordered positions
	int rowLength = 3;
	for (int i = 0; i < numInstances; ++i){
		glm::vec3 offset(4 * (i % rowLength), 0.f, 4 * (i / rowLength));
		modelMats[i] = glm::translate<GLfloat>(offset + firstPos);
	}
	models.at(0)->bind();
	GLuint modelMatBuf;
	glGenBuffers(1, &modelMatBuf);
	glBindBuffer(GL_ARRAY_BUFFER, modelMatBuf);
	glBufferData(GL_ARRAY_BUFFER, numInstances * sizeof(glm::mat4), &modelMats[0], GL_STATIC_DRAW);
	//Enable each column of the matrix
	for (int i = 0; i < numInstances; ++i){
		for (int j = 0; j < 4; ++j){
			glEnableVertexAttribArray(3 + j);
			glVertexAttribPointer(3 + j, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4),
				(void*)(sizeof(glm::vec4) * j));
			glVertexAttribDivisor(3 + j, 1);
		}
	}

	if (util::logGLError("Pre-loop error check")){
		return 1;
	}

	//For tracking fps
	float frameTime = 0.0;
	bool printFps = false;
	int start = SDL_GetTicks();
	SDL_Event e;
	bool quit = false;
	while (!quit){
		while (SDL_PollEvent(&e)){
			if (e.type == SDL_QUIT || (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)){
				quit = true;
			}
			if (e.type == SDL_KEYDOWN){
				switch (e.key.keysym.sym){
				case SDLK_f:
					printFps = !printFps;
					break;
				default:
					break;
				}
			}
		}
		//For testing just do the shadow map pass
		//Shadow map pass
		renderShadowMap(shadowFbo, models, numInstances);
/*
		//First pass
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		//The instanced cubes
		models.at(0)->bind();
		glDrawElementsInstanced(GL_TRIANGLES, models.at(0)->elems(), GL_UNSIGNED_SHORT,
			NULL, numInstances);

		//Draw the floor
		models.at(1)->bind();
		glDrawElements(GL_TRIANGLES, models.at(1)->elems(), GL_UNSIGNED_SHORT, 0);

		util::logGLError("post first pass");

		//Second pass
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		quad.bind();
		glDrawElements(GL_TRIANGLES, quad.elems(), GL_UNSIGNED_SHORT, 0);
*/
		util::logGLError("post second pass");

		SDL_GL_SwapWindow(win);
		int end = SDL_GetTicks();
		//Keep a smoothed average of the time per frame
		frameTime = 0.9 * (end - start) / 1000.f + 0.1 * frameTime;
		start = end;
		if (printFps){
			std::cout << "frame time: " << frameTime << "ms\n";
		}
	}
	glDeleteBuffers(1, &viewProjUBO);
	glDeleteBuffers(1, &modelMatBuf);

	glDeleteFramebuffers(1, &shadowFbo);
	glDeleteTextures(1, &shadowTex);

	glDeleteFramebuffers(1, &fbo);
	glDeleteTextures(3, texBuffers);
	
	SDL_GL_DeleteContext(context);
	SDL_DestroyWindow(win);

	return 0;
}
std::vector<Model*> setupModels(const GLuint vpUBO, const GLuint lightUbo){
	std::vector<Model*> models;
	GLint progStatus = util::loadProgram("res/vinstanced.glsl", "res/fshader.glsl");
	if (progStatus == -1){
		std::cerr << "Failed to load program\n";
		return models;
	}
	GLuint program = progStatus;
	glUseProgram(program);

	//Load a texture for the polyhedron
	glActiveTexture(GL_TEXTURE4);
	GLuint texture = util::loadTexture("res/texture.bmp");
	glBindTexture(GL_TEXTURE_2D, texture);
	GLuint texUnif = glGetUniformLocation(program, "tex_diffuse");
	glUniform1i(texUnif, 4);
	//Set the view/proj uniform block
	GLuint vpBlockIdx = glGetUniformBlockIndex(program, "ViewProj");
	glBindBufferBase(GL_UNIFORM_BUFFER, vpBlockIdx, vpUBO);

	GLuint shadowProgram = util::loadProgram("res/vshadow_point_instanced.glsl", "res/fshadow.glsl",
		"res/gshadow_instanced.glsl");
	glUseProgram(shadowProgram);
	GLuint lightBlockIdx = glGetUniformBlockIndex(shadowProgram, "PointLight");
	std::cout << "Light block idx in shadow program: " << lightBlockIdx << "\n";
	glBindBufferBase(GL_UNIFORM_BUFFER, lightBlockIdx, lightUbo);
	//With suzanne the self-shadowing is much easier to see
	Model *polyhedron = new Model("res/suzanne.obj", program, shadowProgram);
	models.push_back(polyhedron);

	//TODO: Perhaps in the future a way to share programs across models and optimize drawing
	//order to reduce calls to glUseProgram? could also share proj/view matrices with UBOs
	progStatus = util::loadProgram("res/vshader.glsl", "res/fshader.glsl");
	if (progStatus == -1){
		std::cerr << "Failed to load program\n";
		return models;
	}
	program = progStatus;
	glUseProgram(program);

	//Load a texture for the floor
	glActiveTexture(GL_TEXTURE5);
	texture = util::loadTexture("res/texture2.bmp");
	glBindTexture(GL_TEXTURE_2D, texture);
	texUnif = glGetUniformLocation(program, "tex_diffuse");
	glUniform1i(texUnif, 5);

	vpBlockIdx = glGetUniformBlockIndex(program, "ViewProj");
	glBindBufferBase(GL_UNIFORM_BUFFER, vpBlockIdx, vpUBO);

	shadowProgram = util::loadProgram("res/vshadow.glsl", "res/fshadow.glsl");
	Model *floor = new Model("res/quad.obj", program, shadowProgram);
	//Get it laying perpindicularish to the light direction and behind the camera some
	floor->scale(glm::vec3(8.f, 8.f, 1.f));
	floor->rotate(glm::rotate(-90.f, 1.f, 0.f, 0.f));
	models.push_back(floor);

	return models;
}
void setupShadowMap(GLuint &fbo, GLuint &tex){
	glActiveTexture(GL_TEXTURE3);
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_CUBE_MAP, tex);
	//Setup each face of the cube
	for (int i = 0; i < 6; ++i){
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT32F,
			512, 512, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	}
	//No mip maps
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	//Setup depth comparison mode
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, tex, 0);
	glDrawBuffer(GL_NONE);

	GLenum fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	switch (fboStatus){
	case GL_FRAMEBUFFER_UNDEFINED:
		std::cout << "Shadow FBO incomplete: undefined\n";
		break;
	case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
		std::cout << "Shadow FBO incomplete: attachment\n";
		break;
	case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
		std::cout << "Shadow FBO incomplete: missing attachment\n";
		break;
	case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
		std::cout << "Shadow FBO incomplete: draw buffer\n";
		break;
	case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
		std::cout << "Shadow FBO incomplete: read buffer\n";
		break;
	case GL_FRAMEBUFFER_UNSUPPORTED:
		std::cout << "Shadow FBO incomplete: unsupported\n";
		break;
	case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
		std::cout << "Shadow FBO incomplete: multisample\n";
		break;
	case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
		std::cout << "Shadow FBO incomplete: layer targets\n";
		break;
	default:
		break;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	util::logGLError("Setup shadow map fbo & texture");
}
void renderShadowMap(GLuint &fbo, const std::vector<Model*> &models, const int numInstances){
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glClear(GL_DEPTH_BUFFER_BIT);
	//Polygon offset fill helps resolve depth-fighting
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(2.f, 4.f);
	//Only draw the instanced foreground objects
	models.at(0)->bindShadow();
	glDrawElementsInstanced(GL_TRIANGLES, models.at(0)->elems(), GL_UNSIGNED_SHORT,
		NULL, numInstances);

	glDisable(GL_POLYGON_OFFSET_FILL);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

