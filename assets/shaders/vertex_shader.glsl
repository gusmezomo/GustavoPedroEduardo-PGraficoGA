// Vertex shader: posiciona cada vértice e repassa a UV.
#version 330 core
layout(location = 0) in vec3 vertex_posicao; // posição do vértice (atributo 0)
layout(location = 1) in vec2 vertex_textura;  // coordenada de textura (atributo 1)

uniform mat4 model, view, proj; // objeto -> mundo, mundo -> câmera, câmera -> tela
out vec2 TexCoord;

void main() {
    TexCoord = vertex_textura; // repassa a UV para o fragment shader
    gl_Position = proj * view * model * vec4(vertex_posicao, 1.0); // ordem: model primeiro, proj por último
}
