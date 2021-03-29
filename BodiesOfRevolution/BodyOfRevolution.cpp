#include "BodyOfRevolution.h"
#include "Tools.h"
#include "Vector.h"

bool BodyOfRevolution::createShaderProgram()
{
    this->shaderProgram = 0;

    const GLchar vsh[] =
        "#version 330\n"
        ""
        "layout(location = 0) in vec3 a_position;"
        "layout(location = 1) in vec3 a_normal;"
        ""
        "uniform mat4 u_mvp;"
        "uniform mat4 u_mv;"
        "uniform mat3 u_n;"
        ""
        "out vec3 v_normal;"
        "out vec3 v_position;"
        ""
        "void main()"
        "{"
        "   vec4 p0 = vec4(a_position, 1.0);"
        "   v_normal = transpose(inverse(u_n)) * normalize(a_normal);"
        "   v_position = vec3(u_mv * p0);"
        "   gl_Position = u_mvp * p0;"
        "}"
        ;

    const GLchar fsh[] =
        "#version 330\n"
        ""
        "in vec3 v_normal;"
        "in vec3 v_position;"
        ""
        "layout(location = 0) out vec4 o_color;"
        ""
        "void main()"
        "{"
        "   vec3 color = vec3(0.0, 1.0, 0.13);"
        ""
        "   vec3 E = vec3(0.0, 0.0, 0.0);"
        "   vec3 L = vec3(5.0, 5.0, 0.0);"
        "   float S = 64.0;"
        ""
        "   vec3 n = normalize(v_normal);"
        "   vec3 l = normalize(L - v_position);"
        ""
        "   float d = max(dot(n, l), 0.3);"
        ""
        "   vec3 e = normalize(E - v_position);"
        "   vec3 h = normalize(l + e);"
        ""
        "   float s = pow(max(dot(n, h), 0.0), S);"
        ""
        "   o_color = vec4(color * d + s * vec3(1.0, 1.0, 1.0), 1.0);"
        "   o_color.rgb = pow(o_color.rgb, vec3(1.0 / 2.2));"
        "}"
        ;

    GLuint vertexShader, fragmentShader;

    vertexShader = createShader(vsh, GL_VERTEX_SHADER);
    fragmentShader = createShader(fsh, GL_FRAGMENT_SHADER);

    this->shaderProgram = createProgram(vertexShader, fragmentShader);

    this->uMVP = glGetUniformLocation(this->shaderProgram, "u_mvp");
    this->uMV = glGetUniformLocation(this->shaderProgram, "u_mv");
    this->uN = glGetUniformLocation(this->shaderProgram, "u_n");

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return this->shaderProgram != 0;
}

