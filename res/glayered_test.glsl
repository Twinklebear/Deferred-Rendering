#version 330 core

/*
 * Geometry shader for generating geometry for rendering to each face of
 * a cube map, emits the primitive transformed with the face's view/projection matrix.
 * Perhaps later do some culling and only emit triangles that are visible to that face?
 */

layout(triangles) in;
layout(triangle_strip, max_vertices = 18) out;

//The views for the layers and the perspective proj. matrix
uniform mat4 view[6];
uniform mat4 proj;

flat out int layer;

void main(void){
	for (int l = 0; l < 6; ++l){
		for (int i = 0; i < gl_in.length(); ++i){
			gl_Layer = l;
			layer = l;
			gl_Position = proj * view[l] * gl_in[i].gl_Position;
			EmitVertex();
		}
		EndPrimitive();
	}
}

