#include "Points.h"
#include "Tools.h"

void Points::updateBuffers()
{
    glBindBuffer(GL_ARRAY_BUFFER, this->model.vbo);
    glBufferData(GL_ARRAY_BUFFER, this->points.size() * sizeof(GLfloat), this->points.data(), GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->model.ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLint), this->indices.data(), GL_DYNAMIC_DRAW);

    this->model.indexCount = indices.size();
}

void Points::add(Vector2 point)
{
    for (float& element : point.elements)
        this->pointCenters.push_back(element);

    this->point2DCenters.push_back(Point2D(point[0], point[1]));

    float x = point[0], y = point[1];

    this->points.push_back(x - sideLength / 2), this->points.push_back(y - sideLength / 2);
    this->points.push_back(x + sideLength / 2), this->points.push_back(y + sideLength / 2);
    this->points.push_back(x - sideLength / 2), this->points.push_back(y + sideLength / 2);
    this->points.push_back(x + sideLength / 2), this->points.push_back(y - sideLength / 2);

    indices.push_back(this->numberOfDots);
    indices.push_back(this->numberOfDots + 1);
    indices.push_back(this->numberOfDots + 2);
    indices.push_back(this->numberOfDots);
    indices.push_back(this->numberOfDots + 3);
    indices.push_back(this->numberOfDots + 1);

    this->numberOfDots += 4;
    this->numberOfPoints++;

    updateBuffers();
}

void Points::pop()
{
    this->points.erase(this->points.end() - 8, this->points.end());
    this->indices.erase(this->indices.end() - 6, this->indices.end());
    this->point2DCenters.erase(this->point2DCenters.end() - 1, this->point2DCenters.end());
    this->numberOfDots -= 4;
    this->numberOfPoints--;

    updateBuffers();
}

bool Points::createModel()
{
    glGenVertexArrays(1, &this->model.vao);
    glBindVertexArray(this->model.vao);

    glGenBuffers(1, &this->model.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, this->model.vbo);

    glGenBuffers(1, &this->model.ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->model.ibo);

    this->model.indexCount = indices.size();

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (const GLvoid*)0);

    return this->model.vbo != 0 && this->model.ibo != 0 && this->model.vao != 0;
}

bool Points::createShaderProgram()
{
    this->shaderProgram = 0;

    const GLchar vsh[] =
        "#version 330\n"
        ""
        "layout(location = 0) in vec2 a_position;"
        ""
        "uniform mat4 u_mvp;"
        ""
        "void main()"
        "{"
        "    gl_Position = u_mvp * vec4(a_position, 0.0, 1.0);"
        "}"
        ;

    const GLchar fsh[] =
        "#version 330\n"
        ""
        "layout(location = 0) out vec4 o_color;"
        ""
        "void main()"
        "{"
        "    o_color = vec4(1.0, 0.0, 0.0, 1.0);"
        "}"
        ;

    GLuint vertexShader, fragmentShader;

    vertexShader = createShader(vsh, GL_VERTEX_SHADER);
    fragmentShader = createShader(fsh, GL_FRAGMENT_SHADER);

    this->shaderProgram = createProgram(vertexShader, fragmentShader);

    this->uMVP = glGetUniformLocation(this->shaderProgram, "u_mvp");

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return this->shaderProgram != 0;
}

void Points::draw(double deltaTime, Matrix4& lookAt, Matrix4& perspective)
{
    glUseProgram(this->shaderProgram);
    glBindVertexArray(this->model.vao);

    Matrix4 s_umv = perspective * lookAt;

    glUniformMatrix4fv(this->uMVP, 1, GL_TRUE, s_umv.elements);

    glDrawElements(GL_TRIANGLES, this->model.indexCount, GL_UNSIGNED_INT, NULL);
}

void Points::cleanup()
{
    if (this->shaderProgram != 0)
        glDeleteProgram(this->shaderProgram);
    if (this->model.vbo != 0)
        glDeleteBuffers(1, &this->model.vbo);
    if (this->model.ibo != 0)
        glDeleteBuffers(1, &this->model.ibo);
    if (this->model.vao != 0)
        glDeleteVertexArrays(1, &this->model.vao);
}
