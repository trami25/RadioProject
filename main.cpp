#define _USE_MATH_DEFINES
#include <cmath>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <GL/glew.h>
#include <GL/glut.h>
#include <GLFW/glfw3.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <direct.h>




unsigned int loadTexture(const char* filepath) {
    stbi_set_flip_vertically_on_load(true);


    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    char cwd[1024];
    _getcwd(cwd, sizeof(cwd));
    std::cout << "Working dir: " << cwd << "\n";


    int width, height, nrChannels;
    unsigned char* data = stbi_load(filepath, &width, &height, &nrChannels, 0);
    if (data) {
        GLenum format;
        if (nrChannels == 1)
            format = GL_RED;
        else if (nrChannels == 3)
            format = GL_RGB;
        else if (nrChannels == 4)
            format = GL_RGBA;
        else {
            std::cerr << "Unsupported number of channels: " << nrChannels << std::endl;
            stbi_image_free(data);
            return 0;
        }

        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        // Set texture parameters (important!)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    else {
        std::cerr << "Failed to load texture: " << filepath << std::endl;
    }

    stbi_image_free(data);
    return texture;
}


// Globalne promenljive za slajder
float sliderValue = 0.5f; // Početna vrednost (50%)
bool isDragging = false;
float sliderX = -0.4f; // Početni položaj slajdera
float sliderWidth = 0.8f; // Širina slajdera
float sliderY = -0.5f; // Y pozicija slajdera
float sliderHeight = 0.05f; // Visina slajdera

float sliderX2 = 0.3f;       // Trenutna X pozicija centra
float sliderSpeed2 = 0.5f;   // Brzina kretanja (u jedinicama po sekundi)
int direction2 = -1;         // -1 = levo, 1 = desno
float deltaTime = 0.0f; // vreme između dva frejma
float lastFrame = 0.0f; // vreme prethodnog frejma


bool isRadioOn = true; // Početno stanje: radio je uključen
float buttonX = -0.8f; // X pozicija dugmeta
float buttonY = -0.8f; // Y pozicija dugmeta
bool isAMMode = true; // Početno stanje: AM režim

// Dugme za uključivanje/isključivanje
float powerButtonX = -0.4f; // X koordinata (levo od centra)
float powerButtonY = 0.35f; // Y koordinata (iznad tela radija)

// Dugme za izbor režima AM/FM
float modeButtonX = 0.2f; // X koordinata (desno od centra)
float modeButtonY = 0.35f; // Y koordinata (iznad tela radija)
float buttonWidth = 0.2f;  // Širina dugmadi
float buttonHeight = 0.1f; // Visina dugmadi

// Parametri za kazaljku
float needleX = 0.0f; // Početna X pozicija kazaljke
float needleYStart = -0.2f; // Početna Y pozicija (dno kazaljke)
float needleYEnd = -0.1f;    // Krajnja Y pozicija (vrh kazaljke)
float needleWidth = 0.02f;  // Debljina kazaljke

// Parametri za skalu
float scaleStartX = -0.4f; // Početna X pozicija skale
float scaleEndX = 0.4f;    // Krajnja X pozicija skale
float scaleY = -0.25f;     // Y pozicija skale
int numDivisions = 20;     // Broj podela na skali


// Parametri za display
float displayStartX = -0.8f; // Početak displaya
float displayEndX = 0.8f;    // Kraj displaya
float displayY = 0.35f;      // Y pozicija displaya
float textSpeed = 0.001f;    // Brzina pomeranja teksta
float textPosition = displayEndX; // Početna pozicija teksta
std::string currentStation = ""; // Trenutna stanica

bool isAntennaExtended = false; // Početni status antene
float antennaHeight = 0.1f;     // Početna visina antene
float maxAntennaHeight = 0.5f;  // Maksimalna visina antene
float antennaX = 0.0f;          // X pozicija antene (centar radija)
float antennaWidth = 0.02f;     // Širina antene

struct Station {
    float rangeStart; // Početak opsega na skali
    float rangeEnd;   // Kraj opsega na skali
    std::string name; // Naziv stanice
};

