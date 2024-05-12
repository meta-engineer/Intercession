#ifndef ASSIMP_CONVERTERS_H
#define ASSIMP_CONVERTERS_H

#include<assimp/matrix4x4.h>
#include<glm/glm.hpp>

namespace assimp_converters
{
    inline glm::mat4 convert_matrix4(const aiMatrix4x4& from)
    {
        glm::mat4 to;
        //the a,b,c,d in assimp is the row ; the 1,2,3,4 is the column
        to[0][0] = from.a1; to[1][0] = from.a2; to[2][0] = from.a3; to[3][0] = from.a4;
        to[0][1] = from.b1; to[1][1] = from.b2; to[2][1] = from.b3; to[3][1] = from.b4;
        to[0][2] = from.c1; to[1][2] = from.c2; to[2][2] = from.c3; to[3][2] = from.c4;
        to[0][3] = from.d1; to[1][3] = from.d2; to[2][3] = from.d3; to[3][3] = from.d4;
        return to;
    }

    inline glm::vec3 convert_vec3(const aiVector3D& from)
    {
        glm::vec3 to;
        to.x = from.x;
        to.y = from.y;
        to.z = from.z;
        // TODO: check vec3 member ordering to directly return constructor
        return to;
    }

    inline glm::quat convert_quat(const aiQuaternion& from)
    {
        glm::quat to;
        to.x = from.x;
        to.y = from.y;
        to.z = from.z;
        to.w = from.w;
        return to;
    }
}


#endif // ASSIMP_CONVERTERS_H