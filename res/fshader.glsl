#version 330

in vec3 f_color;
in vec2 f_uv;

out vec4 color;

void main(){
	color = 0.6f * vec4(f_color, 1.f) + 0.4f * vec4(f_uv, 1.f, 1.f);
}
