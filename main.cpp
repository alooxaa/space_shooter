// Space Shooter: A/D or arrows = move, Space = fire, R = restart, Esc = quit.
#include <glad/gl.h> // Include before GLFW.
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <ctime>

using namespace std;

// Shaders and drawing.
namespace {
GLuint program = 0, vao = 0, vbo = 0, ebo = 0;
GLint rectLocation, colorLocation;

// GLSL: scale a unit square, move it, then convert pixels to clip coordinates.
const char* vertexSource = R"(
#version 330 core
layout(location = 0) in vec2 position;
uniform vec4 rect;
uniform vec2 screenSize;
void main() {
    vec2 pixel = rect.xy + position * rect.zw;
    vec2 clip = vec2(2.0 * pixel.x / screenSize.x - 1.0,
                    1.0 - 2.0 * pixel.y / screenSize.y);
    gl_Position = vec4(clip, 0.0, 1.0);
}
)";

// GLSL: each fragment of the shape receives the chosen RGB color.
const char* fragmentSource = R"(
#version 330 core
uniform vec3 color;
out vec4 fragmentColor;
void main() {
    fragmentColor = vec4(color, 1.0);
}
)";

GLuint compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[2048];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        fprintf(stderr, "Shader compilation failed: %s\n", log);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

} //

bool initRenderer(int width, int height) {
    GLuint vertex = compileShader(GL_VERTEX_SHADER, vertexSource);
    GLuint fragment = compileShader(GL_FRAGMENT_SHADER, fragmentSource);
    if (!vertex || !fragment) {
        glDeleteShader(vertex);
        glDeleteShader(fragment);
        return false;
    }
    program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char log[2048];
        glGetProgramInfoLog(program, sizeof(log), nullptr, log);
        fprintf(stderr, "Shader linking failed: %s\n", log);
        glDeleteProgram(program);
        program = 0;
        return false;
    }
    // Four corners: 0--1, 3--2. Two triangles, then two diagonals for the X.
    const float vertices[] = {0, 0,  1, 0,  1, 1,  0, 1};
    const unsigned int indices[] = {0, 1, 2,  0, 2, 3,  0, 2, 1, 3};
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    rectLocation = glGetUniformLocation(program, "rect");
    colorLocation = glGetUniformLocation(program, "color");
    glUseProgram(program);
    glUniform2f(glGetUniformLocation(program, "screenSize"), float(width), float(height));
    return true;
}

void rectangle(float x, float y, float w, float h, float red, float green, float blue) {
    glUseProgram(program); // Activate shaders before uniforms and drawing.
    glUniform4f(rectLocation, x, y, w, h);
    glUniform3f(colorLocation, red, green, blue);
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
}

void drawCross(float x, float y, float w, float h) {
    glUseProgram(program);
    glUniform4f(rectLocation, x, y, w, h);
    glUniform3f(colorLocation, 1.0f, 0.3f, 0.3f);
    glBindVertexArray(vao);
    // Skip the six triangle indices and draw the two diagonals.
    glDrawElements(GL_LINES, 4, GL_UNSIGNED_INT,
                   reinterpret_cast<void*>(6 * sizeof(unsigned int)));
}

void destroyRenderer() {
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
    glDeleteProgram(program);
}

// Game state and logic.
const int WIDTH = 800, HEIGHT = 600;
struct Box {
    float x, y, w, h;
    bool active = true;
};
Box player, enemies[5], bullets[16]; // number enemies and bullets
int score = 0, lives = 3;
float cooldown = 0, elapsed = 0;

// Two rectangles collide when their horizontal AND vertical ranges overlap.
bool overlaps(const Box& a, const Box& b) {
    return a.x < b.x + b.w && a.x + a.w > b.x &&
           a.y < b.y + b.h && a.y + a.h > b.y;
}

void spawnEnemy(Box& enemy) {
    enemy = {float(20 + rand() % 725),
             float(-60 - rand() % 420), 36, 28};
}

void restartGame() {
    player = {378, 530, 44, 32};
    score = 0;
    lives = 3; // your lives in the start
    cooldown = elapsed = 0;
    for (Box& enemy : enemies) spawnEnemy(enemy);
    for (Box& bullet : bullets) bullet.active = false;
}

