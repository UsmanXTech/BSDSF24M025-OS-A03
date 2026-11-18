#include "shell.h"

/*
 * handle_builtin - built-in commands (Feature 2)
 *
 * Why built-ins must run in the shell process itself:
 * fork() + execvp() can only run *external* programs. A child process
 * cannot change its parent's working directory, and exiting a child
 * would leave the shell running. So commands like cd and exit have to
 * be executed by the shell process directly, BEFORE fork() is called.
 *
 * Returns 1 if arglist[0] was a built-in (handled here),
 *         0 if it is not a built-in (caller must run execute()).
 */
int handle_builtin(char** arglist) {
    if (arglist == NULL || arglist[0] == NULL) {
        return 0;
    }

    /* exit: terminate the shell gracefully (no fork, no child). */
    if (strcmp(arglist[0], "exit") == 0) {
        printf("Shell exited.\n");
        exit(0);
    }

    /* cd <directory>: change the shell's own working directory. */
    if (strcmp(arglist[0], "cd") == 0) {
        const char* dir = arglist[1];

        /* Extra arguments are a user error, not silently ignored. */
        if (arglist[1] != NULL && arglist[2] != NULL) {
            fprintf(stderr, "cd: too many arguments\n");
            return 1;
        }

        /* Bare "cd" goes to the user's home directory (bash behaviour). */
        if (dir == NULL) {
            dir = getenv("HOME");
            if (dir == NULL) {
                fprintf(stderr, "cd: HOME not set\n");
                return 1;
            }
        }

        /* chdir() is the syscall that changes the process's directory. */
        if (chdir(dir) != 0) {
            /* perror prints "cd: <system error message>". */
            perror("cd");
        }
        return 1;
    }

    /* help: list the built-in commands this shell supports. */
    if (strcmp(arglist[0], "help") == 0) {
        printf("myshell - built-in commands:\n");
        printf("  exit            Terminate the shell\n");
        printf("  cd <directory>  Change the current working directory\n");
        printf("  help            Show this help message\n");
        printf("  history         List previous commands with line numbers\n");
        printf("  jobs            List background jobs with their PIDs\n");
        printf("  set             List all shell variables (NAME=value)\n");
        printf("  VARNAME=value   Assign a variable (no spaces around =)\n");
        printf("  $VARNAME        Expand a variable before executing\n");
        printf("  if ... then ... else ... fi   Multi-line conditional:\n");
        printf("                  runs the then-block if the if-command exits 0,\n");
        printf("                  else the else-block (else is optional).\n");
        printf("Use !n to re-execute history entry number n.\n");
        printf("Any other command is run as an external program.\n");
        return 1;
    }

    /* history: list stored commands with line numbers from 1. */
    if (strcmp(arglist[0], "history") == 0) {
        print_history();
        return 1;
    }

    /* jobs (Feature 6): list live background jobs with PIDs.
       Finished jobs are already removed by reap_zombies(), which runs
       before every prompt. */
    if (strcmp(arglist[0], "jobs") == 0) {
        print_jobs();
        return 1;
    }

    /* set (Feature 8): list all defined shell variables. */
    if (strcmp(arglist[0], "set") == 0) {
        print_variables();
        return 1;
    }

    return 0; /* Not a built-in: caller runs it as an external command. */
}
