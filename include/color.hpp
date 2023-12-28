#include <string>

namespace style
{
    enum FONT_COLOR
    {
        red = 31,
        green,
        yellow,
        blue,
        purple,
        cyan,
        white
    };

    enum FONT_STYLE
    {
        none = 0,
        bold,
        faint,
        italic,
        underlined,
        blink
    };

    inline const std::string apply(const FONT_STYLE fs, const FONT_COLOR fc)
    {
        return "\033[" + std::to_string(fs) + ";" + std::to_string(fc) + "m";
    }

    inline const std::string reset() {
        return "\033[0m";
    }
}