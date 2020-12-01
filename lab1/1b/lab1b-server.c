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
#include <sys/socket.h>
#include <zlib.h>
#include <fcntl.h>
#include <netinet/in.h>

z_stream from_server;
z_stream to_server;

int port_flag = 0;
int shell_flag = 0;
int compress_flag = 0;
char* port_arg;
char* shell_arg;
int shutdown_flag = 0;
int sock = 0;
int new_sock = 0;

int to_shell[2];
int from_shell[2];
pid_t child_pid;
int pipe_closed = 0;

void close_zstreams() {
    deflateEnd(&to_server);
    inflateEnd(&from_server);
}

void compress_bytes(int num_chars, char* buffer, char* c_buffer) {
    from_server.avail_in = (uInt) num_chars;
    from_server.next_in = (Bytef *) buffer;
    from_server.avail_out = 512;
    from_server.next_out = (Bytef *) c_buffer;

    do {
        if (deflate(&from_server, Z_SYNC_FLUSH) == Z_STREAM_ERROR) {
            fprintf(stderr, "Error %d: Server could not compress data.\nMessage: %s\n", errno, strerror(errno));
            exit(1);
        }
    } while (from_server.avail_in > 0);
}

void decompress_bytes(int num_chars, char* buffer, char* c_buffer) {
    to_server.avail_in = (uInt) num_chars;
    to_server.next_in = (Bytef *) buffer;
    to_server.avail_out = 512;
    to_server.next_out = (Bytef *) c_buffer;

    do {
        if (inflate(&to_server, Z_SYNC_FLUSH) == Z_STREAM_ERROR) {
            fprintf(stderr, "Error %d: Server could not decompress data.\nMessage: %s\n", errno, strerror(errno));
            exit(1);
        }
    } while (to_server.avail_in > 0);
}

int socket_input_event() {
    char s_buffer[512];
    char cs_buffer[512];

    // Read characters into buffer, 512 at a time.
    int num_chars = read(new_sock, s_buffer, 512);
    if (num_chars < 0) {
        fprintf(stderr, "Error %d: Could not read character(s).\nMessage: %s\n", errno, strerror(errno));
        exit(1);
    }
    if (num_chars == 0) {
        return 0;
    }

    if (compress_flag == 1) {
        memcpy(cs_buffer, s_buffer, num_chars);
        decompress_bytes(num_chars, cs_buffer, s_buffer);
        num_chars = 512 - to_server.avail_out;
        // memcpy(s_buffer, cs_buffer, num_chars);
    }
    
    // Write characters from buffer. Handle CR, LF, and EOF cases too.
    for (int i = 0; i < num_chars; i++) {
        char ch[2];
        switch (s_buffer[i]) {
            case 0x0D:
            case 0x0A:
                ch[0] = '\r';
                ch[1] = '\n';
                if (write(to_shell[1], &ch[1], 1) < 0) {
                    fprintf(stderr, "Error %d: Could not write newline to shell.\nMessage: %s\n", errno, strerror(errno));
                    exit(1);
                }
                break;
            case 0x04:
                ch[0] = '^';
                ch[1] = 'D';
                if (pipe_closed == 0) {
                    close(to_shell[1]);
                    pipe_closed = 1;
                }
                break;
            case 0x03:
                ch[0] = '^';
                ch[1] = 'C';
                if (kill(child_pid, SIGINT) < 0) {
                    fprintf(stderr, "Error %d: Shell process could not be killed.\nMessage: %s\n", errno, strerror(errno));
                    exit(1);
                }
                break;
            default:
                if (write(to_shell[1], &(s_buffer[i]), 1) < 0) {
                    fprintf(stderr, "Error %d: Could not write character to shell.\nMessage: %s\n", errno, strerror(errno));
                    exit(1);
                }
        }
    }
    return 1;
}

