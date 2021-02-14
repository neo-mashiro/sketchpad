//#include <windows.h>
//#include <GL/freeglut.h>
//
//#include <cmath>
//#include <define.h>
//
//
//const int N_TURNS = 10;
//const int N_VERTICES_PER_TURN = 360;
//const int N_VERTICES_TOTAL = N_VERTICES_PER_TURN * N_TURNS;
//
//
//void Init() {
//    glClearColor(1.0, 1.0, 1.0, 1.0);
//    glColor3f(1.0, 0.0, 0.0);
//    glMatrixMode(GL_PROJECTION);
//    glLoadIdentity();
//    gluOrtho2D(-500.0, 500.0, -500.0, 500.0);
//    glMatrixMode(GL_MODELVIEW);
//}
//
//void Display() {
//    vertex2 vertices[N_VERTICES_TOTAL];
//
//    double center_x = 0.0, center_y = 0.0, radius = 0.0;
//
//    double delta = 0.2;  // radius step size
//    double alpha = 2.0 * PI / (double)N_VERTICES_PER_TURN;  // auto angle step size
//    double theta = 153.0 * PI / 180.0;                      // fixed angle step size
//
//    int i = 0;
//    while ((i < N_VERTICES_TOTAL) && (radius < 400.0)) {
//        // draw spiral with auto angle step size
//        vertices[i][0] = center_x + radius * cos(alpha * i);
//        vertices[i][1] = center_y + radius * sin(alpha * i);
//
//        // draw nice patterns and artifacts with fixed angle step size
//        //vertices[i][0] = center_x + radius * cos(theta * i);
//        //vertices[i][1] = center_y + radius * sin(theta * i);
//
//        i++;
//        radius += delta;
//    }
//
//    glClear(GL_COLOR_BUFFER_BIT);
//    glColor3f(1.0, 0.0, 0.0);
//    glPointSize(1);
//    glLineWidth(1.4);
//
//    glBegin(GL_LINE_STRIP);
//    for (int k = 0; k < i; k++) {
//        glVertex2fv(vertices[k]);
//    }
//    glEnd();
//
//    glFlush();
//}
//
//void KeyPressed(unsigned char key, int x, int y) {}