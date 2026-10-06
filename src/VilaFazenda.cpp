// Vila Virtual - Fazenda

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <cmath> // sin() e cos()
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// janela e programa de shader
GLFWwindow* Window = nullptr;
GLuint Shader_programm = 0;

// agrupa uma geometria já enviada para a GPU
// no lugar de cada VAO ser uma variável solta como nas aulas
struct Geometria {
    GLuint vao;
    int nVertices;
};
// Uma Geometria para cada forma base, reaproveitada por vários objetos
Geometria Cubo;
Geometria Piramide;
Geometria Cilindro;
Geometria Cone;
Geometria Vaca;   // vaca.obj
Geometria Trator; // trator.obj

// PI, usado no cálculo do cilindro e animações
const float PI = 3.14159265f;

// IDs das texturas.
GLuint TexGrama = 0;
GLuint TexMadeira = 0;
GLuint TexPaleta = 0; // paleta_fazenda.png: 16 quadrados de cor usados por vaca e trator

// tamanho da janela e tempo do último frame (delta time)
int WIDTH = 1280;
int HEIGHT = 720;

float Tempo_entre_frames = 0.0f;

// estado da camera livre (posição, direção) e do mouse
// veio de Aula4Exerc2
float Cam_speed = 5.0f;
glm::vec3 Cam_pos = glm::vec3(0.0f, 3.0f, 18.0f); // começa afastado, olhando (-Z) para a fazenda
glm::vec3 Cam_front = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 Cam_up = glm::vec3(0.0f, 1.0f, 0.0f);

float Cam_yaw = 0.0f; // yaw = 0 já olha para -Z
float Cam_pitch = 0.0f;

double lastX = WIDTH / 2.0;
double lastY = HEIGHT / 2.0;
bool primeiro_mouse = true;

// dia e noite. 
//N_estavaApertada evita trocar várias vezes ao segurar a tecla
bool Noite = false; // false = dia, true = noite
bool N_estavaApertada = false; // estado da tecla N no frame anterior

// campo de visao da camera, diminui para dar zoom. de Aula4Exerc2
float Cam_fov = 67.0f;

// le um arquivo de shader e devolve o texto
// de Aula2Exerc3_4
std::string leShaderDoArquivo(const char* caminhoArquivo) {
    std::ifstream arquivoShader(caminhoArquivo);
    if (!arquivoShader.is_open()) {
        std::cerr << "ERRO: Nao foi possivel abrir o arquivo do shader: " << caminhoArquivo << std::endl;
        return "";
    }
    std::stringstream shaderStream;
    shaderStream << arquivoShader.rdbuf();
    arquivoShader.close();
    return shaderStream.str();
}

// carrega a textura pra GPU.
// de Aula5Ex8.3. agora devolve o ID da textura (antes gravava
// na global texture1), para podermos ter várias texturas
GLuint carregaTextura(std::string filePATH) {
    // inverte a imagem da textura, o OPENGL le de baixo pra cima
    stbi_set_flip_vertically_on_load(true);

    GLuint textura = 0;
    int width, height, nrChannels;
    unsigned char* data = stbi_load(filePATH.c_str(), &width, &height, &nrChannels, 0);

    if (data) {
        // jpg = 3 canais (RGB), png com transparência = 4 (RGBA)
        GLenum format = GL_RGB;
        if (nrChannels == 1) format = GL_RED;
        else if (nrChannels == 3) format = GL_RGB;
        else if (nrChannels == 4) format = GL_RGBA;

        // cria o ID e liga a textura para configurar
        glGenTextures(1, &textura);
        glBindTexture(GL_TEXTURE_2D, textura);

        // GL_REPEAT: UV fora de 0..1 repete a imagem; filtros com mipmap
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // envia os pixels para a GPU e gera os mipmaps (versões menores)
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    } else {
        std::cerr << "Falha ao ler textura: " << filePATH << std::endl;
    }

    // libera a cópia da RAM, a GPU já guardou a imagem
    stbi_image_free(data);
    return textura;
}

// guarda o novo tamanho da janela quando ela é redimensionada
// igual a  Aula4Exerc2
void redimensionaCallback(GLFWwindow* window, int w, int h) {
    WIDTH = w;
    HEIGHT = h;
}

// Gira a câmera com o movimento do mouse (yaw e pitch).
// igual a  Aula4Exerc2
void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (primeiro_mouse) {
        lastX = xpos;
        lastY = ypos;
        primeiro_mouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // invertido: y da tela cresce para baixo

    lastX = xpos;
    lastY = ypos;

    float sensibilidade = 0.1f;
    xoffset *= sensibilidade;
    yoffset *= sensibilidade;

    Cam_yaw -= xoffset;
    Cam_pitch += yoffset;

    // limita o pitch para a câmera não virar de cabeça para baixo
    if (Cam_pitch > 89.0f) Cam_pitch = 89.0f;
    if (Cam_pitch < -89.0f) Cam_pitch = -89.0f;
}

