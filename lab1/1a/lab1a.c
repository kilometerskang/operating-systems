// NAME: Miles Kang
// EMAIL: milesjkang@gmail.com
// ID: 405106565

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <termios.h>
#include <string.h>
#include <getopt.h>
#include <poll.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

struct termios saved_attr;
int to_shell[2];
int from_shell[2];
pid_t child_pid;
char* shell_arg;
int pipe_closed = 0;
int shutdown = 0;

void reset_input_mode() {
    if (tcsetattr(STDIN_FILENO, TCSANOW, &saved_attr) < 0) {
        fprintf(stderr, "Error %d: Could not set terminal attributes.\nMessage: %s\n", errno, strerror(errno));
        exit(1);
    }
}

void set_input_mode() {
    struct termios new_attr;

    if (tcgetattr(STDIN_FILENO, &saved_attr) < 0) {
        fprintf(stderr, "Error %d: Could not get terminal attributes.\nMessage: %s\n", errno, strerror(errno));
        exit(1);
    }
    atexit(reset_input_mode);

    if (tcgetattr(STDIN_FILENO, &new_attr) < 0) {
        fprintf(stderr, "Error %d: Could not get terminal attributes.\nMessage: %s\n", errno, strerror(errno));
        exit(1);
    }

    new_attr.c_iflag = ISTRIP;
    new_attr.c_oflag = 0;
    new_attr.c_lflag = 0;

    if (tcsetattr(STDIN_FILENO, TCSANOW, &new_attr) < 0) {
        fprintf(stderr, "Error %d: Could not set terminal attributes.\nMessage: %s\n", errno, strerror(errno));
        exit(1);
    }
}

void basic_read_write() {
    char buffer[1024];

    while (1) {
        // Read characters into buffer, 1024 at a time.
        int num_chars = read(STDIN_FILENO, buffer, 1024);
        if (num_chars < 0) {
            fprintf(stderr, "Error %d: Could not read character(s).\nMessage: %s\n", errno, strerror(errno));
            exit(1);
        }
        
        // Write characters from buffer. Handle CR, LF, and EOF cases too.
        for (int i = 0; i < num_chars; i++) {
            char ch[2];
            switch (buffer[i]) {
                case 0x0D:
                case 0x0A:
                    ch[0] = '\r';
                    ch[1] = '\n';
                    if (write(STDOUT_FILENO, ch, 2) < 0) {
                        fprintf(stderr, "Error %d: Could not write characters to stdout.\nMessage: %s\n", errno, strerror(errno));
                        exit(1);
                    }
                    break;
                case 0x04:
                    ch[0] = '^';
                    ch[1] = 'D';
                    if (write(STDOUT_FILENO, ch, 2) < 0) {
                        fprintf(stderr, "Error %d: Could not write characters to stdout.\nMessage: %s\n", errno, strerror(errno));
                        exit(1);
                    }
                    // Will restore terminal mode on exit.
                    exit(0);
                default:
                    if (write(STDOUT_FILENO, &(buffer[i]), 1) < 0) {
                        fprintf(stderr, "Error %d: Could not write character to stdout.\nMessage: %s\n", errno, strerror(errno));
                        exit(1);
                    }
            }
        }
    }
}

