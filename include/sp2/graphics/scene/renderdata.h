#ifndef SP2_GRAPHICS_SCENE_RENDERDATA_H
#define SP2_GRAPHICS_SCENE_RENDERDATA_H

#include <sp2/string.h>
#include <SFML/Graphics/RenderTarget.hpp>
#include <vector>
#include <memory>

namespace sp {

class MeshData;
class RenderData
{
public:
    enum class RenderType
    {
        Normal,
        Transparent,
        Additive,
    };
    RenderType type;
    std::shared_ptr<MeshData> mesh;
    string texture;
    
    RenderData();
    
    bool operator<(const RenderData& data) const;
};

};//!namespace sp

#endif//SP2_GRAPHICS_SCENE_RENDERDATA_H
