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

LiveLife::LiveLife(int64_t height, int64_t width) : Life(height, width) {
    live_points_ = new std::unordered_set<Point>();
    new_live_points_ = new std::unordered_set<Point>();
}

LiveLife::~LiveLife() {
    delete new_live_points_;
    delete live_points_;
}

void LiveLife::AddLivePoint(const Point& p) {
    live_points_->insert(p);
}

// void LiveLife::Step() {
//     std::unordered_set<Point> to_check;
//     // Add all possibly affected points as well.
//     for (auto p : LivePoints()) {
//         to_check.insert(Point((p.x - 1 + width_) % width_, (p.y - 1 + height_) % height_));
//         to_check.insert(Point((p.x - 1 + width_) % width_, p.y));
//         to_check.insert(Point((p.x - 1 + width_) % width_, (p.y + 1) % height_));
//         to_check.insert(Point(p.x, (p.y - 1 + height_) % height_));
//         to_check.insert(Point(p.x, p.y));
//         to_check.insert(Point(p.x, (p.y + 1) % height_));
//         to_check.insert(Point((p.x + 1) % width_, (p.y - 1 + height_) % height_));
//         to_check.insert(Point((p.x + 1) % width_, p.y));
//         to_check.insert(Point((p.x + 1) % width_, (p.y + 1) % height_));
//     }
//     for (auto p : to_check) {
//         if (IsLiveCell(p)) {
//             new_live_points_->insert(p);
//         }
//     }
//     auto* temp = live_points_;
//     live_points_ = new_live_points_;
//     new_live_points_ = temp;
//     new_live_points_->clear();
// }
//
// bool LiveLife::IsLiveCell(const Point& p) {
//     int accum =
//         (live_points_->find(Point((p.x - 1 + width_) % width_, (p.y - 1 + height_) % height_)) != live_points_->end()) +
//         (live_points_->find(Point((p.x - 1 + width_) % width_, p.y)) != live_points_->end()) +
//         (live_points_->find(Point((p.x - 1 + width_) % width_, (p.y + 1) % height_)) != live_points_->end()) +
//         (live_points_->find(Point(p.x, (p.y - 1 + height_) % height_)) != live_points_->end()) +
//         (live_points_->find(Point(p.x, p.y)) != live_points_->end()) +
//         (live_points_->find(Point(p.x, (p.y + 1) % height_)) != live_points_->end()) +
//         (live_points_->find(Point((p.x + 1) % width_, (p.y - 1 + height_) % height_)) != live_points_->end()) +
//         (live_points_->find(Point((p.x + 1) % width_, p.y)) != live_points_->end()) +
//         (live_points_->find(Point((p.x + 1) % width_, (p.y + 1) % height_)) != live_points_->end());
//     // If sum is 3, it must be live (either 2 neighbors + self or 3 neighbors).
//     // If 4, it keeps current state (either 3 neighbors and alive, or 4 neighbors and dead).
//     return accum == 3 || (accum == 4 && live_points_->find(p) != live_points_->end());
// }

// This version does not care about overflow because it uses the entire int64 space.
void LiveLife::Step() {
    std::unordered_set<Point> to_check;
    // Add all possibly affected points as well.
    for (auto p : LivePoints()) {
        for (int dx = -1; dx <= 1; dx++) {
            for (int dy = -1; dy <= 1; dy++) {
                to_check.insert(Point(p.x + dx, p.y + dy));
            }
        }
    }
    for (auto p : to_check) {
        if (IsLiveCell(p)) {
            new_live_points_->insert(p);
        }
    }
    auto* temp = live_points_;
    live_points_ = new_live_points_;
    new_live_points_ = temp;
    new_live_points_->clear();
}

bool LiveLife::IsLiveCell(const Point& p) {
    int accum =
        (live_points_->find(Point(p.x - 1, p.y - 1)) != live_points_->end()) +
        (live_points_->find(Point(p.x - 1, p.y - 0)) != live_points_->end()) +
        (live_points_->find(Point(p.x - 1, p.y + 1)) != live_points_->end()) +
        (live_points_->find(Point(p.x - 0, p.y - 1)) != live_points_->end()) +
        (live_points_->find(Point(p.x - 0, p.y - 0)) != live_points_->end()) +
        (live_points_->find(Point(p.x - 0, p.y + 1)) != live_points_->end()) +
        (live_points_->find(Point(p.x + 1, p.y - 1)) != live_points_->end()) +
        (live_points_->find(Point(p.x + 1, p.y - 0)) != live_points_->end()) +
        (live_points_->find(Point(p.x + 1, p.y + 1)) != live_points_->end());
    // If sum is 3, it must be live (either 2 neighbors + self or 3 neighbors).
    // If 4, it keeps current state (either 3 neighbors and alive, or 4 neighbors and dead).
    return accum == 3 || (accum == 4 && live_points_->find(p) != live_points_->end());
}

std::vector<const Point> LiveLife::LivePoints() {
    return std::vector<const Point>(live_points_->begin(), live_points_->end());
}

void LiveLife::Print() {
    std::cout << "-------------------------------------" << std::endl;
    for (int64_t y = height_ - 1; y >= 0 ; y--) {
        for (int64_t x = 0; x < width_; x++) {
            std::cout << (live_points_->find(Point(x, y)) != live_points_->end() ? "*" : " ");
        }
        std::cout << std::endl;
    }
}

}  // namespace conway