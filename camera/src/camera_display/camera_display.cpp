#pragma once

#include "camera_display.hpp"
#include "../../../libs/imgui/imgui.h"

#include <thread>


namespace camera_display
{
    using CS = ConnectionStatus;


    std::thread connect_th;
}


/* api */

namespace camera_display
{
    


    void init_async(CameraState& state)
    {
        
        state.connection = ConnectionStatus::Disconnected;

        connect_th = std::thread([&]()
        {
            state.connection = CS::Connecting;
            state.cameras = camera_usb::enumerate_cameras();
            state.connection = state.cameras.count > 0 ? CS::Connected : CS::Disconnected;
        });
    }


    void close(CameraState& state)
    {
        connect_th.join();

        camera_usb::close();

        state.connection = ConnectionStatus::Disconnected;
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