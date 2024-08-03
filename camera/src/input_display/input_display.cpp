#include "input_display.hpp"
#include "../../../libs/qsprintf/qsprintf.hpp"
#include "../../../libs/span/span.hpp"
#include "../../../libs/util/numeric.hpp"

#include <cassert>
#include <array>

#include <cstdio>


namespace img = image;
namespace sp = span;
namespace num = numeric;

using Image = img::Image;
using ImageView = img::ImageView;
using SubView = img::SubView;
using Pixel = img::Pixel;
using Buffer32 = img::Buffer32;
using Buffer8 = img::Buffer8;
using FilterView = img::GrayView;
using SubFilter = img::GraySubView;


/* image files */

namespace input_display
{
    template <typename SRC>
    static ImageView load_embedded_image32(SRC const& src, img::Buffer32& buffer)
    {
        auto dst = img::make_view(src.width, src.height, buffer);

        auto len = src.width * src.height;
        auto lut = (Pixel*)src.color24_lut;

        for (u32 i = 0; i < len; i++)
        {
            auto p = lut[src.color_ids[i]];
            p.alpha = src.alpha[i];

            dst.matrix_data_[i] = p;
        }

        return dst;
    }


    static img::Buffer32 create_pixel_memory()
    {
#include "../res/image/res_pixel_total.cpp"

        return img::create_buffer32(res_pixel_total, "input_display pixels");
    }


    static ImageView load_controller_view(img::Buffer32& buffer)
    {
#include "../res/image/controller.cpp"

        return load_embedded_image32(controller, buffer);
    }


    static ImageView load_keyboard_view(img::Buffer32& buffer)
    {
#include "../res/image/keyboard.cpp"

        return load_embedded_image32(keyboard, buffer);
    }


    static ImageView load_mouse_view(img::Buffer32& buffer)
    {
#include "../res/image/mouse.cpp"

        return load_embedded_image32(mouse, buffer);
    }
}


/* text rendering */

namespace input_display
{
    constexpr u32 TEXT_HEIGHT = 8;
    constexpr auto TEXT_COLOR = img::to_pixel(0);


    static img::GrayView make_ascii_view(char c)
    {
#include "../../../resources/ascii_5.cpp"

        auto& ascii = ascii_chars;

        auto id = c - ' ';

        img::GrayView view{};
        view.width = (u32)ascii.widths[id];
        view.height = (u32)ascii.height;
        view.matrix_data_ = (u8*)ascii.u8_pixel_data[id];

        return view;
    }


    static img::Pixel mask_char(u8 mask, img::Pixel p)
    {
        return (mask) ? TEXT_COLOR : p;
    }


    static void render_text(cstr text, img::SubView const& dst)
    {
        auto const len = span::strlen(text);

        Rect2Du32 d_range = { 0 };

        u32 width = 0;
        u32 height = 0;
        i32 w_remaining = dst.width;

        for (u32 i = 0; i < len && w_remaining > 0; i++)
        {
            auto ch_filter = make_ascii_view(text[i]);

            width = num::min(ch_filter.width, (u32)w_remaining);
            height = num::min(ch_filter.height, dst.height);

            d_range.x_end += width;
            d_range.y_end = height;

            auto s_view = img::sub_view(ch_filter, img::make_rect(width, height));
            auto d_view = img::sub_view(dst, d_range);
            
            img::transform(s_view, d_view, mask_char);

            d_range.x_begin = d_range.x_end;
            w_remaining -= width;
        }
    }
}


/* color table */

namespace input_display
{
    constexpr auto WHITE = img::to_pixel(255, 255, 255);
    constexpr auto BLACK = img::to_pixel(0, 0, 0);
    constexpr auto TRANSPARENT = img::to_pixel(0, 0, 0, 0);
    constexpr auto BLUE = img::to_pixel(0, 75, 168);
    constexpr auto LIGHT_BLUE = img::to_pixel(23, 190, 187);
    constexpr auto GRAY = img::to_pixel(115, 140, 153);


