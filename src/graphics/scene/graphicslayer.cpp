#include <sp2/graphics/scene/graphicslayer.h>
#include <sp2/graphics/scene/renderpass.h>
#include <sp2/logging.h>
#include <sp2/graphics/opengl.h>
#include <sp2/graphics/shader.h>

namespace sp {

SceneGraphicsLayer::SceneGraphicsLayer(int priority)
: GraphicsLayer(priority)
{
}

SceneGraphicsLayer::~SceneGraphicsLayer()
{
    for(RenderPass* pass : render_passes)
        delete pass;
}

void SceneGraphicsLayer::render(sf::RenderTarget& window)
{
    int pixel_width = viewport.width * window.getSize().x;
    int pixel_height = viewport.height * window.getSize().y;
    glViewport(viewport.left * window.getSize().x, viewport.top * window.getSize().y, pixel_width, pixel_height);
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthFunc(GL_LEQUAL);
    
    float aspect_ratio = double(pixel_width) / double(pixel_height);

    //TODO: Figure out proper rendering order.
    for(RenderPass* pass : render_passes)
    {
        string target = pass->getTargetLayer();
        if (target == "window")
            pass->render(window, this, aspect_ratio);
        else
            pass->render(*targets[target], this, aspect_ratio);
    }

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    sf::Shader::bind(nullptr);
}

bool SceneGraphicsLayer::onPointerDown(io::Pointer::Button button, sf::Vector2f position, int id)
{
    return false;
}

void SceneGraphicsLayer::onPointerDrag(sf::Vector2f position, int id)
{
}

void SceneGraphicsLayer::onPointerUp(sf::Vector2f position, int id)
{
}

void SceneGraphicsLayer::addRenderPass(P<RenderPass> render_pass)
{
    render_passes.add(render_pass);
}

void SceneGraphicsLayer::createRenderTarget(string name)
{
    targets[name] = new sf::RenderTexture();
}

};//!namespace sp
