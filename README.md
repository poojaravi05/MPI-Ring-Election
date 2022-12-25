# 474-Project2
Project 2: Ring Election with Chang-Roberts Algorithm

Description: The program runs the Chang-Robert's algorithm for leader election in unidirectional ring topology.
The program is designed in a way that the user can give 2 inputs (No. of nodes and the initiator) in the execution command itself.

Programming language used: C

Execution steps:
  1. Start the Visual Studio Code
  2. Connect to the SSH host (ECS server)
  3. Click on terminal on the top and select New Terminal
  4. For Compiling - mpicc filename.c -o filename
  5. For Execution - mpirun -n 7 prac 3 
     (here 7 - no. of nodes, 3 - initiator)
