#ifndef TAG_COMPONENT_H
#define TAG_COMPONENT_H

//#include "intercession_pch.h"
#include <string>

namespace pleep
{
    // maintain a unique id for each entity
    // id should not (unexpectantly) change over the entity lifetime
    struct TagComponent
    {
        std::string tag;

        bool operator==(const TagComponent& other)
        {
            return tag == other.tag;
        }
    };
}

#endif // TAG_COMPONENT_H