// Cria a janela, os callbacks e carrega o OpenGL.
// usado da Aula4/Exerc2.
void inicializaOpenGL() {
    if (!glfwInit()) exit(EXIT_FAILURE);

    Window = glfwCreateWindow(WIDTH, HEIGHT, "Vila Virtual - Fazenda", NULL, NULL);
    if (!Window) {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwSetWindowSizeCallback(Window, redimensionaCallback);
    glfwSetCursorPosCallback(Window, mouse_callback);
    glfwSetInputMode(Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // esconde e prende o mouse
    glfwMakeContextCurrent(Window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) exit(EXIT_FAILURE);

    //print comandos no terminal
    std::cout << "controles:" << std::endl;
    std::cout << "  W A S D - mover" << std::endl;
    std::cout << "  Q / E   - desce / sobe" << std::endl;
    std::cout << "  Z       - zoom" << std::endl;
    std::cout << "  N       - dia e noite" << std::endl;
}

// Cria a pirâmide: 18 vértices = 6 triângulos (2 da base + 4 laterais)
// feito com base no cubo da Ex8.2
void inicializaPiramide() {
    glGenVertexArrays(1, &Piramide.vao);
    glBindVertexArray(Piramide.vao);

    // A(-0.5,-0.5,-0.5) B(0.5,-0.5,-0.5) C(0.5,-0.5,0.5) D(-0.5,-0.5,0.5) T = topo (0,0.5,0)
    // UV vista de cima: u = x + 0.5, v = z + 0.5 (o topo fica em 0.5, 0.5)
    float vertices[] = {
        // POSIÇÕES             // TEXTURAS (U, V)
        // base (2 triângulos)
        -0.5f, -0.5f, -0.5f,    0.0f, 0.0f, // A
         0.5f, -0.5f, -0.5f,    1.0f, 0.0f, // B
         0.5f, -0.5f,  0.5f,    1.0f, 1.0f, // C
        -0.5f, -0.5f, -0.5f,    0.0f, 0.0f, // A
         0.5f, -0.5f,  0.5f,    1.0f, 1.0f, // C
        -0.5f, -0.5f,  0.5f,    0.0f, 1.0f, // D

        // lado de trás (A, B, topo)
        -0.5f, -0.5f, -0.5f,    0.0f, 0.0f, // A
         0.5f, -0.5f, -0.5f,    1.0f, 0.0f, // B
         0.0f,  0.5f,  0.0f,    0.5f, 0.5f, // T

        // lado direito (B, C, topo)
         0.5f, -0.5f, -0.5f,    1.0f, 0.0f, // B
         0.5f, -0.5f,  0.5f,    1.0f, 1.0f, // C
         0.0f,  0.5f,  0.0f,    0.5f, 0.5f, // T

        // lado da frente (C, D, topo)
         0.5f, -0.5f,  0.5f,    1.0f, 1.0f, // C
        -0.5f, -0.5f,  0.5f,    0.0f, 1.0f, // D
         0.0f,  0.5f,  0.0f,    0.5f, 0.5f, // T

        // lado esquerdo (D, A, topo)
        -0.5f, -0.5f,  0.5f,    0.0f, 1.0f, // D
        -0.5f, -0.5f, -0.5f,    0.0f, 0.0f, // A
         0.0f,  0.5f,  0.0f,    0.5f, 0.5f  // T
    };

    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // atributo 0 = posição, atributo 1 = UV (stride de 5 floats, igual ao cubo)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    Piramide.nVertices = 18;
}

// Coloca um vértice (posição + uv = 5 floats) no vetor
// feito para o gerador do cilindro não repetir 5 push_back por vértice
void adicionaVertice(std::vector<float>& v, float x, float y, float z, float u, float t) {
    v.push_back(x);
    v.push_back(y);
    v.push_back(z);
    v.push_back(u);
    v.push_back(t);
}

// Gera cilindro (ou cone, se raioTopo = 0) com um laço de cos/sin; base com raio 0.5, altura -0.5 a 0.5
// feito com base ExerAula3. Alterado: faz lateral e tampas
Geometria geraCilindro(int lados, float raioTopo) {
    std::vector<float> vertices;
    float raioBase = 0.5f;

    for (int i = 0; i < lados; i++) {
        // angulos do início e do fim da fatia i (coordenadas polares)
        float theta0 = 2.0f * PI * i / lados;
        float theta1 = 2.0f * PI * (i + 1) / lados;

        float x0 = cos(theta0), z0 = sin(theta0);
        float x1 = cos(theta1), z1 = sin(theta1);

        // UV da lateral: u de 0 a 1 em volta; v = 0 embaixo e 1 em cima
        float u0 = (float)i / lados;
        float u1 = (float)(i + 1) / lados;

        // lateral: 2 triângulos formando um retângulo entre a base e o topo
        adicionaVertice(vertices, raioBase * x0, -0.5f, raioBase * z0, u0, 0.0f);
        adicionaVertice(vertices, raioBase * x1, -0.5f, raioBase * z1, u1, 0.0f);
        adicionaVertice(vertices, raioTopo * x1,  0.5f, raioTopo * z1, u1, 1.0f);

        adicionaVertice(vertices, raioBase * x0, -0.5f, raioBase * z0, u0, 0.0f);
        adicionaVertice(vertices, raioTopo * x1,  0.5f, raioTopo * z1, u1, 1.0f);
        adicionaVertice(vertices, raioTopo * x0,  0.5f, raioTopo * z0, u0, 1.0f);

        // tampa de baixo: triângulo (centro, theta1, theta0); UV = x * 0.5 + 0.5 cabe o círculo em 0..1
        adicionaVertice(vertices, 0.0f, -0.5f, 0.0f, 0.5f, 0.5f);
        adicionaVertice(vertices, raioBase * x1, -0.5f, raioBase * z1, x1 * 0.5f + 0.5f, z1 * 0.5f + 0.5f);
        adicionaVertice(vertices, raioBase * x0, -0.5f, raioBase * z0, x0 * 0.5f + 0.5f, z0 * 0.5f + 0.5f);

        // tampa de cima: só se o topo tem raio (no cone é um ponto)
        if (raioTopo > 0.0f) {
            adicionaVertice(vertices, 0.0f, 0.5f, 0.0f, 0.5f, 0.5f);
            adicionaVertice(vertices, raioTopo * x0, 0.5f, raioTopo * z0, x0 * 0.5f + 0.5f, z0 * 0.5f + 0.5f);
            adicionaVertice(vertices, raioTopo * x1, 0.5f, raioTopo * z1, x1 * 0.5f + 0.5f, z1 * 0.5f + 0.5f);
        }
    }

    Geometria g;
    glGenVertexArrays(1, &g.vao);
    glBindVertexArray(g.vao);

    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    // tamanho em bytes = nº de floats * sizeof(float) (sizeof do vector não serve)
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    // mesmo layout do cubo: posição (0) e UV (1), stride de 5 floats
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    g.nVertices = vertices.size() / 5; // 5 floats por vértice
    return g;
}

// Lê um .obj (faces triangulares v/vt/vn) e devolve a Geometria pronta (VBO de 8 floats por vértice)
// base da Aula5/Ex8.3 mas devolve uma Geometria (antes um int)
Geometria loadSimpleOBJ(std::string filePATH) {
    Geometria g;
    g.vao = 0;
    g.nVertices = 0;

    // listas lidas do arquivo, cada uma indexada separadamente pelas faces
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec2> texCoords;
    std::vector<glm::vec3> normals;

    // buffer final para a GPU: pos(3) + uv(2) + normal(3) = 8 floats por vértice
    std::vector<GLfloat> vBuffer;

    std::ifstream arqEntrada(filePATH.c_str());
    if (!arqEntrada.is_open()) {
        std::cerr << "Erro ao tentar ler o arquivo " << filePATH << std::endl;
        return g;
    }

    std::string line;
    while (std::getline(arqEntrada, line)) {
        std::istringstream ssline(line);
        std::string word;
        ssline >> word;

        if (word == "v") {            // posição
            glm::vec3 vertice;
            ssline >> vertice.x >> vertice.y >> vertice.z;
            vertices.push_back(vertice);
        } else if (word == "vt") {    // coordenada de textura
            glm::vec2 vt;
            ssline >> vt.s >> vt.t;
            texCoords.push_back(vt);
        } else if (word == "vn") {    // normal (fica no VBO, mas não é usada no shader)
            glm::vec3 normal;
            ssline >> normal.x >> normal.y >> normal.z;
            normals.push_back(normal);
        } else if (word == "f") {     // face: 3 grupos "v/vt/vn"
            while (ssline >> word) {
                int vi = 0, ti = 0, ni = 0;
                std::istringstream ss(word);
                std::string index;

                // os índices do .obj começam em 1, por isso o "- 1"
                if (std::getline(ss, index, '/'))
                    vi = !index.empty() ? std::stoi(index) - 1 : 0;
                if (std::getline(ss, index, '/'))
                    ti = !index.empty() ? std::stoi(index) - 1 : 0;
                if (std::getline(ss, index))
                    ni = !index.empty() ? std::stoi(index) - 1 : 0;

                vBuffer.push_back(vertices[vi].x);
                vBuffer.push_back(vertices[vi].y);
                vBuffer.push_back(vertices[vi].z);

                // sem "vt" no arquivo, usa UV (0,0) em vez de acessar vetor vazio
                if (texCoords.empty()) {
                    vBuffer.push_back(0.0f);
                    vBuffer.push_back(0.0f);
                } else {
                    vBuffer.push_back(texCoords[ti].s);
                    vBuffer.push_back(texCoords[ti].t);
                }

                vBuffer.push_back(normals[ni].x);
                vBuffer.push_back(normals[ni].y);
                vBuffer.push_back(normals[ni].z);
            }
        }
    }
    arqEntrada.close();

    GLuint VBO, VAO;

    glGenBuffers(1, &VBO); // cria o VBO
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vBuffer.size() * sizeof(GLfloat), vBuffer.data(), GL_STATIC_DRAW); // envia os dados à GPU

    glGenVertexArrays(1, &VAO); // cria o VAO
    glBindVertexArray(VAO);

    int stride = 8 * sizeof(GLfloat); // 1 vértice = 8 floats

    // atributo 0: posição, offset 0
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (GLvoid*)0);
    glEnableVertexAttribArray(0);
    // atributo 1: UV, offset de 3 floats
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);
    // atributo 2: normal, offset de 5 floats (não usada no shader)
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (GLvoid*)(5 * sizeof(GLfloat)));
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    g.vao = VAO;
    g.nVertices = (int)(vBuffer.size() / 8); // 8 floats por vértice
    return g;
}

