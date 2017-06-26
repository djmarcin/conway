#include <memory>
#include <iostream>

#include <unistd.h>

#include "life.h"

int main(int argc, char** argv) {
    std::cout << "Starting Conway!" << std::endl;

    //std::unique_ptr<conway::Life> life(new conway::ArrayLife(256, 256));
    std::unique_ptr<conway::Life> life(new conway::LiveLife(1L<<40, 1L<<40));
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
    // R-pentomino
    life->AddLivePoint(conway::Point(3,1));
    life->AddLivePoint(conway::Point(3,2));
    life->AddLivePoint(conway::Point(3,3));
    life->AddLivePoint(conway::Point(4,3));
    life->AddLivePoint(conway::Point(2,2));
    for (int i = 0; i < 10000; i++) {
        life->Step(); //life->Print(); usleep(100000);
    }
    std::vector<const conway::Point> live_points = life->LivePoints();
    for (auto p : live_points) {
        std::cout << "LivePoint: (" << p.x << "," << p.y << ")" << std::endl;
    }

    return 0;
}
