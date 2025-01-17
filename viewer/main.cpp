#include <GLFW/glfw3.h>

#include <glad/glad.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <iostream>
#include <memory>
#include <vector>

#include <cstdlib>

#include "app.h"

#include "DroidSans.h"

#ifdef _WIN32
#include <Windows.h>
#endif

#ifdef near
#undef near
#endif

#ifdef far
#undef far
#endif

namespace {

#if 0
void
updateViewport(GLFWwindow* window)
{
  int w = 0;
  int h = 0;
  glfwGetFramebufferSize(window, &w, &h);
  glViewport(0, 0, w, h);
}

struct Camera final
{
  float azimuth{ 30.0f };

  float altitude{ 30.0f };

  float fov{ glm::radians(45.0f) };

  float distance{ 100 };

  float near{ 1.0 };

  float far{ 1000.0 };

  glm::vec3 getPosition() const
  {
    const auto rot_y = glm::rotate(glm::radians(azimuth), glm::vec3(0, 1, 0));
    const auto rot_x = glm::rotate(glm::radians(altitude), glm::vec3(1, 0, 0));
    return glm::vec3(rot_y * rot_x * glm::vec4(0, 0, -1, 1)) * distance;
  };

  glm::mat4 perspective(const float aspect) const { return glm::perspective(fov, aspect, near, far); }

  glm::mat4 view() const
  {
    const auto eye = getPosition();

    return glm::lookAt(eye, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
  }
};

class App final
{
public:
  explicit App(GLFWwindow* window)
    : m_window(window)
  {
  }

  bool init()
  {
    if (!m_renderer.compileShaders(std::cerr))
      return false;

    generateNewTerrain();

    return true;
  }

  void run()
  {
    while (!glfwWindowShouldClose(m_window)) {

      checkCameraController();

      renderFrame();
    }
  }

private:
  void checkCameraController()
  {
    if (!ImGui::GetIO().WantCaptureMouse) {

      if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
        const auto delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left);
        m_camera.azimuth += delta.x / ImGui::GetIO().DisplaySize.x;
        m_camera.altitude += -delta.y / ImGui::GetIO().DisplaySize.y;
      }
    }
  }

  void renderFrame()
  {
    int w = 0;
    int h = 0;
    glfwGetFramebufferSize(m_window, &w, &h);
    const float aspect = static_cast<float>(w) / h;

    const glm::mat4 mvp = m_camera.perspective(aspect) * m_camera.view();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (m_terrain && m_erosionFilter.inErodeState())
      m_erosionFilter.erode(*m_terrain);

    if (m_terrain) {
      m_renderer.setMVP(mvp);
      m_renderer.setLightDir(m_light);
      m_renderer.setTotalMetersX(m_terrain->totalMetersX());
      m_renderer.setTotalMetersY(m_terrain->totalMetersY());

      m_renderer.render(*m_terrain);

      if (m_erosionFilter.inErodeState())
        m_renderer.renderWater(*m_terrain);
    }

    renderMainMenuBar();

    ImGui::Begin("Options");

    ImGui::BeginTabBar("Option Tabs");

    if (ImGui::BeginTabItem("Setup")) {
      renderTerrainOptions();
      ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("Camera")) {
      ImGui::SliderFloat("Camera Azimuth", &m_camera.azimuth, 0.0f, 360.0);
      ImGui::SliderFloat("Camera Altitude", &m_camera.altitude, 0.0f, 90.0f);
      ImGui::SliderFloat("Camera Distance", &m_camera.distance, m_camera.near, m_camera.far);
      ImGui::SliderFloat("Camera FOV", &m_camera.fov, glm::radians(10.0f), glm::radians(90.0f));
      ImGui::EndTabItem();
    }

    if (m_terrain && ImGui::BeginTabItem("Noise Filter")) {
      m_noiseFilter.renderGui(*m_terrain);
      ImGui::EndTabItem();
    }

    if (m_terrain && ImGui::BeginTabItem("Erosion Filter")) {
      m_erosionFilter.renderGui(*m_terrain);
      ImGui::EndTabItem();
    }

    ImGui::EndTabBar();

    ImGui::End();

  }

