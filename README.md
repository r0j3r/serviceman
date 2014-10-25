serviceman
==========

    - boots processes in parallel. dependencies are managed by third party
        ipc mechanisms
    - uses definition files for starting processes
    - it manages system and user processes
    - it can be controlled by the administrator and users to stop and
        restart processes
    - it has the ability to run processes on loading, on demand and on
        a set time
    - monitor and restart long running processes in a flexible way.
        restarting crashed daemons are throttled and stopped if needed
    - it is small and modular
    - can be made robust against buggy system library updates
    - computes process start time from cron style time specs precisely.
        this allows the cpu to idle longer

TODO

     - public admin interface
    - fix memory leaks
    - RB Tree replacement for timed start and throttle queue
    - add options for controlling process resource limits
    - add options for setting user and group ids of processes
    - multiple date entries in cron spec
    - proper error notification in the servctl parser

NICE TO HAVE

    - hash table replacement of child_process circular lists
    - a way to handle leap seconds in the internal perpetual calendar [facepalm]
    - inetd-like capabilities 