std::vector<Station> amStations = {
    { -0.4f, -0.3f, "AM1" },
    { -0.2f, -0.1f, "AM22" },
    {  0.0f,  0.1f, "AM333" },
    {  0.2f,  0.3f, "AM4444" },
    {  0.3f,  0.4f, "AM55555" }
};

std::vector<Station> fmStations = {
    { -0.35f, -0.25f, "FMA" },
    { -0.15f, -0.05f, "FMBB" },
    {  0.05f,  0.15f, "FMCCC" },
    {  0.25f,  0.35f, "FMDDDD" }
};

// Funkcije za učitavanje i kompajliranje shader-a
unsigned int compileShader(GLenum type, const char* source);
unsigned int createShaderProgram(const char* vertexPath, const char* fragmentPath);
std::string readFile(const char* filePath);

// Uniform za vreme (animacija)
float timeUniform = 1.0f;

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(-1.0, 1.0, -1.0, 1.0); // Ortogonalna projekcija
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}




// Modifikacija funkcije `getActiveStation` da proveri antenu
std::string getActiveStation(float needleX, bool isAMMode) {
    if (!isAntennaExtended) {
        return ""; // Ako je antena sklopljena, nema stanica
    }

    const std::vector<Station>& stations = isAMMode ? amStations : fmStations;
    for (const auto& station : stations) {
        if (needleX >= station.rangeStart && needleX <= station.rangeEnd) {
            return station.name;
        }
    }

    return ""; // Nema aktivne stanice
}



void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    float step = 0.02f; // Korak pomeranja kazaljke
    needleX += yoffset * step;

    // Ograničenje kazaljke na ivice skale
    if (needleX < scaleStartX) needleX = scaleStartX;
    if (needleX > scaleEndX) needleX = scaleEndX;
}


void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    // Promena režima (AM/FM)
    if (key == GLFW_KEY_M && action == GLFW_PRESS) {
        isAMMode = !isAMMode;
      
    }

    // Izvlačenje antene
    if (key == GLFW_KEY_A && action == GLFW_PRESS) {
        antennaHeight += 0.05f; // Povećanje visine antene
        if (antennaHeight > maxAntennaHeight) {
            antennaHeight = maxAntennaHeight; // Ograničenje na maksimum
        }
        isAntennaExtended = true; // Antena je sada aktivna
       
    }

    // Sklapanje antene
    if (key == GLFW_KEY_Z && action == GLFW_PRESS) {
        antennaHeight -= 0.05f; // Smanjenje visine antene
        if (antennaHeight <= 0.0f) {
            antennaHeight = 0.0f;        // Ograničenje na minimum
            isAntennaExtended = false;  // Antena je sada neaktivna
        }
       
    }
}

// Funkcija za crtanje teksta
void drawText(float x, float y, const char* text, float r, float g, float b, float alpha) {
    std::cout << "Crtam tekst: '" << text << "' na poziciji (" << x << ", " << y << ")\n";

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix(); // Sačuvaj trenutnu matricu
    glLoadIdentity(); // Resetuj matricu na identitet

    glColor4f(r, g, b, alpha); // Postavi boju teksta
    glRasterPos2f(x, y); // Postavi početnu poziciju za crtanje

    // Proveri da li je pozicija u okviru NDC
    if (x < -1.0f || x > 1.0f || y < -1.0f || y > 1.0f) {
        std::cerr << "Pozicija teksta je van vidljive oblasti!\n";
    }

    for (int i = 0; text[i] != '\0'; i++) {
        glutBitmapCharacter(GLUT_BITMAP_8_BY_13, text[i]);
    }

    glPopMatrix(); // Vrati prethodnu matricu
}


void drawScrollingText(float x, float y, float width, float height, const std::string& text, float& offset) {
    if (text.empty()) return;

  

    // Animacija teksta
    offset -= 0.005f; // Pomeraj teksta ulevo
    if (offset + text.size() * 0.02f < -1.0f) {
        offset = 1.0f; // Resetuje poziciju teksta kada izađe iz oblasti
    }

    // Iscrtavanje teksta
    float textX = x + offset;
    drawText(textX, y + height / 2 - 0.02f, text.c_str(), 1.0f, 1.0f, 1.0f, 1.0f);
}



