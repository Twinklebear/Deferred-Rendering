#version 330

in vec3 f_color;
in vec4 f_normal;
in vec2 f_uv;

layout(location = 0) out vec4 diffuse;
layout(location = 1) out vec4 normal;

void main(){
	diffuse = vec4(f_color, 1.f);
	//Scale the normal values from [-1,1] range to [0,1] range
	normal = ((f_normal - vec4(-1.f, -1.f, -1.f, 0.f) / 2.f));
	normal.w = 1.f;
}