// Envia o cubo para a GPU, gera as outras formas e carrega texturas e modelos
// cubo da Aula5/Ex8.2. e acrescenta pirâmide, cilindro, cone e os .obj.
void inicializaObjetos() {
    // cubo: um VBO intercalando posição (3 floats) e UV (2 floats)
    glGenVertexArrays(1, &Cubo.vao);
    glBindVertexArray(Cubo.vao);

    float vertices[] = {
        // POSIÇÕES             // TEXTURAS (U, V)
        // face frontal
         0.5f,  0.5f,  0.5f,    1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,    1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,    0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,    0.0f, 1.0f,
         0.5f,  0.5f,  0.5f,    1.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,    0.0f, 0.0f,

        // face traseira
         0.5f,  0.5f, -0.5f,    1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,    1.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,    0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,    0.0f, 1.0f,
         0.5f,  0.5f, -0.5f,    1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,    0.0f, 0.0f,

        // face esquerda
        -0.5f, -0.5f,  0.5f,    1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,    1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,    0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,    0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,    0.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,    1.0f, 1.0f,

        // face direita
         0.5f, -0.5f,  0.5f,    0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,    0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,    1.0f, 0.0f,
         0.5f, -0.5f, -0.5f,    1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,    1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,    0.0f, 1.0f,

        // face baixo
        -0.5f, -0.5f,  0.5f,    0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,    1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,    1.0f, 0.0f,
         0.5f, -0.5f, -0.5f,    1.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,    0.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,    0.0f, 1.0f,

        // face cima
        -0.5f,  0.5f,  0.5f,    0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,    1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,    1.0f, 1.0f,
         0.5f,  0.5f, -0.5f,    1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,    0.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,    0.0f, 0.0f
    };

    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // atributo 0: posição, stride de 5 floats, offset 0
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // atributo 1: UV, offset pula os 3 floats da posição
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    Cubo.nVertices = 36;

    inicializaPiramide();
    Cilindro = geraCilindro(32, 0.5f); // topo com o mesmo raio da base
    Cone = geraCilindro(32, 0.0f);     // topo com raio zero = ponta

    TexGrama = carregaTextura("../assets/tex/grama.jpg");
    TexMadeira = carregaTextura("../assets/tex/madeira.jpg");

    TexPaleta = carregaTextura("../assets/Modelos3D/paleta_fazenda.png"); // paleta de cores da vaca e do trator

    Vaca = loadSimpleOBJ("../assets/Modelos3D/vaca.obj");
    Trator = loadSimpleOBJ("../assets/Modelos3D/trator.obj");
}

