# PleepNet Framework

Based on the great teachings of Javidx9:
https://www.youtube.com/watch?v=2hNdkYInj4g

## File organization
PleepNet is a framework built ontop of Asio to provide the networking needs for Intercession
clients and timeslice servers (aswell as coordinator/dispatcher servers eventually)

Right now all files/resources are contained in the networking/ subdirectory, but better
partitioning may make clearer functionality

There are some self-contained utility headers like net_message.h and ts_deque.h
Aswell as parts of the simulation engine proper, like net_dynamo.h

It may make sense to separate the utils into a "PleepNet" library.

Especially because those "common" definitions will have to be inherited by each of the applications

Because servers & clients have different behaviour there is no network_dynamo.h, but instead source/server/server_network_dynamo.h, and source/client/client_network_dynamo.h, which are used by their respective contexts.

## Architecture
Intercession will require a client (simulation with rendering), a server (simulation without rendering), and a dispatch server (no simulation). So we need a way to setup this project to produce multiple executables which all borrow from a common codebase.

Looks like the best way to do this is to have a common source files list (source/source_files.cmake)
which will get prepended to a client source files list and a server source files list.

Then, since there can only be 1 CMakeLists tree, all the CMakeLists will have to be updated to
build a IntercessionEgnine library which is linked to 
IntercessionClient and an IntercessionServer (and an IntercessionDispatcher),
with appropriate options/linkages/includes/installs

There may be a some unused (not common) parts in IntercessionEngine between the two apps,
but hopefully not too much to cause any problems

Multiple timeslice servers could each be individual processes, and a script could invoke each one as pass commandline params, or a config file to each of them. This means each timeslice will only know about the others through the network (maybe I can investigate ipc?)
Maybe there is some safety to having each server on a seperate process. 

Alternatively, multiple timeslice servers could each be spun from a central process who configures them. This might make interrogating the health of each more convenient. The central process could act as diagnostics after setup and watch the thread health of each.