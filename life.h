#ifndef CONWAY_LIFE_H
#define CONWAY_LIFE_H

#include <cstdint>
#include <functional>
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
struct hash<conway::Point> {
    hash<int64_t> int64_hash;

    size_t operator() (const conway::Point& p) const {
        return 31 * int64_hash(p.x) ^ int64_hash(p.y);  // Vaguely stolen from Java.
    }
};
}  // namespace std

namespace conway {

class Life {
    protected:
    int64_t height_;
    int64_t width_;
    int64_t generation_;

    public:
    Life(int64_t height, int64_t width) : height_(height), width_(width), generation_(0) {}

    int64_t generation() { return generation_; }

    virtual void AddLivePoint(const Point& p) = 0;
    void AddLivePoint(int64_t x, int64_t y) { this->AddLivePoint(Point(x, y)); }
    void Step() {
        generation_ += 1;
        this->DoStep();
    }

    virtual std::vector<const Point> LivePoints() = 0;
    virtual void Print() = 0;

    protected:
    virtual void DoStep() = 0;
};

class ArrayLife : public Life  {
    char* matrix_;
    char* temp_;

    public:
    ArrayLife(int64_t height, int64_t width);
    ~ArrayLife();

    void AddLivePoint(const Point& p) override;

    std::vector<const Point> LivePoints() override;
    void Print() override;

    protected:
    void DoStep() override;
};

class LiveLife : public Life {
    std::unique_ptr<std::unordered_set<Point>> live_points_;
    std::unique_ptr<std::unordered_set<Point>> new_live_points_;
    std::unique_ptr<std::unordered_set<Point>> checked_;

    public:
    LiveLife(int64_t height, int64_t width);
    ~LiveLife();

    void AddLivePoint(const Point& p) override;
    std::vector<const Point> LivePoints() override;
    void Print() override;

    protected:
    void DoStep() override;

    private:
    bool IsLiveCell(const Point& p);
};

}  // namespace conway

#endif