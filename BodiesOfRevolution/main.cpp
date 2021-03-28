#include <iostream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <chrono>
#include "Matrix.h"
#include "Vector.h"
#include <vector>
#include "tbezier.h"

using namespace std;

GLFWwindow* g_window;
int screen_width = 800, screen_height = 600;

GLfloat lastX = screen_width / 2.0f, lastY = screen_height / 2.0f;
GLfloat yaw = -90.0f, pitch = 0.0f;
bool firstMouse = true;

bool to_rotate = true;

GLuint g_shaderProgram;
GLint g_uMVP;
GLint g_uMV;
GLint g_uN;

chrono::time_point<chrono::system_clock> g_callTime;

class Model
{
public:
    GLuint vbo;
    GLuint ibo;
    GLuint vao;
    GLsizei indexCount;
};

Model g_model;

bool keys[1024];
bool mouse_buttons[32];

Vector3 cameraPos = Vector3(0.0f, 0.0f, 1.0f);
Vector3 cameraFront = Vector3(0.0f, 0.0f, -1.0f);
Vector3 cameraUp = Vector3(0.0f, 1.0f, 0.0f);

GLuint createShader(const GLchar* code, GLenum type);

GLuint createProgram(GLuint vsh, GLuint fsh);

bool createShaderProgram();

bool createModel();

bool init();

void reshape(GLFWwindow* window, int width, int height);

void draw(double deltaTime);

void cleanup();

bool initOpenGL();

void tearDownOpenGL();

GLfloat to_radians(GLfloat degrees);

GLfloat to_degrees(GLfloat radians);

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);

void do_movement(double deltaTime);

void mouse_callback(GLFWwindow* window, double xpos, double ypos);

void createBodyOfRevolution();

enum class Projection
{
    orthogonal, 
    perspective
};

Projection g_proj = Projection::orthogonal;
bool bodieOfRevoultionCreated = false;

Matrix4 createProjectionMatrix(float far, float near, float fov, int width, int height, Projection proj);

Matrix4 g_P = createProjectionMatrix(100.0f, 0.1f, 40.0f, screen_width, screen_height, g_proj);

class Points
{
public:
    Model model;
    GLuint shaderProgram;
    GLint uMVP;

    vector<GLfloat> pointCenters; // центры точек

    vector<Point2D> point2DCenters; // центры точек, описанных как класс Point2D(x, y)

    vector<GLfloat> points; // точки аппроксимации точки, т.к. кажда€ точка аппроксимируетс€ квадратом

    vector<GLint> indices; // индексы дл€ отрисовки аппроксимированных точек

    float sideLength = 7.5f;
    int numberOfDots = 0;
    int numberOfPoints = 0;

    void add(Vector2 point)
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

        glBindBuffer(GL_ARRAY_BUFFER, this->model.vbo);
        glBufferData(GL_ARRAY_BUFFER, this->points.size() * sizeof(GLfloat), this->points.data(), GL_DYNAMIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->model.ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLint), this->indices.data(), GL_DYNAMIC_DRAW);

        this->model.indexCount = indices.size();

        /*cout << "Point centers:" << endl;
        for (int i = 0; i < this->pointCenters.size(); i+=2)
        {
            cout << "( " << this->pointCenters[i] << ", " << this->pointCenters[i + 1] << " )" << endl;
        }
        cout << endl;

        cout << "Points:" << endl;
        for (int i = 0; i < this->points.size(); i += 2)
        {
            cout << "( " << this->points[i] << ", " << this->points[i + 1] << " )" << endl;
        }
        cout << endl;

        cout << "Point 2D centers:" << endl;
        for (Point2D& p : this->point2DCenters)
            p.show();
        cout << endl;

        cout << "Indices:" << endl;
        for (int i = 0; i < this->indices.size(); i++)
        {
            cout << this->indices[i] << endl;
        }
        cout << endl;*/
    }

    bool createModel()
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

    bool createShaderProgram()
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

    void draw(double deltaTime, Matrix4 lookAt)
    {
        glUseProgram(this->shaderProgram);
        glBindVertexArray(this->model.vao);

        Matrix4 s_umv = g_P * lookAt;

        glUniformMatrix4fv(this->uMVP, 1, GL_TRUE, s_umv.elements);

        glDrawElements(GL_TRIANGLES, this->model.indexCount, GL_UNSIGNED_INT, NULL);
    }

    void cleanup()
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
};