  void renderTerrainOptions()
  {
    if (m_terrain) {

      float totalMetersX = m_terrain->totalMetersX();
      float totalMetersY = m_terrain->totalMetersY();

      ImGui::SliderFloat("Total Meters (X)", &totalMetersX, 1.0f, 4097.0);
      ImGui::SliderFloat("Total Meters (Y)", &totalMetersY, 1.0f, 4097.0);

      m_terrain->setTotalMetersX(totalMetersX);
      m_terrain->setTotalMetersY(totalMetersY);

      if (ImGui::Button("Close")) {
        m_terrain.reset();
      }

    } else {

      ImGui::SliderInt("Terrain Width", &m_generateTerrainWidth, 2, 4096);
      ImGui::SliderInt("Terrain Height", &m_generateTerrainHeight, 2, 4096);

      if (ImGui::Button("Generate Initial Terrain"))
        generateNewTerrain();
    }
  }

  void renderMainMenuBar()
  {
    if (ImGui::BeginMainMenuBar()) {

      if (ImGui::BeginMenu("File")) {

        ImGui::EndMenu();
      }

      if (ImGui::BeginMenu("Edit")) {

        if (ImGui::MenuItem("Undo")) {
        }

        if (ImGui::MenuItem("Redo")) {
        }

        ImGui::EndMenu();
      }

      ImGui::EndMainMenuBar();
    }
  }

  void generateNewTerrain() { m_terrain.reset(new Terrain(m_generateTerrainWidth, m_generateTerrainHeight)); }

private:
  GLFWwindow* m_window{ nullptr };

  Renderer m_renderer;

  Camera m_camera;

  std::unique_ptr<Terrain> m_terrain;

  ErosionFilter m_erosionFilter;

  NoiseFilter m_noiseFilter;

  glm::vec3 m_light{ glm::normalize(glm::vec3(1, 1, 0)) };

  int m_generateTerrainWidth{ 256 };

  int m_generateTerrainHeight{ 256 };
};
#endif

int
mainLoop(GLFWwindow* window)
{
  ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF(DroidSans_compressed_data, DroidSans_compressed_size, 16.0);

  ImGui::GetIO().Fonts->Build();

  ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

  auto& style = ImGui::GetStyle();
  style.FrameRounding = 4;
  style.WindowRounding = 4;
  style.ChildRounding = 4;
  style.PopupRounding = 4;
  style.WindowBorderSize = 0;

  auto app = App::Create(window);

  app->Setup();

  while ((glfwWindowShouldClose(window) != GLFW_TRUE) && (app->GetStatus() == App::Status::kRunning)) {

    glfwPollEvents();

    if (ImGui::IsKeyPressed(ImGuiKey_Escape, /*repeat=*/false)) {
      glfwSetWindowShouldClose(window, GLFW_TRUE);
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::DockSpaceOverViewport();

    app->Step();

    ImGui::Render();

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window);
  }

  const auto exit_code = app->GetStatus() == App::Status::kSuccess ? EXIT_SUCCESS : EXIT_FAILURE;

  app->Teardown();

  return exit_code;
}

} // namespace

#ifdef _WIN32
int
WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
#else
int
main(int argc, char** argv)
#endif
{
  if (glfwInit() != GLFW_TRUE) {
    std::cerr << "Failed to initialize GLFW." << std::endl;
    return EXIT_FAILURE;
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
  glfwWindowHint(GLFW_SAMPLES, 4);
  glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
  glfwWindowHint(GLFW_DEPTH_BITS, 32);

  GLFWwindow* window = glfwCreateWindow(1280, 720, "TinyErode Viewer", nullptr, nullptr);
  if (!window) {
    std::cerr << "Failed to initialize GLFW." << std::endl;
    glfwTerminate();
    return EXIT_FAILURE;
  }

  glfwMakeContextCurrent(window);

  gladLoadGLES2Loader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress));

  glfwSwapInterval(1);

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glEnable(GL_SAMPLES);

  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glClearColor(0, 0, 0, 1);

  IMGUI_CHECKVERSION();

  ImGui::CreateContext();
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 300 es");

  const auto exitCode{ mainLoop(window) };

  glDisable(GL_SAMPLES);
  glDisable(GL_BLEND);
  glDisable(GL_DEPTH_TEST);

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  glfwDestroyWindow(window);

  glfwTerminate();

  return exitCode;
}
