#pragma once

#include <stdint.h>

#include <string>

enum class UiFlags : uint32_t
{
  kHelp = 1,
  kViewport = 2,
  kLayers
};

[[nodiscard]] constexpr auto
IsSet(const UiFlags flags, const UiFlags flag) -> bool
{
  return (static_cast<uint32_t>(flags) & static_cast<uint32_t>(flag)) != 0;
}

[[nodiscard]] constexpr auto
Unset(const UiFlags flags, const UiFlags flag) -> UiFlags
{
  return static_cast<UiFlags>(static_cast<uint32_t>(flags) & (static_cast<uint32_t>(flag) ^ 0xffff'ffffU));
}

[[nodiscard]] auto
ParseUiFlags(const char* str, UiFlags* flags) -> bool;

[[nodiscard]] auto
WriteUiFlags(UiFlags flags) -> std::string;
