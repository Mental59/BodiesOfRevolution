#pragma once

#include <math.h>
#include <vector>

#define EPSILON 1.0e-5

#define RESOLUTION 16

#define C 2.0

bool IS_ZERO(double v);

int SIGN(double v);

class Point2D
{
public:

    double x, y;

    Point2D();

    Point2D(double _x, double _y);

    Point2D operator +(const Point2D& p) const;

    Point2D operator -(const Point2D& p) const;

    Point2D operator *(double v) const;

    void normalize();

    static Point2D absMin(const Point2D& p1, const Point2D& p2);
};

class Segment
{
public:
    /**
     * Bezier control points.
     */
    Point2D points[4];

    /**
     * Calculate the intermediate curve points.
     *
     * @param t - parameter of the curve, should be in [0; 1].
     * @return intermediate Bezier curve point that corresponds the given parameter.
     */
    Point2D calc(double t);
};

bool tbezierSO0(const std::vector<Point2D>& values, std::vector<Segment>& curve);