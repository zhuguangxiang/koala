
#include "task.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void *hello(void *arg)
{
    printf("[proc-%u]task-%lu: hello, world\n", current_pid(), current_tid());
    task_yield();
    printf("[proc-%u]task-%lu: running\n", current_pid(), current_tid());
    task_yield();
    printf("[proc-%u]task-%lu: good bye\n", current_pid(), current_tid());
    return NULL;
}

int main(int argc, char *argv[])
{
    init_procs(3);
    for (int i = 0; i < 100; i++) task_create(hello, NULL, NULL);

    sleep(1);
    task_yield();

    while (1) {
        printf("[proc-%u]No more tasks\n", current_pid());
        sleep(1);
    }
    return 0;
}