#pragma once

#include "camera_display.hpp"
#include "../../../libs/imgui/imgui.h"

#include <thread>


namespace cam = camera_usb;


namespace camera_display
{
    using CS = ConnectionStatus;


    std::thread connect_th;
}


namespace camera_display
{
    constexpr auto im_gray(f32 value)
    {
        return ImVec4(value, value, value, 1.0f);
    }


    static void camera_properties_table(cam::CameraList const& cameras)
    {
        enum class columns : int
        {
            radio = 0,
            width,
            height,
            fps,
            format,
            vendor,
            product,
            serial,
            count
        };

        int table_flags = ImGuiTableFlags_BordersInnerV;
        auto table_dims = ImVec2(0.0f, 0.0f);

        static int camera_id = 0;

        //ImU32 cell_bg_color = ImGui::GetColorU32(im_gray(0.1f));

        auto const setup_columns = []()
        {
            ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 40.0f);
            ImGui::TableSetupColumn("W", ImGuiTableColumnFlags_WidthFixed, 30.0f);
            ImGui::TableSetupColumn("H", ImGuiTableColumnFlags_WidthFixed, 30.0f);
            ImGui::TableSetupColumn("FPS", ImGuiTableColumnFlags_WidthFixed, 30.0f);
            ImGui::TableSetupColumn("Format", ImGuiTableColumnFlags_WidthFixed, 50.0f);
            ImGui::TableSetupColumn("Vendor", ImGuiTableColumnFlags_WidthFixed, 50.0f);
            ImGui::TableSetupColumn("Product", ImGuiTableColumnFlags_WidthFixed, 50.0f);
            ImGui::TableSetupColumn("SN", ImGuiTableColumnFlags_WidthFixed, 50.0f);
            ImGui::TableHeadersRow();
        };

        auto const table_row = [](cam::Camera const& camera)
        {
            ImGui::TableSetColumnIndex((int)columns::radio);
            ImGui::RadioButton(camera.label.begin, &camera_id, camera.id);

            ImGui::TableSetColumnIndex((int)columns::width);
            ImGui::Text("%u", camera.frame_width);

            ImGui::TableSetColumnIndex((int)columns::height);
            ImGui::Text("%u", camera.frame_height);

            ImGui::TableSetColumnIndex((int)columns::fps);
            ImGui::Text("%u", camera.fps);

            ImGui::TableSetColumnIndex((int)columns::format);
            ImGui::Text("%s", camera.format.begin);

            ImGui::TableSetColumnIndex((int)columns::vendor);
            ImGui::Text("%s", camera.vendor.begin);

            ImGui::TableSetColumnIndex((int)columns::product);
            ImGui::Text("%s", camera.product.begin);

            ImGui::TableSetColumnIndex((int)columns::serial);
            ImGui::Text("%s", camera.serial_number.begin);
        };

        if (!ImGui::BeginTable("CameraPropertiesTable", (int)columns::count, table_flags, table_dims)) 
        { 
            return; 
        }

        setup_columns();

        for (u32 i = 0; i < cameras.count; i++)
        {
            ImGui::TableNextRow();
            table_row(cameras.list[i]);
        }

        ImGui::EndTable();
    }
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
        if (state.connection == ConnectionStatus::Connected)
        {
            camera_properties_table(state.cameras);
        }
        else
        {
            ImGui::Text("Connecting...");
        }
        
    }
}