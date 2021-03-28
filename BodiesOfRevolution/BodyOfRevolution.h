#pragma once
#include "Model.h"
#include <GLFW/glfw3.h>
#include <vector>
#include "tbezier.h"
#include "Matrix.h"

class BodyOfRevolution
{
public:
    GLuint shaderProgram;
    GLint uMVP;
    GLint uMV;
    GLint uN;
    Model model;

    bool bodyCreated = false;

    bool createShaderProgram();

    bool createModel(const std::vector<Point2D>& points);

    void createBodyOfRevolution(const std::vector<Point2D>& points, Vector3& cameraPos);

    void draw(double deltaTime, Matrix4& perspective, Vector3& cameraPos, Vector3& cameraFront, Vector3& cameraUp);

    void cleanup();
};