    static constexpr std::array<Pixel, 5> COLOR_TABLE = 
    {
        TRANSPARENT,
        BLACK,
        WHITE,      
        BLUE,
        LIGHT_BLUE
    };


    enum class ColorId : u8
    {
        Trasnparent = 0,
        Black = 1,
        White = 2,
        Blue = 3,
        LightBlue = 4
    };


    static constexpr u8 to_u8(ColorId id)
    {
        return static_cast<u8>(id);
    }


    static u8 to_filter_color_id(Pixel p)
    {
        if (p.alpha == 0)
        {
            return to_u8(ColorId::Trasnparent);
        }

        if (p.red == 0 && p.green == 0 && p.blue == 0)
        {
            return to_u8(ColorId::Black);
        }

        return to_u8(ColorId::White);
    }


    bool can_set_color_id(u8 current_id)
    {
        return current_id > to_u8(ColorId::Black);
    }


    Pixel to_render_color(u8 mask, Pixel color)
    {
        if (mask == 0)
        {
            return color;
        }

        return COLOR_TABLE[mask];
    }
}


/* controller filter */

namespace input_display
{
    class ControllerFilter
    {
    public:
        FilterView filter;

        static constexpr u32 count = 16;

        union
        {
            SubFilter buttons[count];

            struct
            {
                SubFilter btn_dpad_up;
                SubFilter btn_dpad_down;
                SubFilter btn_dpad_left;
                SubFilter btn_dpad_right;
                SubFilter btn_a;
                SubFilter btn_b;
                SubFilter btn_x;
                SubFilter btn_y;
                SubFilter btn_start;
                SubFilter btn_back;
                SubFilter btn_sh_left;
                SubFilter btn_sh_right;
                SubFilter btn_tr_left;
                SubFilter btn_tr_right;
                SubFilter btn_st_left;
                SubFilter btn_st_right;
            };
        };
    };


    static void make_controller_filter(ControllerFilter& controller)
    {
        auto& view = controller.filter;

        controller.btn_dpad_up    = img::sub_view(view, img::make_rect(22, 33,  9, 16));
        controller.btn_dpad_down  = img::sub_view(view, img::make_rect(22, 60,  9, 16));
        controller.btn_dpad_left  = img::sub_view(view, img::make_rect( 5, 50, 16,  9));
        controller.btn_dpad_right = img::sub_view(view, img::make_rect(32, 50, 16,  9));

        controller.btn_a = img::sub_view(view, img::make_rect(159, 63, 13, 13));
        controller.btn_b = img::sub_view(view, img::make_rect(174, 48, 13, 13));
        controller.btn_x = img::sub_view(view, img::make_rect(144, 48, 13, 13));
        controller.btn_y = img::sub_view(view, img::make_rect(159, 33, 13, 13));

        controller.btn_start = img::sub_view(view, img::make_rect(103, 24, 14, 7));
        controller.btn_back  = img::sub_view(view, img::make_rect( 75, 24, 14, 7));

        controller.btn_sh_left  = img::sub_view(view, img::make_rect( 18, 22, 17,  7));
        controller.btn_sh_right = img::sub_view(view, img::make_rect(157, 22, 17, 7));
        controller.btn_tr_left  = img::sub_view(view, img::make_rect( 18,  5, 17, 13));
        controller.btn_tr_right = img::sub_view(view, img::make_rect(157,  5, 17, 13));

        controller.btn_st_left  = img::sub_view(view, img::make_rect( 60, 45, 23, 23));
        controller.btn_st_right = img::sub_view(view, img::make_rect(109, 45, 23, 23));
    }
}


/* keyboard filter */

namespace input_display
{
    class KeyboardFilter
    {
    public:
        FilterView filter;

        static constexpr u32 count = 9;
        
        union 
        {
            SubFilter keys[count];

