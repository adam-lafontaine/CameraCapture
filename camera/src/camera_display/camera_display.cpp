#pragma once

#include "camera_display.hpp"
#include "../../../libs/imgui/imgui.h"


namespace camera_display
{
    bool init(CameraState& state)
    {
        return true;
    }


    void show_cameras(CameraState& state)
    {
        ImGui::Text("Cameras go here...");
    }
}