#version 330

//Selects the color for the fragment based on its layer.
const vec4 layer_colors[2] = vec4[](
	vec4(1.f, 0.f, 0.f, 1.f), vec4(0.f, 0.f, 1.f, 1.f)
);

flat in int layer;

out vec4 color;

void main(void){
	color = layer_colors[layer];
}

