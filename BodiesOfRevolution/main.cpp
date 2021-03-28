#include <iostream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <chrono>
#include <vector>
#include "Points.h"
#include "Curve.h"
#include "BodyOfRevolution.h"

/*

Control settings:

When creating body:
Enter - start building body of revolution
Left mouse button - make point
BackSpace - remove last point

When body created:
W - move forward
A - move left
D - move right
S - move backward
Q - move down
E - move up
Mouse to look around

*/

using namespace std;

enum class Projection
{
    orthogonal,
    perspective
};

Projection g_proj = Projection::orthogonal;

GLFWwindow* g_window;
int screen_width = 800, screen_height = 600;

GLfloat lastX = screen_width / 2.0f, lastY = screen_height / 2.0f;
GLfloat yaw = -90.0f, pitch = 0.0f;
bool firstMouse = true;

Vector3 cameraPos = Vector3(0.0f, 0.0f, 1.0f);
Vector3 cameraFront = Vector3(0.0f, 0.0f, -1.0f);
Vector3 cameraUp = Vector3(0.0f, 1.0f, 0.0f);

chrono::time_point<chrono::system_clock> g_callTime;

bool keys[1024];

Points points;
Curve curve;
BodyOfRevolution bodyOfRevolution;

GLuint createShader(const GLchar* code, GLenum type);

GLuint createProgram(GLuint vsh, GLuint fsh);

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

Matrix4 createProjectionMatrix(float far, float near, float fov, int width, int height, Projection proj);

Matrix4 g_P = createProjectionMatrix(100.0f, 0.1f, 40.0f, screen_width, screen_height, g_proj);

int main()
{
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

bool init()
{
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);

    glEnable(GL_DEPTH_TEST);

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
    // Clear color buffer.
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    bodyOfRevolution.draw(deltaTime, g_P, cameraPos, cameraFront, cameraUp);
    
    if(!bodyOfRevolution.bodyCreated)
    {
        Matrix4 lookAt = createLookAtMatrix(cameraPos, cameraPos + cameraFront, cameraUp);
        points.draw(deltaTime, lookAt, g_P);
        curve.draw(deltaTime, lookAt, g_P);
    }  
}

void cleanup()
{
    bodyOfRevolution.cleanup();
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

    if (key == GLFW_KEY_ENTER && action == GLFW_PRESS)
    {     
        if (!bodyOfRevolution.bodyCreated)
        {
            bodyOfRevolution.createBodyOfRevolution(curve.points2D, cameraPos);
            if (bodyOfRevolution.bodyCreated)
            {
                g_proj = Projection::perspective;
                g_P = createProjectionMatrix(200.0f, 0.1f, 40.0f, screen_width, screen_height, g_proj);
                glfwSetCursorPosCallback(g_window, mouse_callback);
                glfwSetInputMode(g_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                glfwSetMouseButtonCallback(g_window, NULL);
            }
        }      
    }

    if (key == GLFW_KEY_BACKSPACE && action == GLFW_PRESS)
    {
        if (points.numberOfPoints >= 1)
        {
            points.pop();
            curve.calculateCurvePoints(points.point2DCenters);
        }
    }

    if (bodyOfRevolution.bodyCreated)
    {
        if (action == GLFW_PRESS)
            keys[key] = true;
        else if (action == GLFW_RELEASE)
            keys[key] = false;
    } 
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        double xpos, ypos;
        glfwGetCursorPos(g_window, &xpos, &ypos);
        float sx = xpos;
        float sy = ((float)screen_height - ypos);
        points.add(Vector2(sx, sy));

        curve.calculateCurvePoints(points.point2DCenters);
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
    GLfloat yoffset = lastY - ypos; // Обратный порядок вычитания потому что оконные Y-координаты возрастают с верху вниз 
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

void do_movement(double deltaTime)
{
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
