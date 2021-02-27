#include "define.h"
#include "utils.h"

const int N_TURNS = 5;
const int N_VERTICES_PER_TURN = 360;
const int N_VERTICES_MAX = N_VERTICES_PER_TURN * N_TURNS;

Window window{};

void SetupWindow() {
    window.title = "Spiral";
    SetupDefaultWindow();
    window.display_mode = GLUT_SINGLE | GLUT_RGB;
}

vertex2 vertices[N_VERTICES_MAX];
unsigned int n_vertices = 0;

void Init() {
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glColor3f(1.0, 0.0, 0.0);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(-500.0, 500.0, -500.0, 500.0);
    glMatrixMode(GL_MODELVIEW);

    double center_x = 0.0, center_y = 0.0, radius = 0.0;

    double delta = 0.2;  // radius step size
    double alpha = 2.0 * PI / (double)N_VERTICES_PER_TURN;  // auto angle step size
    double theta = 153.0 * PI / 180.0;                      // fixed angle step size

    while ((n_vertices < N_VERTICES_MAX) && (radius < 400.0)) {
        // draw spiral with auto angle step size
        vertices[n_vertices][0] = center_x + radius * cos(alpha * n_vertices);
        vertices[n_vertices][1] = center_y + radius * sin(alpha * n_vertices);

        // draw nice patterns and artifacts with fixed angle step size
        //vertices[n_vertices][0] = center_x + radius * cos(theta * n_vertices);
        //vertices[n_vertices][1] = center_y + radius * sin(theta * n_vertices);

        n_vertices++;
        radius += delta;
    }
}

void Display() {
    glClear(GL_COLOR_BUFFER_BIT);
    glColor3f(1.0, 0.0, 0.0);
    glPointSize(1);
    glLineWidth(1.0f);

    glBegin(GL_LINE_STRIP);
    for (int k = 0; k < n_vertices; k++) {
        glVertex2fv(vertices[k]);
    }
    glEnd();

    glFlush();
}

void Reshape(int width, int height) {
    DefaultReshapeCallback(width, height);
}

void Keyboard(unsigned char key, int x, int y) {
    DefaultKeyboardCallback(key, x, y);
}

void Mouse(int button, int state, int x, int y) {}
void Idle(void) {}
void Motion(int x, int y) {}
void PassiveMotion(int x, int y) {}
void Cleanup() {}