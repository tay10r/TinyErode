#pragma once

#include "ui_flags.h"

class MainMenuBar final
{
public:
  explicit MainMenuBar(UiFlags* flags);

  void Step();

protected:
  void FlagCheckbox(const char* label, UiFlags flag);

private:
  UiFlags* flags_{};
};
