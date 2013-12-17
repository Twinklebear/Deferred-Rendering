#version 330 core

//Just draw to NDC for now

layout(location = 0) in vec3 pos;
layout(location = 2) in vec3 uv;

out vec3 f_uv;

void main(void){
	gl_Position = vec4(pos, 1.f);
	f_uv = uv;
}

