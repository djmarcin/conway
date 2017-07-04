#include "life.h"

#include <cstdlib>
#include <iostream>

namespace conway {

LiveLife::LiveLife()
    : live_points_(new std::vector<const Point>()),
      weights_(new std::unordered_map<const Point, int>()) {
    // Picked via experimentation.
    weights_->max_load_factor(0.33);
}

LiveLife::~LiveLife() {}

void LiveLife::AddLivePoint(const Point& p) {
    live_points_->push_back(p);
}

// Iterates over all the live points and applies influence to the 3x3 block surrounding it.
// Influence is stored in a hashtable which means we require 9 hashtable lookups per live point.
// Afterwards, we iterate over the hashtable and replace the live_points_ vector with those points.
// The main benefit of this algorithm over a matrix based algorithm is that memory usage scales
// with number of points and not size of the board.
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
        // Helps reduce memory allocations because most points need
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


const BlockLife::BlockArray BlockLife::EMPTY_BLOCK = BlockArray{{0}};

BlockLife::BlockLife()
  : blocks_(new std::unordered_map<const Point, BlockArray>()),
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
    BlockArray& block = blocks_->emplace(toBlockIndex(p), EMPTY_BLOCK).first->second;
    Point blockCoord = toBlockCoordinates(p);
    block[blockCoord.y * BLOCK_DIM + blockCoord.x] = 1;
}

void BlockLife::DoStep() {
    for (const auto& p : *blocks_) {
        DoStepForBlock(p.first, p.second);
    }

    // Iterate over new_blocks_ converting weights into live and dead points.
    // Remove any blocks that do not have any live points so we don't have bother checking them next generation.
    for (auto it = new_blocks_->begin(); it != new_blocks_->end();) {
        bool block_has_points = false;
        for (int i = 0; i < it->second.size(); i++) {
            it->second[i] = it->second[i] == 3 || it->second[i] == 13 || it->second[i] == 14;
            block_has_points |= it->second[i];
        }
        if (!block_has_points) {
            it = new_blocks_->erase(it);
        } else {
            ++it;
        }
    }
    new_blocks_.swap(blocks_);
    new_blocks_->clear();
}

// Apply influence to each block of 9 cells around any live cell.
// Uses a 32x32 (1024 byte) block of memory which eliminates the need for hashtable
// lookups for the inner 30x30 square and reduces the number of hashtable lookups
// for the outer region to just one per neighboring region.
// Like LiveLife, this supports much larger boards than the simple matrix based approach,
// but it trades additional memory use and unrolled loops for speed in computing the next generation.
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
                (*b5)[(y - 0) * BLOCK_DIM + (x - 0)] += 11;  // Same +10 trick as above.
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
    for (auto pair : *blocks_) {
        for (int y = 0; y < BLOCK_DIM; y++) {
            for (int x = 0; x < BLOCK_DIM; x++) {
                if (pair.second[y * BLOCK_DIM + x] == 1) {
                    live_points.emplace_back(pair.first.x + x, pair.first.y + y);
                }
            }
        }
    }
    return live_points;
}

}  // namespace conway