            struct
            {
                SubFilter key_1;
                SubFilter key_2;
                SubFilter key_3;
                SubFilter key_4;
                SubFilter key_w;
                SubFilter key_a;
                SubFilter key_s;
                SubFilter key_d;
                SubFilter key_space;
            };            
        };

    };


    static void make_keyboard_filter(KeyboardFilter& keyboard)
    {
        auto& view = keyboard.filter;
        
        keyboard.key_1 = img::sub_view(view, img::make_rect(21,  3, 14, 14));
        keyboard.key_2 = img::sub_view(view, img::make_rect(39,  3, 14, 14));
        keyboard.key_3 = img::sub_view(view, img::make_rect(57,  3, 14, 14));
        keyboard.key_4 = img::sub_view(view, img::make_rect(75,  3, 14, 14));
        keyboard.key_w = img::sub_view(view, img::make_rect(48, 21, 14, 14));
        keyboard.key_a = img::sub_view(view, img::make_rect(35, 39, 14, 14));
        keyboard.key_s = img::sub_view(view, img::make_rect(53, 39, 14, 14));
        keyboard.key_d = img::sub_view(view, img::make_rect(71, 39, 14, 14));
        keyboard.key_space = img::sub_view(view, img::make_rect(84, 75, 104, 14));
    }
}


/* mouse filter */

namespace input_display
{
    class MouseFilter
    {
    public:
        FilterView filter;

        static constexpr u32 count = 3;

        union 
        {
            SubFilter buttons[count];

            struct
            {
                SubFilter btn_left;
                SubFilter btn_middle;
                SubFilter btn_right;
            };
        };
    };


    static void make_mouse_filter(MouseFilter& mouse)
    {
        auto& view = mouse.filter;

        mouse.btn_left   = img::sub_view(view, img::make_rect( 2, 2, 28, 29));
        mouse.btn_middle = img::sub_view(view, img::make_rect(34, 2, 12, 29));
        mouse.btn_right  = img::sub_view(view, img::make_rect(50, 2, 28, 29));
    }
}


/* state */

namespace input_display
{

    class StateData
    {
    public:
        ImageView display_view;

        SubView controller_view;
        SubView keyboard_view;
        SubView mouse_view;

        ControllerFilter controller_filter;
        KeyboardFilter keyboard_filter;
        MouseFilter mouse_filter;        

        StringView mouse_coords;
        SubView mouse_coords_view;

        Buffer32 pixel_data;
        Buffer8 u8_data;
    };


    static bool create_state_data(IOState& state)
    {
        auto data = mem::malloc<StateData>("input_display::StateData");
        if (!data)
        {
            return false;
        }

        state.data_ = data;

        return true;
    }


    static void destroy_state_data(IOState& state)
    {
        if (!state.data_)
        {
            return;
        }

        auto& state_data = *state.data_;

        mb::destroy_buffer(state_data.pixel_data);
        mb::destroy_buffer(state_data.u8_data);
        mem::free(state.data_);
    }
}


/* init */

namespace input_display
{
    static void init_controller_filter(ControllerFilter& filter, ImageView const& src, Buffer8& buffer)
    {
        auto& view = filter.filter;
        view = img::make_view(src.width, src.height, buffer);
        
        img::transform(src, view, to_filter_color_id);

        make_controller_filter(filter);
    }


    static void init_keyboard_filter(KeyboardFilter& filter, ImageView const& src, Buffer8& buffer)
    {
        auto& view = filter.filter;
        view = img::make_view(src.width, src.height, buffer);
        
        img::transform(src, view, to_filter_color_id);

        make_keyboard_filter(filter);
    }


    static void init_mouse_filter(MouseFilter& filter, ImageView const& src, Buffer8& buffer)
    {
        auto& view = filter.filter;
        view = img::make_view(src.width, src.height, buffer);
        
        img::transform(src, view, to_filter_color_id);

        make_mouse_filter(filter);        
    }


