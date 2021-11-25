#include <GLFW/glfw3.h>

#include <glad/glad.h>

#include <glm/glm.hpp>

#include <glm/gtc/matrix_transform.hpp>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <iostream>
#include <memory>

#include <cstdlib>

#include "Terrain.h"

namespace {

void
updateViewport(GLFWwindow* window)
{
  int w = 0;
  int h = 0;
  glfwGetFramebufferSize(window, &w, &h);
  glViewport(0, 0, w, h);
}

} // namespace

int
main(int argc, char** argv)
{
  if (glfwInit() != GLFW_TRUE) {
    std::cerr << "Failed to initialize GLFW." << std::endl;
    return EXIT_FAILURE;
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_ANY_PROFILE);
  glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);

  GLFWwindow* window = glfwCreateWindow(1280, 720, "TinyErode Viewer", nullptr, nullptr);
  if (!window) {
    std::cerr << "Failed to initialize GLFW." << std::endl;
    glfwTerminate();
    return EXIT_FAILURE;
  }

  glfwMakeContextCurrent(window);

  gladLoadGLES2Loader((GLADloadproc)glfwGetProcAddress);

  glfwSwapInterval(1);

  glEnable(GL_DEPTH_TEST);

  glClearColor(0, 0, 0, 1);

  updateViewport(window);

  std::unique_ptr<Terrain> terrain;

  IMGUI_CHECKVERSION();

  ImGui::CreateContext();

  ImGui_ImplGlfw_InitForOpenGL(window, true);

  ImGui_ImplOpenGL3_Init("#version 300 es");

  const float near = 0.1;
  const float far = 5000;

  int genTerrainWidth = 1023;
  int genTerrainHeight = 1023;

  int randomizeSeed = 0;

  float camAzimuth = 0;
  float camAltitude = glm::radians(45.0f);
  float camDistance = 1000;
  float camFov = glm::radians(45.0f);

  auto getCamPosition = [&camAzimuth, &camAltitude, &camDistance]() -> glm::vec3 {
    const float x = camDistance * std::cos(camAzimuth) * std::sin(camAltitude);
    const float y = camDistance * std::sin(camAzimuth) * std::sin(camAltitude);
    const float z = camDistance * std::cos(camAltitude);
    return glm::vec3(y, z, -x);
  };

  while (!glfwWindowShouldClose(window)) {

    glfwPollEvents();

    int w = 0;
    int h = 0;
    glfwGetFramebufferSize(window, &w, &h);
    const float aspect = float(w) / h;

    const glm::mat4 proj = glm::perspective(camFov, aspect, near, far);
    const glm::mat4 view = glm::lookAt(getCamPosition(), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
    const glm::mat4 mvp = proj * view;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (terrain)
      terrain->render(mvp);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Options");

    ImGui::SliderInt("Terrain Width", &genTerrainWidth, 1, 4095);
    ImGui::SliderInt("Terrain Height", &genTerrainHeight, 1, 4095);

    if (ImGui::Button("Generate Initial Terrain")) {

      terrain.reset(new Terrain(genTerrainWidth, genTerrainHeight));

      terrain->compileShaders(std::cerr);
    }

    ImGui::Separator();

    ImGui::SliderInt("Randomize Seed", &randomizeSeed, 0, 0x3fffffff);

    if (ImGui::Button("Randomize")) {
      // TODO
    }

    ImGui::Separator();

    ImGui::SliderFloat("Camera Azimuth", &camAzimuth, 0.0f, glm::radians(180.0f));
    ImGui::SliderFloat("Camera Altitude", &camAltitude, 0.0f, glm::radians(90.0f));
    ImGui::SliderFloat("Camera Distance", &camDistance, near, far);
    ImGui::SliderFloat("Camera FOV", &camFov, glm::radians(10.0f), glm::radians(90.0f));

    ImGui::End();

    ImGui::Render();

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window);
  }

  glDisable(GL_DEPTH_TEST);

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  glfwDestroyWindow(window);

  glfwTerminate();

  return EXIT_SUCCESS;
}
