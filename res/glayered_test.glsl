#version 330

/*
 * Geometry shader for generating geometry for rendering to each face of
 * a cube map for shadow mapping. Shader is invoked per face and emits the primitive
 * transformed with the faces view/projection matrix. Incoming geometry should be
 * in world space with the point light at the origin. The instance id is the face to write
 * to using the same indices that OpenGL uses (ie. 0 = pos x, 1 = neg x, etc)
 * Perhaps later do some culling and only emit triangles that are visible to that face?
 * I suppose to do this for GL 3.3 I'd have to do it w/o geometry shader instancing somehow.
 */

layout(triangles) in;
layout(triangle_strip, max_vertices = 6) out;

//The views for the layers and the perspective proj. matrix
uniform mat4 view;
uniform mat4 proj;

flat out int layer;

void main(void){
	for (int l = 0; l < 2; ++l){
		for (int i = 0; i < gl_in.length(); ++i){
			gl_Layer = l;
			layer = l;
			gl_Position = proj * view * gl_in[i].gl_Position;
			EmitVertex();
		}
		EndPrimitive();
	}
}