    static bool init_display_view(IOState& state)
    {
        auto buffer = create_pixel_memory();

        auto controller_v = load_controller_view(buffer);
        auto keyboard_v = load_keyboard_view(buffer);
        auto mouse_v = load_mouse_view(buffer);

        auto const ctlr_w = controller_v.width;
        auto const ctlr_h = controller_v.height;

        auto const kbd_w = keyboard_v.width;
        auto const kbd_h = keyboard_v.height;

        auto const mse_w = mouse_v.width;
        auto const mse_h = mouse_v.height;

        // images side by side
        u32 display_width = ctlr_w + kbd_w + mse_w;
        u32 display_height = num::max(num::max(ctlr_h, kbd_h), mse_h);

        constexpr u32 mouse_coord_capacity = sizeof("(0000, 0000)");

        auto& state_data = *state.data_;
        auto& buffer32 = state_data.pixel_data;
        auto& buffer8 = state_data.u8_data;

        buffer32 = img::create_buffer32(display_width * display_height, "input_display");        
        buffer8 = img::create_buffer8(display_width * display_height + mouse_coord_capacity);

        state.display = img::make_view(display_width, display_height, buffer32);

        auto controller_r = img::make_rect(0, 0, ctlr_w, ctlr_h);
        state_data.controller_view = img::sub_view(state.display, controller_r);
        init_controller_filter(state_data.controller_filter, controller_v, buffer8);

        auto keyboard_r = img::make_rect(controller_r.x_end, 0, kbd_w, kbd_h);
        state_data.keyboard_view = img::sub_view(state.display, keyboard_r);
        init_keyboard_filter(state_data.keyboard_filter, keyboard_v, buffer8);

        auto mouse_r = img::make_rect(keyboard_r.x_end, 0, mse_w, mse_h);
        state_data.mouse_view = img::sub_view(state.display, mouse_r);
        init_mouse_filter(state_data.mouse_filter, mouse_v, buffer8);        

        state_data.mouse_coords = sp::make_view(mouse_coord_capacity, buffer8);

        auto const coord_x = mse_w / 8;
        auto const coord_y = mse_h / 2;
        auto const coord_width = mse_w * 3 / 4;
        auto const coords = img::make_rect(coord_x, coord_y, coord_width, TEXT_HEIGHT);

        state_data.mouse_coords_view = img::sub_view(state_data.mouse_view, coords);

        mb::destroy_buffer(buffer);

        return true;
    }
}


/* update */

namespace input_display
{
    static void set_filter_color(SubFilter const& filter, ColorId color_id)
    {
        img::fill_if(filter, to_u8(color_id), can_set_color_id);
    }


    static void apply_filters(FilterView const& filter, SubView const& dst)
    {
        img::transform(filter, dst, to_render_color);
    }


    static void update_controller_buttons(input::Input const& input, IOState& state)
    {
        constexpr auto key_on = ColorId::LightBlue;
        constexpr auto key_off = ColorId::Blue;

        auto& controller = input.controller;
        auto& state_data = *state.data_;
        auto& filter = state_data.controller_filter;
        auto& view = state_data.controller_view;

        auto const map_input = [](auto const& btn, auto const& filter)
        {
            auto color_id = btn.is_down ? key_on :key_off;
            set_filter_color(filter, color_id);
        };

        map_input(controller.btn_dpad_up, filter.btn_dpad_up);
        map_input(controller.btn_dpad_down, filter.btn_dpad_down);
        map_input(controller.btn_dpad_left, filter.btn_dpad_left);
        map_input(controller.btn_dpad_right, filter.btn_dpad_right);

        map_input(controller.btn_a, filter.btn_a);
        map_input(controller.btn_b, filter.btn_b);
        map_input(controller.btn_x, filter.btn_x);
        map_input(controller.btn_y, filter.btn_y);

        map_input(controller.btn_start, filter.btn_start);
        map_input(controller.btn_back, filter.btn_back);

        map_input(controller.btn_shoulder_left, filter.btn_sh_left);
        map_input(controller.btn_shoulder_right, filter.btn_sh_right);

        auto color_id = controller.trigger_left > 0.0f ? key_on :key_off;
        set_filter_color(filter.btn_tr_left, color_id);

        color_id = controller.trigger_right > 0.0f ? key_on :key_off;
        set_filter_color(filter.btn_tr_right, color_id);

        color_id = controller.btn_stick_left.is_down || controller.stick_left.magnitude > 0.3f ? key_on :key_off;
        set_filter_color(filter.btn_st_left, color_id);

        color_id = controller.btn_stick_right.is_down || controller.stick_right.magnitude > 0.3f ? key_on :key_off;
        set_filter_color(filter.btn_st_right, color_id);

        apply_filters(filter.filter, view);
    }


