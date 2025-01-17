#include "ui_flags.h"

#include <stdio.h>

#include <sstream>

auto
ParseUiFlags(const char* str, UiFlags* flags) -> bool
{
  uint32_t value{};

  if (sscanf(str, "%u", &value) != 1) {
    return false;
  }

  *flags = static_cast<UiFlags>(value);

  return true;
}

auto
WriteUiFlags(UiFlags flags) -> std::string
{
  std::ostringstream stream;
  stream << static_cast<uint32_t>(flags);
  return stream.str();
}