void keyboard_input_event() {
    char buffer[1024];

    // Read characters into buffer, 1024 at a time.
    int num_chars = read(STDIN_FILENO, buffer, 1024);
    if (num_chars < 0) {
        fprintf(stderr, "Error %d: Could not read character(s).\nMessage: %s\n", errno, strerror(errno));
        exit(1);
    }
    
    // Write characters from buffer. Handle CR, LF, and EOF cases too.
    for (int i = 0; i < num_chars; i++) {
        char ch[2];
        switch (buffer[i]) {
            case 0x0D:
            case 0x0A:
                ch[0] = '\r';
                ch[1] = '\n';
                if (write(STDOUT_FILENO, ch, 2) < 0) {
                    fprintf(stderr, "Error %d: Could not write characters to stdout.\nMessage: %s\n", errno, strerror(errno));
                    exit(1);
                }
                if (write(to_shell[1], &ch[1], 1) < 0) {
                    fprintf(stderr, "Error %d: Could not write newline to shell.\nMessage: %s\n", errno, strerror(errno));
                    exit(1);
                }
                break;
            case 0x04:
                ch[0] = '^';
                ch[1] = 'D';
                if (write(STDOUT_FILENO, ch, 2) < 0) {
                    fprintf(stderr, "Error %d: Could not write characters to stdout.\nMessage: %s\n", errno, strerror(errno));
                    exit(1);
                }
                if (pipe_closed == 0) {
                    close(to_shell[1]);
                    pipe_closed = 1;
                }
                break;
            case 0x03:
                ch[0] = '^';
                ch[1] = 'C';
                if (write(STDOUT_FILENO, ch, 2) < 0) {
                    fprintf(stderr, "Error %d: Could not write characters to stdout.\nMessage: %s\n", errno, strerror(errno));
                    exit(1);
                }
                if (kill(child_pid, SIGINT) < 0) {
                    fprintf(stderr, "Error %d: Shell process could not be killed.\nMessage: %s\n", errno, strerror(errno));
                    exit(1);
                }
                break;
            default:
                if (write(STDOUT_FILENO, &(buffer[i]), 1) < 0) {
                    fprintf(stderr, "Error %d: Could not write character to stdout.\nMessage: %s\n", errno, strerror(errno));
                    exit(1);
                }
                if (write(to_shell[1], &(buffer[i]), 1) < 0) {
                    fprintf(stderr, "Error %d: Could not write character to shell.\nMessage: %s\n", errno, strerror(errno));
                    exit(1);
                }
        }
    }
}

void shell_input_event() {
    char buffer[1024];

    // Read characters into buffer, 1024 at a time.
    int num_chars = read(from_shell[0], buffer, 1024);
    if (num_chars < 0) {
        fprintf(stderr, "Error %d: Could not read character(s).\nMessage: %s\n", errno, strerror(errno));
        exit(1);
    }
    
    // Write characters from buffer. Handle CR, LF, and EOF cases too.
    for (int i = 0; i < num_chars; i++) {
        char ch[2];
        switch (buffer[i]) {
            case 0x0D:
            case 0x0A:
                ch[0] = '\r';
                ch[1] = '\n';
                if (write(STDOUT_FILENO, ch, 2) < 0) {
                    fprintf(stderr, "Error %d: Could not write characters to stdout.\nMessage: %s\n", errno, strerror(errno));
                    exit(1);
                }
                break;
            case 0x04:
                ch[0] = '^';
                ch[1] = 'D';
                if (write(STDOUT_FILENO, ch, 2) < 0) {
                    fprintf(stderr, "Error %d: Could not write characters to stdout.\nMessage: %s\n", errno, strerror(errno));
                    exit(1);
                }
                shutdown = 1;
                break;
            default:
                if (write(STDOUT_FILENO, &(buffer[i]), 1) < 0) {
                    fprintf(stderr, "Error %d: Could not write character to stdout.\nMessage: %s\n", errno, strerror(errno));
                    exit(1);
                }
        }
    }
}

void child_process() {
    // Close write end of pipe from terminal to shell.
    if (close(to_shell[1]) == -1) {
        fprintf(stderr, "Error %d: File could not be closed.\nMessage: %s\n", errno, strerror(errno));
        exit(1);
    }
    // Close read end of pipe from shell to terminal.
    if (close(from_shell[0]) == -1) {
        fprintf(stderr, "Error %d: File could not be closed.\nMessage: %s\n", errno, strerror(errno));
        exit(1);
    }
    // Redirect read end of to_shell pipe to stdin.
    if (close(STDIN_FILENO) == -1) {
        fprintf(stderr, "Error %d: File could not be closed.\nMessage: %s\n", errno, strerror(errno));
        exit(1);
    }
    if (dup(to_shell[0]) == -1) {
        fprintf(stderr, "Error %d: File could not be duplicated.\nMessage: %s\n", errno, strerror(errno));
        exit(1);
    }
    if (close(to_shell[0]) == -1) {
        fprintf(stderr, "Error %d: File could not be closed.\nMessage: %s\n", errno, strerror(errno));
        exit(1);
    }
    // Redirect write end of from_shell pipe to stdout and stderr.
    if (close(STDOUT_FILENO) == -1) {
        fprintf(stderr, "Error %d: File could not be closed.\nMessage: %s\n", errno, strerror(errno));
        exit(1);
    }
    if (dup(from_shell[1]) == -1) {
        fprintf(stderr, "Error %d: File could not be duplicated.\nMessage: %s\n", errno, strerror(errno));
        exit(1);
    }
    if (close(STDERR_FILENO) == -1) {
        fprintf(stderr, "Error %d: File could not be closed.\nMessage: %s\n", errno, strerror(errno));
        exit(1);
    }
    if (dup(from_shell[1]) == -1) {
        fprintf(stderr, "Error %d: File could not be duplicated.\nMessage: %s\n", errno, strerror(errno));
        exit(1);
    }
    if (close(from_shell[1]) == -1) {
        fprintf(stderr, "Error %d: File could not be closed.\nMessage: %s\n", errno, strerror(errno));
        exit(1);
    }

    // Execute command passed in from terminal.
    char* exec_args[2];
    exec_args[0] = shell_arg;
    exec_args[1] = NULL;
    if (execvp(shell_arg, exec_args) == -1) {
        fprintf(stderr, "Error %d: Shell argument could not be executed.\nMessage: %s\n", errno, strerror(errno));
        exit(1);
    }
}

