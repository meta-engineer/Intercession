# Intercession

*noun*: the action of intervening on behalf of another

Intercession is a game about unfettered, unrestricted (within reason), and mind-bending time travel mechanics.

The mission of the project is to fulfill all the fantasies of time travel mechanics, and because of innovations made in the Azzarverse, seamlessly avoid the classic paradoxes that have stopped anyone before from attempting such a system.

---

## Basics of time travel

(numbers subject to change upon testing)

Players can move about the game world (referred to as a cosmos) in 3D space, as well as jumping between 5 discrete time-slices of the past. Each time slice is progressing in realtime in lockstep.

5 servers maintain the asychronous simulation of the cosmos. Servers are linked in a parent child relationship. A parent is further ahead in the cosmos' timeline than its child. Each server is separated by 2 minutes.

Actions/events which occur in 1 server are buffered and passed down to the child server, who then processes them after the delay as if they are real-time events coming from a (special trusted) client.

Every entity will have a time-clone of themselves re-performing all their actions on the previous time-slices

Entities who are both in "their present" and on the same time-slice interact as normal.

Interacting with a time-clone will "propagate" those changes up to parents, splitting a copy of the previous buffered timelines, and modifying them aswell as the active state on each parent server.

Their are several cases for interactions which we will outline the resolution to. The general rule is: if an interaction causes a paradox the Arbiter solves each case by adding a single "third-party" entity/event who takes the place of the future-most entity. All future states of the past-most entity are removed and split into a separate time-line.

extra note: timelines are maintained in the system memory until their divergence point leaves the total jumpable window (5x2 minutes) and the trigger event is no longer re-producable and they are considered unreachable.
Any nested splits on that timeline are attached to the parent of the deleted timeline, and their triggers are made active. (note because they are now 2 degrees removed from reality, they are unlikely to happen, but there are special cases where a nested timeline could still be reached)

## Paradox cases

### Interacting with your past self:
After the interacting event, the state of the timeclone is maintained, the Arbiter adds a relevant solution (EX: pushing yourself, causes you to trip in the past instead). The player takes control of their past self. All actions in the time-line buffer (as well as those after the player has time-jumped) and split into a time-line.

### Interacting with another player time-clone:
The victim player is "pulled" into the past, their future actions split into a inactive timeline. (they should be given a chance to "react" with a quick-time-event like, given they weren't in control of their past self).
The player can restore the time-line where they had been only by jumping FURTHER back and stopping the attacking player.

### Interacting with an object in the past:
Objects changed in the past become themselves time-line affecting objects.

This section needs further thought: How to propagate dynamic objects states into the future. Given they were produced in realtime and it would be improbable to re-simulate their actions quickly to change their future state.

---

## System Architecture
Entry point is main.cpp::main() which parses commandline args, builds and runs the app root AppGateway.

AppGateway manages external system resource apis (windowing, audio, networking) and owns the CosmosManager.

CosmosManager uses the apis to contruct dynamos which are business logic systems a cosmos would need.

Cosmos holds the ECS and calls out the the dynamos to update the cosmos.