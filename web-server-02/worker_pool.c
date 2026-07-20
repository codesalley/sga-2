#define _POSIX_C_SOURCE 200809L

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define NWORKERS  6
#define TIMEOUT   5   
#define GRACE     2    

static struct {
    pid_t  pid;
    time_t started;
    int    alive;
    int    termed;     
} w[NWORKERS];


static volatile sig_atomic_t child_died = 0;

static void on_sigchld(int sig)
{
    (void)sig;
    child_died = 1;   
}


static void run_worker(int id)
{
    if (id == 2) {
        signal(SIGTERM, SIG_IGN);
        printf("  [worker %d pid=%d] wedged, ignoring SIGTERM\n", id, getpid());
        fflush(stdout);
        for (;;)
            pause();                 
    }

    int secs = (id == 5) ? TIMEOUT + 4 : 1 + id % 3;
    printf("  [worker %d pid=%d] working for %ds\n", id, getpid(), secs);
    fflush(stdout);
    sleep((unsigned)secs);
    _exit(0);
}

static void reap(void)
{
    int status;
    pid_t pid;

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        if (WIFEXITED(status))
            printf("[sup] pid %d exited status %d\n", pid, WEXITSTATUS(status));
        else if (WIFSIGNALED(status))
            printf("[sup] pid %d killed by signal %d\n", pid, WTERMSIG(status));

        for (int i = 0; i < NWORKERS; i++)
            if (w[i].pid == pid)
                w[i].alive = 0;
    }
}

static void enforce_timeouts(void)
{
    time_t now = time(NULL);

    for (int i = 0; i < NWORKERS; i++) {
        if (!w[i].alive)
            continue;

        int age = (int)(now - w[i].started);

        if (age >= TIMEOUT + GRACE && w[i].termed) {
            printf("[sup] worker %d (pid %d) ignored SIGTERM -> SIGKILL\n", i, w[i].pid);
            kill(w[i].pid, SIGKILL);   
        } else if (age >= TIMEOUT && !w[i].termed) {
            printf("[sup] worker %d (pid %d) exceeded %ds -> SIGTERM\n", i, w[i].pid, TIMEOUT);
            kill(w[i].pid, SIGTERM);
            w[i].termed = 1;
        }
    }
}

static int alive_count(void)
{
    int n = 0;
    for (int i = 0; i < NWORKERS; i++)
        n += w[i].alive;
    return n;
}

int main(void)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof sa);
    sa.sa_handler = on_sigchld;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        printf("[ERROR]: error caught");
        return EXIT_FAILURE;
    }

    printf("[sup] pid %d spawning %d workers, timeout %ds\n",
           getpid(), NWORKERS, TIMEOUT);

    for (int i = 0; i < NWORKERS; i++) {
        fflush(stdout);

        pid_t pid = fork();
        if (pid < 0) {                  
            perror("fork");
            break;
        }
        if (pid == 0) {
            signal(SIGCHLD, SIG_DFL);
            signal(SIGTERM, SIG_DFL);
            run_worker(i);
        }

        w[i].pid = pid;
        w[i].started = time(NULL);
        w[i].alive = 1;
        printf("[sup] forked worker %d -> pid %d\n", i, pid);
    }

    while (alive_count() > 0) {
        if (child_died) {
            child_died = 0;           
                                         
            reap();
        }
        enforce_timeouts();
        reap();                         
                                      
        sleep(1);
    }

    printf("[sup] all workers reaped, no zombies remain. exiting.\n");
    return EXIT_SUCCESS;
}
