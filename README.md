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
    - can be made robust against buggy system libraries
    - computes process start time from cron style time specs precisely.
        this allows the cpu to idle longer
