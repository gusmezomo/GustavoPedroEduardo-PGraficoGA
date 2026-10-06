// fragment shader cor do objeto, textura e luz global.
#version 330 core
in vec2 TexCoord;

uniform vec4 cor;             // cor do objeto (também tinge a textura)
uniform bool usaTextura;      // true = usa a textura, false = só a cor
uniform sampler2D texture1;   // textura ligada na unidade GL_TEXTURE0
uniform vec2 escalaUV;        // quantas vezes a textura repete
uniform float luz;            // brilho global (1.0 = dia, menor = noite)

out vec4 frag_colour;

void main() {
    vec4 cor_final;
    if (usaTextura) {
        cor_final = texture(texture1, TexCoord * escalaUV) * cor; // UV * escalaUV repete a imagem
    } else {
        cor_final = cor;
    }
    frag_colour = cor_final * vec4(vec3(luz), 1.0); // escurece RGB, mantém alfa
}
