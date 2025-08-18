// mtsh.c — Empty Shell (pronounced “em-tee-shell”)
// Build: cc -std=c11 -Wall -Wextra -O2 -o mtsh mtsh.c
// Run:   ./mtsh

// --- POSIX feature set ---
// Request POSIX.1-2008 (2008 + September) so we get modern POSIX functions
// like getline(), and consistent prototypes across systems.
// This must be defined *before* including any system headers.
#define _POSIX_C_SOURCE 200809L

// --- Standard / system headers ---
#include <stdio.h>      // printf(), fprintf(), perror(), putchar()
#include <stdlib.h>     // exit(), free(), getenv()
#include <string.h>     // strlen(), strcmp(), strtok(), strsignal()
#include <unistd.h>     // fork(), execvp(), chdir()
#include <sys/wait.h>   // waitpid(), WIFEXITED(), WEXITSTATUS(), WIFSIGNALED(), WTERMSIG()
#include <errno.h>      // errno variable, error codes for perror()
#include <fcntl.h>      // open(), O_CREAT, O_WRONLY, O_TRUNC, dup2()


// --- Shell configuration constants ---
// Upper limit on number of arguments we will parse into argv[].
// Chosen for simplicity; real shells grow argv dynamically.
// One slot is always kept free for the NULL terminator.
#define MAX_ARGS 64   // Hard limit to keep things simple

// Helpers for debugging 
/*
static void dump_bytes(const char *label, const char *s) {
    fprintf(stderr, "%s: '", label);
    for (const unsigned char *p=(const unsigned char*)s; *p; ++p)
        //fprintf(stderr, "\\x%02X", *p);
        fprintf(stderr, " %02X", *p);
    fprintf(stderr, "'  (text: %s)\n", s);
}

static void debug_tokens(char *argv[]) {
    fprintf(stderr, "[DBG] tokens:\n");
    for (int i = 0; argv[i]; ++i) dump_bytes("  arg", argv[i]);
}
*/

// ------------------ Trim trailing '\n' from getline() input ------------------
static void chomp(char *s) {
    size_t n = strlen(s);
    if (n && s[n-1] == '\n') s[n-1] = '\0'; // If the last character is '\n', replace it with '\0'.
    // This modifies the input string in place, which is fine here since we own it.
    // getline() always allocates memory for the line, so we can safely modify it.
    // This is a common pattern in C to remove trailing newlines from input.
    // Note: This function assumes the input string is null-terminated, which getline() guarantees. 
}

// ------------------ Split a line into argv[] by whitespace only (no quotes/escapes yet) ------------------
static int split(char *line, char *argv[], int maxargs) {
    int argc = 0; // Argument count, starts at 0 
    char *tok = strtok(line, " \t"); // Split by spaces and tabs. 
    // Note that strtok modifies the input string, which is fine here.
    while (tok && argc < maxargs - 1) { // -1 to leave space for NULL terminator 
        argv[argc++] = tok;
        tok = strtok(NULL, " \t");
    }
    argv[argc] = NULL;
    return argc;
}

// ------------------ Cleanup helper ------------------
static void cleanup(char **linep) {
    free(*linep);
    *linep = NULL;
}

// ------------------Built-in commands: exit, cd ------------------
static int try_builtin(char *argv[], char **linep) {
    if (!argv[0]) return 1; // Nothing to do, returns 1 meaning command is handled. 
    if (strcmp(argv[0], "exit") == 0) {
        cleanup(linep); // Free the line buffer before exiting 
        exit(0);
    }
    if (strcmp(argv[0], "cd") == 0) {
        const char *dest = argv[1] ? argv[1] : getenv("HOME");
        if (!dest) { fprintf(stderr, "cd: HOME not set\n"); return 1; }
        if (chdir(dest) != 0) perror("cd");
        return 1; // handled
    }
    return 0; // Not a builtin
}

