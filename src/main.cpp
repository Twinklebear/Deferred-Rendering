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
 * TODO: Setup proper ref-counting for GL objects so I don't need to return a pointer
 * The view and proj matrices are the viewing and projection matrices for the scene
 * Texture units 0-2 are reserved for the deferred pass and 3 is used by the shadow map
 * but 4+ will be used by model textures
 * As a side note, the handles to the textures created here are lost and leaked
 * not a big deal now, but should make a wrapper around Texture2D that can be
 * associated with a Model or something
 */
std::vector<Model*> setupModels(const glm::mat4 &view, const glm::mat4 &proj);
/*
 * Setup the depth buffer for the shadow map pass and return the texture
 * and framebuffer in the params passed. The texture will be active in
 * GL_TEXTURE3
 */
void setupShadowMap(GLuint &fbo, GLuint &tex);
/*
 * Perform the shadow map rendering pass
 */
void renderShadowMap(GLuint &fbo, const std::vector<Model*> &models);

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
	
	glm::mat4 projection = glm::perspective(75.f,
		WIN_WIDTH / static_cast<float>(WIN_HEIGHT), 1.f, 100.f);
	glm::vec4 viewPos = glm::vec4(0.f, 0.f, 5.f, 1.f);
	glm::mat4 view = glm::lookAt(glm::vec3(viewPos), glm::vec3(0.f, 0.f, 0.f),
		glm::vec3(0.f, 1.f, 0.f));
	glm::vec4 viewDir(0.f, 0.f, 1.f, 0.f);

	std::vector<Model*> models = setupModels(view, projection);

	//The spotlight direction and position, simply place the light a bit back along
	//the direction it's coming from
	glm::vec4 lightDir = glm::normalize(glm::vec4(1.f, 0.f, 1.f, 0.f));
	glm::vec4 lightPos = 6 * lightDir;
	lightPos.w = 1.f;
	//Setup the light's view & projection matrix for the light
	glm::mat4 lightView = glm::lookAt(glm::vec3(lightPos), glm::vec3(0.f, 0.f, 0.f),
		glm::vec3(0.f, 1.f, 0.f));
	//The max angle the spotlight illuminates off of the direction
	GLfloat spotlightAngle = 20.f;
	//The full FoV y of the spotlight is 2*angle to account for +/-
	glm::mat4 lightVP = glm::perspective(2.f * spotlightAngle, 1.f, 1.f, 100.f)
		* lightView;

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
	GLint progStatus = util::loadProgram("res/vsecondpass.glsl",
		"res/fsecondpass.glsl");
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

	GLuint invProjUnif = glGetUniformLocation(quadProg, "inv_proj");
	GLuint invViewUnif = glGetUniformLocation(quadProg, "inv_view");
	glm::mat4 invProj = glm::inverse(projection);
	glm::mat4 invView = glm::inverse(view);
	glUniformMatrix4fv(invProjUnif, 1, GL_FALSE, glm::value_ptr(invProj));
	glUniformMatrix4fv(invViewUnif, 1, GL_FALSE, glm::value_ptr(invView));

	GLuint lightDirUnif = glGetUniformLocation(quadProg, "light_dir");
	GLuint lightPosUnif = glGetUniformLocation(quadProg, "light_pos");
	GLuint viewPosUnif = glGetUniformLocation(quadProg, "view_pos");
	GLuint spotlightAngleUnif = glGetUniformLocation(quadProg, "spotlight_angle");
	glUniform4fv(lightDirUnif, 1, glm::value_ptr(lightDir));
	glUniform4fv(lightPosUnif, 1, glm::value_ptr(lightPos));
	glUniform4fv(viewPosUnif, 1, glm::value_ptr(viewPos));
	glUniform1f(spotlightAngleUnif, std::cos(spotlightAngle * 3.145 / 180.0));

	//Shadow map is bound to texture unit 3
	GLuint shadowMapUnif = glGetUniformLocation(quadProg, "shadow_map");
	glUniform1i(shadowMapUnif, 3);
	GLuint lightVPUnif = glGetUniformLocation(quadProg, "light_vp");
	glUniformMatrix4fv(lightVPUnif, 1, GL_FALSE, glm::value_ptr(lightVP));

	//We render the second pass onto a quad drawn to the NDC
	Model quad("res/quad.obj", quadProg);

	//Setup the shadow map
	GLuint shadowTex, shadowFbo;
	setupShadowMap(shadowFbo, shadowTex);
	for (Model *m : models){
		m->setShadowVP(lightVP);
	}
	
	//Setup a debug output quad to be drawn to NDC after all other rendering
	GLuint dbgProgram = util::loadProgram("res/vforward.glsl",
		"res/fforward_lum.glsl");
	Model dbgOut("res/quad.obj", dbgProgram);
	dbgOut.scale(glm::vec3(0.3f, 0.3f, 1.f));
	dbgOut.translate(glm::vec3(-0.7f, 0.7f, 0.f));
	glUseProgram(dbgProgram);
	GLuint dbgTex = glGetUniformLocation(dbgProgram, "tex");
	glUniform1i(dbgTex, 3);

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
				//Move the main subject around (model 0)
				switch (e.key.keysym.sym){
					case SDLK_f:
						printFps = !printFps;
						break;
					case SDLK_a:
						models.at(0)->translate(frameTime * glm::vec3(-2.f, 0.f, 0.f));
						break;
					case SDLK_d:
						models.at(0)->translate(frameTime * glm::vec3(2.f, 0.f, 0.f));
						break;
					case SDLK_w:
						models.at(0)->translate(frameTime * glm::vec3(0.f, 2.f, 0.f));
						break;
					case SDLK_s:
						models.at(0)->translate(frameTime * glm::vec3(0.f, -2.f, 0.f));
						break;
					case SDLK_z:
						models.at(0)->translate(frameTime * glm::vec3(0.f, 0.f, -2.f));
						break;
					case SDLK_x:
						models.at(0)->translate(frameTime * glm::vec3(0.f, 0.f, 2.f));
						break;
					case SDLK_q:
						models.at(0)->rotate(glm::rotate<GLfloat>(frameTime * -45.f, 0.f, 1.f, 0.f));
						break;
					case SDLK_e:
						models.at(0)->rotate(glm::rotate<GLfloat>(frameTime * 45.f, 0.f, 1.f, 0.f));
						break;
					default:
						break;
				}
			}
		}
		//Shadow map pass
		renderShadowMap(shadowFbo, models);

		//First pass
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		for (Model *m : models){
			m->bind();
			glDrawElements(GL_TRIANGLES, m->elems(), GL_UNSIGNED_SHORT, 0);
		}
		util::logGLError("post first pass");

		//Second pass
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		quad.bind();
		glDrawElements(GL_TRIANGLES, quad.elems(), GL_UNSIGNED_SHORT, 0);

		util::logGLError("post second pass");

		//Draw debug texture
		glDisable(GL_DEPTH_TEST);
		//Unset the compare mode so that we can draw it properly
		glActiveTexture(GL_TEXTURE3);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
		dbgOut.bind();
		glDrawElements(GL_TRIANGLES, dbgOut.elems(), GL_UNSIGNED_SHORT, 0);
		//Set it back to the shadow map compare mode
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
		glEnable(GL_DEPTH_TEST);

		SDL_GL_SwapWindow(win);
		int end = SDL_GetTicks();
		//Keep a smoothed average of the time per frame
		frameTime = 0.9 * (end - start) / 1000.f + 0.1 * frameTime;
		start = end;
		if (printFps){
			std::cout << "frame time: " << frameTime << "ms\n";
		}
	}
	glDeleteFramebuffers(1, &shadowFbo);
	glDeleteTextures(1, &shadowTex);

	glDeleteFramebuffers(1, &fbo);
	glDeleteTextures(3, texBuffers);
	
	SDL_GL_DeleteContext(context);
	SDL_DestroyWindow(win);

	return 0;
}
std::vector<Model*> setupModels(const glm::mat4 &view, const glm::mat4 &proj){
	std::vector<Model*> models;
	GLint progStatus = util::loadProgram("res/vshader.glsl", "res/fshader.glsl");
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
	//Pass the view/projection matrices
	GLint projUnif = glGetUniformLocation(program, "proj");
	GLint viewUnif = glGetUniformLocation(program, "view");
	glUniformMatrix4fv(projUnif, 1, GL_FALSE, glm::value_ptr(proj));
	glUniformMatrix4fv(viewUnif, 1, GL_FALSE, glm::value_ptr(view));

	GLuint shadowProgram = util::loadProgram("res/vshadow.glsl", "res/fshadow.glsl");
	//With suzanne the self-shadowing is much easier to see
	Model *polyhedron = new Model("res/suzanne.obj", program, shadowProgram);
	polyhedron->translate(glm::vec3(1.f, 0.f, 1.f));
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
	
	projUnif = glGetUniformLocation(program, "proj");
	viewUnif = glGetUniformLocation(program, "view");
	glUniformMatrix4fv(projUnif, 1, GL_FALSE, glm::value_ptr(proj));
	glUniformMatrix4fv(viewUnif, 1, GL_FALSE, glm::value_ptr(view));

	shadowProgram = util::loadProgram("res/vshadow.glsl", "res/fshadow.glsl");
	Model *floor = new Model("res/quad.obj", program, shadowProgram);
	//Get it laying perpindicularish to the light direction and behind the camera some
	floor->scale(glm::vec3(3.f, 3.f, 1.f));
	floor->rotate(glm::rotate(20.f, 0.f, 1.f, 0.f));
	models.push_back(floor);

	return models;
}
void setupShadowMap(GLuint &fbo, GLuint &tex){
	glActiveTexture(GL_TEXTURE3);
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	//Will just use a shadow map equal to the window dimensions
	//Must use specific formats for depth_stencil attachment
	//Mesa refuses to accept this type/format combination, what is valid?
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F,
		WIN_WIDTH, WIN_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	//No mip maps
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	//Setup depth comparison mode
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE,	GL_COMPARE_REF_TO_TEXTURE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
	//Don't wrap edges
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, tex, 0);
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
void renderShadowMap(GLuint &fbo, const std::vector<Model*> &models){
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glClear(GL_DEPTH_BUFFER_BIT);
	//Polygon offset fill helps resolve depth-fighting
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(2.f, 4.f);
	for (Model *m : models){
		m->bindShadow();
		glDrawElements(GL_TRIANGLES, m->elems(), GL_UNSIGNED_SHORT, 0);
	}
	glDisable(GL_POLYGON_OFFSET_FILL);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

}