class Curve
{
public:
    Model model;
    GLuint shaderProgram;
    GLint uMVP;

    vector<GLfloat> points; // точки кривой
    vector<GLint> indices; // индексы дл€ отрисовки кривых
    vector<Point2D> points2D;

    bool calculateCurvePoints(const vector<Point2D>& values)
    {
        vector<Segment> curve;
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
        
        //cout << "Point count = " << this->pointCount << endl;
        //cout << this->points.size() << endl;

        glBindBuffer(GL_ARRAY_BUFFER, this->model.vbo);
        glBufferData(GL_ARRAY_BUFFER, this->points.size() * sizeof(GLfloat), this->points.data(), GL_DYNAMIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->model.ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLint), this->indices.data(), GL_DYNAMIC_DRAW);

        this->model.indexCount = indices.size();

        return res;
    }

    bool createModel()
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

    bool createShaderProgram()
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

    void draw(double deltaTime, Matrix4 lookAt)
    {
        glUseProgram(this->shaderProgram);
        glBindVertexArray(this->model.vao);

        Matrix4 s_umv = g_P * lookAt;

        glUniformMatrix4fv(this->uMVP, 1, GL_TRUE, s_umv.elements);

        glDrawElements(GL_LINE_STRIP, this->model.indexCount, GL_UNSIGNED_INT, NULL);
    }

    void cleanup()
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

};

Points points;
Curve curve;

int main()
{
    /*const int n = 4;

    int* indices = new int[(n - 1) * n * 6];

    int i, k;
    for (i = 0, k = 0; i < (n - 1) * (n - 1) * 6; k++)
    {
        for (int count = 0; count < n - 1; count++, i += 6, k++)
        {
            indices[i] = k;
            indices[i + 1] = k + n;
            indices[i + 2] = k + 1;
            indices[i + 3] = k + n;
            indices[i + 4] = k + n + 1;
            indices[i + 5] = k + 1;
            cout << indices[i] << " " << indices[i + 1] << " " << indices[i + 2] << " " << indices[i + 3] << " " << indices[i + 4] << " " << indices[i + 5] << " | ";
        }
        cout << endl;
    }

    for (int cnt = 0, k = n * (n - 1); cnt < n - 1; cnt++, i += 6)
    {
        indices[i] = cnt;
        indices[i + 1] = cnt + k + 1;
        indices[i + 2] = cnt + k;
        indices[i + 3] = cnt;
        indices[i + 4] = cnt + 1;
        indices[i + 5] = cnt + k + 1;
        cout << indices[i] << " " << indices[i + 1] << " " << indices[i + 2] << " " << indices[i + 3] << " " << indices[i + 4] << " " << indices[i + 5] << " | ";
    }
    cout << endl;

    delete[] indices;*/

    // Initialize OpenGL
    if (!initOpenGL())
        return -1;

    // Initialize graphical resources.
    bool isOk = init();

    if (isOk)
    {

        glfwSetKeyCallback(g_window, key_callback);
        glfwSetMouseButtonCallback(g_window, mouse_button_callback);

        g_callTime = chrono::system_clock::now();

        // Main loop until window closed or escape pressed.
        while (!glfwWindowShouldClose(g_window))
        {
            auto callTime = chrono::system_clock::now();
            chrono::duration<double> elapsed = callTime - g_callTime;
            g_callTime = callTime;

            double deltaTime = elapsed.count();
            // Draw scene.
            draw(deltaTime);

            // Swap buffers.
            glfwSwapBuffers(g_window);
            // Poll window events.
            glfwPollEvents();

            do_movement(deltaTime);
        }
    }

    // Cleanup graphical resources.
    cleanup();

    // Tear down OpenGL.
    tearDownOpenGL();

    return isOk ? 0 : -1;
}

