#pragma once

#include "camera_display.hpp"
#include "../../../libs/imgui/imgui.h"


namespace camera_display
{
    bool init(CameraState& state)
    {
        state.cameras = camera_usb::enumerate_cameras();
        return true;
    }


    void show_cameras(CameraState& state)
    {
        if (!ImGui::CollapsingHeader("Cameras"))
        {
            return; 
        }

        for (u32 i = 0; i < state.cameras.count; i++)
        {
            auto& camera = state.cameras.list[i];

            ImGui::Text("w: %u | h: %u | fps: %u | %s", camera.frame_width, camera.frame_height, camera.fps, camera.format);
        }
    }
}