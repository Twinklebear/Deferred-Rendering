#version 330 core

in vec4 f_normal;

out vec4 color;

void main(void){
	color = (f_normal + vec4(1)) / 2;
}