void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);

        int width, height;
        glfwGetWindowSize(window, &width, &height);

        // Transformacija u NDC
        float ndcX = ((float)xpos / width) * 2.0f - 1.0f;
        float ndcY = 1.0f - ((float)ypos / height) * 2.0f;

        // Provera da li je klik unutar dugmeta
        if (ndcX >= powerButtonX && ndcX <= powerButtonX + buttonWidth &&
            ndcY >= powerButtonY && ndcY <= powerButtonY + buttonHeight) {
            isRadioOn = !isRadioOn;
            std::cout << "Radio toggled: " << (isRadioOn ? "ON" : "OFF") << "\n";
            return;
        }


        // Provera da li je klik unutar slajdera
        if (ndcX >= sliderX && ndcX <= sliderX + sliderWidth &&
            ndcY >= sliderY && ndcY <= sliderY + sliderHeight) {
            isDragging = true;
            sliderValue = (ndcX - sliderX) / sliderWidth; // Proporcionalno postavljanje vrednosti
            sliderValue = fmax(0.0f, fmin(1.0f, sliderValue)); // Ograničenje vrednosti na [0, 1]
        }
    }

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
        isDragging = false;
    }
}

// Funkcija za crtanje antene
void drawAntenna(unsigned int shaderProgram) {
    float antennaVertices[] = {
        antennaX - antennaWidth / 2, 0.3f,                   // Donji levi ugao
        antennaX + antennaWidth / 2, 0.3f,                   // Donji desni ugao
        antennaX + antennaWidth / 2, 0.3f + antennaHeight,   // Gornji desni ugao
        antennaX - antennaWidth / 2, 0.3f + antennaHeight    // Gornji levi ugao
    };

    unsigned int VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(antennaVertices), antennaVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glUseProgram(shaderProgram);
    float color[3] = { 0.7f, 0.7f, 0.7f }; // Svetlo siva boja za antenu
    glUniform3f(glGetUniformLocation(shaderProgram, "color"), color[0], color[1], color[2]);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    glDeleteBuffers(1, &VBO);
    glDeleteVertexArrays(1, &VAO);
}


void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    if (isDragging) {
        int width, height;
        glfwGetWindowSize(window, &width, &height);

        // Transformacija u NDC
        float ndcX = ((float)xpos / width) * 2.0f - 1.0f;

        // Ažuriranje vrednosti slajdera
        sliderValue = (ndcX - sliderX) / sliderWidth;
        sliderValue = fmax(0.0f, fmin(1.0f, sliderValue)); // Ograničenje vrednosti na [0, 1]
        timeUniform = sliderValue; // Ažuriranje uniformne promenljive za vreme
        std::cout << "Slider Value Updated: " << sliderValue << "\n";
    }
}

