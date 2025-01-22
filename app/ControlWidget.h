#pragma once

#include "Config.h"

enum class ControlButton
{
  NONE,
  RESET
};

class ControlWidget final
{
public:
  auto Step(Config* config) -> ControlButton;
};
