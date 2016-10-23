#include <sp2/scene/node.h>
#include <sp2/scene/scene.h>
#include <sp2/collision/shape.h>
#include <cmath>

namespace sp {

SceneNode::SceneNode(P<SceneNode> parent)
: scene(parent->scene), parent(parent)
{
    parent->children.add(this);
    collision_body2d = nullptr;
}

SceneNode::SceneNode(P<Scene> scene)
: scene(scene)
{
}

P<SceneNode> SceneNode::getParent()
{
    return parent;
}

P<Scene> SceneNode::getScene()
{
    return scene;
}

PVector<SceneNode> SceneNode::getChildren()
{
    return children;
}

void SceneNode::setPosition(sp::Vector2d position)
{
    translation.x = position.x;
    translation.y = position.y;
    updateLocalTransform();
}

void SceneNode::setPosition(sp::Vector3d position)
{
    translation = position;
    updateLocalTransform();
}

void SceneNode::setRotation(Quaterniond rotation)
{
    this->rotation = rotation;
    updateLocalTransform();
}

void SceneNode::setRotation(double rotation)
{
    this->rotation = Quaterniond::fromAngle(rotation);
    updateLocalTransform();
}

sp::Vector2d SceneNode::getLocalPosition2D()
{
    return sp::Vector2d(translation.x, translation.y);
}

double SceneNode::getLocalRotation2D()
{
    sp::Vector2d v = rotation * sp::Vector2d(1, 0);
    return std::atan2(v.y, v.x) / pi * 180.0f;
}

sp::Vector2d SceneNode::getGlobalPosition2D()
{
    return global_transform * sp::Vector2d(0, 0);
}

double SceneNode::getGlobalRotation2D()
{
    sp::Vector2d v = global_transform.applyDirection(sp::Vector2d(1, 0));
    return std::atan2(v.y, v.x) / pi * 180.0f;
}

void SceneNode::setCollisionShape(const collision::Shape& shape)
{
    shape.create(this);
}

void SceneNode::updateLocalTransform()
{
    local_transform = Matrix4x4d::translate(translation) * Matrix4x4d::fromQuaternion(rotation);
    updateGlobalTransform();
}

void SceneNode::updateGlobalTransform()
{
    if (parent)
        global_transform = parent->global_transform * local_transform;
    else
        global_transform = local_transform;

    for(P<SceneNode> n : children)
        n->updateGlobalTransform();
}

};//!namespace sp
