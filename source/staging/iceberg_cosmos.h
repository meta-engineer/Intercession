#ifndef ICEBERG_COSMOS_H
#define ICEBERG_COSMOS_H

//#include "intercession_pch.h"
#include <memory>

#include "events/event_broker.h"
#include "core/dynamo_cluster.h"

namespace pleep
{
    std::shared_ptr<Cosmos> build_iceberg_cosmos(
        std::shared_ptr<EventBroker> eventBroker, 
        DynamoCluster& dynamoCluster
    );
}

#endif // ICEBERG_COSMOS_H