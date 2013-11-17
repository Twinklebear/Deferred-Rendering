#version 330

uniform sampler2D tex_diffuse;

in vec4 f_normal;
in vec2 f_uv;

layout(location = 0) out vec4 diffuse;
layout(location = 1) out vec4 normal;

void main(void){
	diffuse = texture(tex_diffuse, f_uv);
	//Scale the normal values from [-1,1] range to [0,1] range
	normal = ((f_normal + 1.f) / 2.f);
	normal.w = 0.f;
}

