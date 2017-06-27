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

    public:
    Life(int64_t height, int64_t width) : height_(height), width_(width) {}

    virtual void AddLivePoint(const Point& p) = 0;
    virtual void Step() = 0;

    virtual std::vector<const Point> LivePoints() = 0;
    virtual void Print() = 0;
};

class ArrayLife : public Life  {
    char* matrix_;
    char* temp_;

    public:
    ArrayLife(int64_t height, int64_t width);
    ~ArrayLife();

    void AddLivePoint(const Point& p) override;
    void Step() override;

    std::vector<const Point> LivePoints() override;
    void Print() override;
};

class LiveLife : public Life {
    std::unordered_set<Point>* live_points_;
    std::unordered_set<Point>* new_live_points_;

    public:
    LiveLife(int64_t height, int64_t width);
    ~LiveLife();

    void AddLivePoint(const Point& p) override;
    void Step() override;
    std::vector<const Point> LivePoints() override;
    void Print() override;

    private:
    bool IsLiveCell(const Point& p);
};

}  // namespace conway

#endif