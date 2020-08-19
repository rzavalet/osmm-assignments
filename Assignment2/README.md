# Assignment 2: A simple Process Scheduler in DOS

Using the program from Assignment 1, write a program that simulates a Process
Scheduler.

The program must define four tasks that need to be executed concurrently. The
scheduling of such tasks is controlled by a Interrupt Service Routine that
executed upon receiving the IRQ8 interrupt.

The scheduler must use a round robin approach. 

The definition of the Register Set in DOS was provided in the lecture slides. A
suggested definition for the PCB was also provided.

Demo:

![Assignment Demo](https://github.com/rzavalet/osmm-assignments/blob/master/Assignment2/assignment_2.gif?raw=true)
