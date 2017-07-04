#include <algorithm>
#include <fstream>
#include <cmath>
#include <memory>
#include <iostream>
#include <regex>

#include <GLUT/glut.h>

#include "life.h"

static bool paused = true;
static std::unique_ptr<conway::Life> life;
static int scaleFactor = 8;
static int delay_ms = 100;
static std::pair<int64_t, int64_t> viewport_center{0, 0};
static std::pair<int64_t, int64_t> x_range{-1L << scaleFactor, 1L << scaleFactor};
static std::pair<int64_t, int64_t> y_range{-1L << scaleFactor, 1L << scaleFactor};

void displayCallback() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    gluOrtho2D(-1, 1, -1, 1);

    // Display goes wild at the edges of the int64 space so try to downsample to 2^16
    int downscale = std::max(0, scaleFactor - 16);
    int window_dim = 1 << (scaleFactor - 1 - downscale);
    glBegin(GL_POINTS);
    glColor4f(1.0, 1.0, 1.0, 1.0);
    for (const auto& p : life->LivePoints()) {
        glVertex2d((double)((p.x - viewport_center.first) >> downscale) / window_dim,
                   (double)((p.y - viewport_center.second) >> downscale) / window_dim);
    }
    glEnd();

    glRasterPos2d(-0.95, 0.9);
    char buf[512];
    sprintf(buf, "Generation: %lld - Delay: %dms - Scale: 2^%d [(%lld, %lld), (%lld, %lld)]",
            life->generation(), delay_ms, scaleFactor, x_range.first, y_range.second, x_range.second, y_range.first);
    for (char c : buf) {
        if (c == '\0') { break; }
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, c);
    }
    glutSwapBuffers();
}

void timerCallback(int unused) {
    if (!paused) {
        life->Step();
    }
    glutTimerFunc(delay_ms, timerCallback, 0);
}

void displayRateCallback(int unused) {
    glutPostRedisplay();
    glutTimerFunc(30, displayRateCallback, 0);
}

void fpsCallback(int unused) {
    std::cout << "Generation: " << life->generation() << std::endl;
    glutTimerFunc(1000, fpsCallback, 0);
}

void PrintLivePoints() {
    for (const conway::Point p : life->LivePoints()) {
        std::cout << "LivePoint: (" << p.x << ", " << p.y << ")" << std::endl;
    }
}

void correctZoom() {
    if (scaleFactor == 64) {
        viewport_center = std::make_pair(0,0);
        x_range = std::make_pair(std::numeric_limits<int64_t>::min(), std::numeric_limits<int64_t>::max());
        y_range = std::make_pair(std::numeric_limits<int64_t>::min(), std::numeric_limits<int64_t>::max());
    } else {
        int64_t window_dim = 1L << (scaleFactor - 1);
        viewport_center.first = std::max(viewport_center.first, std::numeric_limits<int64_t>::min() + window_dim);
        viewport_center.first = std::min(viewport_center.first, std::numeric_limits<int64_t>::max() - window_dim);
        viewport_center.second = std::max(viewport_center.second, std::numeric_limits<int64_t>::min() + window_dim);
        viewport_center.second = std::min(viewport_center.second, std::numeric_limits<int64_t>::max() - window_dim);
        x_range = std::make_pair(viewport_center.first - window_dim, viewport_center.first + window_dim);
        y_range = std::make_pair(viewport_center.second - window_dim, viewport_center.second + window_dim);
    }
}

void keyCallback(unsigned char key, int x, int y) {
    switch (key) {
        case '+':
            scaleFactor = std::max(8, scaleFactor - 1);
            correctZoom();
            break;
        case '-':
            scaleFactor = std::min(64, scaleFactor + 1);
            correctZoom();
            break;
        case 'w':
            viewport_center.second += 1L << (scaleFactor - 3);
            correctZoom();
            break;
        case 'a':
            viewport_center.first -= 1L << (scaleFactor - 3);
            correctZoom();
            break;
        case 's':
            viewport_center.second -= 1L << (scaleFactor - 3);
            correctZoom();
            break;
        case 'd':
            viewport_center.first += 1L << (scaleFactor - 3);
            correctZoom();
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
        case 'n':
            life->Step();
            break;
        case 'l':
            PrintLivePoints();
            break;
        default:
            break;
    }
}

void LoadRLE(std::string filename, conway::Life* life) {
    std::regex comment_re("#.*");
    std::regex header_re("x = ([0-9]+), y = ([0-9]+).*");
    std::regex rle_re("(?:(?:[0-9]+)?(?:[ob$])|!)");
    std::regex rle_token_re("([0-9]+)([ob$])");
    std::smatch match;
    int x, y;
    int x_pos, y_pos;

    std::ifstream in(filename);
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

void ReadInput() {
    std::regex point_re("[(](-?[0-9]+), (-?[0-9-]+)[)] *");
    std::string line;
    std::smatch match;
    while (std::getline(std::cin, line)) {
        if (std::regex_match(line, match, point_re)) {
            life->AddLivePoint(std::stoll(match[1].str()), std::stoll(match[2].str()));
        } else {
            printf("Unrecognized line in input: %s\n", line.c_str());
        }
    }
}

int main(int argc, char** argv) {
    std::cout << "Starting Conway!" << std::endl;

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGB);
    glutInitWindowSize(800, 800);
    glutCreateWindow("Conway's Game of Life");

    // life.reset(new conway::LiveLife());
    life.reset(new conway::BlockLife());
    if (argc == 2) {
        LoadRLE(argv[1], life.get());
    } else {
        ReadInput();
    }

    glClearColor(0.0,0.0,0.3,1.0);
    glutDisplayFunc(displayCallback);
    glutTimerFunc(30, displayRateCallback, 0);
    glutTimerFunc(delay_ms, timerCallback, 0);
    glutTimerFunc(1000, fpsCallback, 0);
    glutKeyboardFunc(keyCallback);

    glutMainLoop();
    return 0;
}
