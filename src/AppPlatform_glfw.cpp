#include "AppPlatform_glfw.h"

float AppPlatform_glfw::getPixelsPerMillimeter() {
#ifdef __EMSCRIPTEN__
    return 10.0f;
#else
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    if (!monitor) return 10.0f;

    int width_mm = 0, height_mm = 0;
    glfwGetMonitorPhysicalSize(monitor, &width_mm, &height_mm);
    if (width_mm <= 0) return 10.0f;

    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    if (!mode || mode->width <= 0) return 10.0f;

    return (float)mode->width / (float)width_mm;
#endif
}