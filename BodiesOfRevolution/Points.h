#pragma once
#include <GL/glew.h>
#include "Model.h"
#include <vector>
#include "tbezier.h"
#include "Vector.h"
#include "Matrix.h"

class Points
{
public:
    Model model;
    GLuint shaderProgram;
    GLint uMVP;

    std::vector<GLfloat> pointCenters; // центры точек

    std::vector<Point2D> point2DCenters; // центры точек, описанных как класс Point2D(x, y)

    std::vector<GLfloat> points; // точки аппроксимации точки, т.к. каждая точка аппроксимируется квадратом

    std::vector<GLint> indices; // индексы для отрисовки аппроксимированных точек

    float sideLength = 7.5f;
    int numberOfDots = 0;
    int numberOfPoints = 0;

    void updateBuffers();

    void add(Vector2 point);

    void pop();

    bool createModel();

    bool createShaderProgram();

    void draw(double deltaTime, Matrix4& lookAt, Matrix4& perspective);

    void cleanup();
};
