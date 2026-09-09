# POSIX Concurrency Simulators & Process Scheduler 

This repository contains C implementations of a multi-level process scheduler and a concurrent Inter-Process Communication (IPC) simulator. 

> ** Academic Context:** This project was developed for the Operating Systems laboratory at the Public University of Navarre (UPNA). The architectural design, process topology, and core system requirements were provided by the university's engineering department. My work consisted of writing the C code to execute these specifications, managing POSIX system calls, and ensuring memory-safe execution without busy-waiting.

##  Project Structure

### 1. Hydraulic System Simulator (IPC)
A concurrent system simulating fluid flow, managed by 6 interdependent processes communicating without global variables or active waiting.
* **Manager:** Initializes and safely destroys the IPC infrastructure (Message Queues, Shared Memory, Semaphores, Pipes) and spawns the child processes.
* **FillDeposit & Pump:** Manage fluid resources using semaphores to ensure mutual exclusion when updating shared memory.
* **Flowmeter:** Monitors fluid flow and enqueues Type-2 alert messages into the Message Queue if thresholds are exceeded.
* **Sink:** Decrements resources and triggers signals (`SIGUSR1`) to the monitor process.
* **Monitor:** Uses a multi-process approach to concurrently read alert messages based on priority and handle incoming `SIGUSR1` signals to print system status.

### 2. Multi-level Process Scheduler
A user-level scheduler managing processes through a non-preemptive 3-level queue system:
* **Level 1:** Round Robin (4-second quantum).
* **Level 2:** Priority Scheduling (lowest value = highest priority).
* **Level 3:** First-Come, First-Served (FCFS).

## Tech Stack
* **Language:** C
* **OS:** Linux (POSIX compliant)
* **IPC Mechanisms:** Pipes, Signals, Semaphores, Shared Memory, Message Queues.
