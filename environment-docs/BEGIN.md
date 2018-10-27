# A HACKER'S GUIDE

This project started as my whim to train an RL model capabale of handling
complex tasks such as a strategy games. Life is difficult and life is
hard. - YJB [Init]

The first dev guide can be found 
[here](https://github.com/yashbonde/freeciv-web-environment/blob/develop/environment-docs/freeciv-doc/HACKING).

## Terminology

The original HACKING doc discusses how the Freeciv is confused about the
term `player`, to avoid this throughout this work we will be using the 
`agent` whenever we discuss about the entity who is taking actions for
any `player`.

## Basic

Freeciv is a client-server style game where processing for each is done at 
the server's end. This way the integrity of software was maintained while
preventing unwarranted actions from client's side. The loop for the server
can basically be shown as this:
```
while(server_state == RUN_GAME_STATUS) { /* looped per turn */
    do_ai_stuff();     /* do the ai controlled players */
    sniff_packets();   /* get player requests and process them */
    end_turn();        /* main turn update */
    next_year();
    }
```
Our main tap into this code is the `sniff_packets()` function which
waits for packets or stdin(server-op commands). Since the environment
will be a class that is able to send actions of the agent to the game 
and returns the agent with reward and state.

:warning: We are still unsure about the reward structure (whether to 
provide one at all).

Original HACKING doc also talks about the map structure, but more on that
later.

## Network

The netbase code for client is in `freeciv/freeciv/client/clinet.c` and
that for server is as `freeciv/freeciv/server/sernet.c`.
