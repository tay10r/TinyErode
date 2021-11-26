#include <GLFW/glfw3.h>

#include <glad/glad.h>

#include <glm/glm.hpp>

#include <glm/gtc/matrix_transform.hpp>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <iostream>
#include <memory>
#include <vector>

#include <cstdlib>

#include "ErosionFilter.h"
#include "NoiseFilter.h"
#include "Renderer.h"
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

int
mainLoop(GLFWwindow* window)
{
  Renderer renderer;

  if (!renderer.compileShaders(std::cerr))
    return EXIT_FAILURE;

  std::unique_ptr<Terrain> terrain;

  const float near = 0.1;
  const float far = 5000;

  int genTerrainWidth = 1023;
  int genTerrainHeight = 1023;

  ErosionFilter erosionFilter;

  NoiseFilter noiseFilter;

  float camAzimuth = glm::radians(30.0f);
  float camAltitude = glm::radians(30.0f);
  float camDistance = 350;
  float camFov = glm::radians(45.0f);

  float lightX = 1;
  float lightY = 1;
  float lightZ = 1;

  auto getCamPosition = [&camAzimuth, &camAltitude, &camDistance]() -> glm::vec3 {
    const float altitude = glm::radians(90.0f) - camAltitude;
    const float x = camDistance * std::cos(camAzimuth) * std::sin(altitude);
    const float y = camDistance * std::sin(camAzimuth) * std::sin(altitude);
    const float z = camDistance * std::cos(altitude);
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

    if (terrain && erosionFilter.inErodeState())
      erosionFilter.erode(*terrain);

    if (terrain) {
      renderer.setMVP(mvp);

      renderer.setLightDir(glm::vec3(lightX, lightY, lightZ));

      renderer.setMetersPerPixel(terrain->metersPerPixel());

      renderer.render(*terrain);

      if (erosionFilter.inErodeState())
        renderer.renderWater(*terrain);
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Options");

    ImGui::BeginTabBar("Option Tabs");

    if (ImGui::BeginTabItem("Setup")) {

      ImGui::SliderInt("Terrain Width", &genTerrainWidth, 1, 4095);
      ImGui::SliderInt("Terrain Height", &genTerrainHeight, 1, 4095);

      if (ImGui::Button("Generate Initial Terrain"))
        terrain.reset(new Terrain(genTerrainWidth, genTerrainHeight));

      if (terrain) {
        ImGui::Separator();

        float metersPerPixel = terrain->metersPerPixel();
        ImGui::SliderFloat("Meters per Pixel", &metersPerPixel, 0.01, 10.0);
        terrain->setMetersPerPixel(metersPerPixel);
      }

      ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("Camera")) {
      ImGui::SliderFloat("Camera Azimuth", &camAzimuth, 0.01f, glm::radians(180.0f));
      ImGui::SliderFloat("Camera Altitude", &camAltitude, 0.01f, glm::radians(90.0f));
      ImGui::SliderFloat("Camera Distance", &camDistance, near, far);
      ImGui::SliderFloat("Camera FOV", &camFov, glm::radians(10.0f), glm::radians(90.0f));
      ImGui::EndTabItem();
    }

    if (terrain && ImGui::BeginTabItem("Noise Filter")) {
      noiseFilter.renderGui(*terrain);
      ImGui::EndTabItem();
    }

    if (terrain && ImGui::BeginTabItem("Erosion Filter")) {
      erosionFilter.renderGui(*terrain);
      ImGui::EndTabItem();
    }

    ImGui::EndTabBar();

    ImGui::End();

    ImGui::Render();

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window);
  }

  return true;
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
  glfwWindowHint(GLFW_SAMPLES, 4);
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
  glEnable(GL_BLEND);
  glEnable(GL_SAMPLES);

  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glClearColor(0, 0, 0, 1);

  updateViewport(window);

  IMGUI_CHECKVERSION();

  ImGui::CreateContext();

  ImGui_ImplGlfw_InitForOpenGL(window, true);

  ImGui_ImplOpenGL3_Init("#version 300 es");

  mainLoop(window);

  glDisable(GL_SAMPLES);
  glDisable(GL_BLEND);
  glDisable(GL_DEPTH_TEST);

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  glfwDestroyWindow(window);

  glfwTerminate();

  return EXIT_SUCCESS;
}
