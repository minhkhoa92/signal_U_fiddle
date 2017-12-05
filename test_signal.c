
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>

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


/*pause();
//exit(0);//*/
}

void handler(int signo)
{
printf("see if this reaches\n");
}
