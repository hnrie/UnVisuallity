//
// Created by user on 28/05/2025.
//

#include "key_interface.hpp"

#include "globals.h"
//#include "luarmor.hpp"
#include "lazy_importer/include/lazy_importer.hpp"
#include "xorstr/xorstr.h"

extern std::optional<const std::string> get_hwid();

void key_interface::render() {

  /*
    *   ImGui::SetNextWindowSize(ImVec2(500,280), ImGuiCond_Once);
      ImGui::SetNextWindowSizeConstraints(ImVec2(450, 230), ImVec2(FLT_MAX, FLT_MAX));

      if (ImGui::Begin(OBF("Visual | Key System"))) {

          ImVec2 content_avail = ImGui::GetContentRegionAvail();
          content_avail.y -= ImGui::GetFrameHeightWithSpacing();

          static std::string buffer(512, '\0');

          ImGui::InputText(OBF("##editor"), buffer.data(), buffer.size() + 1);



}

ImGui::SameLine();

if (ImGui::Button(OBF("Get Key"))) {
    ShellExecuteA(nullptr, OBF("open"), OBF("https://ads.luarmor.net/get_key?for=Visual_Free_Key_System-RgBFPjgOeiDR"), nullptr, nullptr, SW_SHOWNORMAL);
}
    }

ImGui::End();
   */
}
