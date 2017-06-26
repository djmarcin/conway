#include "life.h"

#include <cstdlib>
#include <iostream>

namespace conway {

ArrayLife::ArrayLife(int64_t height, int64_t width) : Life(height, width) {
    matrix_ = (char*)calloc(sizeof(char) * height_ * width_, sizeof(char));
    temp_ = (char*)calloc(sizeof(char) * height_ * width_, sizeof(char));
}

ArrayLife::~ArrayLife() {
    free(temp_);
    free(matrix_);
}

#define INDEX(m, x, y) m[(y) * width_ + (x)]
void ArrayLife::AddLivePoint(const Point& p) {
    INDEX(matrix_, p.x, p.y) = 1;
}

void ArrayLife::Step() {
    for (int64_t y = 0; y < height_; y++) {
        for (int64_t x = 0; x < width_; x++) {
            int accum = 
                INDEX(matrix_, (x - 1 + width_) % width_, (y - 1 + height_) % height_) +
                INDEX(matrix_, (x - 1 + width_) % width_, y) +
                INDEX(matrix_, (x - 1 + width_) % width_, (y + 1) % height_) +
                INDEX(matrix_, x, (y - 1 + height_) % height_) +
                INDEX(matrix_, x, y) +
                INDEX(matrix_, x, (y + 1) % height_) +
                INDEX(matrix_, (x + 1) % width_, (y - 1 + height_) % height_) +
                INDEX(matrix_, (x + 1) % width_, y) +
                INDEX(matrix_, (x + 1) % width_, (y + 1) % height_);
            INDEX(temp_, x, y) = accum == 3 ? 1 : accum == 4 ? INDEX(matrix_, x, y) : 0;
        }
    }
    // Swap pointers
    char* t = matrix_;
    matrix_ = temp_;
    temp_ = t;
}

std::vector<const Point> ArrayLife::LivePoints() {
    std::vector<const Point> live_points;
    for (int64_t y = 0; y < height_; y++) {
        for (int64_t x = 0; x < width_; x++) {
            if (INDEX(matrix_, x, y)) {
                live_points.emplace_back(Point(x, y));
            }
        }
    }
    return live_points;
}

void ArrayLife::Print() {
    std::cout << "-------------------------------------" << std::endl;
    for (int64_t y = height_ - 1; y >= 0 ; y--) {
        for (int64_t x = 0; x < width_; x++) {
            std::cout << (INDEX(matrix_, x, y) ? "*" : " ");
        }
        std::cout << std::endl;
    }    
}

}  // namespace conway