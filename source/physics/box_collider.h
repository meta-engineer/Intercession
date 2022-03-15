#ifndef BOX_COLLIDER_H
#define BOX_COLLIDER_H

//#include "intercession_pch.h"
#include <string>
#include <glm/glm.hpp>

#include "physics/i_collider.h"
#include "logging/pleep_log.h"

namespace pleep
{
    class BoxCollider : public ICollider
    {
    public:
        BoxCollider()
            : ICollider(ColliderType::box)
            , m_dimensions(glm::vec3(1.0f))
        {
        }
        // dimensions is actually optional with default glm::vec3(1.0f)
        BoxCollider(glm::vec3 dimensions)
            : BoxCollider()
        {
            m_dimensions = dimensions;
        }

        // Double dispatch other
        bool intersects(ICollider* other, 
            TransformComponent& thisTransform,
            TransformComponent& otherTransform) override
        {
            // any integral return values should be inverted
            return other->intersects(this, otherTransform, thisTransform);
        }
        
        bool intersects(BoxCollider* other, 
            TransformComponent& thisTransform,
            TransformComponent& otherTransform) override
        {
            PLEEPLOG_DEBUG("Testing collision between types: box and box");
            UNREFERENCED_PARAMETER(other);
            UNREFERENCED_PARAMETER(thisTransform);
            UNREFERENCED_PARAMETER(otherTransform);
            return false;
        }

    private:
        // distance each face is from origin (in local space) like a radius
        // opposite faces will be uniform distance from entity origin
        glm::vec3 m_dimensions;
    };
}

#endif // BOX_COLLIDER_H