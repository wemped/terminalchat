# terminalchat

A server and two client types that act as a chatroom.

The idea is you open three terminal windows, one for the server, one for a participant, and another for an observer.
64 participants and 64 observers can connect to one server. Max message size is 1024 characters.

to run, compile the three files prog2_server.c, prog2_observer.c, prog2_participant.c

run the server with two port numbers: ./server 10000 10001

run the participant in another terminal with the server's ip address, and its participant port #: ./participant 127.0.0.1 10000

run the observer in another terminal with the server's ip address, and its observer port #: ./observer 127.0.0.1 10001
