#ifndef COSMOS_BUILDER_H
#define COSMOS_BUILDER_H

//#include "intercession_pch.h"
#include <memory>

#include "cosmos.h"

namespace pleep
{
    // Utility for constructing Cosmoses in a standardized way
    // Clients/Servers will use shared CosmosBuilder configs to synchronize themselves
    // Cosmoses should remain independant of any particular synchros/components
    // But the builder will have to explicitly have all usable types included and enumed
    class CosmosBuilder
    {
    public:
        // Builder will need access to the Contexts dynamos to provide to built synchros
        CosmosBuilder();
        ~CosmosBuilder();

        // Describe all the initial conditions (registrations) for a Cosmos
        struct Config
        {
            // Registered Components
            // Registered Synchros
        };

        // Called by CosmosContext to setup their held Cosmos
        // use config parameters to setup
        // return shared pointer? Context may need to regenerate config
        std::shared_ptr<Cosmos> big_bang(CosmosBuilder::Config config);

        // Can we retroactively generate a config from an existing cosmos?
        // or should context just save its config?
        // If a cosmos was changed after we built it, we should be able to capture it
        CosmosBuilder::Config scan(std::shared_ptr<Cosmos> cosmos);

    private:
        
    };
}

#endif // COSMOS_BUILDER_H