int main() {
    int argc = 0;
    char** argv = nullptr;
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);

    if (!glfwInit()) {
        std::cerr << "GLFW nije mogao da se ucita! :(\n";
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);


    GLFWwindow* window = glfwCreateWindow(800, 800, "Radio - Telo i Zvučnik", NULL, NULL);
    if (!window) {
        std::cerr << "Prozor nije mogao da se napravi! :(\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(-1.0, 1.0, -1.0, 1.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetScrollCallback(window, scroll_callback);

    if (glewInit() != GLEW_OK) {
        std::cerr << "GLEW nije mogao da se ucita! :'(\n";
        return -1;
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


    float quadVertices[] = {
        // positions     // texCoords
         1.0f,  1.0f,     1.0f, 1.0f,  // top right
         0.6f,  1.0f,     0.0f, 1.0f,  // top left
         0.6f,  0.8f,     0.0f, 0.0f,  // bottom left

         1.0f,  1.0f,     1.0f, 1.0f,  // top right
         0.6f,  0.8f,     0.0f, 0.0f,  // bottom left
         1.0f,  0.8f,     1.0f, 0.0f   // bottom right
    };

    unsigned int quadVAO, quadVBO;
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);

    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // texture coord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);


    // === Verteksi za telo radija (pravougaonik) ===
    float bodyVertices[] = {
        -0.5f, -0.3f, // Donji levi ugao
         0.5f, -0.3f, // Donji desni ugao
         0.5f,  0.3f, // Gornji desni ugao
        -0.5f,  0.3f  // Gornji levi ugao
    };

    unsigned int bodyIndices[] = {
        0, 1, 2, // Prvi trougao
        2, 3, 0  // Drugi trougao
    };

    unsigned int bodyVAO, bodyVBO, bodyEBO;
    glGenVertexArrays(1, &bodyVAO);
    glGenBuffers(1, &bodyVBO);
    glGenBuffers(1, &bodyEBO);

    glBindVertexArray(bodyVAO);

    glBindBuffer(GL_ARRAY_BUFFER, bodyVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(bodyVertices), bodyVertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bodyEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(bodyIndices), bodyIndices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // === INDIKATOR ===

    float sliderWidth2 = 0.2f;
    float sliderHeight2 = 0.1f;
    float sliderY2 = 0.2f; // Visina u gornjem delu tela

    float sliderVertices2[] = {
        //  x, y,         u, v
        0.3f - sliderWidth2 / 2, sliderY2 - sliderHeight2 / 2, 0.0f, 0.0f, // bottom-left
        0.3f + sliderWidth2 / 2, sliderY2 - sliderHeight2 / 2, 1.0f, 0.0f, // bottom-right
        0.3f + sliderWidth2 / 2, sliderY2 + sliderHeight2 / 2, 1.0f, 1.0f, // top-right
        0.3f - sliderWidth2 / 2, sliderY2 + sliderHeight2 / 2, 0.0f, 1.0f  // top-left
    };


    unsigned int sliderIndices2[] = {
        0, 1, 2,
        2, 3, 0
    };

    unsigned int sliderVAO, sliderVBO, sliderEBO;
    glGenVertexArrays(1, &sliderVAO);
    glGenBuffers(1, &sliderVBO);
    glGenBuffers(1, &sliderEBO);

    glBindVertexArray(sliderVAO);

    glBindBuffer(GL_ARRAY_BUFFER, sliderVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(sliderVertices2), sliderVertices2, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sliderEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(sliderIndices2), sliderIndices2, GL_STATIC_DRAW);

    // Pozicija: lokacija 0, 2 floats (x, y)
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Tekstura: lokacija 1, 2 floats (u, v)
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);




    // === Verteksi za zvučnik (krug) ===
    const int circleSegments = 100;
    std::vector<float> circleVertices; // x, y, u, v
    circleVertices.push_back(0.0f); // Centar x
    circleVertices.push_back(0.0f); // Centar y
    circleVertices.push_back(0.5f); // Centar u
    circleVertices.push_back(0.5f); // Centar v

    for (int i = 0; i <= circleSegments; i++) {
        float angle = i * 2.0f * M_PI / circleSegments;
        float x = 0.2f * cos(angle);
        float y = 0.2f * sin(angle);
        float u = 0.5f + 0.5f * cos(angle); // [0,1] mapirano na teksturu
        float v = 0.5f + 0.5f * sin(angle);

        circleVertices.push_back(x);
        circleVertices.push_back(y);
        circleVertices.push_back(u);
        circleVertices.push_back(v);
    }

    unsigned int speakerVAO, speakerVBO;
    glGenVertexArrays(1, &speakerVAO);
    glGenBuffers(1, &speakerVBO);

    glBindVertexArray(speakerVAO);
    glBindBuffer(GL_ARRAY_BUFFER, speakerVBO);
    glBufferData(GL_ARRAY_BUFFER, circleVertices.size() * sizeof(float), circleVertices.data(), GL_STATIC_DRAW);

    // Lokacija 0: pozicija (x,y)
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Lokacija 1: teksturna koordinata (u,v)
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);


    // Verteksi za lampicu (krug)
    const int lampSegments = 50;
    std::vector<float> lampVertices = { 0.0f, 0.0f }; // Centar kruga
    for (int i = 0; i <= lampSegments; i++) {
        float angle = i * 2.0f * M_PI / lampSegments;
        lampVertices.push_back(0.05f * cos(angle)); // Poluprečnik lampice = 0.05
        lampVertices.push_back(0.05f * sin(angle));
    }

    unsigned int lampVAO, lampVBO;
    glGenVertexArrays(1, &lampVAO);
    glGenBuffers(1, &lampVBO);

    glBindVertexArray(lampVAO);
    glBindBuffer(GL_ARRAY_BUFFER, lampVBO);
    glBufferData(GL_ARRAY_BUFFER, lampVertices.size() * sizeof(float), lampVertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);


    // Verteksi za progress bar
    float progressBarVertices[] = {
        sliderX, sliderY,                        // Donji levi
        sliderX + sliderValue * sliderWidth, sliderY, // Donji desni
        sliderX + sliderValue * sliderWidth, sliderY + sliderHeight, // Gornji desni
        sliderX, sliderY + sliderHeight         // Gornji levi
    };

    unsigned int progressBarVAO, progressBarVBO;
    glGenVertexArrays(1, &progressBarVAO);
    glGenBuffers(1, &progressBarVBO);

    glBindVertexArray(progressBarVAO);
    glBindBuffer(GL_ARRAY_BUFFER, progressBarVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(progressBarVertices), progressBarVertices, GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Verteksi za okvir slajdera
    float sliderOutlineVertices[] = {
        sliderX, sliderY,                        // Donji levi
        sliderX + sliderWidth, sliderY,          // Donji desni
        sliderX + sliderWidth, sliderY + sliderHeight, // Gornji desni
        sliderX, sliderY + sliderHeight          // Gornji levi
    };

    unsigned int sliderOutlineVAO, sliderOutlineVBO;
    glGenVertexArrays(1, &sliderOutlineVAO);
    glGenBuffers(1, &sliderOutlineVBO);

    glBindVertexArray(sliderOutlineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, sliderOutlineVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(sliderOutlineVertices), sliderOutlineVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    float powerButtonVertices[] = {
     powerButtonX, powerButtonY,                             // Donji levi ugao
     powerButtonX + buttonWidth, powerButtonY,               // Donji desni ugao
     powerButtonX + buttonWidth, powerButtonY + buttonHeight, // Gornji desni ugao
     powerButtonX, powerButtonY + buttonHeight              // Gornji levi ugao
    };

    float modeButtonVertices[] = {
    modeButtonX, modeButtonY,                             // Donji levi ugao
    modeButtonX + buttonWidth, modeButtonY,               // Donji desni ugao
    modeButtonX + buttonWidth, modeButtonY + buttonHeight, // Gornji desni ugao
    modeButtonX, modeButtonY + buttonHeight              // Gornji levi ugao
    };


    unsigned int powerButtonVAO, powerButtonVBO;
    glGenVertexArrays(1, &powerButtonVAO);
    glGenBuffers(1, &powerButtonVBO);

    glBindVertexArray(powerButtonVAO);
    glBindBuffer(GL_ARRAY_BUFFER, powerButtonVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(powerButtonVertices), powerButtonVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);


    unsigned int modeButtonVAO, modeButtonVBO;
    glGenVertexArrays(1, &modeButtonVAO);
    glGenBuffers(1, &modeButtonVBO);

    glBindVertexArray(modeButtonVAO);
    glBindBuffer(GL_ARRAY_BUFFER, modeButtonVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(modeButtonVertices), modeButtonVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    float needleVertices[] = {
    needleX - needleWidth / 2, needleYStart, // Leva donja tačka
    needleX + needleWidth / 2, needleYStart, // Desna donja tačka
    needleX + needleWidth / 2, needleYEnd,   // Desna gornja tačka
    needleX - needleWidth / 2, needleYEnd    // Leva gornja tačka
    };

    unsigned int needleVAO, needleVBO;
    glGenVertexArrays(1, &needleVAO);
    glGenBuffers(1, &needleVBO);

    glBindVertexArray(needleVAO);
    glBindBuffer(GL_ARRAY_BUFFER, needleVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(needleVertices), needleVertices, GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    std::vector<float> scaleVertices;

    // Generisanje podela
    for (int i = 0; i <= numDivisions; i++) {
        float x = scaleStartX + i * (scaleEndX - scaleStartX) / numDivisions;
        scaleVertices.push_back(x);
        scaleVertices.push_back(scaleY);          // Donja tačka podeoka
        scaleVertices.push_back(x);
        scaleVertices.push_back(scaleY + 0.05f); // Gornja tačka podeoka
    }

    unsigned int scaleVAO, scaleVBO;
    glGenVertexArrays(1, &scaleVAO);
    glGenBuffers(1, &scaleVBO);

    glBindVertexArray(scaleVAO);
    glBindBuffer(GL_ARRAY_BUFFER, scaleVBO);
    glBufferData(GL_ARRAY_BUFFER, scaleVertices.size() * sizeof(float), scaleVertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);


    // Kreiranje shader programa
    unsigned int bodyShaderProgram = createShaderProgram("body.vert", "body.frag");
    unsigned int speakerShaderProgram = createShaderProgram("speaker.vert", "speaker.frag");
    unsigned int gridShaderProgram = createShaderProgram("grid.vert", "grid.frag");
    unsigned int lampShaderProgram = createShaderProgram("lamp.vert", "lamp.frag");
    unsigned int fontTexture = loadTexture("res/signature.png");
    unsigned int textShaderProgram = createShaderProgram("text.vert", "text.frag");
    unsigned int speakerTexture = loadTexture("res/radio.png");
    unsigned int sliderTexture = loadTexture("res/fm.png");
	unsigned int sliderShaderProgram = createShaderProgram("slider.vert", "slider.frag");

    // Glavna petlja
    while (!glfwWindowShouldClose(window)) {
        // Obrada ulaza
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, true);
        }

        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        // Povećanje uniformnog vremena za animaciju zvučnika
        timeUniform += 0.01f;

        // Ciscenje ekrana
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Crtanje tela radija (pravougaonik)
        float bodyColor[3] = { 0.5f, 0.5f, 0.5f };

        glUseProgram(bodyShaderProgram);
        glUniform3f(glGetUniformLocation(bodyShaderProgram, "color"), bodyColor[0], bodyColor[1], bodyColor[2]);
        glBindVertexArray(bodyVAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        glUseProgram(textShaderProgram); // bind your shader
        glBindVertexArray(quadVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, fontTexture);
        glUniform1i(glGetUniformLocation(textShaderProgram, "uTexture"), 0); // texture unit 0

        glDrawArrays(GL_TRIANGLES, 0, 6);

       

        drawAntenna(bodyShaderProgram);
        // Prikaz teksta displaya
        currentStation = getActiveStation(needleX, isAMMode);

        if (isRadioOn && !currentStation.empty()) {
            // Pomeranje teksta ulevo
            textPosition -= textSpeed;

            // Resetovanje pozicije kada tekst izađe izvan granice
            float textWidth = currentStation.size() * 0.06f; // Približna širina teksta
            if (textPosition + textWidth < displayStartX) {
                textPosition = displayEndX;
                std::cout << "Text reset to starting position: " << textPosition << std::endl;
            }
         

            // Crtanje teksta
          
           //drawText(textPosition, displayY,currentStation.c_str(), 1.0f, 1.0f, 1.0f, 1.0f);
        }

        GLenum error = glGetError();
        if (error != GL_NO_ERROR) {
            std::cerr << "OpenGL greska: " << gluErrorString(error) << std::endl;
        }

        // Ažuriraj poziciju slidera
       


        if (isRadioOn) {
            // Lampica svetli i koristi animaciju
            glUseProgram(lampShaderProgram);
            glUniform1f(glGetUniformLocation(lampShaderProgram, "uTime"), timeUniform);
            glBindVertexArray(lampVAO);
            glDrawArrays(GL_TRIANGLE_FAN, 0, lampSegments + 2);
            glBindVertexArray(0);
        }
        else {
            // Lampica je siva
            float grayColor[3] = { 0.5f, 0.5f, 0.5f };
            glUseProgram(bodyShaderProgram); // Shader za boju
            glUniform3f(glGetUniformLocation(bodyShaderProgram, "color"), grayColor[0], grayColor[1], grayColor[2]);
            glBindVertexArray(lampVAO);
            glDrawArrays(GL_TRIANGLE_FAN, 0, lampSegments + 2);
            glBindVertexArray(0);
        }

        if (isRadioOn && !currentStation.empty()) {
            // Zvučnik vibrira
            glUseProgram(speakerShaderProgram);

            // Binduj teksturu
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, speakerTexture);
            glUniform1i(glGetUniformLocation(speakerShaderProgram, "uTexture"), 0);

            // Uniformi za vreme i intenzitet
            glUniform1f(glGetUniformLocation(speakerShaderProgram, "uTime"), glfwGetTime());
            glUniform1f(glGetUniformLocation(speakerShaderProgram, "uIntensity"), sliderValue); // sliderValue u opsegu 0–1

            // Crtanje
            glBindVertexArray(speakerVAO);
            glDrawArrays(GL_TRIANGLE_FAN, 0, circleSegments + 2);


            // Crtanje zaštitne mreže
            glUseProgram(gridShaderProgram);
            glBindVertexArray(speakerVAO); // Koristi isti VAO kao za membranu
            glDrawArrays(GL_TRIANGLE_FAN, 0, circleSegments + 2);
            glBindVertexArray(0);

            // stanice
        
            glUseProgram(sliderShaderProgram);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, sliderTexture);
            glUniform1i(glGetUniformLocation(sliderShaderProgram, "uTexture"), 0);

            glBindVertexArray(sliderVAO);
            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
            glBindVertexArray(0);


            sliderX2 -= sliderSpeed2 * deltaTime;

            // Ako izađe skroz levo, vrati se na desnu stranu
            if (sliderX2 < -0.5f - sliderWidth2 / 2) {
                sliderX2 = 0.5f + sliderWidth2 / 2;
            }

            // Ažuriraj vertekse
            float newSliderVertices[] = {
                //  x, y,      u, v
                sliderX2 - sliderWidth2 / 2, sliderY2 - sliderHeight2 / 2,   0.0f, 0.0f,
                sliderX2 + sliderWidth2 / 2, sliderY2 - sliderHeight2 / 2,   1.0f, 0.0f,
                sliderX2 + sliderWidth2 / 2, sliderY2 + sliderHeight2 / 2,   1.0f, 1.0f,
                sliderX2 - sliderWidth2 / 2, sliderY2 + sliderHeight2 / 2,   0.0f, 1.0f
            };


            glBindBuffer(GL_ARRAY_BUFFER, sliderVBO);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(newSliderVertices), newSliderVertices);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }
        else {
            // Zvučnik statičan
            glUseProgram(bodyShaderProgram); // Shader za boju
            glUniform3f(glGetUniformLocation(bodyShaderProgram, "color"), 1.0f, 1.0f, 1.0f); // Beli zvučnik
            glBindVertexArray(speakerVAO);
            glDrawArrays(GL_TRIANGLE_FAN, 0, circleSegments + 2);
            glBindVertexArray(0);

            // Crtanje zaštitne mreže
            glUseProgram(gridShaderProgram);
            glBindVertexArray(speakerVAO); // Koristi isti VAO kao za membranu
            glDrawArrays(GL_TRIANGLE_FAN, 0, circleSegments + 2);
            glBindVertexArray(0);
        }



        // Ažuriranje verteksa progress bara
        progressBarVertices[2] = sliderX + sliderValue * sliderWidth; // Donji desni
        progressBarVertices[4] = sliderX + sliderValue * sliderWidth; // Gornji desni

        glBindBuffer(GL_ARRAY_BUFFER, progressBarVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(progressBarVertices), progressBarVertices);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // Crtanje progress bara
        glUseProgram(bodyShaderProgram); // Ili shader koji koristiš za boje
        glBindVertexArray(progressBarVAO);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        glBindVertexArray(0);

        // Crtanje okvira slajdera
        glBindVertexArray(sliderOutlineVAO);
        glDrawArrays(GL_LINE_LOOP, 0, 4);
        glBindVertexArray(0);

        // Boja dugmeta za uključivanje/isključivanje
        float powerButtonColor[3];
        if (isRadioOn) {
            powerButtonColor[0] = 0.0f; // Zelena
            powerButtonColor[1] = 1.0f;
            powerButtonColor[2] = 0.0f;
        }
        else {
            powerButtonColor[0] = 1.0f; // Crvena
            powerButtonColor[1] = 0.0f;
            powerButtonColor[2] = 0.0f;
        }

        glUseProgram(bodyShaderProgram);
        glUniform3f(glGetUniformLocation(bodyShaderProgram, "color"), powerButtonColor[0], powerButtonColor[1], powerButtonColor[2]);
        glBindVertexArray(powerButtonVAO);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        glBindVertexArray(0);



        // Boja dugmeta za izbor režima
        float modeButtonColor[3];
        if (isAMMode) {
            modeButtonColor[0] = 0.0f; // Plava
            modeButtonColor[1] = 0.0f;
            modeButtonColor[2] = 1.0f;
        }
        else {
            modeButtonColor[0] = 1.0f; // Žuta
            modeButtonColor[1] = 1.0f;
            modeButtonColor[2] = 0.0f;
        }

        glUseProgram(bodyShaderProgram);
        glUniform3f(glGetUniformLocation(bodyShaderProgram, "color"), modeButtonColor[0], modeButtonColor[1], modeButtonColor[2]);
        glBindVertexArray(modeButtonVAO);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        glBindVertexArray(0);

        needleVertices[0] = needleX - needleWidth / 2; // Leva donja tačka
        needleVertices[2] = needleX + needleWidth / 2; // Desna donja tačka
        needleVertices[4] = needleX + needleWidth / 2; // Desna gornja tačka
        needleVertices[6] = needleX - needleWidth / 2; // Leva gornja tačka

        glBindBuffer(GL_ARRAY_BUFFER, needleVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(needleVertices), needleVertices);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glUseProgram(bodyShaderProgram);
        float scaleColor[3] = { 1.0f, 1.0f, 1.0f }; // Bela boja
        glUniform3f(glGetUniformLocation(bodyShaderProgram, "color"), scaleColor[0], scaleColor[1], scaleColor[2]);
        glBindVertexArray(scaleVAO);
        glDrawArrays(GL_LINES, 0, scaleVertices.size() / 2);
        glBindVertexArray(0);


        float needleColor[3] = { 1.0f, 0.0f, 0.0f }; // Crvena boja
        glUniform3f(glGetUniformLocation(bodyShaderProgram, "color"), needleColor[0], needleColor[1], needleColor[2]);
        glBindVertexArray(needleVAO);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        glBindVertexArray(0);



        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Oslobađanje resursa
    glDeleteVertexArrays(1, &bodyVAO);
    glDeleteBuffers(1, &bodyVBO);
    glDeleteBuffers(1, &bodyEBO);
    glDeleteVertexArrays(1, &speakerVAO);
    glDeleteBuffers(1, &speakerVBO);
    glDeleteProgram(bodyShaderProgram);
    glDeleteProgram(speakerShaderProgram);
    glDeleteProgram(gridShaderProgram);

    glfwTerminate();
    return 0;
}

