#include "run.h"

#include <GLFW/glfw3.h>

#include <glad/glad.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <implot.h>

#include <iostream>
#include <memory>
#include <vector>

#include <cstdlib>

#include "App.h"

#include "DroidSans.h"

#include "extension.h"

namespace {

auto
run_main_loop(GLFWwindow* window) -> bool
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

  const auto success = app->GetStatus() == App::Status::kSuccess;

  app->Teardown();

  return success;
}

} // namespace

auto
run_landbrush_app(extension_registry& exreg, const std::string& app_name) -> bool
{
  if (glfwInit() != GLFW_TRUE) {
    std::cerr << "Failed to initialize GLFW." << std::endl;
    return false;
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
  glfwWindowHint(GLFW_SAMPLES, 4);
  glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
  glfwWindowHint(GLFW_DEPTH_BITS, 32);

  GLFWwindow* window = glfwCreateWindow(1280, 720, app_name.c_str(), nullptr, nullptr);
  if (!window) {
    std::cerr << "Failed to initialize GLFW." << std::endl;
    glfwTerminate();
    return false;
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

  ImGui::GetIO().IniFilename = "landbrush.ini";

  auto* implotContext = ImPlot::CreateContext();

  auto success = run_main_loop(window);

  glDisable(GL_SAMPLES);
  glDisable(GL_BLEND);
  glDisable(GL_DEPTH_TEST);

  ImPlot::DestroyContext(implotContext);

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  glfwDestroyWindow(window);

  glfwTerminate();

  return true;
}
