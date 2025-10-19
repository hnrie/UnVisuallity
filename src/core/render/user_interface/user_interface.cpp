//
// Created by savage on 18.04.2025.
//

#include "user_interface.h"

#include "entry.h"
#include "globals.h"
#include "imgui.h"
//#include "TextEditor.h"
#include "src/rbx/taskscheduler/taskscheduler.h"
#include "xorstr/xorstr.h"

void user_interface::render() {

    ImGui::SetNextWindowSize(ImVec2(600,400), ImGuiCond_Once);

    if (ImGui::Begin(OBF("Executor"), nullptr)) {
        ImGui::BeginTabBar(OBF("##tabbar"));

        ImGui::Spacing();
        if (ImGui::BeginTabItem(OBF("Editor"))) {
           // this->text_editor.Render(OBF("##editor"), ImVec2(ImGui::GetContentRegionAvail().x,
            //                                       ImGui::GetContentRegionAvail().y - 20));

            if (ImGui::Button(OBF("Execute")))
            {
                //g_taskscheduler->queue_script(this->text_editor.GetText());
            }

            ImGui::SameLine();

            if (ImGui::Button(OBF("Clear")))
            {
               // this->text_editor.SetText(OBF(""));
            };
            ImGui::SameLine();

            if (ImGui::Button(OBF("Execute from clipboard")))
            {
                g_taskscheduler->queue_script( ImGui::GetClipboardText() );
            };

            ImGui::EndTabItem();
        }
        ImGui::Spacing();
        if (ImGui::BeginTabItem("Settings")) {
            ImGui::EndTabItem();
        }


        ImGui::EndTabBar();
    }

    ImGui::End();
}