void parent_process() {
    // Close read end of to_shell pipe and write end of from_shell pipe.
    close(to_shell[0]);
    close(from_shell[1]);

    struct pollfd fds[2];
    int time;

    // Both polls wait for input or error events.
    fds[0].fd = STDIN_FILENO;
    fds[0].events = POLLIN | POLLHUP | POLLERR;
    fds[1].fd = from_shell[0];
    fds[1].events = POLLIN | POLLHUP | POLLERR;

    while (1) {
        if ((time = poll(fds, 2, 0)) < 0) {
            fprintf(stderr, "Error %d: Poll could not be executed.\nMessage: %s\n", errno, strerror(errno));
            exit(1);
        }
        if (time > 0) {
            if (fds[0].revents & POLLIN) {
                keyboard_input_event();
            }
            if (fds[1].revents & POLLIN) {
                shell_input_event();
            }
            if (fds[0].revents & (POLLHUP | POLLERR)) {
                shutdown = 1;
            }
            if (fds[1].revents & (POLLHUP | POLLERR)) {
                shutdown = 1;
            }
        }
        if (shutdown == 1) {
            break;
        }
    }

    if (pipe_closed == 0) {
        close(to_shell[1]);
        pipe_closed = 1;
    }

    int exit_status;
    if (waitpid(child_pid, &exit_status, 0) < 0) {
        fprintf(stderr, "Error %d: waitpid failed.\nMessage: %s\n", errno, strerror(errno));
        exit(1);
    }

    int low_bits = exit_status & 0x007f;
	int hi_byte = exit_status >> 8;
	fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\n", low_bits, hi_byte);
	exit(0);
}

int main(int argc, char* argv[]) {
    int shell_flag = 0;

    // Get arguments.
    const struct option long_options[] = {
        {"shell", required_argument, 0, 1}
    };

    while (1) {
        int c = getopt_long(argc, argv, "", long_options, NULL);
        if (c == -1) break;

        switch(c) {
            case 1:
                shell_flag = 1;
                shell_arg = optarg;
                break;
            default:
                fprintf(stderr, "Error %d: Unrecognized argument was given.\nMessage: %s\n", errno, strerror(errno));
                exit(1);
        }
    }

    // Set terminal mode.
    set_input_mode();

    // No shell argument.
    if (shell_flag == 0) {
        basic_read_write();
    }

    // Shell argument.
    // Create pipes going to and from the shell.
    if (pipe(to_shell) < 0) {
        fprintf(stderr, "Error %d: Pipe from terminal to shell unsuccessful.\nMessage: %s\n", errno, strerror(errno));
        exit(1);
    }
    if (pipe(from_shell) < 0) {
        fprintf(stderr, "Error %d: Pipe from shell to terminal unsuccessful.\nMessage: %s\n", errno, strerror(errno));
        exit(1);
    }

    // Fork.
    child_pid = fork();
    if (child_pid == -1) {
        fprintf(stderr, "Error %d: Fork unsuccessful.\nMessage: %s\n", errno, strerror(errno));
        exit(1);
    }
    // Child process.
    if (child_pid == 0) {
        child_process();
    }
    // Parent process.
    else {
        parent_process();
    }
}
