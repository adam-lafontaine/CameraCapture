#include "../../../libs/image/image.hpp"

#include <filesystem>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <cstdio>
#include <cassert>

namespace img = image;
namespace fs = std::filesystem;

using Image = img::Image;


/* image files */

namespace
{
    const auto ROOT_DIR = fs::path("/home/adam/Repos/CameraCapture/camera");
    const auto IMAGE_DIR = ROOT_DIR / "assets/image";
    const auto DST_DIR = ROOT_DIR / "src/res/image";
    const auto RESOURCE_CPP_PATH = DST_DIR / "res.cpp";


    struct CodeFile
    {
        cstr src_filename;
        cstr dst_name;
    };


    std::vector<CodeFile> input_display_code_files = {

        { "controller.png", "controller" },
        { "keyboard.png",   "keyboard" },
        { "mouse.png",      "mouse" }
    };
}


namespace
{
    inline bool load_image(fs::path const& path, Image& image)
    {
        return img::read_image_from_file(path.string().c_str(), image);
    }


    static bool write_to_cpp_file(std::string const& str, cstr name)
    {
        auto filepath = DST_DIR / (std::string(name) + ".cpp");

        std::ofstream file(filepath);

        if (!file.is_open())
        {
            return false;
        }

        file << str;

        file.close();

        return true;
    }
}


namespace
{
    class ImageDef
    {
    public:
        u32 width;
        u32 height;

        std::vector<u32> color24_lut;

        std::vector<u16> colors_ids;
        std::vector<u8> alpha;
    };


    static u16 get_lut_id(ImageDef& def, u32 color24)
    {
        for (u16 i = 0; i < def.color24_lut.size(); i++)
        {
            if (color24 == def.color24_lut[i])
            {
                return i;
            }
        }

        auto id = (u16)def.color24_lut.size();
        def.color24_lut.push_back(color24);

        return id;
    }


    static ImageDef to_image_def(img::ImageView const& view)
    {
        ImageDef def{};
        
        auto const n_pixels = view.width * view.height;

        def.width = view.width;
        def.height = view.height;

        def.color24_lut.reserve(512);
        def.colors_ids.resize(n_pixels);
        def.alpha.resize(n_pixels);
        
        def.color24_lut.push_back(0);

        u32 i = 0;
        img::for_each_pixel(view, [&](img::Pixel c)
        {
            def.alpha[i] = c.alpha;

            if (!c.alpha)
            {
                def.colors_ids[i] = 0;
            }
            else
            {
                c.alpha = 0;

                u32 color24 = img::as_u32(c);
                def.colors_ids[i] = get_lut_id(def, color24);
            }
            
            ++i;
        });

        assert(def.color24_lut.size() < 512);

        return def;
    }


    static std::string to_cpp_text(img::Image const& raw_image, CodeFile file)
    {
        auto name = file.dst_name;

        auto def = to_image_def(img::make_view(raw_image));

        std::ostringstream oss;

        oss
        << "// " << name << ".cpp\n"

        << "static constexpr struct\n"
        << "{\n"
        << "    unsigned width;\n"
        << "    unsigned height;\n";

        oss
        << "    unsigned color24_lut[" << def.color24_lut.size() << "];\n"
        << "    unsigned " << (def.color24_lut.size() < 256 ? "char" : "short") << " color_ids[" << def.colors_ids.size() << "];\n"
        << "    unsigned char alpha[" << def.alpha.size() << "];\n"
        << "}\n"
        << name << " = \n"
        << "{\n"
        << "    " << def.width << ", // width\n"
        << "    " << def.height << ", // height\n";

        oss
        << "\n"

        << "    { // color24_lut\n"
        << "    ";

        oss << std::hex;

        u32 n = 0;
        for (u32 i = 0; i < def.color24_lut.size(); i++)
        {
            oss << "0x"<< def.color24_lut[i] << ", ";
            ++n;
            if (n == 20)
            {
                oss << "\n    ";
                n = 0;
            }
        }

        oss << std::dec
        << "\n"
        << "    }, // color24_lut\n"
        << "\n"
        << "    { // color_ids\n";

        for (u32 y = 0; y < def.height; y++)
        {
            auto begin = y * def.width;
            oss << "    ";        
            for (u32 x = 0; x < def.width; x++)
            {
                oss << std::setw(3) << (int)def.colors_ids[begin + x] << ", ";
            }
            oss << "\n";
        }

        oss
        << "    }, // color_ids\n"
        << "\n"
        << "    { // alpha\n";

        for (u32 y = 0; y < def.height; y++)
        {
            auto begin = y * def.width;
            oss << "    ";
            for (u32 x = 0; x < def.width; x++)
            {
                oss << std::setw(3) << (int)def.alpha[begin + x] << ", ";
            }
            oss << "\n";
        }

        oss
        << "    }, // alpha\n"

        << "};\n"
        ;

        return oss.str();
    }


    static bool image_to_code(Image const& image, CodeFile file)
    {
        auto text = to_cpp_text(image, file);
        return write_to_cpp_file(text, file.dst_name);
    }


    static bool write_resource_file()
    {
        auto filepath = RESOURCE_CPP_PATH;

        std::ofstream file(filepath);

        if (!file.is_open())
        {
            return false;
        }

        for (auto item : input_display_code_files)
        {
            file << "#include \"" << item.dst_name << ".cpp\"\n";
        }

        file.close();

        return true;
    }
}


int main()
{
    Image image;

    for (auto item : input_display_code_files)
    {
        printf("%s: ", item.src_filename);

        printf("load: ");
        if(!load_image(IMAGE_DIR / item.src_filename, image))
        {
            printf("FAIL\n");
            return EXIT_FAILURE;
        }

        printf("OK | write: ");

        if (!image_to_code(image, item))
        {
            printf("FAIL\n");
            return EXIT_FAILURE;
        }

        printf("OK\n");

        img::destroy_image(image);
    }

    printf("resource file: ");
    if (write_resource_file())
    {        
        printf("OK\n");
    }
    else
    {
        printf("FAIL\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

#include "../../../libs/image/image.cpp"
#include "../../../libs/span/span.cpp"
#include "../../../libs/alloc_type/alloc_type.cpp"
#include "../../../libs/stb_image/stb_image_options.hpp"