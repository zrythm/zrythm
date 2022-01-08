#version 150

// this shader simply passes the input vertices
// to the fragment shader.

in vec2 posIn;

void main() {
    gl_Position = vec4(posIn.x, posIn.y, 0, 1.0);
}