// Lê, compila os dois shaders.
// usado de Aula4/Exerc2.
void inicializaShaders() {
    std::string vertexCode = leShaderDoArquivo("../assets/shaders/vertex_shader.glsl");
    std::string fragmentCode = leShaderDoArquivo("../assets/shaders/fragment_shader.glsl");

    const char* vertex_shader = vertexCode.c_str();
    const char* fragment_shader = fragmentCode.c_str();

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vertex_shader, NULL);
    glCompileShader(vs);

    // mostra o erro no terminal se o shader não compilar
    GLint success;
    char infoLog[512];
    glGetShaderiv(vs, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vs, 512, NULL, infoLog);
        std::cerr << "Erro no vertex shader:\n" << infoLog << std::endl;
    }

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fragment_shader, NULL);
    glCompileShader(fs);

    glGetShaderiv(fs, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fs, 512, NULL, infoLog);
        std::cerr << "Erro no fragment shader:\n" << infoLog << std::endl;
    }

    Shader_programm = glCreateProgram();
    glAttachShader(Shader_programm, vs);
    glAttachShader(Shader_programm, fs);
    glLinkProgram(Shader_programm);

    glGetProgramiv(Shader_programm, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(Shader_programm, 512, NULL, infoLog);
        std::cerr << "Erro na linkagem do shader:\n" << infoLog << std::endl;
    }

    glDeleteShader(vs); // já estão dentro do programa
    glDeleteShader(fs);
}

// calcula a direção para onde a câmera olha a partir de yaw e pitch
// igual a Aula4/Exerc2
void atualizaDirecaoCamera() {
    glm::vec3 front;
    front.x = sin(glm::radians(-Cam_yaw)) * cos(glm::radians(Cam_pitch));
    front.y = sin(glm::radians(Cam_pitch));
    front.z = -cos(glm::radians(-Cam_yaw)) * cos(glm::radians(Cam_pitch));
    Cam_front = glm::normalize(front);
}

