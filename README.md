serviceman
==========

new init implementation copied from apple's ;aunchd design
copies apple design of starting all daemons at once let the daemons coosrdinate over ipc frameworks
layered service architecture where each layer dont need another service in the same layer. therefore all services in a layer can be started at the same time
pid1 is a simple and small as possible. all does is reap orphaned processes and start the service manager
service manager does all the hard work. this is equivalent to apple launchd
