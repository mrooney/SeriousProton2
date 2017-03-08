#ifndef SP2_SCENE_NODE_H
#define SP2_SCENE_NODE_H

#include <sp2/math/matrix4x4.h>
#include <sp2/math/quaternion.h>
#include <sp2/script/bindingObject.h>
#include <sp2/pointerList.h>
#include <sp2/graphics/scene/renderdata.h>

class b2Body;

namespace sp {
namespace collision {
class Shape;
class Shape2D;
}
class SceneNode;
class CollisionInfo
{
public:
    P<SceneNode> other;
    float force;
    sp::Vector2d position;
    sp::Vector2d normal;
};
class Scene;
class RenderData;
class SceneNode : public ScriptBindingObject
{
public:
    SceneNode(P<SceneNode> parent);
    virtual ~SceneNode();

    P<SceneNode> getParent();
    P<Scene> getScene();
    PList<SceneNode>& getChildren();
    void setParent(P<SceneNode> new_parent);
    
    void setPosition(sp::Vector2d position);
    void setPosition(sp::Vector3d position);
    void setRotation(double rotation);
    void setRotation(Quaterniond rotation);
    void setLinearVelocity(sp::Vector2d velocity);
    void setLinearVelocity(sp::Vector3d velocity);
    void setAngularVelocity(double velocity);
    void setAngularVelocity(Quaterniond velocity);
    
    sp::Vector2d getLocalPosition2D();
    double getLocalRotation2D();
    sp::Vector2d getGlobalPosition2D();
    double getGlobalRotation2D();
    sp::Vector2d getLocalPoint2D(sp::Vector2d v);
    sp::Vector2d getGlobalPoint2D(sp::Vector2d v);
    
    sp::Vector2d getLinearVelocity2D();
    double getAngularVelocity2D();
    
    const Matrix4x4d& getGlobalTransform() const { return global_transform; }
    const Matrix4x4d& getLocalTransform() const { return local_transform; }
    
    void setCollisionShape(const collision::Shape& shape);
    bool testCollision(sp::Vector2d position);
    
    //Event called every frame.
    //The delta is the time in seconds passed sinds the previous frame, multiplied by the global game speed.
    //Called when the game is paused with delta = 0
    virtual void onUpdate(float delta) {}
    //Event called 30 times per second. Not called when the game is paused.
    virtual void onFixedUpdate() {}
    //Event called when 2 nodes collide. Not called when the game is paused.
    virtual void onCollision(CollisionInfo& info) {}
    
    RenderData render_data;
private:
    SceneNode(Scene* scene);

    P<Scene> scene;
    P<SceneNode> parent;
    PList<SceneNode> children;
    b2Body* collision_body2d;
    
    Vector3d translation;
    Quaterniond rotation;
    
    Matrix4x4d global_transform;
    Matrix4x4d local_transform;
    
    void updateLocalTransform();
    void updateGlobalTransform();
    
    void modifyPositionByPhysics(sp::Vector2d position, double rotation);
    void modifyPositionByPhysics(sp::Vector3d position, Quaterniond rotation);
    
    friend class Scene;
    friend class collision::Shape2D;
};

};//!namespace sp

#endif//SP2_SCENE_NODE_H
