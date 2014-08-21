serviceman
==========

new init implementation copied from apple's launchd design

copies apple design of starting all daemons at once and let the daemons coordinate over ipc frameworks

layered service architecture where each layer dont need another service in the same layer. therefore all services in a layer can be started at the same time

pid 1 is a simple and small as possible. all it does is reap orphaned processes and start the service manager. copied from EWONTFIX

service manager does all the hard work. this is equivalent to apple launchd before it bacame pid 1 in mac os x
