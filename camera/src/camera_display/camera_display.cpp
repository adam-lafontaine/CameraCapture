#pragma once

#include "camera_display.hpp"
#include "../../../libs/imgui/imgui.h"

#include <thread>


namespace cam = camera_usb;


namespace camera_display
{
    inline constexpr cstr decode_status(cam::ConnectionStatus s)
    {
        using S = cam::ConnectionStatus;

        switch (s)
        {
        case S::Disconnected: return "No cameras";
        case S::Connecting:   return "Connecting...";
        case S::Connected:    return "Cameras OK";

        default: return "UNKN";
        }
    }


    inline constexpr cstr decode_status(cam::CameraStatus s)
    {
        using S = cam::CameraStatus;

        switch (s)
        {
        case S::Inactive:  return "I";
        case S::Active:    return "A";
        case S::Open:      return "O";
        case S::Streaming: return "S";

        default: return "UNKN";
        }
    }
}


/* threads */

namespace camera_display
{
    static std::thread connect_th;
    static bool connect_end = false;

    static std::thread camera_th;
    static bool camera_end = false;



    static void join_threads(CameraState const& state)
    {
        static bool connect = false;

        if (connect_end)
        {
            connect_th.join();
            connect_end = false;
        }

        if (camera_end)
        {
            camera_th.join();
            camera_end = false;
        }
    }
}


namespace camera_display
{
    static void toggle_activate_async(cam::Camera& camera)
    {
        using S = cam::CameraStatus;

        camera_th = std::thread([&]()
        {
            camera_end = false;

            switch (camera.status)
            {
            case S::Inactive:
                return;
            
            case S::Active:
                cam::open_camera(camera);
                break;

            default:
                cam::close_camera(camera);
                break;

            }

            camera_end = true;
        });
    }
}


namespace camera_display
{
    constexpr auto im_gray(f32 value)
    {
        return ImVec4(value, value, value, 1.0f);
    }


    static void camera_properties_table(cam::CameraList& cameras)
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
            activate,
            status,
            count
        };

        int table_flags = ImGuiTableFlags_BordersInnerV;
        auto table_dims = ImVec2(0.0f, 0.0f);

        static int camera_id = 0;

        //ImU32 cell_bg_color = ImGui::GetColorU32(im_gray(0.1f));

        auto const setup_columns = []()
        {
            ImGui::TableSetupColumn("Camera", ImGuiTableColumnFlags_WidthFixed, 40.0f);
            ImGui::TableSetupColumn("W", ImGuiTableColumnFlags_WidthFixed, 30.0f);
            ImGui::TableSetupColumn("H", ImGuiTableColumnFlags_WidthFixed, 30.0f);
            ImGui::TableSetupColumn("FPS", ImGuiTableColumnFlags_WidthFixed, 30.0f);
            ImGui::TableSetupColumn("Format", ImGuiTableColumnFlags_WidthFixed, 50.0f);
            ImGui::TableSetupColumn("Vendor", ImGuiTableColumnFlags_WidthFixed, 50.0f);
            ImGui::TableSetupColumn("Product", ImGuiTableColumnFlags_WidthFixed, 50.0f);
            ImGui::TableSetupColumn("SN", ImGuiTableColumnFlags_WidthFixed, 50.0f);
            ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 80.0f);
            ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed, 50.0f);
            ImGui::TableHeadersRow();
        };

        auto const table_row = [](cam::Camera& camera)
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

            ImGui::TableSetColumnIndex((int)columns::status);
            ImGui::Text("%s", decode_status(camera.status));

            ImGui::TableSetColumnIndex((int)columns::activate);
            if (ImGui::Button("Toggle"))
            {
                toggle_activate_async(camera);
            }
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
        connect_th = std::thread([&]()
        {
            connect_end = false;
            state.cameras = camera_usb::enumerate_cameras();
            connect_end = true;
        });
    }


    void close(CameraState& state)
    {
        camera_usb::close(state.cameras);
    }


    void show_cameras(CameraState& state)
    {   
        join_threads(state);

        camera_properties_table(state.cameras);
        
        ImGui::Text("%s", decode_status(state.cameras.status));
        
    }
}