````
 ___________________________________________________
|                                                   |
|          ________ ___                             |
|         /  __   //  /                             |
|        /  /_/  //  /_____   _____   _______       |
|       /  _____//  //  __ \ /  __ \ /  __   /      |
|      /  /     /  //  ____//  ____//  /_/  /       |
|     /__/     /__/ \____/  \____/ /  _____/        |
|                                 /  /      soft    |
|                                /__/               |
|___________________________________________________|
````
# Intercession

**noun**: the action of intervening on behalf of another

Intercession is a simulation of unfettered, unrestricted (within capabilities), and mind-bending time travel mechanics.

The mission of the project is to fulfill all the fantasies of time travel fiction, and because of innovations made in the Azzarverse, seamlessly avoid the classic paradoxes that have stopped anyone before from attempting such a system.

Inspired by the likes of Portal, Intercession aims to produce and polish 1 single mechanic: time travel.

Like Portal's "portals" this single mechanic will give-way to emergent behaviours and gameplay when coupled with other trivial systems like basic player movement and rigid-body physics.

Additionally, non-massive peer-to-peer multiplayer would again increase the possibilities of emergent gameplay, as well as make a more marketable product.

---

## Basics of time travel

### Definitions
- Entity: A thing which exists
- Worldline: Progression of causality an entity follows through time
- Temporal-Entity: The collection of all states of an entity throughout a single worldline
- Timeslice: a server, maintaining the simulation of a particular moment in time
- Timeline: A series of timeslices, which cover a window of time
- Interaction: any circumstance where one entity changes another
- Interception: an Interaction where one entity is a time traveller, causing the future to change.
- Divergence: A shift in an entity's worldline caused by an interception
- Paradox: A series of worldlines which create a self-perpetuating loop
- Intercession: A shift in entities' worldlines made by the Arbiter to avoid a potential paradox
- Arbiter: The decision routine which determines the solution to an Intercession
- Reconstruction: The overwriting of the physicalized timeline to account for interceptions/intercessions.
- Propogation: The natural progression of history moving into the past
- Timejump: An "unnatural" movement of an Entity directly switching into another point in time

(numbers subject to change upon testing)

Players can move about the game world (referred to as a cosmos) in 3D space, as well as jumping between multiple discrete timeslices of the past. Each timeslice is progressing in realtime in a staggered lockstep.

Each server maintains an asychronous simulation of the cosmos. Servers are linked in a parent child relationship. A parent is further in the future than its child. Each parent & child are separated by some small unit of time, maybe 10-30 seconds.

Actions/events which occur in 1 server are buffered and passed down to the child server, who then processes them after the delay as if they are real-time events coming from a (specially trusted) client.

Every entity will have a past propagation of themselves repeating all their actions on the previous timeslices.

Two entities who are both in their "subjective present" timeslice will interact as normal (physics/collision).

Interactions between entities **not** in their "subjective present" (one has timejumped) will change the future, and "reconstruct" those changes upwards to parents, splitting the previous buffered timelines, and modifying the active state on each parent server.

Their are several cases for novel interactions which we will outline the resolution to. The general rule is: if an interaction causes a paradox then the "Arbiter" solves each case by adding a single "third-party" entity/event who takes the place of the future-most entity, ensuring its new state is maintained. All future states of the past-most entity are removed and split into a separate worldline.

## Paradox cases

### Intercepting with your past self:
After the interacting event, the diverged state of the past player entity is maintained. If this state leads to a future which contradicts the inciting divergence the Arbiter adds an accomidating solution (EX: pushing yourself causes you to instead trip on a rock and fall in the same way). The entity's worldline is reconstructed with this change (assuming generously that the player entity will _try_ to take the same actions despite the change, perhaps even timejumping as well). This causes the future-most state of the player entity to change, which the player takes control of.

### Interacting with another player's past:
After the interacting event, the diverged state of the past player entity is maintained. These effects cause the future to reconstruct.
(Should they be given a chance to "react" with a quick-time-event, given they weren't in control of their past self to see the interaction/attack coming?).
The player can restore the timeline where they had been only by jumping FURTHER back and stopping the attacking player's intercession.

### Interacting with an object in the past:
Objects changed in the past become themselves time-line affecting objects and can cause reconstructions. Because npc/inanimate objects can have predictable actions. When a "timeline affecting" entity interacts with them (a player) they immediately enter a "superposition" in all future timeslices. This allows the server to process and propagate their behaviour until they can determine what state they should be in the future. NOTE: "Superposition" objects may just be a visual effect to communicate that "THIS OBJECT IS SUBJECT TO DISSAPEAR AT ANY MOMENT", or may make it uninteractable. Whichever creates less bugs/more fun will be chosen

This section needs further thought: How to propagate dynamic objects states into the future (like npcs which can move). Given they were produced in realtime and it may be improbable to re-simulate their actions quickly to change their future state. Given ~10 minutes total time window, it will have to be tested if all 10 minutes can be resimulated in a quick enough period such that their "superposition state" doesn't create too much interruption.

---

## System Architecture
Entry point is source/client_main.cpp::main() and source/server_main.cpp::main() which parses commandline args, builds and runs the app root "AppGateway".

"AppGateway" manages external system resource apis (windowing, audio, networking) and owns the "CosmosContext".

"CosmosContext" uses the apis to contruct dynamos which are business logic systems a cosmos would need.

"Cosmos" holds the ECS and calls out to the dynamos with packets of ECS data to update them.

TBD...

---

## Backup
Necessary data to backup is:
- cmake_scripts/*
- config/*
- packaging/*
- source/*
- .gitignore
- .gitmodules
- CmakeLists.txt
- Notes.md
- README.md
- external/CmakeLists.txt
- external/README.md
- external/source_files.cmake

## Bibliography
Special thanks to the following resources, programmers, and teachers for their invaluable and selfless work which I was able to learn from and adapt to produce this project. (This does not include the libraries which I used directly).

It is amazing what you can learn for free on the internet.

* https://learnopengl.com/ - Fundamentals of Opengl and graphics concepts
* http://www.opengl-tutorial.org/ - Geometry and graphics concepts/resources
* https://www.thecherno.com/ - Extensive resources and topics related to game engines, based on the Hazel project
* https://community.onelonecoder.com/ - Aka Javidx9. Extensive c++ topics and inspiration to build things yourself
* https://www.youtube.com/c/CodeTechandTutorials - c++ and project management (cmake) walkthroughs
* https://austinmorlan.com/ - Walkthrough and intuition of ECS architecture
* http://hitokageproduction.com/article/11 - Walkthrough and intuitions of 3D collision and physics
* https://github.com/Votuko/steins-gate-mechanics - Explanation and discussion of time travel and world line mechanics