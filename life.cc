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

const BlockLife::BlockArray BlockLife::EMPTY_BLOCK = BlockArray{{0}};

BlockLife::BlockLife(int64_t height, int64_t width)
  : Life(height, width),
    blocks_(new std::unordered_map<const Point, BlockArray>()),
    new_blocks_(new std::unordered_map<const Point, BlockArray>()) {
}

BlockLife::~BlockLife() {}

Point BlockLife::toBlockIndex(const Point& p) {
    return Point(p.x & BLOCK_MASK, p.y & BLOCK_MASK);
}

Point BlockLife::toBlockCoordinates(const Point& p) {
    return Point(p.x & (~BLOCK_MASK), p.y & (~BLOCK_MASK));
}

void BlockLife::AddLivePoint(const Point& p) {
    BlockArray& block = blocks_->emplace(Point(p.x & BLOCK_MASK, p.y & BLOCK_MASK), EMPTY_BLOCK).first->second;
    Point blockCoord = toBlockCoordinates(p);
    block[blockCoord.y * BLOCK_DIM + blockCoord.x] = 1;
}

void BlockLife::DoStep() {
    for (const auto& p : *blocks_) {
        DoStepForBlock(p.first, p.second);
    }

    blocks_->clear();
    for (auto& p : *new_blocks_) {
        bool block_has_points = false;
        for (int i = 0; i < p.second.size(); i++) {
            p.second[i] = p.second[i] == 3 || p.second[i] == 13 || p.second[i] == 14;
            block_has_points |= p.second[i];
        }
        if (block_has_points) {
            blocks_->insert(p);
        }
    }
    new_blocks_->clear();
}

