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

void ArrayLife::DoStep() {
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
                live_points.emplace_back(x, y);
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

LiveLife::LiveLife(int64_t height, int64_t width)
    : Life(height, width),
      live_points_(new std::vector<const Point>()),
      weights_(new std::unordered_map<const Point, int>()) {
    // Picked via experimentation.
    weights_->max_load_factor(0.33);
}

LiveLife::~LiveLife() {}

void LiveLife::AddLivePoint(const Point& p) {
    live_points_->push_back(p);
}

// This version does not care about overflow because it uses the entire int64 space.
void LiveLife::DoStep() {
    // For each live point, add 1 influence to the surrounding 8 cells.
    for (auto p : *live_points_) {
        weights_->emplace(Point(p.x - 1, p.y - 1), 0).first->second++;
        weights_->emplace(Point(p.x - 1, p.y - 0), 0).first->second++;
        weights_->emplace(Point(p.x - 1, p.y + 1), 0).first->second++;
        weights_->emplace(Point(p.x - 0, p.y - 1), 0).first->second++;
        // Add 1+10 to signal this cell was alive so we don't need to look it up later.
        weights_->emplace(Point(p.x - 0, p.y - 0), 0).first->second += 11;
        weights_->emplace(Point(p.x - 0, p.y + 1), 0).first->second++;
        weights_->emplace(Point(p.x + 1, p.y - 1), 0).first->second++;
        weights_->emplace(Point(p.x + 1, p.y - 0), 0).first->second++;
        weights_->emplace(Point(p.x + 1, p.y + 1), 0).first->second++;
    }
    live_points_->clear();
    for (auto it = weights_->begin(); it != weights_->end();) {
        // Basic generational garbage collection --
        // if the weight is still zero on this generation, erase it.
        // Helps minimize memory allocations because most points need
        // to be rechecked from generation to generation.
        if (it->second == 0) {
            it = weights_->erase(it);
            continue;
        }
        if (it->second == 3 || it->second == 13 || it->second == 14) {
            live_points_->push_back(it->first);
        }
        it->second = 0;
        it++;
    }
}

std::vector<const Point> LiveLife::LivePoints() {
    return *live_points_;
}

// Not implemented
void LiveLife::Print() {}

}  // namespace conway