GLuint createShader(const GLchar* code, GLenum type)
{
    GLuint result = glCreateShader(type);

    glShaderSource(result, 1, &code, NULL);
    glCompileShader(result);

    GLint compiled;
    glGetShaderiv(result, GL_COMPILE_STATUS, &compiled);

    if (!compiled)
    {
        GLint infoLen = 0;
        glGetShaderiv(result, GL_INFO_LOG_LENGTH, &infoLen);
        if (infoLen > 0)
        {
            char* infoLog = (char*)alloca(infoLen);
            glGetShaderInfoLog(result, infoLen, NULL, infoLog);
            cout << "Shader compilation error" << endl << infoLog << endl;
        }
        glDeleteShader(result);
        return 0;
    }

    return result;
}

GLuint createProgram(GLuint vsh, GLuint fsh)
{
    GLuint result = glCreateProgram();

    glAttachShader(result, vsh);
    glAttachShader(result, fsh);

    glLinkProgram(result);

    GLint linked;
    glGetProgramiv(result, GL_LINK_STATUS, &linked);

    if (!linked)
    {
        GLint infoLen = 0;
        glGetProgramiv(result, GL_INFO_LOG_LENGTH, &infoLen);
        if (infoLen > 0)
        {
            char* infoLog = (char*)alloca(infoLen);
            glGetProgramInfoLog(result, infoLen, NULL, infoLog);
            cout << "Shader program linking error" << endl << infoLog << endl;
        }
        glDeleteProgram(result);
        return 0;
    }

    return result;
}

bool createShaderProgram()
{
    g_shaderProgram = 0;

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
        "   vec3 color = vec3(0.23, 0.75, 0.21);"
        ""
        "   vec3 E = vec3(0.0, 0.0, 0.0);"
        "   vec3 L = vec3(5.0, 5.0, 0.0);"
        "   float S = 90.0;"
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
        "}"
        ;

    GLuint vertexShader, fragmentShader;

    vertexShader = createShader(vsh, GL_VERTEX_SHADER);
    fragmentShader = createShader(fsh, GL_FRAGMENT_SHADER);

    g_shaderProgram = createProgram(vertexShader, fragmentShader);

    g_uMVP = glGetUniformLocation(g_shaderProgram, "u_mvp");
    g_uMV = glGetUniformLocation(g_shaderProgram, "u_mv");
    g_uN = glGetUniformLocation(g_shaderProgram, "u_n");

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return g_shaderProgram != 0;
}

bool createModel()
{
    const int n = curve.points2D.size();
    const int indices_size = (n - 1) * n * 6;

    Matrix4 rotate = createRotateXMatrix(360.0f / n);

    GLfloat* vertices = new GLfloat[6 * n * n];

    GLuint* indices = new GLuint[indices_size];

    // ¬ычисление нормалей дл€ кривой
    Point2D v = curve.points2D[1] - curve.points2D[0];
    v.normalize();

    vertices[0] = curve.points2D[0].x;
    vertices[1] = curve.points2D[0].y;
    vertices[2] = 0.0f;
    vertices[3] = -v.y;
    vertices[4] = v.x;
    vertices[5] = 0.0f;

    v = curve.points2D[n - 2] - curve.points2D[n - 1];
    v.normalize();

    vertices[6 * n * n - 6] = curve.points2D[n - 1].x;
    vertices[6 * n * n - 5] = curve.points2D[n - 1].y;
    vertices[6 * n * n - 4] = 0.0f;
    vertices[6 * n * n - 3] = v.y;
    vertices[6 * n * n - 2] = -v.x;
    vertices[6 * n * n - 1] = 0.0f;

    for (int i = 6, k = 1; i < (n - 1) * 6; i += 6, k++)
    {
        Point2D v1 = curve.points2D[k] - curve.points2D[k - 1];
        v1.normalize();

        Point2D v2 = curve.points2D[k + 1] - curve.points2D[k];
        v2.normalize();

        Point2D sumV = v1 + v2;
        sumV.normalize();

        vertices[i] = curve.points2D[k].x;
        vertices[i + 1] = curve.points2D[k].y;
        vertices[i + 2] = 0.0f;
        vertices[i + 3] = -sumV.y;
        vertices[i + 4] = sumV.x;
        vertices[i + 5] = 0.0f;
    }

    // ѕостроение сетки
    for (int i = 6 * n; i < n * n * 6; i += 6 * n)
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

    // ѕостроение индексов отрисовки
    int i, k;
    for (i = 0, k = 0; i < (n - 1) * (n - 1) * 6; k++)
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

    for (int cnt = 0, k = n * (n - 1); cnt < n - 1; cnt++, i += 6)
    {
        indices[i] = cnt;
        indices[i + 1] = cnt + k + 1;
        indices[i + 2] = cnt + k;
        indices[i + 3] = cnt;
        indices[i + 4] = cnt + 1;
        indices[i + 5] = cnt + k + 1;
    }

    glGenVertexArrays(1, &g_model.vao);
    glBindVertexArray(g_model.vao);

    glGenBuffers(1, &g_model.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, g_model.vbo);
    glBufferData(GL_ARRAY_BUFFER, 6 * n * n * sizeof(GLfloat), vertices, GL_STATIC_DRAW);

    glGenBuffers(1, &g_model.ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_model.ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_size * sizeof(GLuint), indices, GL_STATIC_DRAW);

    delete[] vertices;
    delete[] indices;

    g_model.indexCount = indices_size;

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (const GLvoid*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (const GLvoid*)(3 * sizeof(GLfloat)));

    return g_model.vbo != 0 && g_model.ibo != 0 && g_model.vao != 0;
}