int shell_input_event() {
    char buffer[512];
    // char c_buffer[512];

    // Read characters into buffer, 512 at a time.
    int num_chars = read(from_shell[0], buffer, 512);
    if (num_chars < 0) {
        fprintf(stderr, "Error %d: Could not read character(s).\nMessage: %s\n", errno, strerror(errno));
        exit(1);
    }
    if (num_chars == 0) {
        return 0;
    }

    if (compress_flag == 1) {
        char c_buffer[512];
        memcpy(c_buffer, buffer, num_chars);
        compress_bytes(num_chars, buffer, c_buffer);
        num_chars = 512 - from_server.avail_out;

        // fprintf(stderr, "%d ", num_chars);
        // memcpy(c_buffer, buffer, num_chars);
        // compress_bytes(num_chars, c_buffer, buffer);
        // num_chars = 512 - from_server.avail_out;
        // fprintf(stderr, "%d ", num_chars);

        // Write characters from buffer. Handle CR, LF, and EOF cases too.
        for (int i = 0; i < num_chars; i++) {
            char ch[2];
            switch (c_buffer[i]) {
                case 0x0D:
                case 0x0A:
                    ch[0] = '\r';
                    ch[1] = '\n';
                    if (write(new_sock, ch, 2) < 0) {
                        fprintf(stderr, "Error %d: Could not write characters to server.\nMessage: %s\n", errno, strerror(errno));
                        exit(1);
                    }
                    break;
                case 0x04:
                    ch[0] = '^';
                    ch[1] = 'D';
                    if (write(new_sock, ch, 2) < 0) {
                        fprintf(stderr, "Error %d: Could not write characters to server.\nMessage: %s\n", errno, strerror(errno));
                        exit(1);
                    }
                    shutdown_flag = 1;
                    break;
                default:
                    if (write(new_sock, &(c_buffer[i]), 1) < 0) {
                        fprintf(stderr, "Error %d: Could not write character to server.\nMessage: %s\n", errno, strerror(errno));
                        exit(1);
                    }
            }
        }
    }
    else {
        // Write characters from buffer. Handle CR, LF, and EOF cases too.
        for (int i = 0; i < num_chars; i++) {
            char ch[2];
            switch (buffer[i]) {
                case 0x0D:
                case 0x0A:
                    ch[0] = '\r';
                    ch[1] = '\n';
                    if (write(new_sock, ch, 2) < 0) {
                        fprintf(stderr, "Error %d: Could not write characters to server.\nMessage: %s\n", errno, strerror(errno));
                        exit(1);
                    }
                    break;
                case 0x04:
                    ch[0] = '^';
                    ch[1] = 'D';
                    if (write(new_sock, ch, 2) < 0) {
                        fprintf(stderr, "Error %d: Could not write characters to server.\nMessage: %s\n", errno, strerror(errno));
                        exit(1);
                    }
                    shutdown_flag = 1;
                    break;
                default:
                    if (write(new_sock, &(buffer[i]), 1) < 0) {
                        fprintf(stderr, "Error %d: Could not write character to server.\nMessage: %s\n", errno, strerror(errno));
                        exit(1);
                    }
            }
        }
    }
    return 1;
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
    fds[0].fd = new_sock;
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
                if (socket_input_event() == 0) {
                    break;
                }
            }
            if (fds[0].revents & (POLLHUP | POLLERR)) {
                break;
            }
            if (fds[1].revents & POLLIN) {
                if (shell_input_event() == 0) {
                    break;
                }
            }
            if (fds[1].revents & (POLLHUP | POLLERR)) {
                break;
            }
        }
        if (shutdown_flag == 1) {
            break;
        }
    }

    if (pipe_closed == 0) {
        close(to_shell[1]);
        pipe_closed = 1;
    }

    close(sock);
    close(new_sock);
    if (compress_flag == 1) {
        deflateEnd(&from_server);
        inflateEnd(&to_server);
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
    // Get arguments.
    const struct option long_options[] = {
        {"port", required_argument, 0, 1},
        {"shell", required_argument, 0, 2},
        {"compress", no_argument, 0 ,3}
    };

    while (1) {
        int c = getopt_long(argc, argv, "", long_options, NULL);
        if (c == -1) break;

        switch(c) {
            case 1:
                port_flag = 1;
                port_arg = optarg;
                break;
            case 2:
                shell_flag = 1;
                shell_arg = optarg;
                break;
            case 3:
                compress_flag = 1;
                from_server.zalloc = Z_NULL;
                from_server.zfree = Z_NULL;
                from_server.opaque = Z_NULL;
                if (deflateInit(&from_server, Z_DEFAULT_COMPRESSION) != Z_OK) {
                    fprintf(stderr, "Error %d: Server could not create compression stream.\nMessage: %s\n", errno, strerror(errno));
                    exit(1);
                }
                to_server.zalloc = Z_NULL;
                to_server.zfree = Z_NULL;
                to_server.opaque = Z_NULL;
                if (inflateInit(&to_server) != Z_OK) {
                    fprintf(stderr, "Error %d: Server could not create decompression stream.\nMessage: %s\n", errno, strerror(errno));
                    exit(1);
                }
                atexit(close_zstreams);
                break;
            default:
                fprintf(stderr, "Error %d: Unrecognized/no argument was given.\nMessage: %s\n", errno, strerror(errno));
                exit(1);
        }
    }

    // No port argument.
    if (port_flag == 0) {
        fprintf(stderr, "Error %d: Requires --port argument.\nMessage: %s\n", errno, strerror(errno));
        exit(1);
    }

    // No shell argument.
    if (shell_flag == 0) {
        fprintf(stderr, "Error %d: Requires --shell argument.\nMessage: %s\n", errno, strerror(errno));
        exit(1);
    }

    struct sockaddr_in server_address;
    int address_length = sizeof(server_address);

    // Create socket and bind to server.
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        fprintf(stderr, "Error %d: Socket could not be created.\nMessage: %s\n", errno, strerror(errno));
        exit(1);
    }

    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(atoi(port_arg));

    if (bind(sock, (struct sockaddr *) &server_address, address_length) < 0) {
        fprintf(stderr, "Error %d: Could not bind to server.\nMessage: %s\n", errno, strerror(errno));
        exit(1);
    }

    // Listen for incoming connections.
    if (listen(sock, 5) < 0) {
        fprintf(stderr, "Error %d: Could not listen for incoming connections.\nMessage: %s\n", errno, strerror(errno));
        exit(1);
    }
    // Accept incoming connections.
    new_sock = accept(sock, (struct sockaddr *) &server_address, (socklen_t *) &address_length);
    if (new_sock < 0) {
        fprintf(stderr, "Error %d: Could not accept incoming connection.\nMessage: %s\n", errno, strerror(errno));
        exit(1);
    }

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