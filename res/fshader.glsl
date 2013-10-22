#version 330

in vec3 f_color;
in vec3 f_normal;
in vec2 f_uv;

layout(location = 0) out vec3 diffuse;
layout(location = 1) out vec3 normal;

void main(){
	diffuse = f_color;
	//Scale the normal values from [-1,1] range to [0,1] range
	normal = ((f_normal - vec3(-1.f, -1.f, -1.f) / 2.f));
}

