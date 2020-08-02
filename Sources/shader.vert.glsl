#version 450

in vec3 pos;

uniform mat4 wvp;

void main() {
	gl_Position = wvp * vec4(pos, 1.0);
}
