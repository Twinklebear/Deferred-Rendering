#version 330

//Simple textured forward rendering shader for when I want to output
//some debug textures to screen. gDEBugger is great but I can't seem
//to interact with the program

uniform sampler2D tex;

in vec2 f_uv;

out vec4 color;

void main(void){
	color = texture(tex, f_uv);
}