void createBodyOfRevolution()
{
    if (curve.points2D.size() >= 3)
    {
        g_proj = Projection::perspective;
        g_P = createProjectionMatrix(200.0f, 0.1f, 40.0f, screen_width, screen_height, g_proj);
        bodieOfRevoultionCreated = createShaderProgram() && createModel();
        if (bodieOfRevoultionCreated)
        {
            cameraPos[2] = 60.0f;
            glfwSetCursorPosCallback(g_window, mouse_callback);
            glfwSetInputMode(g_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }
}

bool init()
{
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);

    glEnable(GL_DEPTH_TEST);

    //bool mainProgramCreated = createShaderProgram() && createModel();
    bool pointsProgramCreated = points.createShaderProgram() && points.createModel();
    bool curveProgramCreated = curve.createShaderProgram() && curve.createModel();

    return pointsProgramCreated;
}

void reshape(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);

    screen_width = width;
    screen_height = height;

    g_P = createProjectionMatrix(200.0f, 0.1f, 40.0f, screen_width, screen_height, g_proj);
}

void draw(double deltaTime)
{
    // Draw main scene
    static float rotationAngle = 0.0f;

    // Clear color buffer.
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (bodieOfRevoultionCreated)
    {
        glUseProgram(g_shaderProgram);
        glBindVertexArray(g_model.vao);

        static Matrix4 scale = createScaleMatrix(0.05, 0.05, 0.05);

        Matrix4 M = createRotateXMatrix(to_degrees(rotationAngle)) *
            createRotateZMatrix(to_degrees(rotationAngle)) * scale;

        Matrix4 V = createLookAtMatrix(cameraPos, cameraPos + cameraFront, cameraUp);

        Matrix4 MV = V * M;
        Matrix4 MVP = g_P * MV;
        Matrix3 N = getMainMinor(MV);

        glUniformMatrix4fv(g_uMVP, 1, GL_TRUE, MVP.elements);
        glUniformMatrix4fv(g_uMV, 1, GL_TRUE, MV.elements);
        glUniformMatrix3fv(g_uN, 1, GL_TRUE, N.elements);

        glDrawElements(GL_TRIANGLES, g_model.indexCount, GL_UNSIGNED_INT, NULL);

        if (to_rotate)
            rotationAngle = fmodf(rotationAngle + deltaTime, 2.0f * PI);
    }
    else
    {
        Matrix4 lookAt = createLookAtMatrix(cameraPos, cameraPos + cameraFront, cameraUp);

        points.draw(deltaTime, lookAt);
        curve.draw(deltaTime, lookAt);
    }  
}

void cleanup()
{
    if (g_shaderProgram != 0)
        glDeleteProgram(g_shaderProgram);
    if (g_model.vbo != 0)
        glDeleteBuffers(1, &g_model.vbo);
    if (g_model.ibo != 0)
        glDeleteBuffers(1, &g_model.ibo);
    if (g_model.vao != 0)
        glDeleteVertexArrays(1, &g_model.vao);

    points.cleanup();
    curve.cleanup();
}

