#version 330 core

uniform samplerCube tex;

in vec3 f_uv;

out vec4 color;

void main(void){
	color = texture(tex, f_uv);
}