void onKey(GLFWwindow* window, int key, int, int action, int) {
    if (action != GLFW_PRESS) return;
    if (key == GLFW_KEY_R) restartGame();
    if (key == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(window, GLFW_TRUE);
}

// dt is elapsed seconds: speed * dt gives distance moved this frame.
void updateGame(GLFWwindow* window, float dt) {
    if (lives == 0) return;
    elapsed += dt;
    if (glfwGetKey(window, GLFW_KEY_A) || glfwGetKey(window, GLFW_KEY_LEFT))
        player.x -= 620 * dt; // if you want change your speed
    if (glfwGetKey(window, GLFW_KEY_D) || glfwGetKey(window, GLFW_KEY_RIGHT))
        player.x += 620 * dt; // if you want change your speed
    player.x = clamp(player.x, 0.0f, WIDTH - player.w);

    cooldown -= dt;
    if (glfwGetKey(window, GLFW_KEY_SPACE) && cooldown <= 0) {
        for (Box& bullet : bullets) {
            if (bullet.active) continue;
            bullet = {player.x + 19, player.y - 16, 6, 16};
            cooldown = 0.1f;
            break; // Fire one bullet, then wait before firing again.
        }
    }
    for (Box& bullet : bullets) {
        if (!bullet.active) continue;
        bullet.y -= 650 * dt; // bullet speed
        if (bullet.y + bullet.h < 0) bullet.active = false;
    }
    float speed = min(300.0f, 110.0f + (score / 5) * 20.0f);
    for (Box& enemy : enemies) {
        enemy.y += speed * dt;
        for (Box& bullet : bullets) {
            if (bullet.active && overlaps(bullet, enemy)) {
                bullet.active = false;
                ++score;
                spawnEnemy(enemy);
                break;
            }
        }
        // Both a collision and an escaped enemy cost one life.
        if (overlaps(player, enemy) || enemy.y >= HEIGHT) {
            --lives;
            spawnEnemy(enemy);
            if (lives == 0) return;
        }
    }
}

void drawGame(GLFWwindow* window) {
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);
    glClearColor(0.02f, 0.03f, 0.09f, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    // Moving stars wrap back to the top when they reach the bottom.
    for (int i = 0; i < 65; ++i) {
        int y = (i * 83 + int(elapsed * 35)) % HEIGHT;
        rectangle(float(i * 137 % WIDTH), float(y), 2, 2, 0.35f, 0.45f, 0.65f);
    }
    for (const Box& bullet : bullets)
        if (bullet.active)
            rectangle(bullet.x, bullet.y, bullet.w, bullet.h, 1, 0.85f, 0.25f);
    for (const Box& enemy : enemies) {
        rectangle(enemy.x, enemy.y, enemy.w, enemy.h, 0.95f, 0.25f, 0.35f);
        rectangle(enemy.x + 6, enemy.y + 8, 7, 6, 1, 0.85f, 0.3f);
        rectangle(enemy.x + 23, enemy.y + 8, 7, 6, 1, 0.85f, 0.3f);
    }
    if (lives > 0) {
        rectangle(player.x, player.y + 12, player.w, 20, 0.15f, 0.65f, 0.95f);
        rectangle(player.x + 15, player.y, 14, 32, 0.4f, 0.9f, 1);
        rectangle(player.x + 18, player.y + 32, 8, 10, 1, 0.55f, 0.15f);
    }
    rectangle(0, 0, WIDTH, 38, 0.04f, 0.06f, 0.13f);
    for (int i = 0; i < lives; ++i)
        rectangle(18 + i * 30, 12, 20, 14, 0.25f, 0.9f, 0.65f);
    // Progress within the current group of five hits.
    rectangle(650, 14, 130, 10, 0.15f, 0.2f, 0.3f);
    rectangle(650, 14, float(score % 5) * 26, 10, 0.95f, 0.65f, 0.2f);
    if (lives == 0) drawCross(360, 260, 80, 80);
    char title[180];
    snprintf(title, sizeof(title),
        "%s | Score: %d | Lives: %d | Level: %d | A/D: move | Space: fire | R: restart | Esc: quit",
        lives > 0 ? "Space Shooter" : "GAME OVER", score, lives,
        1 + min(score / 5, 10));
    glfwSetWindowTitle(window, title);
}

int main() {
    glfwSetErrorCallback([](int, const char* message) {
        fprintf(stderr, "GLFW: %s\n", message);
    });
    if (!glfwInit()) return 1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Space Shooter", nullptr, nullptr);
    if (!window) { glfwTerminate(); return 1; }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    if (!gladLoadGL(glfwGetProcAddress) || !initRenderer(WIDTH, HEIGHT)) {
        fprintf(stderr, "Could not initialize OpenGL 3.3 rendering.\n");
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }
    glfwSetKeyCallback(window, onKey);
    srand(static_cast<unsigned int>(time(nullptr)));
    restartGame();
    double previousTime = glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        double now = glfwGetTime();
        // Cap long frames so objects cannot jump through each other.
        float dt = float(min(now - previousTime, 0.02));
        previousTime = now;
        glfwPollEvents();
        updateGame(window, dt);
        drawGame(window);
        glfwSwapBuffers(window);
    }
    destroyRenderer();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
