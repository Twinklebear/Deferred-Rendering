#ifndef MODEL_H
#define MODEL_H

#include <string>
#include <GL/glew.h>

/*
 * A simple very light abstraction of a 3d model, will
 * load a Wavefront OBJ model file and setup the VAO
 * assuming that the program inputs are 0,1,2: pos, normals, uv
 * must also be assigned a program to use for rendering, but other
 * program inputs must be set separately
 */
class Model {
	GLuint vao;
	//The vbo and ebo
	GLuint buf[2];
	size_t nElems;
	GLuint program;

public:
	/*
	 * Load the model from a file and give it a shader program to use
	 */
	Model(const std::string &file, GLuint program);
	/*
	 * Free the model's buffers and program
	 */
	~Model();
	/*
	 * Bind the model and its program for rendering
	 */
	void bind();
	/*
	 * Get the number of elements in the element buffer
	 */
	size_t elems();

private:
	/*
	 * Load the model from the file and setup the VAO
	 */
	void load(const std::string &file);
};

#endif

