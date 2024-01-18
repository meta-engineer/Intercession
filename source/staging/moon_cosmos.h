#ifndef MOON_COSMOS_H
#define MOON_COSMOS_H

//#include "intercession_pch.h"
#include <memory>

#include "events/event_broker.h"
#include "core/dynamo_cluster.h"

namespace pleep
{
    std::shared_ptr<Cosmos> build_moon_cosmos(
        std::shared_ptr<EventBroker> eventBroker, 
        DynamoCluster& dynamoCluster
    );
}

#endif // MOON_COSMOS_H