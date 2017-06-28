#include <algorithm>
#include <fstream>
#include <memory>
#include <iostream>
#include <regex>

#include <unistd.h>
#include <GLUT/glut.h>

#include "life.h"

static bool paused = true;
static std::unique_ptr<conway::Life> life;
static int scaleFactor = 54;
static int delay_ms = 100;

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
    glutTimerFunc(delay_ms, timerCallback, 0);
    glutPostRedisplay();
}

void PrintLivePoints() {
    for (const conway::Point p : life->LivePoints()) {
        std::cout << "LivePoint: (" << p.x << ", " << p.y << ")" << std::endl;
    }
}

void keyCallback(unsigned char key, int x, int y) {
    switch (key) {
        case '+':
            scaleFactor = std::min(56, scaleFactor + 1);
            break;
        case '-':
            scaleFactor = std::max(0, scaleFactor - 1);
            break;
        case '{':
            delay_ms += 10;
            break;
        case '}':
            delay_ms = std::max(0, delay_ms - 10);
            break;
        case 'p':
            paused = !paused;
            break;
        case 's':
            life->Step();
            break;
        case 'l':
            PrintLivePoints();
            break;
        default:
            break;
    }
}

void ParseInput(conway::Life* life) {
    std::regex comment_re("#.*");
    std::regex header_re("x = ([0-9]+), y = ([0-9]+).*");
    std::regex rle_re("(?:(?:[0-9]+)?(?:[ob$])|!)");
    std::regex rle_token_re("([0-9]+)([ob$])");
    std::smatch match;
    int x, y;
    int x_pos, y_pos;

    std::ifstream in("rle/noahsark.rle");
    std::string line;
    while (std::getline(in, line)) {
        if (std::regex_match(line, match, comment_re)) {
            continue;
        } else if (std::regex_match(line, match, header_re)) {
            x = std::stoi(match[1].str());
            y = std::stoi(match[2].str());
            x_pos = -x / 2;
            y_pos = y / 2;
        } else {
            std::sregex_iterator next(line.begin(), line.end(), rle_re);
            std::sregex_iterator end;
            while (next != end) {
                std::string token = next->str();
                if (token == "o" || token == "b" || token == "$") {
                    token = "1" + token;
                }
                if (std::regex_match(token, match, rle_token_re)) {
                    int n = std::stoi(match[1].str());
                    std::string type = match[2].str();
                    if (type == "b") {
                        x_pos += n;
                    } else if (type == "o") {
                        for (int i = 0; i < n; i++) {
                            life->AddLivePoint(x_pos++, y_pos);
                        }
                    } else if (type == "$") {
                        x_pos = -x / 2;
                        y_pos -= n;
                    }
                }
                if (token == "!") {
                    return;
                }
                next++;
            }
        }
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
    // // R-pentomino
    // life->AddLivePoint(conway::Point(3,1));
    // life->AddLivePoint(conway::Point(3,2));
    // life->AddLivePoint(conway::Point(3,3));
    // life->AddLivePoint(conway::Point(4,3));
    // life->AddLivePoint(conway::Point(2,2));

    ParseInput(life.get());

    glClearColor(0.0,0.0,0.3,1.0);
    glutDisplayFunc(displayCallback);
    glutTimerFunc(delay_ms, timerCallback, 0);
    glutKeyboardFunc(keyCallback);

    glutMainLoop();
    return 0;
}
