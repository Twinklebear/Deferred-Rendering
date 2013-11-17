#version 330

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 uv;

out vec2 f_uv;
out vec2 ndc_pos;

void main(void){
	gl_Position = vec4(position, 1.f);
	f_uv = uv;
	ndc_pos = position.xy;
}

