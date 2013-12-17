#version 330 core

//Selects the color for the fragment based on its layer.
const vec4 colors[6] = vec4[](
	vec4(1.f, 0.f, 0.f, 1.f),
	vec4(0.f, 1.f, 0.f, 1.f),
	vec4(0.f, 0.f, 1.f, 1.f),
	vec4(1.f, 1.f, 0.f, 1.f),
	vec4(1.f, 0.f, 1.f, 1.f),
	vec4(0.f, 1.f, 1.f, 1.f)
);

flat in int fcolor_idx;

out vec4 color;

void main(void){
	color = colors[fcolor_idx];
}

