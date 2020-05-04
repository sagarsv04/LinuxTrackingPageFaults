# LinuxTrackingPageFaults

A simple kernel module to track page faults using process id.


## Author :

Sagar Vishwakarma (svishwa2@binghamton.edu)

State University of New York, Binghamton


## Files :

1)	Makefile                 - Compile the program
2)	pf_probe_A.c             - Kernel module to prints the page fault virtual address
3)	pf_probe_B.c             - Kernel module to plot the page fault virtual address
4)	pf_probe_C.c             - Kernel module to plot the page fault virtual address
5)	user.c                   - User Space C program
6)	page_fault_plot.py       - Python code to plot logs


## Flags :

- PROBE_PRINT     - To print information happening in kernel program
- PROBE_DEBUG     - To print additional information happening in kernel program
- USER_SLEEP      - Wait in user space program between each read
- USER_DEBUG      - To print additional information happening in user program
- CONT_STORE      - To overwrite saved data in buffer


## Run :

- Open a terminal in project directory      : make (to build both kernel and user module)
- Load the print kernel module              : sudo insmod pf_probe_A.ko process_id=<PID> (sudo insmod pf_probe_A.ko process_id=4000)
- Load the plot kernel module               : sudo insmod pf_probe_B.ko process_id=<PID> (sudo insmod pf_probe_B.ko process_id=4000)
- Load the plot kernel module               : sudo insmod pf_probe_C.ko process_id=<PID> (sudo insmod pf_probe_C.ko process_id=4000)
- Unload the kernel module use              : sudo rmmod pf_probe_A
- Run user code                             : sudo ./user


## Note :

- When the kernel module is install it creates a list of all the page faults
- This list is saved globally in kernel space
- A user process access the list in kernel space by accessing proc (ie: opens "/proc/pf_probe_A") and reading from the kernel space
- User process also creates a log in "/out" dir which can be used to generate plots using python file provided.
- Part A module print information using printk()
- Part B module doesn't print information, it prints a plot on terminal when the module is removed
- Part C module doesn't print information, it prints a plot on terminal when the module is removed
- "EXIT_CODE" string is copied to user space if all the page fault info is passed into user space
- This is to stop user space program from continuously keep reading from kernel space