// le o teclado
// base usada Aula4/Exerc2
void trataTeclado() {
    if (glfwGetKey(Window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(Window, true);
    }

    // enquanto Z está apertada, o FOV diminui (aproxima)
    if (glfwGetKey(Window, GLFW_KEY_Z) == GLFW_PRESS) {
        Cam_fov = 20.0f;
    } else {
        Cam_fov = 67.0f;
    }

    // só troca quando a tecla passa de solta para apertada, senão piscaria a cada frame
    bool N_apertada = (glfwGetKey(Window, GLFW_KEY_N) == GLFW_PRESS);
    if (N_apertada && !N_estavaApertada) Noite = !Noite;
    N_estavaApertada = N_apertada; // guarda para o próximo frame

    // vetor para a direita da câmera (produto vetorial); distância = velocidade * tempo do frame
    glm::vec3 Cam_right = glm::normalize(glm::cross(Cam_front, Cam_up));

    if (glfwGetKey(Window, GLFW_KEY_A) == GLFW_PRESS)
        Cam_pos -= Cam_right * Cam_speed * Tempo_entre_frames;
    if (glfwGetKey(Window, GLFW_KEY_D) == GLFW_PRESS)
        Cam_pos += Cam_right * Cam_speed * Tempo_entre_frames;
    if (glfwGetKey(Window, GLFW_KEY_W) == GLFW_PRESS)
        Cam_pos += Cam_front * Cam_speed * Tempo_entre_frames;
    if (glfwGetKey(Window, GLFW_KEY_S) == GLFW_PRESS)
        Cam_pos -= Cam_front * Cam_speed * Tempo_entre_frames;
    if (glfwGetKey(Window, GLFW_KEY_E) == GLFW_PRESS)
        Cam_pos.y += Cam_speed * Tempo_entre_frames;
    if (glfwGetKey(Window, GLFW_KEY_Q) == GLFW_PRESS)
        Cam_pos.y -= Cam_speed * Tempo_entre_frames;
}

// Desenha uma geometria com a matriz model, a cor e a textura.
// usa o uniform de cor da ExerAula3 e a textura da Ex8.2.
void desenhaObjeto(Geometria g, glm::mat4 model, glm::vec4 cor,
                   GLuint textura = 0, glm::vec2 escalaUV = glm::vec2(1.0f)) {
    GLint modelLoc = glGetUniformLocation(Shader_programm, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    GLint corLoc = glGetUniformLocation(Shader_programm, "cor");
    glUniform4fv(corLoc, 1, glm::value_ptr(cor));

    // com textura: liga na unidade 0 e avisa o shader; sem textura: só a cor
    GLint usaTexLoc = glGetUniformLocation(Shader_programm, "usaTextura");
    if (textura != 0) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textura);
        glUniform1i(usaTexLoc, 1);
        glUniform2fv(glGetUniformLocation(Shader_programm, "escalaUV"), 1, glm::value_ptr(escalaUV)); // quantas vezes a textura repete
    } else {
        glUniform1i(usaTexLoc, 0);
    }

    glBindVertexArray(g.vao); // liga o VAO da geometria
    glDrawArrays(GL_TRIANGLES, 0, g.nVertices);
}

// As funções desenha(cada obeto) fazem a mesma coisa
// com a base da Aula4/Exerc2 (rotate + scale por objeto)

// Casa: paredes (cubo 4x3x4), telhado (pirâmide 5x2x5) e porta (cubo 0.9x1.8x0.1)
void desenhaCasa(glm::vec3 posicao, float rotacaoY, float escala) {
    // ordem translate, rotate, scale. vale para todas as partes
    glm::mat4 base = glm::mat4(1.0f);
    base = glm::translate(base, posicao);
    base = glm::rotate(base, glm::radians(rotacaoY), glm::vec3(0.0f, 1.0f, 0.0f));
    base = glm::scale(base, glm::vec3(escala));

    glm::mat4 model;

    // paredes: altura 3 apoiada no chão => centro em y = 1.5
    model = glm::translate(base, glm::vec3(0.0f, 1.5f, 0.0f));
    model = glm::scale(model, glm::vec3(4.0f, 3.0f, 4.0f));
    desenhaObjeto(Cubo, model, glm::vec4(1.0f), TexMadeira);

    // telhado: base em y = 3, altura 2 => centro em y = 4; madeira tingida de vermelho pela cor
    model = glm::translate(base, glm::vec3(0.0f, 4.0f, 0.0f));
    model = glm::scale(model, glm::vec3(5.0f, 2.0f, 5.0f));
    desenhaObjeto(Piramide, model, glm::vec4(0.7f, 0.2f, 0.15f, 1.0f), TexMadeira);

    // porta: na face da frente (z = 2), um pouco para fora para não ficar dentro da parede
    model = glm::translate(base, glm::vec3(0.0f, 0.9f, 2.05f));
    model = glm::scale(model, glm::vec3(0.9f, 1.8f, 0.1f));
    desenhaObjeto(Cubo, model, glm::vec4(0.4f, 0.25f, 0.1f, 1.0f)); // marrom
}

