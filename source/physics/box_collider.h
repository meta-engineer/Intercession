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
        BoxCollider(glm::vec3 dimensions)
            : BoxCollider()
        {
            m_dimensions = dimensions;
        }
        ~BoxCollider()
        {}

        // Box collider must take into account transform matrix.
        bool intersects(std::shared_ptr<ICollider>& other) override
        {
            switch(other->type)
            {
                case(ColliderType::box):
                {
                    PLEEPLOG_DEBUG("Testing collision between types: box and box");
                    // other should be valid as long as we own it as a smart pointer so
                    // we can cast it's raw, as long as the raw pointer dies before the smart one
                    BoxCollider* otherBox = static_cast<BoxCollider*>(other.get());
                    UNREFERENCED_PARAMETER(otherBox);
                    return false;
                }
                default:
                {
                    PLEEPLOG_WARN("No collision implementation between types: " 
                        + std::to_string(this->type) + " and " + std::to_string(this->type));
                    return false;
                }
            }
        }

    private:
        // distance each face is from origin (in local space) like a radius
        // opposite faces will be uniform distance from entity origin
        glm::vec3 m_dimensions;
    };
}

#endif // BOX_COLLIDER_H