bool BodyOfRevolution::createModel(const std::vector<Point2D>& points)
{
    const int revolutions = 128;

    const int n = points.size();

    const int indicesCount = (n - 1) * revolutions * 6;
    const int verticesCount = 6 * n * revolutions;

    Matrix4 rotate = createRotateXMatrix(360.0f / revolutions);

    GLfloat* vertices = new GLfloat[verticesCount];
    GLuint* indices = new GLuint[indicesCount];

    Point2D v = points[1] - points[0];
    v.normalize();

    vertices[0] = points[0].x;
    vertices[1] = points[0].y;
    vertices[2] = 0.0f;
    vertices[3] = -v.y;
    vertices[4] = v.x;
    vertices[5] = 0.0f;

    v = points[n - 2] - points[n - 1];
    v.normalize();

    vertices[6 * n - 6] = points[n - 1].x;
    vertices[6 * n - 5] = points[n - 1].y;
    vertices[6 * n - 4] = 0.0f;
    vertices[6 * n - 3] = v.y;
    vertices[6 * n - 2] = -v.x;
    vertices[6 * n - 1] = 0.0f;

    for (int i = 6, k = 1; i < (n - 1) * 6; i += 6, k++)
    {
        Point2D v1 = points[k] - points[k - 1];
        v1.normalize();

        Point2D v2 = points[k + 1] - points[k];
        v2.normalize();

        Point2D sumV = v1 + v2;
        sumV.normalize();

        vertices[i] = points[k].x;
        vertices[i + 1] = points[k].y;
        vertices[i + 2] = 0.0f;
        vertices[i + 3] = -sumV.y;
        vertices[i + 4] = sumV.x;
        vertices[i + 5] = 0.0f;
    }

    for (int i = 6 * n; i < verticesCount; i += 6 * n)
    {
        for (int j = 0; j < n * 6; j += 6)
        {
            Vector4 prevPoint = Vector4(vertices[i + j - 6 * n], vertices[i + j - 6 * n + 1], vertices[i + j - 6 * n + 2], 1.0f);
            Vector4 prevNormal = Vector4(vertices[i + j - 6 * n + 3], vertices[i + j - 6 * n + 4], vertices[i + j - 6 * n + 5], 1.0f);

            prevPoint = rotate * prevPoint;
            prevNormal = rotate * prevNormal;

            vertices[i + j] = prevPoint[0];
            vertices[i + j + 1] = prevPoint[1];
            vertices[i + j + 2] = prevPoint[2];
            vertices[i + j + 3] = prevNormal[0];
            vertices[i + j + 4] = prevNormal[1];
            vertices[i + j + 5] = prevNormal[2];
        }
    }

    int i, k;
    for (i = 0, k = 0; i < (n - 1) * (revolutions - 1) * 6; k++)
    {
        for (int count = 0; count < n - 1; count++, i += 6, k++)
        {
            indices[i] = k;
            indices[i + 1] = k + n;
            indices[i + 2] = k + 1;
            indices[i + 3] = k + n;
            indices[i + 4] = k + n + 1;
            indices[i + 5] = k + 1;
        }
    }

    for (int cnt = 0, k = n * (revolutions - 1); cnt < n - 1; cnt++, i += 6)
    {
        indices[i] = cnt;
        indices[i + 1] = cnt + k + 1;
        indices[i + 2] = cnt + k;
        indices[i + 3] = cnt;
        indices[i + 4] = cnt + 1;
        indices[i + 5] = cnt + k + 1;
    }

    glGenVertexArrays(1, &this->model.vao);
    glBindVertexArray(this->model.vao);

    glGenBuffers(1, &this->model.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, this->model.vbo);
    glBufferData(GL_ARRAY_BUFFER, verticesCount * sizeof(GLfloat), vertices, GL_STATIC_DRAW);

    glGenBuffers(1, &this->model.ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->model.ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesCount * sizeof(GLuint), indices, GL_STATIC_DRAW);

    delete[] vertices;
    delete[] indices;

    this->model.indexCount = indicesCount;

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (const GLvoid*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (const GLvoid*)(3 * sizeof(GLfloat)));

    return this->model.vbo != 0 && this->model.ibo != 0 && this->model.vao != 0;
}

void BodyOfRevolution::createBodyOfRevolution(const std::vector<Point2D>& points, Vector3& cameraPos)
{
    if (points.size() >= 2)
    {        
        this->bodyCreated = createShaderProgram() && createModel(points);
        if (this->bodyCreated)
        {
            cameraPos[2] = 60.0f;
        }
    }
}

void BodyOfRevolution::draw(double deltaTime, Matrix4& perspective, Vector3& cameraPos, Vector3& cameraFront, Vector3& cameraUp)
{
    if (!this->bodyCreated)
        return;

    static float rotationAngle = 0.0f;

    glUseProgram(this->shaderProgram);
    glBindVertexArray(this->model.vao);

    static Matrix4 scale = createScaleMatrix(0.05, 0.05, 0.05);
    static Matrix4 initialRotate = createRotateZMatrix(-90.0f);

    Matrix4 M = initialRotate * /*createRotateXMatrix(to_degrees(rotationAngle)) *
        createRotateZMatrix(to_degrees(rotationAngle)) **/ scale;

    Matrix4 V = createLookAtMatrix(cameraPos, cameraPos + cameraFront, cameraUp);

    Matrix4 MV = V * M;
    Matrix4 MVP = perspective * MV;
    Matrix3 N = getMainMinor(MV);

    glUniformMatrix4fv(this->uMVP, 1, GL_TRUE, MVP.elements);
    glUniformMatrix4fv(this->uMV, 1, GL_TRUE, MV.elements);
    glUniformMatrix3fv(this->uN, 1, GL_TRUE, N.elements);

    glDrawElements(GL_TRIANGLES, this->model.indexCount, GL_UNSIGNED_INT, NULL);

    /*if (to_rotate)
        rotationAngle = fmodf(rotationAngle + deltaTime, 2.0f * PI);*/
}

void BodyOfRevolution::cleanup()
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