bool initOpenGL()
{
    // Initialize GLFW functions.
    if (!glfwInit())
    {
        cout << "Failed to initialize GLFW" << endl;
        return false;
    }

    // Request OpenGL 3.3 without obsoleted functions.
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create window.
    g_window = glfwCreateWindow(screen_width, screen_height, "Bodies of Revolution OpenGL", NULL, NULL);
    if (g_window == NULL)
    {
        cout << "Failed to open GLFW window" << endl;
        glfwTerminate();
        return false;
    }

    // Initialize OpenGL context with.
    glfwMakeContextCurrent(g_window);

    // Set internal GLEW variable to activate OpenGL core profile.
    glewExperimental = true;

    // Initialize GLEW functions.
    if (glewInit() != GLEW_OK)
    {
        cout << "Failed to initialize GLEW" << endl;
        return false;
    }

    // Ensure we can capture the escape key being pressed.
    glfwSetInputMode(g_window, GLFW_STICKY_KEYS, GL_TRUE);

    // Set callback for framebuffer resizing event.
    glfwSetFramebufferSizeCallback(g_window, reshape);

    return true;
}

void tearDownOpenGL()
{
    // Terminate GLFW.
    glfwTerminate();
}

GLfloat to_radians(GLfloat degrees)
{
    return PI / 180.0f * degrees;
}

GLfloat to_degrees(GLfloat radians)
{
    return 180.0f / PI * radians;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
        to_rotate = !to_rotate;

    if (key == GLFW_KEY_ENTER && action == GLFW_PRESS)
        createBodyOfRevolution();

    if (action == GLFW_PRESS)
        keys[key] = true;
    else if (action == GLFW_RELEASE)
        keys[key] = false;
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (action == GLFW_PRESS)
        mouse_buttons[button] = true;
    else if (action == GLFW_RELEASE)
        mouse_buttons[button] = false;

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        double xpos, ypos;
        glfwGetCursorPos(g_window, &xpos, &ypos);
        float sx = xpos;
        float sy = ((float)screen_height - ypos);
        points.add(Vector2(sx, sy));

        if (points.numberOfPoints >= 2)
            curve.calculateCurvePoints(points.point2DCenters);
    }
}

void do_movement(double deltaTime)
{
    // Camera controls
    GLfloat cameraSpeed = 40.0f * deltaTime;

    if (keys[GLFW_KEY_W])
        cameraPos = cameraPos + cameraSpeed * cameraFront;
    if (keys[GLFW_KEY_S])
        cameraPos = cameraPos - cameraSpeed * cameraFront;
    if (keys[GLFW_KEY_A])
        cameraPos = cameraPos - Vector3::normalize(Vector3::cross(cameraFront, cameraUp)) * cameraSpeed;
    if (keys[GLFW_KEY_D])
        cameraPos = cameraPos + Vector3::normalize(Vector3::cross(cameraFront, cameraUp)) * cameraSpeed;
    if (keys[GLFW_KEY_Q])
        cameraPos = cameraPos - cameraUp * cameraSpeed;
    if (keys[GLFW_KEY_E])
        cameraPos = cameraPos + cameraUp * cameraSpeed;
}

Matrix4 createProjectionMatrix(float far, float near, float fov, int width, int height, Projection proj)
{
    switch (proj)
    {
    case Projection::orthogonal: 
        return createParallelProjectionMatrix(far, near, fov, width, height);
        break;
    case Projection::perspective: 
        return createPerspectiveProjectionMatrix(far, near, fov, width, height);
        break;
    }
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    GLfloat xoffset = xpos - lastX;
    GLfloat yoffset = lastY - ypos; // ќбратный пор€док вычитани€ потому что оконные Y-координаты возрастают с верху вниз 
    lastX = xpos;
    lastY = ypos;

    GLfloat sensitivity = 0.075f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw = fmodf(yaw + xoffset, 360.0f);
    pitch += yoffset;

    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    Vector3 front = Vector3(
        cos(to_radians(yaw)) * cos(to_radians(pitch)),
        sin(to_radians(pitch)),
        sin(to_radians(yaw)) * cos(to_radians(pitch))
    );

    cameraFront = Vector3::normalize(front);
}
