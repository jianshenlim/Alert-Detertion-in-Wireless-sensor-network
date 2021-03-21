Simulation of an alert system consisting of ground temperature sensors in a Wireless Sensor Network and a simulated satellite network. 

Each sensor compares temperature readings between adjacent sensors before sending an alert signal to a base node. Base node then compares readings with satellite temperature results and outputs a log based on predefined requirements. All calls happen in parallel.

Implemented using the Message Passing Interface (MPI) for parallel computing architectures