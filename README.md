## Synopsis

These codes are unfinished. They should display signals between processes on UNIX-like operating systems. In some way the specification of the program is done and there are probably no new elements to be introduced.
These codes utilize POSIX functions and to a much lesser extent SysV functions.


## Code Example

This is from test\_signal.c

```
void handler(int signo);

int main
(
int argc, char **argv
)
{
printf("%d\n", getppid());
printf("%d\n", getpid());
if(signal(SIGUSR1, handler) == SIG_ERR)
{
exit(EXIT_FAILURE);
}
else
pause();


}
void handler(int signo)
{
printf("see if this reaches\n");
}
```
The essential codes are pause() and signal(SIGUSR1, func\_name). The former stops execution and the latter assigns the handler-function func\_name to the signal number SIGUSR1.

## Motivation

Because I solve all my problems in concurrency with Semaphores, and all my messaging with data structures like shared memories, msg-queues, pipes and sockets, I add something as small as signals to my repertoire.

## Installation

Some codes are not supposed to compile.
usage: command <no>|reader <no>|reader <no>|reader <no>|reader ...

## Contributors

Copied from https://www.softprayog.in/programming/interprocess-communication-using-posix-shared-memory-in-linux
Copyright © 2007-2017 SoftPrayog.in. All Rights Reserved.

Suggestion to use malloc\_stats() from www.linuxjournal.com/article/6390


And me, Luu Minh Khoa Ngo

## License

This is purely for educational purpose. Feel free to copy, if you need to.

## Final words

At some point in my life, I will go back and finish the thoughts in this one.

If you find certain points, where I could improve my code, send me a pull request or message. Thank you very much. 
