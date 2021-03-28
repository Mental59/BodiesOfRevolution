#pragma once
#include "Model.h"
#include <vector>
#include "tbezier.h"
#include "Matrix.h"

class Curve
{
public:
    Model model;
    GLuint shaderProgram;
    GLint uMVP;

    std::vector<GLfloat> points;
    std::vector<GLint> indices;
    std::vector<Point2D> points2D;

    void updateBuffers();

    bool calculateCurvePoints(const std::vector<Point2D>& values);

    bool createModel();

    bool createShaderProgram();

    void draw(double deltaTime, Matrix4& lookAt, Matrix4& perspective);

    void cleanup();

};
