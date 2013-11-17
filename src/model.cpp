#include <iostream>
#include <string>
#include <GL/glew.h>
#include "util.h"
#include "model.h"

Model::Model(const std::string &file, GLuint program)
	: vao(0), nElems(0), program(program)
{
	load(file);
}
Model::~Model(){
	glDeleteBuffers(2, buf);
	glDeleteVertexArrays(1, &vao);
	glDeleteProgram(program);
}
void Model::bind(){
	glUseProgram(program);
	glBindVertexArray(vao);
}
size_t Model::elems(){
	return nElems;
}
void Model::load(const std::string &file){
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	glGenBuffers(2, buf);
	if (!util::loadOBJ(file, buf[0], buf[1], nElems)){
		std::cout << "Failed to load model: " << file << "\n";
		return;
	}
	//Position
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(GLfloat), 0);
	//Normals
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(GL_FLOAT),
		(void*)(3 * sizeof(GL_FLOAT)));
	//UVs
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 9 * sizeof(GL_FLOAT),
		(void*)(6 * sizeof(GL_FLOAT)));
}

