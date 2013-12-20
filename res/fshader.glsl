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
//Cube face normals, indexed in gl order: pos_x, neg_x, pos_y, neg_y, pos_z, neg_z
const vec4 cube_normals[6] = vec4[](
	vec4(1.f, 0.f, 0.f, 0.f),
	vec4(-1.f, 0.f, 0.f, 0.f),
	vec4(0.f, 1.f, 0.f, 0.f),
	vec4(0.f, -1.f, 0.f, 0.f),
	vec4(0.f, 0.f, 1.f, 0.f),
	vec4(0.f, 0.f, -1.f, 0.f)
);
//Hardcoded light location for now
const vec4 light_pos = vec4(0.f, 0.f, 0.f, 1.f);

uniform samplerCubeShadow shadow_map;
uniform mat4 light_view[6];
uniform mat4 light_proj;
uniform vec4 view_pos;

flat in int fcolor_idx;
in vec4 world_pos;
in vec4 f_normal;

out vec4 color;

void main(void){
	vec4 v = normalize(view_pos - world_pos);
	vec4 l = normalize(light_pos - world_pos);
	vec4 h = normalize(l + v);

	//Is there a better way to figure out which face we're projected onto?
	//For now we just run through each view dir for the faces and see which one's normal
	//that l is closest too.
	//Maybe this should just be done in the vertex shader? What about tris overlapping multiple faces?
	float max_dot = -1.f;
	int face;
	for (int i = 0; i < 6; ++i){
		float a = dot(-l, cube_normals[i]);
		if (a > max_dot){
			max_dot = a;
			face = i;
		}
	}
	//Project our world pos into the shadow space for the face and scale it into
	//projection space. There are faster ways to do this bit
	vec4 shadow_pos = light_proj * light_view[face] * world_pos;
	shadow_pos /= shadow_pos.w;
	shadow_pos.z = (shadow_pos.z + 1.f) / 2.f;
	float f = texture(shadow_map, vec4(-l.xyz, shadow_pos.z));

	float diff = max(0.f, dot(f_normal, l));
	float spec = max(0.f, dot(f_normal, h));
	if (diff == 0.f){
		spec = 0.f;
	}
	else {
		spec = pow(spec, 50.f);
	}
	vec3 scattered = vec3(0.1f) + f * diff;
	//White specular highlight color
	vec3 reflected = f * vec3(spec * 0.4f);
	color = colors[face];
	color.xyz = min(color.xyz * scattered + reflected, vec3(1.f));
}

