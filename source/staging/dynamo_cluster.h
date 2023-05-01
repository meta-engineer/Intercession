#ifndef DYNAMO_CLUSTER_H
#define DYNAMO_CLUSTER_H

//#include "intercession_pch.h"
#include <memory>

#include "rendering/render_dynamo.h"
#include "inputting/input_dynamo.h"
#include "physics/physics_dynamo.h"
#include "networking/i_network_dynamo.h"
#include "scripting/script_dynamo.h"

namespace pleep
{
    // Collection of all possible dynamos a Context could use (client or server or other)
    // Unused ones are left null
    // All possible Dynamo configurations should be super/subsets of each other
    // We need to differentiate each dynamo type, so it is easier to list
    // them individually instead of using some polymorphism
    // However, each Dynamo could be a polymorphic interface (I_NetworkDynamo)
    struct DynamoCluster
    {
        std::shared_ptr<RenderDynamo>    renderer  = nullptr;
        std::shared_ptr<InputDynamo>     inputter  = nullptr;
        std::shared_ptr<PhysicsDynamo>   physicser = nullptr;
        std::shared_ptr<I_NetworkDynamo> networker = nullptr;
        std::shared_ptr<ScriptDynamo>    scripter  = nullptr;
        //std::shared_ptr<AudioDynamo>     audioer   = nullptr;
    };
}

#endif // DYNAMO_CLUSTER_H