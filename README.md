# Intercession

*noun*: the action of intervening on behalf of another

Intercession is a game about unfettered, unrestricted (within reason), and mind-bending time travel mechanics.

The mission of the project is to fulfill all the fantasies of time travel mechanics, and because of innovations made in the Azzarverse, seamlessly avoid the classic paradoxes that have stopped anyone before from attempting such a system.

Inspired by the likes of Portal Intercession aims to produce and polish 1 single mechanic: time travel.
Like Portal's "portals" this single mechanic will give-way to emergent behaviours and gameplay when coupled with other more trivial components like basic player movement and rigid-body physics.

Additionally, non-massive peer-to-peer multiplayer would again increase the possibilities of emergent gameplay, as well as make a more marketable product in the modern online marketplace.

---

## Basics of time travel

(numbers subject to change upon testing)

Players can move about the game world (referred to as a cosmos) in 3D space, as well as jumping between 5 discrete time-slices of the past. Each time slice is progressing in realtime in a staggered lockstep.

5 servers maintain the asychronous simulation of the cosmos. Servers are linked in a parent child relationship. A parent is further ahead in the cosmos' timeline than its child. Each server is separated by ~2 minutes.

Actions/events which occur in 1 server are buffered and passed down to the child server, who then processes them after the delay as if they are real-time events coming from a (specially trusted) client.

Every entity will have a time-clone of themselves repeating all their actions on the previous time-slices

Two entities who are both in "their subjective present time-slice" and on the same time-slice interact as normal (physics collision).

Interacting with a time-clone will "propagate" those changes up to parents, splitting a copy of the previous buffered timelines, and modifying the active state on each parent server.

Their are several cases for interactions which we will outline the resolution to. The general rule is: if an interaction causes a paradox the Arbiter solves each case by adding a single "third-party" entity/event who takes the place of the future-most entity. All future states of the past-most entity are removed and split into a separate split timeline.

extra note: timelines are maintained in the system memory until their divergence point leaves the total jumpable window (5x2 minutes) and the trigger event is thus no longer re-producable and they are considered unreachable timelines.
Any nested splits on that timeline are then attached to the parent of the deleted timeline, and their triggers are made active. (note because they are now 2 degrees removed from reality, they are unlikely to happen, but there are special cases where a previously nested timeline could still be reached)

## Paradox cases

### Interacting with your past self:
After the interacting event, the state of the timeclone (past-self) is maintained, the Arbiter adds a relevant solution (EX: pushing yourself, causes you to trip in the past instead). The player takes control of their past self. All actions in the time-line buffer (as well as those after the player has time-jumped) and split into a timeline.

### Interacting with another player time-clone:
The victim player is "pulled" into the past, their future actions split into a inactive timeline. (they should be given a chance to "react" with a quick-time-event, given they weren't in control of their past self to see the interaction/attack coming).
The player can restore the timeline where they had been only by jumping FURTHER back and stopping the attacking player's intercession.

### Interacting with an object in the past:
Objects changed in the past become themselves time-line affecting objects and can cause propagations. Because npc/inanimate objects can have predictable actions. When a "timeline affecting" entity interacts with them (a player) they immediately enter a "superposition" i all future time-slices. This allows the server to process and propagate their behaviour until they can determine what state they shoudl be in the future. NOTE: "Superposition" objects may just be a visual effect to communicate that "THIS OBJECT IS SUBJECT TO DISSAPEAR AT ANY MOMENT", or may make it uninteractable. Whichever creates less bugs/more fun will be chosen

This section needs further thought: How to propagate dynamic objects states into the future (like npcs which can move). Given they were produced in realtime and it may be improbable to re-simulate their actions quickly to change their future state. Given ~10 minutes total time window, it will have to be tested if all 10 minutes can be resimulated in a quick enough period such that their "superposition state" doesn't create too much interruption.

---

## System Architecture
Entry point is main.cpp::main() which parses commandline args, builds and runs the app root "AppGateway".

"AppGateway" manages external system resource apis (windowing, audio, networking) and owns the "CosmosContext".

"CosmosContext" uses the apis to contruct dynamos which are business logic systems a cosmos would need.

"Cosmos" holds the ECS and calls out to the dynamos with packets of ECS data to update them.

---

## Bibliography
Special thanks to the following resources, programmers, and teachers for their invaluable and selfless work which I was able to learn from and adapt to produce this project. (This does not include the libraries, which I directly used).

It is amazing what you can learn for free on the internet.

* https://learnopengl.com/ - Fundamentals of Opengl and graphics concepts
* http://www.opengl-tutorial.org/ - Geometry and graphics concepts/resources
* https://www.thecherno.com/ - Extensive resources and topics related to game engines, based on the Hazel project
* https://community.onelonecoder.com/ - Aka Javidx9. Extensive c++ topics and inspiration to build things yourself
* https://www.youtube.com/c/CodeTechandTutorials - c++ and project management (cmake) walkthroughs
* https://austinmorlan.com/ - Walkthrough and intuition of ECS architecture