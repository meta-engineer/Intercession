# ECS
## Entity-Component-~~System~~Synchro Architecture

I've chosen to use synchro instead of system. While "system" fits the theme of
abstract objects like an "entity", the term system is much more commonplace and becomes vauge. 
It also causes the [RAS syndrome](https://en.wikipedia.org/wiki/RAS_syndrome) of saying "ECS system".

The term "entity", while abstract by definition, is not as widely used in computer science (or
general speech for that matter), does not have any other overloaded definitions, 
and because of that more accurately communicates its usage.

The system/synchro is not actually doing any data manipulation, but is simply coupling
the data in the ecs architecture to the more generic engines doing the heavy lifting, 
therefore: "synchro".

"Engines" have also been renamed to "dynamos" to fit the theme and avoid that overloaded term aswell.

Architecture for the ecs is closely following 
https://austinmorlan.com/posts/entity_component_system/