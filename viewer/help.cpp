#include "help.h"

#include <imgui.h>

namespace {

char help[] = R"(Welcome!

To get started with this tool, open up the "View" menu from the main menu bar.
Check the "Viewport" box to open the window that renders the terrain.
)";

} // namespace

void
Help::Step()
{
  ImGui::InputTextMultiline("##Help", &help[0], sizeof(help), ImVec2(-1, -1), ImGuiInputTextFlags_ReadOnly);
}