// Celeiro: corpo (cubo 6x4x8), telhado de 2 placas inclinadas e portão
void desenhaCeleiro(glm::vec3 posicao, float rotacaoY, float escala) {
    glm::mat4 base = glm::mat4(1.0f);
    base = glm::translate(base, posicao);
    base = glm::rotate(base, glm::radians(rotacaoY), glm::vec3(0.0f, 1.0f, 0.0f));
    base = glm::scale(base, glm::vec3(escala));

    glm::mat4 model;

    // corpo: altura 4, centro em y = 2
    model = glm::translate(base, glm::vec3(0.0f, 2.0f, 0.0f));
    model = glm::scale(model, glm::vec3(6.0f, 4.0f, 8.0f));
    desenhaObjeto(Cubo, model, glm::vec4(0.8f, 0.25f, 0.2f, 1.0f), TexMadeira);

    // telhado: 2 placas inclinadas 35 graus em Z formando um "A" sobre o corpo (topo em y = 4)
    // placa da direita: desce para +X (rotação negativa)
    model = glm::translate(base, glm::vec3(1.5f, 5.1f, 0.0f));
    model = glm::rotate(model, glm::radians(-35.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::scale(model, glm::vec3(3.6f, 0.2f, 8.4f));
    desenhaObjeto(Cubo, model, glm::vec4(0.5f, 0.5f, 0.52f, 1.0f)); // cinza

    // placa da esquerda: desce para -X (rotação positiva)
    model = glm::translate(base, glm::vec3(-1.5f, 5.1f, 0.0f));
    model = glm::rotate(model, glm::radians(35.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::scale(model, glm::vec3(3.6f, 0.2f, 8.4f));
    desenhaObjeto(Cubo, model, glm::vec4(0.5f, 0.5f, 0.52f, 1.0f));

    // portão: na face da frente (z = 4)
    model = glm::translate(base, glm::vec3(0.0f, 1.5f, 4.05f));
    model = glm::scale(model, glm::vec3(2.5f, 3.0f, 0.1f));
    desenhaObjeto(Cubo, model, glm::vec4(0.95f, 0.95f, 0.95f, 1.0f)); // branco
}

// Silo: corpo (cilindro 3x7x3) e topo (cone 3.2x1.5x3.2).
void desenhaSilo(glm::vec3 posicao, float rotacaoY, float escala) {
    glm::mat4 base = glm::mat4(1.0f);
    base = glm::translate(base, posicao);
    base = glm::rotate(base, glm::radians(rotacaoY), glm::vec3(0.0f, 1.0f, 0.0f));
    base = glm::scale(base, glm::vec3(escala));

    glm::mat4 model;

    // corpo: altura 7, centro em y = 3.5
    model = glm::translate(base, glm::vec3(0.0f, 3.5f, 0.0f));
    model = glm::scale(model, glm::vec3(3.0f, 7.0f, 3.0f));
    desenhaObjeto(Cilindro, model, glm::vec4(0.65f, 0.68f, 0.72f, 1.0f)); // cinza metálico

    // topo: base em y = 7, altura 1.5, centro em y = 7.75
    model = glm::translate(base, glm::vec3(0.0f, 7.75f, 0.0f));
    model = glm::scale(model, glm::vec3(3.2f, 1.5f, 3.2f));
    desenhaObjeto(Cone, model, glm::vec4(0.5f, 0.53f, 0.57f, 1.0f));
}

// Árvore: tronco (cilindro 0.5x2x0.5) e copa (cone 2.5x3.5x2.5).
void desenhaArvore(glm::vec3 posicao, float rotacaoY, float escala) {
    glm::mat4 base = glm::mat4(1.0f);
    base = glm::translate(base, posicao);
    base = glm::rotate(base, glm::radians(rotacaoY), glm::vec3(0.0f, 1.0f, 0.0f));
    base = glm::scale(base, glm::vec3(escala));

    glm::mat4 model;

    // tronco: altura 2, centro em y = 1
    model = glm::translate(base, glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::scale(model, glm::vec3(0.5f, 2.0f, 0.5f));
    desenhaObjeto(Cilindro, model, glm::vec4(0.4f, 0.26f, 0.13f, 1.0f)); // marrom

    // copa: base em y = 2, altura 3.5, centro em y = 3.75
    model = glm::translate(base, glm::vec3(0.0f, 3.75f, 0.0f));
    model = glm::scale(model, glm::vec3(2.5f, 3.5f, 2.5f));
    desenhaObjeto(Cone, model, glm::vec4(0.1f, 0.35f, 0.15f, 1.0f)); // verde-escuro
}

// Segmento de cerca de 4 m (ao longo de X): 3 postes e 2 ripas, tudo de madeira.
void desenhaCerca(glm::vec3 posicao, float rotacaoY, float escala) {
    glm::mat4 base = glm::mat4(1.0f);
    base = glm::translate(base, posicao);
    base = glm::rotate(base, glm::radians(rotacaoY), glm::vec3(0.0f, 1.0f, 0.0f));
    base = glm::scale(base, glm::vec3(escala));

    glm::mat4 model;

    // 3 postes (altura 1.2, centro em y = 0.6), nas pontas e no meio
    float postesX[3] = { -1.9f, 0.0f, 1.9f };
    for (int i = 0; i < 3; i++) {
        model = glm::translate(base, glm::vec3(postesX[i], 0.6f, 0.0f));
        model = glm::scale(model, glm::vec3(0.2f, 1.2f, 0.2f));
        desenhaObjeto(Cubo, model, glm::vec4(1.0f), TexMadeira);
    }

    // 2 ripas na frente dos postes (z = 0.14), uma baixa e uma alta
    float ripasY[2] = { 0.4f, 0.9f };
    for (int i = 0; i < 2; i++) {
        model = glm::translate(base, glm::vec3(0.0f, ripasY[i], 0.14f));
        model = glm::scale(model, glm::vec3(4.0f, 0.15f, 0.08f));
        desenhaObjeto(Cubo, model, glm::vec4(1.0f), TexMadeira);
    }
}

// Fardo de feno: cilindro 1.2x1.4x1.2 deitado (girado 90 graus em Z, o eixo fica ao longo de X).
void desenhaFardo(glm::vec3 posicao, float rotacaoY, float escala) {
    glm::mat4 base = glm::mat4(1.0f);
    base = glm::translate(base, posicao);
    base = glm::rotate(base, glm::radians(rotacaoY), glm::vec3(0.0f, 1.0f, 0.0f));
    base = glm::scale(base, glm::vec3(escala));

    // deitado, o raio (0.6) vira a altura => centro em y = 0.6
    glm::mat4 model = glm::translate(base, glm::vec3(0.0f, 0.6f, 0.0f));
    model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::scale(model, glm::vec3(1.2f, 1.4f, 1.2f));
    desenhaObjeto(Cilindro, model, glm::vec4(0.85f, 0.7f, 0.3f, 1.0f)); // amarelo-palha
}

// Vaca (.obj): mesmo padrão das outras, mas sem base separada e sem o translate de h/2
// Cor branca para não tingir a paleta. veio do Ex8.3
void desenhaVaca(glm::vec3 posicao, float rotacaoY, float escala) {
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, posicao);
    model = glm::rotate(model, glm::radians(rotacaoY), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::scale(model, glm::vec3(escala));
    desenhaObjeto(Vaca, model, glm::vec4(1.0f), TexPaleta);
}

// Trator (.obj) andando em círculo no campo, uma volta a cada 15 s.
// feito baseado na órbita do desenhaAstro ExerAula3, mas em torno do eixo Y.
void desenhaTrator() {
    float tempo = (float)glfwGetTime();      // segundos desde o início (não depende dos frames)
    float velocidade = 2.0f * PI / 15.0f;    // rad/s: volta completa (2*PI) dividida por 15 s

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(-2.0f, 0.0f, 12.0f));                // centro do círculo (campo)
    model = glm::rotate(model, tempo * velocidade, glm::vec3(0.0f, 1.0f, 0.0f)); // ângulo da órbita
    model = glm::translate(model, glm::vec3(3.5f, 0.0f, 0.0f));                  // raio do círculo
    model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    desenhaObjeto(Trator, model, glm::vec4(1.0f), TexPaleta);
}

// Desenha todos os objetos da cena (chamada duas vezes por frame: visão principal e minimapa)
void desenhaCenario() {
    glm::mat4 model;

    // chão: cubo achatado 40 x 0.2 x 40, com o topo em y = 0
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, -0.1f, 0.0f));
    model = glm::scale(model, glm::vec3(40.0f, 0.2f, 40.0f));
    desenhaObjeto(Cubo, model, glm::vec4(1.0f), TexGrama, glm::vec2(20.0f, 20.0f)); // grama repetida 20x20

    // construções
    desenhaCasa(glm::vec3(-9.0f, 0.0f, -7.0f), 0.0f, 1.0f);
    desenhaCasa(glm::vec3(-14.0f, 0.0f, 1.0f), 25.0f, 1.0f); // girada 25 graus
    desenhaCeleiro(glm::vec3(8.0f, 0.0f, -6.0f), 0.0f, 1.0f);
    desenhaSilo(glm::vec3(13.0f, 0.0f, -8.0f), 0.0f, 1.0f);
    desenhaSilo(glm::vec3(13.0f, 0.0f, -4.0f), 0.0f, 0.8f); // menor (escala 0.8)

    // árvores com escalas e rotações variadas
    desenhaArvore(glm::vec3(-17.0f, 0.0f, -15.0f), 0.0f, 1.2f);
    desenhaArvore(glm::vec3(-17.0f, 0.0f, -3.0f), 60.0f, 0.9f);
    desenhaArvore(glm::vec3(-16.0f, 0.0f, 12.0f), 120.0f, 1.3f);
    desenhaArvore(glm::vec3(16.0f, 0.0f, 14.0f), 30.0f, 0.8f);
    desenhaArvore(glm::vec3(4.0f, 0.0f, 17.0f), 90.0f, 1.0f);
    desenhaArvore(glm::vec3(17.0f, 0.0f, -15.0f), 150.0f, 1.1f);

    // cercado 8x8 com centro em (8, 8): 8 segmentos de 4 m (2 por lado)
    // lados em z (z = 4 e z = 12): ao longo de X, rotação 0
    desenhaCerca(glm::vec3(6.0f, 0.0f, 4.0f), 0.0f, 1.0f);
    desenhaCerca(glm::vec3(10.0f, 0.0f, 4.0f), 0.0f, 1.0f);
    desenhaCerca(glm::vec3(6.0f, 0.0f, 12.0f), 0.0f, 1.0f);
    desenhaCerca(glm::vec3(10.0f, 0.0f, 12.0f), 0.0f, 1.0f);
    // lados em x (x = 4 e x = 12): giram 90 graus para ficar ao longo de Z
    desenhaCerca(glm::vec3(4.0f, 0.0f, 6.0f), 90.0f, 1.0f);
    desenhaCerca(glm::vec3(4.0f, 0.0f, 10.0f), 90.0f, 1.0f);
    desenhaCerca(glm::vec3(12.0f, 0.0f, 6.0f), 90.0f, 1.0f);
    desenhaCerca(glm::vec3(12.0f, 0.0f, 10.0f), 90.0f, 1.0f);

    // fardos de feno: variando só a rotação em Y
    desenhaFardo(glm::vec3(-9.0f, 0.0f, 8.0f), 0.0f, 1.0f);
    desenhaFardo(glm::vec3(-7.5f, 0.0f, 9.0f), 30.0f, 1.0f);
    desenhaFardo(glm::vec3(-10.0f, 0.0f, 10.5f), 90.0f, 1.0f);
    desenhaFardo(glm::vec3(-8.0f, 0.0f, 11.0f), 60.0f, 1.0f);

    // vacas dentro do cercado
    desenhaVaca(glm::vec3(7.0f, 0.0f, 7.0f), 30.0f, 1.0f);
    desenhaVaca(glm::vec3(10.0f, 0.0f, 9.5f), -60.0f, 1.0f);

    // trator: a posição vem do tempo, calculada dentro de desenhaTrator
    desenhaTrator();
}

// sol e lua: uma volta a cada 60 s.
// veio de um planeta em órbita da ExerAula3. agora muda de cor à noite.
void desenhaAstro() {
    float tempo = (float)glfwGetTime();      // segundos desde o início: a animação não depende dos frames
    float velocidade = 2.0f * PI / 60.0f;    // rad/s: volta completa (2*PI) dividida por 60 s

    // rotate antes de translate: o translate afasta o cubo da origem e o rotate gira esse ponto em volta dela
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::rotate(model, tempo * velocidade, glm::vec3(0.0f, 0.0f, 1.0f)); // angulo da órbita
    model = glm::translate(model, glm::vec3(30.0f, 0.0f, 0.0f));                 // raio da órbita
    model = glm::scale(model, glm::vec3(2.0f));                                  // cubo 2x2x2

    // o astro não escurece à noite: luz = 1.0 só para ele
    glUniform1f(glGetUniformLocation(Shader_programm, "luz"), 1.0f);

    // dia: sol amarelo; noite: lua branco-azulada
    glm::vec4 corAstro = Noite ? glm::vec4(0.85f, 0.9f, 1.0f, 1.0f) : glm::vec4(1.0f, 0.85f, 0.2f, 1.0f);
    desenhaObjeto(Cubo, model, corAstro);
}

// loop principal: tempo, dia/noite, camera, visão principal e minimapa.
// veio do loop da Aula4Exerc2 adicionado dia/noite
void inicializaRenderizacao() {
    double tempo_anterior = glfwGetTime();

    glEnable(GL_DEPTH_TEST); // objetos mais perto escondem os mais longe

    while (!glfwWindowShouldClose(Window)) {
        // delta time: tempo que o último frame levou
        double tempo_frame_atual = glfwGetTime();
        Tempo_entre_frames = (float)(tempo_frame_atual - tempo_anterior);
        tempo_anterior = tempo_frame_atual;

        // cor do céu e brilho da cena conforme dia/noite
        float luzCena;
        if (Noite) {
            glClearColor(0.02f, 0.03f, 0.10f, 1.0f); // azul-escuro
            luzCena = 0.35f;
        } else {
            glClearColor(0.53f, 0.81f, 0.92f, 1.0f); // azul claro
            luzCena = 1.0f;
        }
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(Shader_programm);

        // luz global: multiplica a cor final no fragment shader
        GLint luzLoc = glGetUniformLocation(Shader_programm, "luz");
        glUniform1f(luzLoc, luzCena);

        trataTeclado();
        atualizaDirecaoCamera();

        // visão principal: perspectiva em tela cheia
        glViewport(0, 0, WIDTH, HEIGHT);

        // view: câmera na posição do usuário, olhando para Cam_front
        glm::mat4 viewPerspectiva = glm::lookAt(Cam_pos, Cam_pos + Cam_front, Cam_up);
        GLint viewLoc = glGetUniformLocation(Shader_programm, "view");
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(viewPerspectiva));

        // proj: perspectiva com o FOV atual (muda com o zoom); aspecto = largura / altura
        float aspecto = (float)WIDTH / (float)HEIGHT;
        glm::mat4 projPerspectiva = glm::perspective(glm::radians(Cam_fov), aspecto, 0.1f, 100.0f);
        GLint projLoc = glGetUniformLocation(Shader_programm, "proj");
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projPerspectiva));

        desenhaCenario();
        desenhaAstro(); // só na visão principal: no minimapa ficaria entre a câmera e o chão

        // minimapa: a cena é desenhada de novo, de cima, num canto da janela

        // limpa só o depth (mantém a imagem principal) para o mapa não se misturar com ela
        glClear(GL_DEPTH_BUFFER_BIT);

        // desenhaAstro() deixou luz = 1.0: reenvia a luz da cena para o minimapa escurecer à noite
        glUniform1f(luzLoc, luzCena);

        // viewport quadrada de 200 px no canto superior direito
        int mapa_size = 200;
        glViewport(WIDTH - mapa_size, HEIGHT - mapa_size, mapa_size, mapa_size);

        // câmera do mapa a 30 de altura olhando para baixo; up = -Z porque o up normal seria paralelo à visão
        glm::vec3 mapa_pos = glm::vec3(0.0f, 30.0f, 0.0f);
        glm::vec3 mapa_front = glm::vec3(0.0f, -1.0f, 0.0f);
        glm::vec3 mapa_up = glm::vec3(0.0f, 0.0f, -1.0f);

        glm::mat4 viewOrtografica = glm::lookAt(mapa_pos, mapa_pos + mapa_front, mapa_up);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(viewOrtografica));

        // proj ortográfica (sem ponto de fuga): mostra de -22 a +22 em X e Z, cobrindo a fazenda de 40x40
        float limite = 22.0f;
        glm::mat4 projOrtografica = glm::ortho(-limite, limite, -limite, limite, 0.1f, 100.0f);
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projOrtografica));

        desenhaCenario();

        glfwPollEvents();
        glfwSwapBuffers(Window);
    }

    glfwTerminate();
}

// inicializa em ordem e entra no loop.
int main() {
    inicializaOpenGL();
    inicializaObjetos();
    inicializaShaders();
    inicializaRenderizacao();
    return 0;
}