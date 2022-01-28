# ECS
## Entity-Component-~~System~~Synchro Architecture

I've chosen to use the term synchro instead of system. While "system" fits the theme of
abstract objects like an "entity" the term system is much more commonplace and becomes vauge. 
It also causes the [RAS syndrome](https://en.wikipedia.org/wiki/RAS_syndrome) of saying "an ECS system".

The term "entity", while abstract by definition, is not as widely used in computer science (or
general speech for that matter), and does not have any other overloaded definitions, 
therefore it more accurately communicates its usage.

The system/synchro is not actually doing any data manipulation (for the major tasks at least). 
It is simply coupling the data in the ecs architecture to the more abstract engines
which do the heavy lifting, transforming/synchronizing the ecs data, therefore: "synchro".

"Engines" have also been renamed to "dynamos" to fit the theme and avoid an overloaded term as well.

Architecture for the ecs is closely following 
https://austinmorlan.com/posts/entity_component_system/

The core::Cosmos acts at the root user of the ECS registries (called the Coordinator in the resource).
It is the instantiable user-facing object to store/access entities.