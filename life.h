#ifndef CONWAY_LIFE_H
#define CONWAY_LIFE_H

#include <array>
#include <cstdint>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace conway {

struct Point {
    int64_t x;
    int64_t y;

    Point(int64_t x, int64_t y) : x(x), y(y) {}
    Point(const Point &o) : x(o.x), y(o.y) {}

    bool operator==(const Point& o) const {
        return x == o.x && y == o.y;
    }
};

}  // namespace conway

namespace std {
template <>
struct hash<const conway::Point> {
    size_t operator() (const conway::Point& p) const {
        hash<char> int64_hash;
        char hash = 17;
        hash = 31 * hash + int64_hash(p.x);
        hash = 31 * hash + int64_hash(p.y);
        return hash;
    }
};
}  // namespace std

namespace conway {

class Life {
    protected:
    int64_t generation_;

    public:
    Life() : generation_(0) {}

    int64_t generation() { return generation_; }

    virtual void AddLivePoint(const Point& p) = 0;
    void AddLivePoint(int64_t x, int64_t y) { this->AddLivePoint(Point(x, y)); }
    void Step() {
        generation_ += 1;
        this->DoStep();
    }

    virtual std::vector<const Point> LivePoints() = 0;

    protected:
    virtual void DoStep() = 0;
};

class LiveLife : public Life {
    public:
    LiveLife();
    ~LiveLife();

    void AddLivePoint(const Point& p) override;
    std::vector<const Point> LivePoints() override;

    protected:
    void DoStep() override;

    private:
    bool IsLiveCell(const Point& p);

    std::unique_ptr<std::vector<const Point>> live_points_;
    std::unique_ptr<std::unordered_map<const Point, int>> weights_;
};

class BlockLife : public Life {
    public:
    BlockLife();
    ~BlockLife();

    void AddLivePoint(const Point& p) override;
    std::vector<const Point> LivePoints() override;

    protected:
    void DoStep() override;

    private:
    static const int BLOCK_SHIFT = 5;
    static const int BLOCK_DIM = 1 << BLOCK_SHIFT;
    static const int64_t BLOCK_MASK = ~((1 << BLOCK_SHIFT) - 1);
    typedef std::array<char, BLOCK_DIM * BLOCK_DIM> BlockArray;
    static const BlockArray EMPTY_BLOCK;

    void DoStepForBlock(const Point& p, const BlockArray& block);
    Point toBlockIndex(const Point& p);
    Point toBlockCoordinates(const Point& p);

    // Holds a 32x32 block of points.
    std::unique_ptr<std::unordered_map<const Point, BlockArray>> blocks_;
    std::unique_ptr<std::unordered_map<const Point, BlockArray>> new_blocks_;
};

}  // namespace conway

#endif