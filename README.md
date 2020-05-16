# Winsock2 Chatbox

A small personal project which is created out of interest in network programming.

This project focusses on the implementation and usage of sockets, especially Microsoft their implementation `Winsock2.h`.

I divided the project into two layers: 

1. The Networking layer which handles all the packets, responses and state of network activity.
2. The Chatbox layer itself, which is a small program which uses the first layer mainly to setup a connection and chat.

## Goal

The goal of this project is to create a P2P chatbox system which can communicate directly with another client without the use of a server using external IP addresses. This made things more complicated that it needed to be but it gave me great insight on the behaviour of sockets.



## To be implemented

Currently the project is not complete yet and once in awhile I will update this project. The features to still added are:

* Connect with an external network IP address.
* Set up a chat port
* Chat commands







