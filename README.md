# OSLab
Operation System Lab Homework
# Lab0
* Print "Hello World" in `Bochs`.

# Lab1
* Print fibonacci sequence in 2 ways.
    * `fibonacci.asm`: Enter several numbers one by one and it prints the nth of fibonacci sequence.
    * `fibonacci2.asm`: Enter a number and prints 1st ~ nth item of fibonacci sequence.

# Lab2
* Read `FAT12` format disk and execute commands.
    * First it prints all directories and files.
    * Input a directory, it prints the structure of directory.
    * Input a file path, it prints the content of file.
    * Input `count ` + a directory, it prints the number of directories and files in that directory.
* `main.c`: Load `a.img` and execute commands.
* `my_print.asm`: Print a string, called by `main.c`.

# Lab3
* Receive input from keyboard and show them on display.
    * Press `Esc` to enter search mode. In search mode, after inputting some characters, press `Enter`, it will search the matched string of the previous input. Press `Esc` again to exit search mode. In search mode, screen doesn't clears.
    * Screen clears every 20 seconds.  
    
# Lab4
* In `syscall.asm` and `proc.c` (Added __system calls__):
    * `void sys_disp_str(char* str)`: Print the string `str`.
    * `void sys_disp_color_str(char* str, int color)`: Print the string `str` in color `color`.
    * `void sys_process_sleep(int milli_seconds)`: Make process sleep for `milli_seconds` milliseconds.
    * `void sys_process_wakeup(PROCESS *p)`: Wake up process `p`.
    * `void sys_sem_p(SEMAPHORE *s)`: Doing P on semaphore `s`.
    * `void sys_sem_v(SEMAPHORE *s)`: Doing V on semaphore `s`.
* In `main.c`, in order to solve __Sleep Barber Problem__:
    *  `void customer()`: Customer process.
    *  `void come(int customer)`: The No.`customer` customer is coming.
    *  `void haircut(int customer)`: Do hair cut for No.`customer` customer.
    *  `void leave(int customer)`: No.`customer` customer finishes doing hair cut and leave.
    *  `void full(int customer)`: No.`customer` customer finds the shop is full and leave.
    *  `void TestB()` is a barber process.
    *  `void TestC(), void TestD(), void TestE()` are customer process.



