//#include <iostream>
//
//#include <windows.h>
//#include <GL/glew.h>
//#include <GL/freeglut.h>
//#include <GLFW/glfw3.h>
//
//int window;
//
//void Init() {
//	glClearColor(0.0, 0.0, 1.0, 0.0);
//	glMatrixMode(GL_PROJECTION);
//	glLoadIdentity();
//	gluOrtho2D(0.0, 1.0, 0.0, 1.0);
//	glMatrixMode(GL_MODELVIEW);
//	glLoadIdentity();
//}
//
//void Display() {
//	glClear(GL_COLOR_BUFFER_BIT);
//
//	glColor3f(1.0, 0.0, 0.0);
//	glPointSize(6);
//	glLineWidth(6);
//
//	glBegin(GL_POLYGON);
//	glVertex3f(0.25, 0.25, 0.0);
//	glVertex3f(0.75, 0.25, 0.0);
//	glVertex3f(0.75, 0.75, 0.0);
//	glVertex3f(0.25, 0.75, 0.0);
//	glEnd();
//
//	glFlush();
//}
//
//void KeyPressed(unsigned char key, int x, int y) {
//	if (key == VK_ESCAPE) {
//		glutDestroyWindow(window);
//		exit(EXIT_SUCCESS);
//	}
//}
//
//
//int main(int argc, char** argv) {
//	// initialize the GLUT library to configure and create windows
//	glutInit(&argc, argv);
//	glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
//	glutInitWindowSize(512, 512);
//	glutInitWindowPosition(glutGet(GLUT_SCREEN_WIDTH) / 3, glutGet(GLUT_SCREEN_HEIGHT) / 3);
//	window = glutCreateWindow("Boilerplate Window");
//
//	// initialize the GLEW library
//	if (glewInit()) {
//		std::cerr << "Unable to initialize GLEW..." << std::endl;
//		exit(EXIT_FAILURE);
//	}
//
//	Init();
//
//	// register the display and keyboard callbacks
//	glutDisplayFunc(Display);
//	glutKeyboardFunc(KeyPressed);
//
//	// start the infinite main loop
//	glutMainLoop();
//}