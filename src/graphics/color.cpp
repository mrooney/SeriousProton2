#include <sp2/graphics/color.h>

namespace sp {

HsvColor::HsvColor(const sp::Color& color)
{
    auto max = std::max(std::max(color.r, color.g), color.b);
    auto min = std::min(std::min(color.r, color.g), color.b);
    
    auto delta = max - min;
    if (delta <= 0.0)
    {
        hue = 0.0;
        saturation = 0.0;
        value = double(max) * 100;
        return;
    }
    hue = 0.0;
    saturation = double(delta) / double(max) * 100.0;
    value = max * 100;
    if (color.r == max)
        hue = double(color.g - color.b) / double(max);
    else if (color.g == max)
        hue = 2.0 + double(color.b - color.r) / double(max);
    else
        hue = 4.0 + double(color.r - color.g) / double(max);
    hue *= 60.0;
    if (hue < 0)
        hue += 360.0;
}

}//namespace sp
