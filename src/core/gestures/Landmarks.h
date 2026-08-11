#ifndef LANDMARKS_H
#define LANDMARKS_H

#include <vector>
#include <cmath>

struct Point3D {
    double x;
    double y;
    double z;

    Point3D(double x_ = 0.0, double y_ = 0.0, double z_ = 0.0)
        : x(x_), y(y_), z(z_) {}

    double distanceTo(const Point3D& other) const {
        double dx = x - other.x;
        double dy = y - other.y;
        double dz = z - other.z;
        return std::sqrt(dx * dx + dy * dy + dz * dz);
    }
};

struct Landmarks {
    std::vector<Point3D> points;

    Landmarks() {
        points.reserve(21);
    }
};

#endif // LANDMARKS_H