void BlockLife::DoStepForBlock(const Point& block_index, const BlockArray& block) {
    BlockArray *b1,*b2,*b3;
    BlockArray *b4,*b5,*b6;
    BlockArray *b7,*b8,*b9;

    // Determine the blocks we need.
    if (block[(BLOCK_DIM - 1) * BLOCK_DIM + 0] == 1) { b1 = &(new_blocks_->emplace(Point(block_index.x - BLOCK_DIM, block_index.y + BLOCK_DIM), EMPTY_BLOCK).first->second); }
    if (block[(BLOCK_DIM - 1) * BLOCK_DIM + (BLOCK_DIM - 1)] == 1) { b3 = &(new_blocks_->emplace(Point(block_index.x + BLOCK_DIM, block_index.y + BLOCK_DIM), EMPTY_BLOCK).first->second); }
    if (block[0 * BLOCK_DIM + 0] == 1) { b7 = &(new_blocks_->emplace(Point(block_index.x - BLOCK_DIM, block_index.y - BLOCK_DIM), EMPTY_BLOCK).first->second); }
    if (block[0 * BLOCK_DIM + (BLOCK_DIM - 1)] == 1) { b9 = &(new_blocks_->emplace(Point(block_index.x + BLOCK_DIM, block_index.y - BLOCK_DIM), EMPTY_BLOCK).first->second); }
    bool needs_b2 = false;
    bool needs_b4 = false;
    bool needs_b6 = false;
    bool needs_b8 = false;
    for (int i = 0; i < BLOCK_DIM; i++) {
        needs_b2 |= block[(BLOCK_DIM - 1) * BLOCK_DIM + i] == 1;
        needs_b4 |= block[i * BLOCK_DIM + 0] == 1;
        needs_b6 |= block[i * BLOCK_DIM + (BLOCK_DIM - 1)] == 1;
        needs_b8 |= block[0 * BLOCK_DIM + i] == 1;
    }
    if (needs_b2) { b2 = &(new_blocks_->emplace(Point(block_index.x - 0, block_index.y + BLOCK_DIM), EMPTY_BLOCK).first->second); }
    if (needs_b4) { b4 = &(new_blocks_->emplace(Point(block_index.x - BLOCK_DIM, block_index.y - 0), EMPTY_BLOCK).first->second); }
    if (needs_b6) { b6 = &(new_blocks_->emplace(Point(block_index.x + BLOCK_DIM, block_index.y - 0), EMPTY_BLOCK).first->second); }
    if (needs_b8) { b8 = &(new_blocks_->emplace(Point(block_index.x - 0, block_index.y - BLOCK_DIM), EMPTY_BLOCK).first->second); }

    b5 = &(new_blocks_->emplace(Point(block_index.x, block_index.y), EMPTY_BLOCK).first->second);

    // Inner block.
    for (int y = 1; y < BLOCK_DIM - 1; y++) {
        for (int x = 1; x < BLOCK_DIM - 1; x++) {
            if (block[y * BLOCK_DIM + x] == 1) {
                (*b5)[(y - 1) * BLOCK_DIM + (x - 1)] += 1;
                (*b5)[(y - 1) * BLOCK_DIM + (x - 0)] += 1;
                (*b5)[(y - 1) * BLOCK_DIM + (x + 1)] += 1;
                (*b5)[(y - 0) * BLOCK_DIM + (x - 1)] += 1;
                (*b5)[(y - 0) * BLOCK_DIM + (x - 0)] += 11;  // Add 10 more so we don't have to look this point up again.
                (*b5)[(y - 0) * BLOCK_DIM + (x + 1)] += 1;
                (*b5)[(y + 1) * BLOCK_DIM + (x - 1)] += 1;
                (*b5)[(y + 1) * BLOCK_DIM + (x - 0)] += 1;
                (*b5)[(y + 1) * BLOCK_DIM + (x + 1)] += 1;
            }
        }
    }

    // Top Left Corner
    if (block[(BLOCK_DIM - 1) * BLOCK_DIM + 0] == 1) {
        (*b1)[0 * BLOCK_DIM + (BLOCK_DIM - 1)] += 1;
        (*b2)[0 * BLOCK_DIM + 0] += 1;
        (*b2)[0 * BLOCK_DIM + 1] += 1;
        (*b4)[(BLOCK_DIM - 1) * BLOCK_DIM + (BLOCK_DIM - 1)] += 1;
        (*b5)[(BLOCK_DIM - 1) * BLOCK_DIM + 0] += 11;
        (*b5)[(BLOCK_DIM - 1) * BLOCK_DIM + 1] += 1;
        (*b4)[(BLOCK_DIM - 2) * BLOCK_DIM + (BLOCK_DIM - 1)] += 1;
        (*b5)[(BLOCK_DIM - 2) * BLOCK_DIM + 0] += 1;
        (*b5)[(BLOCK_DIM - 2) * BLOCK_DIM + 1] += 1;
    }

    // Top Right Corner
    if (block[(BLOCK_DIM - 1) * BLOCK_DIM + (BLOCK_DIM - 1)]) {
        (*b2)[0 * BLOCK_DIM + (BLOCK_DIM - 2)] += 1;
        (*b2)[0 * BLOCK_DIM + (BLOCK_DIM - 1)] += 1;
        (*b3)[0 * BLOCK_DIM + 0] += 1;
        (*b5)[(BLOCK_DIM - 1) * BLOCK_DIM + (BLOCK_DIM - 2)] += 1;
        (*b5)[(BLOCK_DIM - 1) * BLOCK_DIM + (BLOCK_DIM - 1)] += 11;
        (*b6)[(BLOCK_DIM - 1) * BLOCK_DIM + 0] += 1;
        (*b5)[(BLOCK_DIM - 2) * BLOCK_DIM + (BLOCK_DIM - 2)] += 1;
        (*b5)[(BLOCK_DIM - 2) * BLOCK_DIM + (BLOCK_DIM - 1)] += 1;
        (*b6)[(BLOCK_DIM - 2) * BLOCK_DIM + 0] += 1;
    }

    // Lower Left Corner
    if (block[0 * BLOCK_DIM + 0]) {
        (*b4)[1 * BLOCK_DIM + (BLOCK_DIM - 1)] += 1;
        (*b5)[1 * BLOCK_DIM + 0] += 1;
        (*b5)[1 * BLOCK_DIM + 1] += 1;
        (*b4)[0 * BLOCK_DIM + (BLOCK_DIM - 1)] += 1;
        (*b5)[0 * BLOCK_DIM + 0] += 11;
        (*b5)[0 * BLOCK_DIM + 1] += 1;
        (*b7)[(BLOCK_DIM - 1) * BLOCK_DIM + (BLOCK_DIM - 1)] += 1;
        (*b8)[(BLOCK_DIM - 1) * BLOCK_DIM + 0] += 1;
        (*b8)[(BLOCK_DIM - 1) * BLOCK_DIM + 1] += 1;
    }

    // Lower Right Corner
    if (block[0 * BLOCK_DIM + (BLOCK_DIM - 1)]) {
        (*b5)[1 * BLOCK_DIM + (BLOCK_DIM - 2)] += 1;
        (*b5)[1 * BLOCK_DIM + (BLOCK_DIM - 1)] += 1;
        (*b6)[1 * BLOCK_DIM + 0] += 1;
        (*b5)[0 * BLOCK_DIM + (BLOCK_DIM - 2)] += 1;
        (*b5)[0 * BLOCK_DIM + (BLOCK_DIM - 1)] += 11;
        (*b6)[0 * BLOCK_DIM + 0] += 1;
        (*b8)[(BLOCK_DIM - 1) * BLOCK_DIM + (BLOCK_DIM - 2)] += 1;
        (*b8)[(BLOCK_DIM - 1) * BLOCK_DIM + (BLOCK_DIM - 1)] += 1;
        (*b9)[(BLOCK_DIM - 1) * BLOCK_DIM + 0] += 1;
    }

    // Top row
    for (int i = 1; i < BLOCK_DIM - 1; i++) {
        if (block[(BLOCK_DIM - 1) * BLOCK_DIM + i] == 1) {
            (*b2)[0 * BLOCK_DIM + (i - 1)] += 1;
            (*b2)[0 * BLOCK_DIM + (i - 0)] += 1;
            (*b2)[0 * BLOCK_DIM + (i + 1)] += 1;
            (*b5)[(BLOCK_DIM - 1) * BLOCK_DIM + (i - 1)] += 1;
            (*b5)[(BLOCK_DIM - 1) * BLOCK_DIM + (i - 0)] += 11;
            (*b5)[(BLOCK_DIM - 1) * BLOCK_DIM + (i + 1)] += 1;
            (*b5)[(BLOCK_DIM - 2) * BLOCK_DIM + (i - 1)] += 1;
            (*b5)[(BLOCK_DIM - 2) * BLOCK_DIM + (i - 0)] += 1;
            (*b5)[(BLOCK_DIM - 2) * BLOCK_DIM + (i + 1)] += 1;
        }
    }

    // Bottom row
    for (int i = 1; i < BLOCK_DIM - 1; i++) {
        if (block[0 * BLOCK_DIM + i] == 1) {
            (*b5)[1 * BLOCK_DIM + (i - 1)] += 1;
            (*b5)[1 * BLOCK_DIM + (i - 0)] += 1;
            (*b5)[1 * BLOCK_DIM + (i + 1)] += 1;
            (*b5)[0 * BLOCK_DIM + (i - 1)] += 1;
            (*b5)[0 * BLOCK_DIM + (i - 0)] += 11;
            (*b5)[0 * BLOCK_DIM + (i + 1)] += 1;
            (*b8)[(BLOCK_DIM - 1) * BLOCK_DIM + (i - 1)] += 1;
            (*b8)[(BLOCK_DIM - 1) * BLOCK_DIM + (i - 0)] += 1;
            (*b8)[(BLOCK_DIM - 1) * BLOCK_DIM + (i + 1)] += 1;
        }
    }

    // Left column
    for (int i = 1; i < BLOCK_DIM - 1; i++) {
        if (block[i * BLOCK_DIM + 0] == 1) {
            (*b4)[(i - 1) * BLOCK_DIM + (BLOCK_DIM - 1)] += 1;
            (*b5)[(i - 1) * BLOCK_DIM + 0] += 1;
            (*b5)[(i - 1) * BLOCK_DIM + 1] += 1;
            (*b4)[(i - 0) * BLOCK_DIM + (BLOCK_DIM - 1)] += 1;
            (*b5)[(i - 0) * BLOCK_DIM + 0] += 11;
            (*b5)[(i - 0) * BLOCK_DIM + 1] += 1;
            (*b4)[(i + 1) * BLOCK_DIM + (BLOCK_DIM - 1)] += 1;
            (*b5)[(i + 1) * BLOCK_DIM + 0] += 1;
            (*b5)[(i + 1) * BLOCK_DIM + 1] += 1;
        }
    }

    // Right column
    for (int i = 1; i < BLOCK_DIM - 1; i++) {
        if (block[i * BLOCK_DIM + (BLOCK_DIM - 1)] == 1) {
            (*b5)[(i - 1) * BLOCK_DIM + (BLOCK_DIM - 2)] += 1;
            (*b5)[(i - 1) * BLOCK_DIM + (BLOCK_DIM - 1)] += 1;
            (*b6)[(i - 1) * BLOCK_DIM + 0] += 1;
            (*b5)[(i - 0) * BLOCK_DIM + (BLOCK_DIM - 2)] += 1;
            (*b5)[(i - 0) * BLOCK_DIM + (BLOCK_DIM - 1)] += 11;
            (*b6)[(i - 0) * BLOCK_DIM + 0] += 1;
            (*b5)[(i + 1) * BLOCK_DIM + (BLOCK_DIM - 2)] += 1;
            (*b5)[(i + 1) * BLOCK_DIM + (BLOCK_DIM - 1)] += 1;
            (*b6)[(i + 1) * BLOCK_DIM + 0] += 1;
        }
    }
}

std::vector<const Point> BlockLife::LivePoints() {
    std::vector<const Point> live_points;
    //printf("blocks_->size(): %lu\n", blocks_->size());
    for (auto pair : *blocks_) {
        // printf("block_index (%lld, %lld)\n", pair.first.x, pair.first.y);
        for (int y = 0; y < BLOCK_DIM; y++) {
            for (int x = 0; x < BLOCK_DIM; x++) {
                if (pair.second[y * BLOCK_DIM + x] == 1) {
                    // printf("LivePoint: (%lld, %lld)\n", pair.first.x + x, pair.first.y + y);
                    live_points.emplace_back(pair.first.x + x, pair.first.y + y);
                }
            }
        }
    }
    return live_points;
}

void BlockLife::Print() {}

}  // namespace conway