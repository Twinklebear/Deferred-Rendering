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
//Hardcoded view position and light location for now
const vec4 view_pos = vec4(5.f, 5.f, 5.f, 1.f);
const vec4 light_pos = vec4(0.f, 0.f, 0.f, 1.f);

flat in int fcolor_idx;
in vec4 world_pos;
in vec4 f_normal;

out vec4 color;

void main(void){
	vec4 v = normalize(view_pos - world_pos);
	vec4 l = normalize(light_pos - world_pos);
	vec4 h = normalize(l + v);
	
	float diff = max(0.f, dot(f_normal, l));
	float spec = max(0.f, dot(f_normal, h));
	if (diff == 0.f){
		spec = 0.f;
	}
	else {
		spec = pow(spec, 50.f);
	}
	vec3 scattered = vec3(0.1f, 0.1f, 0.1f) + diff;
	//White specular highlight color
	vec3 reflected = vec3(spec * 0.4f);
	color = colors[fcolor_idx];
	color.xyz = min(color.xyz * scattered + reflected, vec3(1.f));;
}

