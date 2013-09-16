#include <vector>
#include <array>
#include <map>
#include <iostream>
#include <fstream>
#include <string>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include "util.h"

std::string util::readFile(const std::string &fName){
	std::ifstream file(fName);
	if (!file.is_open()){
		std::cout << "Failed to open file: " << fName << std::endl;
		return "";
	}
	return std::string((std::istreambuf_iterator<char>(file)),
    	std::istreambuf_iterator<char>());
}
bool util::loadOBJ(const std::string &fName, GLuint &vbo, GLuint &ebo, size_t &nElems){
	std::ifstream file(fName);
	if (!file.is_open()){
		std::cout << "Failed to find obj file: " << fName << std::endl;
		return false;
	}

	//Temporary storage for the data we read in
	std::vector<glm::vec3> tmpPos, tmpNorm;
	std::vector<glm::vec2> tmpUv;
	//A map to associate a unique vertex with its index
	std::map<std::string, GLushort> vertexIndices;
	//The final ordered packed vertices and indices
	std::vector<glm::vec3> vertexData;
	std::vector<GLushort> indices;

	std::string line;
	while (std::getline(file, line)){
		if (line.empty()){
			continue;
		}
		//Parse vertex info: positions, uv coords and normals
		else if (line.at(0) == 'v'){
			//positions
			if (line.at(1) == ' '){
				tmpPos.push_back(captureVec3(line));
			}
			else if (line.at(1) == 't'){
				tmpUv.push_back(captureVec2(line));
			}
			else if (line.at(1) == 'n'){
				tmpNorm.push_back(captureVec3(line));
			}
		}
		//Parse faces
		else if (line.at(0) == 'f'){
			std::array<std::string, 3> face = captureFaces(line);
			for (std::string &v : face){
				auto fnd = vertexIndices.find(v);
				//If we find the vertex already in the list re-use the index
				//If not we create a new vertex and index
				if (fnd != vertexIndices.end()){
					indices.push_back(fnd->second);
				}
				else {
					std::array<unsigned int, 3> vertex = captureVertex(v);
					//Pack the position, normal and uv into the vertex data, note that obj data is
					//1-indexed so we subtract 1
					vertexData.push_back(tmpPos[vertex[0] - 1]);
					vertexData.push_back(tmpNorm[vertex[2] - 1]);
					vertexData.push_back(glm::vec3(tmpUv[vertex[1] - 1], 0));
					//Store the new index, also subract 1 b/c size 1 => idx 0 
					//and divide by 3 b/c there are 3 components per vertex
					indices.push_back((vertexData.size() - 1) / 3);
					vertexIndices[v] = indices.back();
				}
			}
		}
	}
	nElems = indices.size();
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(glm::vec3), &vertexData[0], GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLushort), &indices[0], GL_STATIC_DRAW);

	return true;
}
glm::vec2 util::captureVec2(const std::string &str){
	glm::vec2 vec;
	sscanf(str.c_str(), "%*s %f %f", &vec.x, &vec.y);
	return vec;
}
glm::vec3 util::captureVec3(const std::string &str){
	glm::vec3 vec;
	sscanf(str.c_str(), "%*s %f %f %f", &vec.x, &vec.y, &vec.z);
	return vec;
}
std::array<std::string, 3> util::captureFaces(const std::string &str){
	std::array<std::string, 3> faces;
	//There's face information between each space in the string, and 3 faces total
	size_t prev = str.find(" ", 0);
	size_t next = prev;
	for (std::string &face : faces){
		next = str.find(" ", prev + 1);
		face = str.substr(prev + 1, next - prev - 1);
		prev = next;
	}
	return faces;
}
std::array<unsigned int, 3> util::captureVertex(const std::string &str){
	std::array<unsigned int, 3> vertex;
	sscanf(str.c_str(), "%u/%u/%u", &vertex[0], &vertex[1], &vertex[2]);
	return vertex;
}
