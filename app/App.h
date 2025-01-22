#pragma once

#include <GLFW/glfw3.h>

#include <memory>

class App
{
public:
  enum class Status
  {
    kRunning,
    kSuccess,
    kFailure
  };

  static auto Create(GLFWwindow* window) -> std::unique_ptr<App>;

  virtual ~App() = default;

  virtual void Setup() = 0;

  virtual void Teardown() = 0;

  virtual void Step() = 0;

  [[nodiscard]] virtual auto GetStatus() -> Status = 0;
};
