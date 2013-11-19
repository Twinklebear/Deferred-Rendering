#version 330

//A simple forward rendering shader for when I want to
//output some debug textures to the screen, it's expected these
//are being drawn to the NDC, perhaps with some scaling/translation

uniform mat4 model;

layout(location = 0) in vec3 position;
layout(location = 2) in vec2 uv;

out vec2 f_uv;

void main(void){
	gl_Position = model * vec4(position, 1.f);
	f_uv = uv;
}

