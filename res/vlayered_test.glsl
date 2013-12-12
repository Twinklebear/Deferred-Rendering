#version 330

//A simple shader for rendering shadow maps

uniform mat4 model;

layout(location = 0) in vec3 position;

void main(void){
	gl_Position = model * vec4(position, 1.f);
}