int main(void) {
    char *line = NULL; 
    size_t cap = 0;
    // You can delete or comment out this part if you don't want the banner.
    printf("                      _             _          _ _ \n");
    printf("  ___ _ __ ___  _ __ | |_ _   _ ___| |__   ___| | |\n");
    printf(" / _ \\ '_ ` _ \\| '_ \\| __| | | / __| '_ \\ / _ \\ | |\n");
    printf("|  __/ | | | | | |_) | |_| |_| \\__ \\ | | |  __/ | |\n");
    printf(" \\___|_| |_| |_| .__/ \\__|\\__, |___/_| |_|\\___|_|_|\n");
    printf("               |_|        |___/                    \n");
    printf(" emptyshell — Minimal Teaching Shell v0.2.0\n");
    printf(" POSIX.1-2008 • Simple • Hackable\n");
    printf("-----------------------------------------------------\n\n");

    // Infinite loop to read commands 
    for (;;) {
        // Prompt        
        printf("> "); // Old-school prompt, just one character
        fflush(stdout); // Ensure prompt is printed before reading input

        // ------------------ Read input ------------------
        ssize_t n = getline(&line, &cap, stdin); // Read a line from stdin.
        // ssize_t is a signed type for sizes, defined in <sys/types.h> which is included via <unistd.h>,
        // getline() expects this type for its return value. 
        // cap = capacity in bytes of the line buffer, getline() will reuse it on subsequent calls,
        // and resize it if it's too small. 
        if (n < 0) { putchar('\n'); break; } // EOF/Ctrl-D exits the infinite loop and ends the shell
        chomp(line); // Remove trailing newline, see its definition above.
        if (line[0] == '\0') continue;      // Ignore empty lines, skip to next iteration

        // ------------------ Parse input ------------------
        char *argv[MAX_ARGS];
        int argc = split(line, argv, MAX_ARGS); // Split the line into arguments by whitespace.
        // split() modifies the input line, so argv[] points to parts of the same memory
        // Note: split() returns the number of arguments parsed, which is stored in argc.
        // If argc is 0, it means the line was empty or contained only whitespace.
        // If argc is 1, it means there is only one argument (the command itself).
        // If argc is greater than 1, it means there are multiple arguments (command + options/arguments).
        // The last element of argv[] is always NULL, which is required for execvp() to know where the arguments end.
        // This is a common pattern in C for handling command-line arguments.
        // Note: We assume the input is well-formed and does not contain more than MAX_ARGS - 1 arguments.
        // If it does, the last argument will be truncated, but this is a simple shell so we don't handle that case.

        if (argc == 0) continue;

        // ------------------ Built-in commands ------------------
        if (try_builtin(argv, &line)) continue; // If it's a built-in command, run it, then skip to next iteration

        // ------------------ exec external command ------------------
        pid_t pid = fork(); // Create a new process, 
        // pid_t is the process ID type from <sys/types.h> via <unistd.h>. 
        // This is usually just an int, but it's good practice to use the type defined by the system.

        // In a way, the fork() is the punch line of the whole thing:
        // fork() creates a new process that is a copy of the current process,
        // after the fork there are two processes running the same code.
        // - If fork() returns a negative value, it means an error occurred.
        // - If fork() returns 0, we are in the child process (the new one).
        // - If fork() returns a positive value, we are in the parent (original) process, and
        //   the return value is the PID of the child process.
        // The child process will execute the command, while the parent will wait for it to finish.
        if (pid < 0) { // Fork failed, error  
            perror("fork"); // Print error 
            continue; // and skip to next iteration
        }
        if (pid == 0) { // We are in the child (new) process
            // Set up signal handling for child process (optional, not implemented here)

            // Redirections (>, <, >>, etc.) go here when implemented 
            // We will start small with only >  
            // Check if argv contains ">"
            for (int i = 0; argv[i] != NULL; i++) {
                if (strcmp(argv[i], ">") == 0) {
                    // argv[i] is ">", argv[i+1] is filename
                    int fd = open(argv[i+1], O_CREAT | O_WRONLY | O_TRUNC, 0644);
                    if (fd < 0) { 
                        fprintf(stderr, "mtsh: cannot open %s for writing: ", argv[i+1]);
                        perror("");   
                        exit(1);           
                    }

                    if (dup2(fd, STDOUT_FILENO) < 0) {
                        perror("mtsh: dup2 stdout"); 
                        _exit(1);
                    }

                    close(fd);

                    argv[i] = NULL; // cut off argv so execvp sees only the command
                    break;
                }
            }


            // Pipes setup if needed (not implemented in this simple shell, but could be added later)




            // ------------------ Execute command ------------------
            // If execvp fails, it will return and we handle the error below
            // Note: execvp searches the PATH environment variable for the command 
            // child: execute (PATH search via execvp) 
            execvp(argv[0], argv); // Execute the command. execvp() does not return on success,
            // it replaces the current process image with a new one.
            // If execvp() returns, it means there was an error
            perror(argv[0]);   // only reached on error
            _exit(127); // Exit child with error code 127 (command not found)
        } else { // pid > 0, we are in the parent process
            // ------------------ Wait for child process ------------------ 
            // There is no job control in this simple shell yet. 
            int status = 0;
            if (waitpid(pid, &status, 0) < 0) { // Wait for the child process to finish
            // waitpid() waits for the child process with PID pid to change state.
            // If the child process has already exited, it returns immediately. 
                perror("waitpid");
            // Check if the child exited normally or was killed by a signal
            } else if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
                // Child exited normally with exit code 0
                // No output needed, just continue 
            } else if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
                // Child exited with a non-zero exit code
                // Print the exit code
                // Note: WEXITSTATUS(status) extracts the exit code from the status 
                fprintf(stderr, "%s: exit code: %d\n", argv[0], WEXITSTATUS(status));
            } else if (WIFSIGNALED(status)) {
                // Child was killed by a signal
                // Print the signal number that killed the child
                // Note: WTERMSIG(status) extracts the signal number from the status
                // This is useful for debugging or understanding why a command failed
                // e.g., if the command was killed by SIGKILL or SIGSEGV 
                fprintf(stderr, "%s: killed by signal %d (%s)\n", argv[0], WTERMSIG(status), 
                strsignal(WTERMSIG(status)));
            }
        }
    }

    cleanup(&line); // Free the line buffer before exiting
    // Note: We don't need to check if line is NULL, as getline() always allocates memory for it
    // and we free it at the end of the program.
    // This is a simple shell, so we don't need to handle memory leaks or other cleanup
    // in a complex way. The OS will reclaim all memory used by the process when it exits. 
    return 0;
}
