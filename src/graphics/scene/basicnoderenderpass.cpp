#include <sp2/graphics/scene/basicnoderenderpass.h>
#include <sp2/scene/scene.h>
#include <sp2/scene/node.h>
#include <sp2/scene/camera.h>
#include <sp2/logging.h>

namespace sp {

BasicNodeRenderPass::BasicNodeRenderPass(string target_layer)
: RenderPass(target_layer)
{
}

void BasicNodeRenderPass::addScene(P<Scene> scene, P<Camera> camera)
{
    scenes.emplace_back();
    SceneWithCamera& scene_data = scenes.back();
    scene_data.scene = scene;
    scene_data.camera = camera;
}

void BasicNodeRenderPass::render(sf::RenderTarget& target, P<GraphicsLayer> layer, float aspect_ratio)
{
    if (!scenes.empty())
    {
        for(SceneWithCamera& scene_data : scenes)
        {
            renderScene(scene_data.scene, scene_data.camera, target, layer, aspect_ratio);
        }
    }else{
        for(Scene* scene : Scene::scenes)
        {
            renderScene(scene, nullptr, target, layer, aspect_ratio);
        }
    }
}

bool BasicNodeRenderPass::onPointerDown(io::Pointer::Button button, Vector2d position, int id)
{
    if (!scenes.empty())
    {
        for(SceneWithCamera& scene_data : scenes)
        {
            if (privateOnPointerDown(scene_data.scene, scene_data.camera, button, position, id))
                return true;
        }
    }
    else
    {
        for(Scene* scene : Scene::scenes)
        {
            if (privateOnPointerDown(scene, nullptr, button, position, id))
                return true;
        }
    }
    return false;
}

bool BasicNodeRenderPass::privateOnPointerDown(P<Scene> scene, P<Camera> camera, io::Pointer::Button button, Vector2d position, int id)
{
    if (!camera)
        camera = scene->getCamera();
    if (!camera)
        return false;
    Matrix4x4d matrix = camera->getGlobalTransform() * camera->getProjectionMatrix().inverse();
    Ray3d ray(matrix * Vector3d(position.x, position.y, 0), matrix * Vector3d(position.x, position.y, -1));
    if (scene->onPointerDown(button, ray, id))
    {
        pointer_scene[id] = scene;
        pointer_camera[id] = camera;
        return true;
    }
    return false;
}

void BasicNodeRenderPass::onPointerDrag(Vector2d position, int id)
{
    auto it = pointer_scene.find(id);
    if (it != pointer_scene.end() && it->second)
    {
        P<Camera> camera = pointer_camera[id];
        Matrix4x4d matrix = camera->getGlobalTransform() * camera->getProjectionMatrix().inverse();
        Ray3d ray(matrix * Vector3d(position.x, position.y, 0), matrix * Vector3d(position.x, position.y, -1));
        it->second->onPointerDrag(ray, id);
    }
}

void BasicNodeRenderPass::onPointerUp(Vector2d position, int id)
{
    auto it = pointer_scene.find(id);
    if (it != pointer_scene.end() && it->second)
    {
        P<Camera> camera = pointer_camera[id];
        Matrix4x4d matrix = camera->getGlobalTransform() * camera->getProjectionMatrix().inverse();
        Ray3d ray(matrix * Vector3d(position.x, position.y, 0), matrix * Vector3d(position.x, position.y, -1));
        it->second->onPointerUp(ray, id);
        pointer_scene.erase(it);
    }
}

void BasicNodeRenderPass::renderScene(P<Scene> scene, P<Camera> camera, sf::RenderTarget& target, P<GraphicsLayer> layer, float aspect_ratio)
{
    if (!camera)
        camera = scene->getCamera();

    if (scene->isEnabled() && camera)
    {
        camera->setAspectRatio(aspect_ratio);
        queue.clear();
        recursiveNodeRender(*scene->getRoot());
        queue.render(camera->getProjectionMatrix(), camera->getGlobalTransform().inverse(), target);
    }
}

void BasicNodeRenderPass::recursiveNodeRender(Node* node)
{
    if (node->render_data.type != sp::RenderData::Type::None && node->render_data.mesh)
        queue.add(node->getGlobalTransform(), node->render_data);

    for(Node* child : node->getChildren())
    {
        recursiveNodeRender(child);
    }
}

};//!namespace sp
