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

#include "FastNoiseLite.h"
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

void
randomize(FastNoiseLite& noise, Terrain& terrain);

const char*
toString(FastNoiseLite::NoiseType noiseType)
{
  switch (noiseType) {
    case FastNoiseLite::NoiseType_OpenSimplex2:
      return "OpenSimplex2";
    case FastNoiseLite::NoiseType_OpenSimplex2S:
      return "OpenSimplex2S";
    case FastNoiseLite::NoiseType_Cellular:
      return "Cellular";
    case FastNoiseLite::NoiseType_Perlin:
      return "Perlin";
    case FastNoiseLite::NoiseType_ValueCubic:
      return "ValueCubic";
    case FastNoiseLite::NoiseType_Value:
      return "Value";
  }

  return "";
}

int
toIndex(FastNoiseLite::NoiseType noiseType)
{
  switch (noiseType) {
    case FastNoiseLite::NoiseType_OpenSimplex2:
      return 0;
    case FastNoiseLite::NoiseType_OpenSimplex2S:
      return 1;
    case FastNoiseLite::NoiseType_Cellular:
      return 2;
    case FastNoiseLite::NoiseType_Perlin:
      return 3;
    case FastNoiseLite::NoiseType_ValueCubic:
      return 4;
    case FastNoiseLite::NoiseType_Value:
      return 5;
  }

  return -1;
}

bool
ImGui_NoiseType(FastNoiseLite::NoiseType* noiseType, ImGuiComboFlags flags = 0)
{
  const char* current = toString(*noiseType);

  const char* items[]{
    "OpenSimplex2", "OpenSimplex2S", "Cellular", "Perlin", "ValueCubic", "Value",
  };

  int currentIndex = -1;

  for (int i = 0; i < 6; i++) {
    if (strcmp(items[i], current) == 0) {
      currentIndex = i;
      break;
    }
  }

  if (ImGui::Combo("NoiseType", &currentIndex, items, 6)) {
    switch (currentIndex) {
      case 0:
        *noiseType = FastNoiseLite::NoiseType_OpenSimplex2;
        break;
      case 1:
        *noiseType = FastNoiseLite::NoiseType_OpenSimplex2S;
        break;
      case 2:
        *noiseType = FastNoiseLite::NoiseType_Cellular;
        break;
      case 3:
        *noiseType = FastNoiseLite::NoiseType_Perlin;
        break;
      case 4:
        *noiseType = FastNoiseLite::NoiseType_ValueCubic;
        break;
      case 5:
        *noiseType = FastNoiseLite::NoiseType_Value;
        break;
    }
    return true;
  }

  return false;
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
  FastNoiseLite fastNoiseLite;
  FastNoiseLite::NoiseType noiseType = FastNoiseLite::NoiseType_Perlin;
  fastNoiseLite.SetNoiseType(noiseType);

  float metersPerPixel = 10;

  float camAzimuth = glm::radians(30.0f);
  float camAltitude = glm::radians(30.0f);
  float camDistance = 50;
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

    if (terrain)
      terrain->render(mvp, metersPerPixel, glm::vec3(lightX, lightY, lightZ));

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

    ImGui::SliderFloat("Meters per Pixel", &metersPerPixel, 0.01, 100.0);

    ImGui::Separator();

    if (ImGui::SliderInt("Randomize Seed", &randomizeSeed, 0, 0x3fffffff))
      fastNoiseLite.SetSeed(randomizeSeed);

    if (ImGui_NoiseType(&noiseType))
      fastNoiseLite.SetNoiseType(noiseType);

    if (ImGui::Button("Randomize") && terrain)
      randomize(fastNoiseLite, *terrain);

    ImGui::Separator();

    ImGui::SliderFloat("Camera Azimuth", &camAzimuth, 0.01f, glm::radians(180.0f));
    ImGui::SliderFloat("Camera Altitude", &camAltitude, 0.01f, glm::radians(90.0f));
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

namespace {

void
randomize(FastNoiseLite& noise, Terrain& terrain)
{
  const int w = terrain.width();
  const int h = terrain.height();

  std::vector<float> heightMap(w * h, 0.0f);

  for (int i = 0; i < (w * h); i++) {

    const int x = i % w;
    const int y = i / w;

    heightMap[i] = noise.GetNoise(float(x), float(y));
  }

  terrain.setHeightMap(heightMap.data(), w, h);
}

} // namespace
