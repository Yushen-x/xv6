#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
    int p2c[2]; // Pipe: parent to child
    int c2p[2]; // Pipe: child to parent
    char buf[1]; // A 1-byte buffer

    // Create pipes
    if(pipe(p2c) < 0 || pipe(c2p) < 0){
        fprintf(2, "pipe creation failed\n");
        exit(1);
    }

    int pid = fork();

    if (pid < 0) {
        fprintf(2, "fork failed\n");
        exit(1);
    }

    if (pid == 0) {
        // --- Child Process ---

        // Close unused pipe ends
        close(p2c[1]); // Child doesn't write to parent-to-child pipe
        close(c2p[0]); // Child doesn't read from child-to-parent pipe

        // Read the byte from the parent
        if (read(p2c[0], buf, sizeof(buf)) != sizeof(buf)) {
            fprintf(2, "child: failed to read from parent\n");
            exit(1);
        }
        
        // Print the "ping" message
        printf("%d: received ping\n", getpid());

        // Write the byte back to the parent
        if (write(c2p[1], buf, sizeof(buf)) != sizeof(buf)) {
            fprintf(2, "child: failed to write to parent\n");
            exit(1);
        }

        // Clean up and exit
        close(p2c[0]);
        close(c2p[1]);
        exit(0);

    } else {
        // --- Parent Process ---

        // Close unused pipe ends
        close(p2c[0]); // Parent doesn't read from parent-to-child pipe
        close(c2p[1]); // Parent doesn't write to child-to-parent pipe

        // Write the byte to the child
        if (write(p2c[1], "a", sizeof(buf)) != sizeof(buf)) {
            fprintf(2, "parent: failed to write to child\n");
            exit(1);
        }
        
        // Wait for the child to send the byte back
        if (read(c2p[0], buf, sizeof(buf)) != sizeof(buf)) {
            fprintf(2, "parent: failed to read from child\n");
            exit(1);
        }

        // Print the "pong" message
        printf("%d: received pong\n", getpid());

        // Clean up, wait for child, and exit
        close(p2c[1]);
        close(c2p[0]);
        wait(0); // Wait for child process to terminate
        exit(0);
    }
}
