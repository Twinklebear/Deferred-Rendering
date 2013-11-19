#ifndef MODEL_H
#define MODEL_H

#include <string>
#include <GL/glew.h>
#include <glm/glm.hpp>

/*
 * A simple very light abstraction of a 3d model, will
 * load a Wavefront OBJ model file and setup the VAO
 * assuming that the program inputs are 0,1,2: pos, normals, uv
 * must also be assigned a program to use for rendering, but other
 * program inputs must be set separately
 * TODO: not hack in shadow pass, perhaps look into sharing shaders better?
 */
class Model {
	GLuint vao;
	//The vbo and ebo
	GLuint buf[2];
	size_t nElems;
	//The regular and shadow pass shader programs
	//shadowProgram will be 0 if this model isn't given a shadow pass program
	GLuint program, shadowProgram;
	//We keep the matrices separate and compose only when sending to GPU
	//to not screw up order of operation
	GLint matUnif, shadowMatUnif;
	glm::mat4 translation, rotation, scaling;

public:
	/*
	 * Load the model from a file and give it a shader program to use
	 * The shader program should take position, normal and uv as inputs 0, 1, 2
	 * and have a uniform mat4 input for the model matrix if not doing instanced
	 * rendering
	 */
	Model(const std::string &file, GLuint program, GLuint shadowProgram = 0);
	/*
	 * Free the model's buffers and program
	 */
	~Model();
	/*
	 * Bind the model and its program for rendering
	 */
	void bind();
	/*
	 * Bind the model and its program for shadow map pass
	 */
	void bindShadow();
	/*
	 * Set the shadow pass view/projection matrix
	 * this is sorta hacked in
	 */
	void setShadowVP(const glm::mat4 &vp);
	/*
	 * Get the number of elements in the element buffer
	 */
	size_t elems();
	/*
	 * Apply some translation to the model
	 */
	void translate(const glm::vec3 &vec);
	/*
	 * Apply a rotation matrix to the model
	 */
	void rotate(const glm::mat4 &rot);
	/*
	 * Apply some scaling to the model
	 */
	void scale(const glm::vec3 &scale);

private:
	/*
	 * Load the model from the file and setup the VAO
	 */
	void load(const std::string &file);
	/*
	 * Update the uniform model matrix being sent to the shader
	 */
	void updateMatrix();
};

#endif

