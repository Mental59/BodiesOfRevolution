#include "Curve.h"
#include "Tools.h"

void Curve::updateBuffers()
{
    glBindBuffer(GL_ARRAY_BUFFER, this->model.vbo);
    glBufferData(GL_ARRAY_BUFFER, this->points.size() * sizeof(GLfloat), this->points.data(), GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->model.ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLint), this->indices.data(), GL_DYNAMIC_DRAW);

    this->model.indexCount = indices.size();
}

bool Curve::calculateCurvePoints(const std::vector<Point2D>& values)
{
    std::vector<Segment> curve;
    bool res = tbezierSO0(values, curve);

    this->points.clear();
    this->indices.clear();
    this->points2D.clear();

    int k = 0;

    if (values.size() == 2)
        for (int i = 0; i < 2; i++)
        {
            this->indices.push_back(k++);

            this->points.push_back(values[i].x);
            this->points.push_back(values[i].y);

            this->points2D.push_back(Point2D(values[i].x, values[i].y));
        }
    else
        for (Segment& s : curve)
            for (int i = 0; i < RESOLUTION; ++i)
            {
                Point2D p = s.calc((double)i / (double)RESOLUTION);

                this->indices.push_back(k++);

                this->points.push_back(p.x);
                this->points.push_back(p.y);

                this->points2D.push_back(Point2D(p.x, p.y));
            }

    updateBuffers();

    return res;
}

bool Curve::createModel()
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

bool Curve::createShaderProgram()
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
        "    o_color = vec4(0.0, 1.0, 0.0, 1.0);"
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

void Curve::draw(double deltaTime, Matrix4& lookAt, Matrix4& perspective)
{
    glUseProgram(this->shaderProgram);
    glBindVertexArray(this->model.vao);

    Matrix4 s_umv = perspective * lookAt;

    glUniformMatrix4fv(this->uMVP, 1, GL_TRUE, s_umv.elements);

    glDrawElements(GL_LINE_STRIP, this->model.indexCount, GL_UNSIGNED_INT, NULL);
}

void Curve::cleanup()
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
