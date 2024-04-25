#include "collision_procedures.h"

namespace pleep
{
    bool null_null_intersect(
        ColliderPacket&,
        ColliderPacket&,
        glm::vec3&, glm::vec3&, float&
    )
    {
        return false;
    }

    bool box_box_intersect(
        ColliderPacket& dataA,
        ColliderPacket& dataB,
        glm::vec3& collisionPoint, glm::vec3& collisionNormal, float& collisionDepth
    )
    {
        assert(dataA.collider.colliderType == ColliderType::box);
        assert(dataB.collider.colliderType == ColliderType::box);

        // SAT algorithm (actually Separating Plane Theorum)
        // we can optimize SPT given our polyhedra are rectangular prisms
        // the size of the projected interval of a prism along its own face normal
        // will always be its sidelength (it is always axis aligned to itself)
        // so we can project its centre (origin) and then +/- its sidelength

        // for each face normal (in both shapes) generate all possible axes
        // then use each of those to generate the vec2 interval
        // NOTE: For edge-to-end collision we also need to check 9 cases for 
        // each face normal cross-product with the 3 of box collider B

        // NOTE: angle between 3d vectors:
        // cos(angle) = v1.x*v2.x + v1.y*v2.y + v1.z*v2.z / (|v1| * |v2|);
        // projection of v1 onto v2:
        // v1_proj = cos(angle) * |v1| * norm(v2)

        // entity transform non-uniform scale might not be applicable to certain colliders
        glm::mat4 localTransformA  = dataA.collider.compose_transform(dataA.transform);
        glm::mat3 normalTransformA = glm::transpose(glm::inverse(glm::mat3(localTransformA)));
        glm::mat4 localTransformB  = dataB.collider.compose_transform(dataB.transform);
        glm::mat3 normalTransformB = glm::transpose(glm::inverse(glm::mat3(localTransformB)));

        std::array<glm::vec3, 15> axes;
        // each calculated interval must be comparable, so they need to be using
        //   equal sized basis vector (aka normalized)
        // we'll wait until the axes are tested to normalize incase of early exits

        // this' face normals (w=0.0f -> no translation)
        axes[0] = normalTransformA * glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
        axes[1] = normalTransformA * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
        axes[2] = normalTransformA * glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);
        // box B's face normals (w=0.0f -> no translation)
        axes[3] = normalTransformB * glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
        axes[4] = normalTransformB * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
        axes[5] = normalTransformB * glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);
        // edge "normals"
        axes[6] = glm::cross(axes[0], axes[3]);
        axes[7] = glm::cross(axes[0], axes[4]);
        axes[8] = glm::cross(axes[0], axes[5]);
        axes[9] = glm::cross(axes[1], axes[3]);
        axes[10]= glm::cross(axes[1], axes[4]);
        axes[11]= glm::cross(axes[1], axes[5]);
        axes[12]= glm::cross(axes[2], axes[3]);
        axes[13]= glm::cross(axes[2], axes[4]);
        axes[14]= glm::cross(axes[2], axes[5]);

        // Maintain data from positive collision tests
        glm::vec3 minPenetrateNormal(0.0f);   // vector of collision force? world-space
        float minPenetrateDepth = INFINITY;   // depth along best normal?

        // continuous collision:
        // get velocity of this relative to box B (thisVelocity - otherVelocity)
        // extend the interval of this by that relative velocity

        for (int a = 0; a < 15; a++)
        {
            // wait as late as possible to normalize each axis
            axes[a] = glm::normalize(axes[a]);
            // CHECK! if colliders are alligned cross products might not work!
            if (isnan(axes[a].x))
                continue;

            glm::vec2 intervalA = project_box(localTransformA, axes[a]);
            glm::vec2 intervalB = project_box(localTransformB, axes[a]);

            // no continuous collision for now
            // TODO: get relative velocity, extend intervalA by velocity
            // TODO: determine direction of normal based on velocities

            float penetration = 0;
            bool flipAxis = false;
            // determine direction of penetration by midpoints until we account for velocities
            if ((intervalA.x + intervalA.y)/2.0f > (intervalB.x + intervalB.y)/2.0f)
            {
                penetration = intervalB.y - intervalA.x;
            }
            else
            {
                penetration = intervalA.y - intervalB.x;
                // other is ahead of this, so flip axis so that it points "away" from other's surface towards this
                flipAxis = true;
            }
            
            // is there a seperating plane for this axis?
            if (penetration <= 0)
            {
                return false;
            }

            // does this axes have the best overlap?
            if (penetration < minPenetrateDepth)
            {
                minPenetrateDepth = penetration;
                minPenetrateNormal = flipAxis ? -axes[a] : axes[a];
            }
        }

        // write to references for collision normal, depth (and collision location?)
        collisionNormal = minPenetrateNormal;
        collisionDepth = minPenetrateDepth;

        // use collisionNormal to find penetrating points
        // we could maybe have tracked & maintained these points during projection...
        // Find all points in contact manifold for each object

        std::vector<glm::vec3> contactManifoldA;
        build_contact_manifold(localTransformA, -collisionNormal, collisionDepth, contactManifoldA);

        std::vector<glm::vec3> contactManifoldB;
        build_contact_manifold(localTransformB, collisionNormal, collisionDepth, contactManifoldB);

        // Solve for collisionPoint depending on size of manifolds found
        if (contactManifoldB.size() == 0 || contactManifoldA.size() == 0)
        {
            PLEEPLOG_ERROR("Contact Manifold(s) could not be built somehow?!");
            return false;
        }
        else if (contactManifoldB.size() == 1)
        {
            // single penetrating vertex on box B
            // prefer using single vertex of box B first
            collisionPoint = contactManifoldB.front();
            return true;
        }
        else if (contactManifoldA.size() == 1)
        {
            // single penetrating vertex on this
            // invert to point to be on surface of other
            collisionPoint = contactManifoldA.front() + (collisionNormal * collisionDepth);
            return true;
        }
        else
        {
            //PLEEPLOG_WARN("Non trivial Manifolds... expect something to go wrong!");

            // use this' manifold to clip box B's manifold
            pseudo_clip_polyhedra(contactManifoldA, contactManifoldB, collisionNormal);

            // something went wrong :(
            assert(!contactManifoldB.empty());
            // TODO: for "production" we may want a emergency clause to use simple average if clipping fails

            // now box B is completely clipped, use it to find "average" contact point

            // if clipped manifold is only 1 vertex then we can short-cut as the above cases do
            if (contactManifoldB.size() == 1)
            {
                collisionPoint = contactManifoldB.front();
                return true;
            }

            // "Rounding algorithm" of manifold plane
            // we want less deep verticies to contribute less to average
            // assign weights based on their dot product with collisionNormal
            // relative to the max depth dot product

            // generate max coeff from clipped manifold
            // and accumulate average of points at max depth, set that as manifold origin
            float maxCoeff = -INFINITY;
            glm::vec3 manifoldOrigin(0.0f);
            float originContributors = 0;
            for (glm::vec3 v : contactManifoldB)
            {
                const float vertCoeff = glm::dot(v, collisionNormal);
                if (vertCoeff > maxCoeff)
                {
                    // reset max & origin
                    manifoldOrigin = v;
                    originContributors = 1;
                    maxCoeff = vertCoeff;
                }
                else if (vertCoeff == maxCoeff)
                {
                    manifoldOrigin += v;
                    originContributors++;
                }
                // ignore points under current max
            }
            // ensure maxCoeff calculation was valid
            assert(originContributors > 0);
            // safe from divide by zero
            manifoldOrigin /= originContributors;

            // scale box B points' weight based on its dot product to collisionNormal
            // linearly scaling out to some min % at a distance of manifoldDepth
            const float manifoldRange = 1.0f - MANIFOLD_MIN_WEIGHT;
            
            collisionPoint = glm::vec3(0.0f);
            for (glm::vec3 v : contactManifoldB)
            {
                const float vertCoeff = glm::dot(v, collisionNormal);
                const glm::vec3 relativeVertex = v - manifoldOrigin;
                // scale dot product coefficient to range [0 : manifoldDepth],
                // then scale that range to range [0 : 1.0]
                float vertWeight = (vertCoeff-maxCoeff+MANIFOLD_DEPTH) / MANIFOLD_DEPTH;
                // make weight non-linear?
                //vertWeight *= vertWeight;
                // then scale that range to range [minWeight : 1.0]
                collisionPoint += relativeVertex * (vertWeight * manifoldRange + MANIFOLD_MIN_WEIGHT);
            }
            collisionPoint /= contactManifoldB.size();
            collisionPoint += manifoldOrigin;

            return true;
        }

        // we should never get here
        //return false;
    }
    
    bool ray_box_intersect(
        ColliderPacket& dataA,
        ColliderPacket& dataB,
        glm::vec3& collisionPoint, glm::vec3& collisionNormal, float& collisionDepth
    )
    {
        assert(dataA.collider.colliderType == ColliderType::ray);
        assert(dataB.collider.colliderType == ColliderType::box);

        // Perform SAT only on the box's axes
        
        glm::mat4 localTransformA   = dataA.collider.compose_transform(dataA.transform);
        glm::mat3 normalTransformA  = glm::transpose(glm::inverse(glm::mat3(localTransformA)));
        glm::mat4 localTransformB   = dataB.collider.compose_transform(dataB.transform);
        glm::mat3 normalTransformB  = glm::transpose(glm::inverse(glm::mat3(localTransformB)));

        std::array<glm::vec3, 6> axes;
        // TODO: determine which axes are actually needed
        // cube faces axes
        axes[0] = normalTransformB * glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
        axes[1] = normalTransformB * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
        axes[2] = normalTransformB * glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);

        // calculate vector from ray origin to box origin
        glm::vec3 rayToBox = dataB.transform.origin - dataA.transform.origin;
        // calulate ray vector
        const glm::vec3 rayOrigin = localTransformA * glm::vec4(0,0,0, 1.0f);
        const glm::vec3 rayEnd    = localTransformA * glm::vec4(0,0,1, 1.0f);
        axes[3] = rayEnd - rayOrigin;

        // calculate normal to ray (perpendicular to box)
        axes[4] = glm::cross(axes[3], rayToBox);
        // calculate normal to ray (towards box)
        axes[5] = glm::cross(axes[4], axes[3]);
        
        // Maintain data from positive collision tests
        glm::vec3 minPenetrateNormal(0.0f);   // vector of collision force? world-space
        float minPenetrateDepth = INFINITY;   // depth along best normal?
        float maxRayParametricValue = 0.0f;   // in lieu of contact manifold

        for (int a = 0; a < axes.size(); a++)
        {
            axes[a] = glm::normalize(axes[a]);
            
            glm::vec2 rayProject  = project_ray(localTransformA,  axes[a]);
            glm::vec2 boxInterval = project_box(localTransformB, axes[a]);

            float penetration = 0;
            float rayParametricValue = 0;
            bool flipAxis = false;

            // determine direction of penetration with ray origin until we account for velocities
            if (rayProject.x < (boxInterval.x + boxInterval.y)/2.0f)
            {
                // if box is ahead of ray, flip axis so it points away from box
                flipAxis = true;
            }

            // if ray origin is inside of box then rayParameter is 0 and penetration is: 
            //      if flipAxis==false min(ray origin, ray end) -> box top
            //      if flipAxis== true box bottom -> max(ray origin, ray end)
            // if ray origin is outside of box (but ray end is inside) then:
            //      if flipAxis==false rayParameter = max(box top, ray end) -> ray origin
            //                      & penetration = ray end -> box top (negative means no collision)
            //      if flipAxis== true rayParameter = ray origin -> min(box bottom, ray end)
            //                      & penetration = box bottom -> ray end (negative means no collision)

            // origin is inside
            if (rayProject.x >= boxInterval.x && rayProject.x <= boxInterval.y)
            {
                rayParametricValue = 0;
                if (!flipAxis)
                {
                    penetration = boxInterval.y - glm::min(rayProject.x, rayProject.y);
                }
                else
                {
                    penetration = glm::max(rayProject.x, rayProject.y) - boxInterval.x;
                }

                // collision must always happen
                //assert(penetration >= 0);
                if (penetration < 0)
                {
                    PLEEPLOG_WARN("Negative penetration for enclosed ray origin");
                    return false;
                }
            }
            // origin is outside
            else
            {
                if (!flipAxis)
                {
                    const float rayRange = rayProject.x - glm::max(boxInterval.y, rayProject.y);
                    const float rayPointDelta = (rayProject.x - rayProject.y);
                    if (rayPointDelta != 0.0f)
                        rayParametricValue = rayRange / rayPointDelta;
                    penetration = boxInterval.y - rayProject.y;
                }
                else
                {
                    const float rayRange = glm::min(boxInterval.x, rayProject.y) - rayProject.x;
                    const float rayPointDelta = (rayProject.y - rayProject.x);
                    if (rayPointDelta != 0.0f)
                        rayParametricValue = rayRange / rayPointDelta;
                    penetration = rayProject.y - boxInterval.x;
                }

                // neither origin OR end are within box interval -> seperating plane for this axis
                if (penetration < 0)
                    return false;
            }

            // does this axes have the best overlap? max ray parameter -> min penetration (like usual SAT)
            if (rayParametricValue >= maxRayParametricValue)
            {
                maxRayParametricValue = rayParametricValue;
                minPenetrateDepth = penetration;
                minPenetrateNormal = flipAxis ? -axes[a] : axes[a];
            }
        }
        // write to references for collision normal, depth
        collisionNormal = minPenetrateNormal;
        collisionDepth = minPenetrateDepth;

        // parametric value must always be valid
        //assert(maxRayParametricValue >= 0 && maxRayParametricValue <= 1);
        if (maxRayParametricValue < 0 || maxRayParametricValue > 1)
        {
            PLEEPLOG_WARN("Ray parametric value outside of possible range [0,1]");
            return false;
        }

        // remember closest parametric value this physics step to avoid double collision
        // TODO: this is order dependant, so not always correct
        if (maxRayParametricValue >= dataA.collider.minParametricValue)
        {
            return false;
        }
        dataA.collider.minParametricValue = maxRayParametricValue;

        //return this->solve_parametric(rayParametricValue);

        // inline solve parametric equation
        collisionPoint = rayOrigin + maxRayParametricValue * (rayEnd-rayOrigin);

        //PLEEPLOG_DEBUG("Ray Collision Point: " + std::to_string(collisionPoint.x) + ", " + std::to_string(collisionPoint.y) + ", " + std::to_string(collisionPoint.z));
        return true;
    }
    
    // just call ray_box and invert
    bool box_ray_intersect(
        ColliderPacket& dataA,
        ColliderPacket& dataB,
        glm::vec3& collisionPoint, glm::vec3& collisionNormal, float& collisionDepth
    )
    {
        if (ray_box_intersect(dataB, dataA, collisionPoint, collisionNormal, collisionDepth))
        {
            // collision metadata returned is relative to passed this, invert to be relative to other
            collisionNormal *= -1.0f;
            collisionPoint = collisionPoint + (collisionNormal * collisionDepth);
            return true;
        }
        return false;
    }

    bool ray_ray_intersect(
        ColliderPacket& dataA,
        ColliderPacket& dataB,
        glm::vec3& collisionPoint, glm::vec3& collisionNormal, float& collisionDepth
    )
    {
        // do rays need to intersect?
        UNREFERENCED_PARAMETER(dataA);
        UNREFERENCED_PARAMETER(dataB);
        UNREFERENCED_PARAMETER(collisionPoint);
        UNREFERENCED_PARAMETER(collisionNormal);
        UNREFERENCED_PARAMETER(collisionDepth);

        return false;
    }


    // PHYSICS RESPONSE PROCEDURES

    void null_null_response(
        ColliderPacket&, PhysicsComponent&, 
        ColliderPacket&, PhysicsComponent&, 
        glm::vec3&, glm::vec3&, float&
    )
    {
        return;
    }

    void rigid_rigid_response(
        ColliderPacket& dataA, PhysicsComponent& physicsA, 
        ColliderPacket& dataB, PhysicsComponent& physicsB, 
        glm::vec3& collisionPoint, glm::vec3& collisionNormal, float& collisionDepth
    )
    {
        // wake physics since collision has occurred?
        //physicsA.isAsleep = false;
        //physicsB.isAsleep = false;

        // STEP 1: Fetch entity properties
        // STEP 1.1: material physics properties
        // check mutability of each object (& test for early exit)
        if (physicsA.mass == INFINITE_MASS && physicsB.mass == INFINITE_MASS)
            return;
        
        // calculate inverse mass for convenience (account for Infinite mass)
        float invMassA = 0;
        if (physicsA.mass != INFINITE_MASS)
        {
            invMassA = 1.0f/physicsA.mass;
        }
        //PLEEPLOG_DEBUG("this inverse mass: " + std::to_string(invMassA));
        float invMassB = 0;
        if (physicsB.mass != INFINITE_MASS)
        {
            invMassB = 1.0f/physicsB.mass;
        }
        //PLEEPLOG_DEBUG("other inverse mass: " + std::to_string(invMassB));
        
        Collider& colliderA = dataA.collider;
        Collider& colliderB = dataB.collider;

        // STEP 1.2: material collision properties
        // TODO: average or product of material properties?
        // energy retained along normal
        const float restitutionFactor     = colliderA.restitution * colliderB.restitution;
        //PLEEPLOG_DEBUG("combined restitution: " + std::to_string(restitutionFactor));
        // max energy lost along tangent
        const float staticFrictionFactor  = colliderA.staticFriction * colliderB.staticFriction;
        // energy lost along tangent
        const float dynamicFrictionFactor = colliderA.dynamicFriction * colliderB.dynamicFriction;
        
        // STEP 2: static resolution
        // TODO: static resolution can cause objects in complex scenarios to "walk around" and not
        //   abide by the stability of the dynamic resolution, this can only be fixed with
        //   a continuous intersect detection and continuous resolution, so its a major refactor

        // STEP 2.1: resolve transform origin and collision point (on other)
        // share static resolution based on relative mass proportion
        // we are guarenteed to have at least one not be INFINITE_MASS 
        // 1 -> move only this, 0 -> move only other
        float massRatio = invMassA/(invMassA + invMassB);
        //PLEEPLOG_DEBUG("MassRatio of this: " + std::to_string(massRatio));

        // collisionNormal is in direction away from "other", towards "this"
        dataA.transform.origin += collisionNormal * collisionDepth * massRatio;
        dataB.transform.origin -= collisionNormal * collisionDepth * (1-massRatio);
        collisionPoint         -= collisionNormal * collisionDepth * (1-massRatio);

        // STEP 3: geometry properties
        // STEP 3.1: transform
        const glm::mat4 modelA = colliderA.compose_transform(dataA.transform);
        const glm::mat4 modelB = colliderB.compose_transform(dataB.transform);
        // STEP 3.1: center of mass
        // TODO: for a compound collider this will be more involved
        //   for now take origin of collider
        const glm::vec3 centerOfMassA = modelA * glm::vec4(0.0f,0.0f,0.0f, 1.0f);
        const glm::vec3 centerOfMassB = modelB * glm::vec4(0.0f,0.0f,0.0f, 1.0f);

        // STEP 3.2: vector describing the "radius" of the rotation
        const glm::vec3 leverA = (collisionPoint - centerOfMassA);
        const glm::vec3 leverB = (collisionPoint - centerOfMassB);

        //PLEEPLOG_DEBUG("This lever: " + std::to_string(leverA.x) + ", " + std::to_string(leverA.y) + ", " + std::to_string(leverA.z));
        //PLEEPLOG_DEBUG("Length of this lever: " + std::to_string(glm::length(leverA)));

        //PLEEPLOG_DEBUG("Other lever: " + std::to_string(leverB.x) + ", " + std::to_string(leverB.y) + ", " + std::to_string(leverB.z));
        //PLEEPLOG_DEBUG("Length of other lever: " + std::to_string(glm::length(leverB)));

        // STEP 3.3 relative velocity vector
        // relative is: this' velocity as viewed by other
        const glm::vec3 relVelocity = ((physicsA.velocity + glm::cross(physicsA.angularVelocity, leverA)) - (physicsB.velocity + glm::cross(physicsB.angularVelocity, leverB)));
        //PLEEPLOG_DEBUG("Relative Velocity at collision: " + std::to_string(relVelocity.x) + ", " + std::to_string(relVelocity.y) + ", " + std::to_string(relVelocity.z));

        // early exit if colliders are already moving away from eachother at collisionPoint
        if (glm::dot(relVelocity, collisionNormal) > 0)
        {
            //PLEEPLOG_DEBUG("Colliding rigid bodies are already moving away from one another, so I won't interupt");
            return;
        }

        // STEP 3.4: angular inertia/moment
        // TODO: can this be optimized? inverse of inverse :(
        // TODO: moment doesn't behave correct with scaled transforms
        //   copy transforms, extract scale, build inertia tensor with scale
        //   then transform tensor with scale-less model transform
        // each collider can restrict it as they see fit
        const glm::mat3 invModelA = glm::inverse(glm::mat3(modelA));
        const glm::mat3 invMomentA = invMassA == 0 ? glm::mat3(0.0f) 
            : glm::inverse(
                glm::transpose(invModelA) 
                * (colliderA.get_inertia_tensor(dataA.transform.scale) * physicsA.mass)
                * invModelA
            );

        const glm::mat3 invModelB = glm::inverse(glm::mat3(modelB));
        const glm::mat3 invMomentB = invMassB == 0 ? glm::mat3(0.0f)
            : glm::inverse(
                glm::transpose(invModelB)
                * (colliderB.get_inertia_tensor(dataB.transform.scale) * physicsB.mass)
                * invModelB
            );

        // STEP 4: determine normal impulse
        const float normalImpulse = (-1.0f * (1+restitutionFactor) * glm::dot(relVelocity, collisionNormal)) /
            (invMassB + invMassA +
                glm::dot(
                    glm::cross(invMomentB * glm::cross(leverB, collisionNormal), leverB) +
                    glm::cross(invMomentA * glm::cross(leverA, collisionNormal), leverA),
                    collisionNormal
                )
            );

        const float contactImpulse = (normalImpulse);
        //PLEEPLOG_DEBUG("Calculated Contact impulse to be: " + std::to_string(contactImpulse));

        // STEP 5: Friction
        // STEP 5.1: Determine velocity perpendicular to normal (tangent along surface)
        const glm::vec3 tangentCross = glm::cross(relVelocity, collisionNormal);
        //PLEEPLOG_DEBUG("tangentCross: " + std::to_string(tangentCross.x) + ", " + std::to_string(tangentCross.y) + ", " + std::to_string(tangentCross.z));
        glm::vec3 collisionTangent = glm::vec3(0.0f);
        if (glm::length2(tangentCross) != 0.0f)
        {
            // cross normal with tangent cross to get tangent direction
            collisionTangent = glm::cross(collisionNormal, tangentCross);
            //PLEEPLOG_DEBUG("Collision Tangent pre-normalize: " + std::to_string(collisionTangent.x) + ", " + std::to_string(collisionTangent.y) + ", " + std::to_string(collisionTangent.z));
            if (glm::length2(collisionTangent) != 0.0f)
                collisionTangent = glm::normalize(collisionTangent);
        }
        
        //PLEEPLOG_DEBUG("Collision Tangent: " + std::to_string(collisionTangent.x) + ", " + std::to_string(collisionTangent.y) + ", " + std::to_string(collisionTangent.z));

        // STEP 5.2: Determine friction impulse
        const float tangentImpulse = -1.0f * glm::dot(relVelocity, collisionTangent) /
            (invMassB + invMassA +
            glm::dot(
                glm::cross(invMomentB * glm::cross(leverB, collisionTangent), leverB) + 
                glm::cross(invMomentA * glm::cross(leverA, collisionTangent), leverA), 
                collisionTangent)
            );
            
        //PLEEPLOG_DEBUG("Calculated Friction impulse: " + std::to_string(tangentImpulse));
        
        // STEP 5.3: Coefficient factors
        // if impulse is less than static max, then aply it (this should negate all colinear velocity)
        // if impulse is greater than static max, multiply it by dynamic coefficient
        const float frictionCone = staticFrictionFactor * contactImpulse;
        //PLEEPLOG_DEBUG("Static friction limit: " + std::to_string(frictionCone));

        const float frictionImpulse = std::abs(tangentImpulse) < std::abs(frictionCone) ? tangentImpulse : tangentImpulse * dynamicFrictionFactor;
        //PLEEPLOG_DEBUG("Limited Friction impulse: " + std::to_string(frictionImpulse));

        // STEP 6: Damping
        // we have restitution/friction coefficients in impulses,
        // but we may need extra damping to avoid stuttering and floating point errors
        // Unfortunately it seems these kind of values need to be experimentally tweaked,
        //   and no single solution works for all cases/scales
        // only angular impulse really needs damping applied (See step 7.5)

        // ALSO, the other factors to change if instability occurs is the collider's
        // manifold epsilon, and its inertia tensor. A larger epsilon or larger tensor
        // values will increase stability
        
        // "slop" damping
        //const float flatDamping = 0.01f;

        // linear percentage damping
        //const float percentDamping = 0.98f;

        // exponential damping which is stronger approaching 0 relative velocity at collision point
        //const float invDampingStrength = 32;
        //const float dynamicDamping = calculate_damping(relVelocity, invDampingStrength);

        // exponential damping relative to difference of angular velocity
        //const float relativeAV2 = glm::length2(physicsA.angularVelocity - physicsB.angularVelocity);
        //const float avDamping = -1.0f / (1.0f + relativeAV2 * invDampingStrength) + 1.0f;


        // STEP 7: dynamic resolution
        // STEP 7.1: resolve linear normal impulse response
        physicsA.velocity  += invMassA * (contactImpulse*collisionNormal);
        physicsB.velocity -= invMassB * (contactImpulse*collisionNormal);

        // STEP 7.2 resolve linear friction impulse response
        physicsA.velocity  += invMassA * (frictionImpulse*collisionTangent);
        physicsB.velocity -= invMassB * (frictionImpulse*collisionTangent);

        // STEP 7.3: resolve angular normal impulse response
        if (colliderA.influenceOrientation)
        {
            const glm::vec3 thisAngularNormalImpulse  = invMomentA * glm::cross(leverA, (contactImpulse*collisionNormal));
            physicsA.angularVelocity  += thisAngularNormalImpulse;
            //PLEEPLOG_DEBUG("This Normal Angular Impulse: " + std::to_string(thisAngularNormalImpulse.x) + ", " + std::to_string(thisAngularNormalImpulse.y) + ", " + std::to_string(thisAngularNormalImpulse.z));
        }
        if (colliderB.influenceOrientation)
        {
            const glm::vec3 otherAngularNormalImpulse = invMomentB * glm::cross(leverB, (contactImpulse*collisionNormal));
            physicsB.angularVelocity -= otherAngularNormalImpulse;
            //PLEEPLOG_DEBUG("Other Normal Angular Impulse: " + std::to_string(-otherAngularNormalImpulse.x) + ", " + std::to_string(-otherAngularNormalImpulse.y) + ", " + std::to_string(-otherAngularNormalImpulse.z));
        }
        

        // STEP 7.4 resolve angular friction impulse response
        if (colliderA.influenceOrientation)
        {
            const glm::vec3 thisAngularFrictionImpulse  = invMomentA * glm::cross(leverA, (frictionImpulse*collisionTangent));
            physicsA.angularVelocity  += thisAngularFrictionImpulse;
            //PLEEPLOG_DEBUG("This Friction Angular Impulse: " + std::to_string(thisAngularFrictionImpulse.x) + ", " + std::to_string(thisAngularFrictionImpulse.y) + ", " + std::to_string(thisAngularFrictionImpulse.z));
        }
        if (colliderB.influenceOrientation)
        {
            const glm::vec3 otherAngularFrictionImpulse = invMomentB * glm::cross(leverB, (frictionImpulse*collisionTangent));
            physicsB.angularVelocity -= otherAngularFrictionImpulse;
            //PLEEPLOG_DEBUG("Other Friction Angular Impulse: " + std::to_string(-otherAngularFrictionImpulse.x) + ", " + std::to_string(-otherAngularFrictionImpulse.y) + ", " + std::to_string(-otherAngularFrictionImpulse.z));
        }

        // STEP 7.5: apply angular dampening
        // we'll linearly damp angular velocities after impulse to try to break out of any equilibriums
        
        if (colliderA.influenceOrientation)
        {
            physicsA.angularVelocity  *= 1.0f - physicsA.collisionAngularDrag;
        }
        if (colliderB.influenceOrientation)
        {
            physicsB.angularVelocity *= 1.0f - physicsB.collisionAngularDrag;
        }
    }

    void spring_rigid_response(
        ColliderPacket& dataA, PhysicsComponent& physicsA, 
        ColliderPacket& dataB, PhysicsComponent& physicsB, 
        glm::vec3& collisionPoint, glm::vec3& collisionNormal, float& collisionDepth
    )
    {
        // find spring force of myself, then apply equal-and-opposite, and apply friction
        
        // wake physics since collision has occurred? maybe this should be in top level dispatch
        //physicsA.isAsleep = false;
        //physicsB.isAsleep = false;

        // STEP 1: Fetch entity properties
        // STEP 1.1: material physics properties
        // check mutability of each object (& test for early exit)
        if (physicsA.mass == INFINITE_MASS && physicsB.mass == INFINITE_MASS)
            return;
            
        // calculate inverse mass for convenience (account for Infinite mass)
        float invMassA = 0;
        if (physicsA.mass != INFINITE_MASS)
        {
            invMassA = 1.0f/physicsA.mass;
        }
        //PLEEPLOG_DEBUG("this inverse mass: " + std::to_string(invMassA));
        float invMassB = 0;
        if (physicsB.mass != INFINITE_MASS)
        {
            invMassB = 1.0f/physicsB.mass;
        }
        //PLEEPLOG_DEBUG("other inverse mass: " + std::to_string(invMassB));

        Collider& colliderA = dataA.collider;
        Collider& colliderB = dataB.collider;
        
        // STEP 1.2: material collision properties
        // only use this's spring properties
        // TODO: centrialize this method for all collision_response()'s
        // max energy lost along tangent
        const float staticFrictionFactor  = colliderA.staticFriction * colliderB.staticFriction;
        // energy lost along tangent
        const float dynamicFrictionFactor = colliderA.dynamicFriction * colliderB.dynamicFriction;

        // STEP 2: Geometry Properties
        // STEP 2.1: compose transform
        const glm::mat4 modelA = colliderA.compose_transform(dataA.transform);
        const glm::mat4 modelB = colliderB.compose_transform(dataB.transform);
        // STEP 2.2: center of mass
        // TODO: for a compound collider this will be more involved
        //   for now take origin of collider
        // TODO: Angular velocity will also have to be applied to this CoM?
        const glm::vec3 centerOfMassA = modelA * glm::vec4(0.0f,0.0f,0.0f, 1.0f);
        const glm::vec3 centerOfMassB = modelB * glm::vec4(0.0f,0.0f,0.0f, 1.0f);
        
        // STEP 2.3: Collision Point Rationalizing
        // Because there is no static resolution, set collision point to midway point
        collisionPoint = collisionPoint - (collisionNormal * collisionDepth)/2.0f;

        // STEP 2.4: vector describing the "radius" of the rotation
        const glm::vec3 leverA = (collisionPoint - centerOfMassA);
        const glm::vec3 leverB = (collisionPoint - centerOfMassB);

        // STEP 2.5 relative velocity vector
        // relative is: this' velocity as viewed by other
        const glm::vec3 relVelocity = ((physicsA.velocity + glm::cross(physicsA.angularVelocity, leverA)) - (physicsB.velocity + glm::cross(physicsB.angularVelocity, leverB)));

        // NO early exit on negative relVelocity because springs will penetrate
        
        // STEP 2.6: angular inertia/moment
        // TODO: can this be optimized? inverse of inverse :(
        // TODO: moment doesn't behave correct with scaled transforms
        //   copy transforms, extract scale, build inertia tensor with scale
        //   then transform tensor with scale-less model transform
        // each collider can restrict it as they see fit
        const glm::mat3 invModelA = glm::inverse(glm::mat3(modelA));
        const glm::mat3 invMomentA = invMassA == 0 ? glm::mat3(0.0f) 
            : glm::inverse(
                glm::transpose(invModelA) 
                * (colliderA.get_inertia_tensor(dataA.transform.scale) * physicsA.mass)
                * invModelA
            );

        const glm::mat3 invModelB = glm::inverse(glm::mat3(modelB));
        const glm::mat3 invMomentB = invMassB == 0 ? glm::mat3(0.0f)
            : glm::inverse(
                glm::transpose(invModelB)
                * (colliderB.get_inertia_tensor(dataB.transform.scale) * physicsB.mass)
                * invModelB
            );

        // STEP 2.7: Spring properties
        // spring length = collisionDepth
        // delta spring length is relativeVelocity along collision Normal
        const glm::vec3 deltaCollisionDepth = glm::dot(relVelocity, collisionNormal) * collisionNormal;
        // if dot product is posative -> A is movign away from B
        // therefore spring is changing posatively

        // STEP 3: Spring Force
        // spring body may have a preference for direction
        // for now we can apply along surface normal
        glm::vec3 restVector = glm::vec3(0.0f, 0.0f, colliderA.restLength);     // mirrors default ray
        restVector = glm::mat3(modelA) * restVector;
        const float scaledRestLength = glm::length(restVector);
        const float springForceMagnitude = (collisionDepth - scaledRestLength) * colliderA.stiffness;
        const glm::vec3 springForce = springForceMagnitude * collisionNormal;
        const glm::vec3 dampedSpringForce = springForce - colliderA.damping * deltaCollisionDepth;

        // STEP 4: Friction
        // STEP 4.1: Determine velocity perpendicular to normal (tangent along surface)
        const glm::vec3 tangentCross = glm::cross(relVelocity, collisionNormal);
        glm::vec3 collisionTangent = glm::vec3(0.0f);
        if (glm::length2(tangentCross) != 0.0f)
        {
            // cross normal with tangent cross to get tangent direction
            collisionTangent = glm::cross(collisionNormal, tangentCross);
            //PLEEPLOG_DEBUG("Collision Tangent pre-normalize: " + std::to_string(collisionTangent.x) + ", " + std::to_string(collisionTangent.y) + ", " + std::to_string(collisionTangent.z));
            if (glm::length2(collisionTangent) != 0.0f)
                collisionTangent = glm::normalize(collisionTangent);
        }
        
        // STEP 4.2: Determine friction impulse
        const float tangentImpulse = -1.0f * glm::dot(relVelocity, collisionTangent) /
            (invMassB + invMassA +
            glm::dot(
                glm::cross(invMomentB * glm::cross(leverB, collisionTangent), leverB) + 
                glm::cross(invMomentA * glm::cross(leverA, collisionTangent), leverA), 
                collisionTangent)
            );
            
        // STEP 4.3: Coefficient factors
        // if impulse is less than static max, then apply it (this should negate all colinear velocity)
        // if impulse is greater than static max, multiply it by dynamic coefficient
        const float frictionCone = staticFrictionFactor * springForceMagnitude;
        //PLEEPLOG_DEBUG("Static friction limit: " + std::to_string(frictionCone));

        const float frictionImpulse = std::abs(tangentImpulse) < std::abs(frictionCone) ? tangentImpulse : tangentImpulse * dynamicFrictionFactor;

        // STEP 5: Dynamic resolution
        // STEP 5.1: resolve linear spring force
        physicsA.acceleration += invMassA * dampedSpringForce;
        physicsB.acceleration -= invMassB * dampedSpringForce;

        // STEP 5.2 resolve linear friction impulse response
        physicsA.velocity += invMassA * (frictionImpulse*collisionTangent);
        physicsB.velocity -= invMassB * (frictionImpulse*collisionTangent);

        // STEP 5.3 resolve angular spring force
        if (colliderA.influenceOrientation)
        {
            physicsA.angularAcceleration  += invMomentA * glm::cross(leverA, dampedSpringForce);
        }
        if (colliderB.influenceOrientation)
        {
            physicsB.angularAcceleration -= invMomentB * glm::cross(leverB, dampedSpringForce);
        }

        // STEP 5.4 resolve angular friction impulse response
        if (colliderA.influenceOrientation)
        {
            physicsA.angularVelocity  += invMomentA * glm::cross(leverA, frictionImpulse*collisionTangent);
        }
        if (colliderB.influenceOrientation)
        {
            physicsB.angularVelocity -= invMomentB * glm::cross(leverB, frictionImpulse*collisionTangent);
        }

        // STEP 5.5: apply angular dampening
        // we'll linearly damp angular velocities after impulse to try to break out of any equilibriums
        // temporary multiply by 2 for stability until spring response is reworked
        if (colliderA.influenceOrientation)
        {
            physicsA.angularVelocity  *= 1.0f - (physicsA.collisionAngularDrag * 2.0f);
        }
        if (colliderB.influenceOrientation)
        {
            physicsB.angularVelocity *= 1.0f - (physicsB.collisionAngularDrag * 2.0f);
        }
    }

    void rigid_spring_response(
        ColliderPacket& dataA, PhysicsComponent& physicsA, 
        ColliderPacket& dataB, PhysicsComponent& physicsB, 
        glm::vec3& collisionPoint, glm::vec3& collisionNormal, float& collisionDepth
    )
    {
        // invert collisionNormal & collisionPoint
        glm::vec3 invCollisionNormal = -collisionNormal;
        glm::vec3 invCollisionPoint = collisionPoint - (collisionNormal * collisionDepth);
        spring_rigid_response(dataB, physicsB, dataA, physicsA, invCollisionPoint, invCollisionNormal, collisionDepth);
    }
}