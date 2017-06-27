#include <algorithm>
#include <memory>
#include <iostream>

#include <unistd.h>
#include <GLUT/glut.h>

#include "life.h"

static bool paused = false;
static std::unique_ptr<conway::Life> life;
static int scaleFactor = 54;

void displayCallback() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    gluOrtho2D(-1,1,-1,1);

    glBegin(GL_POINTS);
    glColor4f(1.0, 1.0, 1.0, 1.0);
    for (auto p : life->LivePoints()) {
        double x = p.x / (double)(std::numeric_limits<int64_t>::max() / (1L << scaleFactor));
        double y = p.y / (double)(std::numeric_limits<int64_t>::max() / (1L << scaleFactor));
        glVertex2d(x, y);
    }
    glEnd();

    glutSwapBuffers();
}

void timerCallback(int unused) {
    if (!paused) {
        life->Step();
    }
    glutTimerFunc(100, timerCallback, 0);
    glutPostRedisplay();
}

void keyCallback(unsigned char key, int x, int y) {
    switch (key) {
        case '+':
            scaleFactor = std::min(56, scaleFactor + 1);
            break;
        case '-':
            scaleFactor = std::max(0, scaleFactor - 1);
            break;
        case 'p':
            paused = !paused;
        default:
            break;
    }
}

int main(int argc, char** argv) {
    std::cout << "Starting Conway!" << std::endl;

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGB);
    glutInitWindowSize(800, 800);
    glutCreateWindow("Conway's Game of Life");

    //std::unique_ptr<conway::Life> life(new conway::ArrayLife(256, 256));
    life.reset(new conway::LiveLife(1L<<40, 1L<<40));
    // // Glider
    // life->AddLivePoint(conway::Point(0,10));
    // life->AddLivePoint(conway::Point(1,10));
    // life->AddLivePoint(conway::Point(2,10));
    // life->AddLivePoint(conway::Point(2,11));
    // life->AddLivePoint(conway::Point(1,12));
    // // Oscillators
    // life->AddLivePoint(conway::Point(20,4));
    // life->AddLivePoint(conway::Point(20,5));
    // life->AddLivePoint(conway::Point(20,6));
    // life->AddLivePoint(conway::Point(24,6));
    // life->AddLivePoint(conway::Point(25,6));
    // life->AddLivePoint(conway::Point(26,6));
    // R-pentomino
    life->AddLivePoint(conway::Point(3,1));
    life->AddLivePoint(conway::Point(3,2));
    life->AddLivePoint(conway::Point(3,3));
    life->AddLivePoint(conway::Point(4,3));
    life->AddLivePoint(conway::Point(2,2));

    glClearColor(0.0,0.0,0.3,1.0);
    glutDisplayFunc(displayCallback);
    glutTimerFunc(100, timerCallback, 0);
    glutKeyboardFunc(keyCallback);

    glutMainLoop();
    return 0;
}