// Funkcija za čitanje fajlova
std::string readFile(const char* filePath) {
    std::ifstream file(filePath, std::ios::binary); // Otvaramo fajl u binarnom modu
    if (!file.is_open()) {
        std::cerr << "Neuspesno otvaranje fajla: " << filePath << std::endl;
        return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();

    // Uklanjamo BOM ako postoji
    if (content.size() >= 3 &&
        (unsigned char)content[0] == 0xEF &&
        (unsigned char)content[1] == 0xBB &&
        (unsigned char)content[2] == 0xBF) {
        content = content.substr(3); // Ignorisanje prvih 3 bajta (BOM)
    }

    return content;
}


// Funkcija za kompajliranje shadera
unsigned int compileShader(GLenum type, const char* source) {
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cout << "Shader greska: " << infoLog << std::endl;
    }
    return shader;
}

// Funkcija za kreiranje shader programa
unsigned int createShaderProgram(const char* vertexPath, const char* fragmentPath) {
    std::string vertexCode = readFile(vertexPath);
    std::string fragmentCode = readFile(fragmentPath);

    if (vertexCode.empty() || fragmentCode.empty()) {
        std::cerr << "Shader kod nije ucitan pravilno!" << std::endl;
        return 0;
    }

    unsigned int vertexShader = compileShader(GL_VERTEX_SHADER, vertexCode.c_str());
    unsigned int fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentCode.c_str());

    unsigned int program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    int success;
    char infoLog[512];
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        std::cout << "Linking greska: " << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return program;
}