    static void update_keyboard_buttons(input::Input const& input, IOState& state)
    {
        constexpr auto key_on = ColorId::LightBlue;
        constexpr auto key_off = ColorId::Blue;

        auto& keyboard = input.keyboard;
        auto& state_data = *state.data_;
        auto& filter = state_data.keyboard_filter;
        auto& view = state_data.keyboard_view;
        
        auto const map_input = [](auto const& btn, auto const& filter)
        {
            auto color_id = btn.is_down ? key_on :key_off;
            set_filter_color(filter, color_id);
        };

        map_input(keyboard.kbd_1, filter.key_1);
        map_input(keyboard.kbd_2, filter.key_2);
        map_input(keyboard.kbd_3, filter.key_3);
        map_input(keyboard.kbd_4, filter.key_4);
        map_input(keyboard.kbd_W, filter.key_w);
        map_input(keyboard.kbd_A, filter.key_a);
        map_input(keyboard.kbd_S, filter.key_s);
        map_input(keyboard.kbd_D, filter.key_d);
        map_input(keyboard.kbd_space, filter.key_space);

        apply_filters(filter.filter, view);
    }


    static void update_mouse_buttons(input::Input const& input, IOState& state)
    {
        constexpr auto key_on = ColorId::LightBlue;
        constexpr auto key_off = ColorId::Blue;

        auto& mouse = input.mouse;
        auto& state_data = *state.data_;
        auto& filter = state_data.mouse_filter;
        auto& view = state_data.mouse_view;
        
        auto color_id = mouse.btn_left.is_down ? key_on : key_off;
        set_filter_color(filter.btn_left, color_id);

        color_id = mouse.btn_right.is_down ? key_on : key_off;
        set_filter_color(filter.btn_right, color_id);

        color_id = (mouse.btn_middle.is_down || mouse.wheel.y != 0) ? key_on : key_off;
        set_filter_color(filter.btn_middle, color_id);

        apply_filters(filter.filter, view);
    }


    void update_mouse_coords(input::Input const& input, IOState& state)
    {
        auto& state_data = *state.data_;
        auto mouse_pos = input.mouse.window_pos;

        auto& coords = state_data.mouse_coords;

        sp::zero_string(coords);
        qsnprintf(coords.begin, coords.capacity, "(%d, %d)", mouse_pos.x, mouse_pos.y);
        coords.length = span::strlen(coords.begin);

        render_text(sp::to_cstr(coords), state_data.mouse_coords_view);
    }
}


namespace input_display
{
    bool init(IOState& state)
    {
        if (!create_state_data(state))
        {
            return false;
        }

        if (!init_display_view(state))
        {
            close(state);
            return false;
        }

        return true;
    }


    void update(input::Input const& input, IOState& state)
    {
        img::fill(state.display, GRAY);
        update_controller_buttons(input, state);
        update_keyboard_buttons(input, state);
        update_mouse_buttons(input, state);
        update_mouse_coords(input, state);
    }


    void close(IOState& state)
    {
        destroy_state_data(state);
    }
}