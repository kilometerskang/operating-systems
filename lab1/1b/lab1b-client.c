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
#include <netdb.h>
#include <netinet/in.h>

z_stream from_server;
z_stream to_server;

struct termios saved_attr;
int port_flag = 0;
int log_flag = 0;
int compress_flag = 0;
char* port_arg;
int log_arg;
int shutdown_flag = 0;
int sock = 0;

void close_zstreams() {
    deflateEnd(&to_server);
    inflateEnd(&from_server);
}

void compress_bytes(int num_chars, char* buffer, char* c_buffer) {
    to_server.avail_in = (uInt) num_chars;
    to_server.next_in = (Bytef *) buffer;
    to_server.avail_out = 512;
    to_server.next_out = (Bytef *) c_buffer;

    do {
        if (deflate(&to_server, Z_SYNC_FLUSH) == Z_STREAM_ERROR) {
            fprintf(stderr, "Error %d: Client could not compress data.\nMessage: %s\n", errno, strerror(errno));
            exit(1);
        }
    } while (to_server.avail_in > 0);
}

void decompress_bytes(int num_chars, char* buffer, char* c_buffer) {
    from_server.avail_in = (uInt) num_chars;
    from_server.next_in = (Bytef *) buffer;
    from_server.avail_out = 512;
    from_server.next_out = (Bytef *) c_buffer;

    do {
        if (inflate(&from_server, Z_SYNC_FLUSH) == Z_STREAM_ERROR) {
            fprintf(stderr, "Error %d: Client could not decompress data.\nMessage: %s\n", errno, strerror(errno));
            exit(1);
        }
    } while (from_server.avail_in > 0);
}

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

int keyboard_input_event() {
    char buffer[512];

    // Read characters into buffer, 512 at a time.
    int num_chars = read(STDIN_FILENO, buffer, 512);
    if (num_chars < 0) {
        fprintf(stderr, "Error %d: Could not read character(s).\nMessage: %s\n", errno, strerror(errno));
        exit(1);
    }
    if (num_chars == 0) {
        return 0;
    }
    
    // Write characters from buffer.
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
            default:
                if (write(STDOUT_FILENO, &(buffer[i]), 1) < 0) {
                    fprintf(stderr, "Error %d: Could not write character to stdout.\nMessage: %s\n", errno, strerror(errno));
                    exit(1);
                }
        }
    }

    if (compress_flag == 1) {
        char c_buffer[512];
        compress_bytes(num_chars, buffer, c_buffer);
        num_chars = 512 - to_server.avail_out;

        // Write buffer to server.
        if (write(sock, c_buffer, num_chars) < 0) {
            fprintf(stderr, "Error %d: Could not write buffer to server.\nMessage: %s\n", errno, strerror(errno));
            exit(1);
        }
        if (log_flag == 1) {
            if (dprintf(log_arg, "SENT %d bytes: ", num_chars) < 0) {
                fprintf(stderr, "Error %d: Could not write to log file.\nMessage: %s\n", errno, strerror(errno));
                exit(1);
            }
            if (write(log_arg, c_buffer, num_chars) < 0) {
                fprintf(stderr, "Error %d: Could not write to log file.\nMessage: %s\n", errno, strerror(errno));
                exit(1);
            }
            if (dprintf(log_arg, "\n") < 0) {
                fprintf(stderr, "Error %d: Could not write to log file.\nMessage: %s\n", errno, strerror(errno));
                exit(1);
            }
        }
    }
    else {
        // Write buffer to server.
        if (write(sock, buffer, num_chars) < 0) {
            fprintf(stderr, "Error %d: Could not write buffer to server.\nMessage: %s\n", errno, strerror(errno));
            exit(1);
        }
        if (log_flag == 1) {
            if (dprintf(log_arg, "SENT %d bytes: ", num_chars) < 0) {
                fprintf(stderr, "Error %d: Could not write to log file.\nMessage: %s\n", errno, strerror(errno));
                exit(1);
            }
            if (write(log_arg, buffer, num_chars) < 0) {
                fprintf(stderr, "Error %d: Could not write to log file.\nMessage: %s\n", errno, strerror(errno));
                exit(1);
            }
            if (dprintf(log_arg, "\n") < 0) {
                fprintf(stderr, "Error %d: Could not write to log file.\nMessage: %s\n", errno, strerror(errno));
                exit(1);
            }
        }
    }
    return 1;
}

int socket_input_event() {
    char s_buffer[512];

    // Read characters into buffer, 512 at a time.
    int num_chars = read(sock, s_buffer, 512);
    if (num_chars < 0) {
        fprintf(stderr, "Error %d: Could not read character(s).\nMessage: %s\n", errno, strerror(errno));
        exit(1);
    }
    if (num_chars == 0) {
        return 0;
    }
    
    if (log_flag == 1) {
        if (dprintf(log_arg, "RECEIVED %d bytes: ", num_chars) < 0) {
            fprintf(stderr, "Error %d: Could not write to log file.\nMessage: %s\n", errno, strerror(errno));
            exit(1);
        }
        if (write(log_arg, s_buffer, num_chars) < 0) {
            fprintf(stderr, "Error %d: Could not write to log file.\nMessage: %s\n", errno, strerror(errno));
            exit(1);
        }
        if (dprintf(log_arg, "\n") < 0) {
            fprintf(stderr, "Error %d: Could not write to log file.\nMessage: %s\n", errno, strerror(errno));
            exit(1);
        }
    }

    if (compress_flag == 1) {
        char cs_buffer[512];
        memcpy(cs_buffer, s_buffer, num_chars);
        decompress_bytes(num_chars, cs_buffer, s_buffer);
        num_chars = 512 - from_server.avail_out;
    }

    // Write buffer to stdout.
    if (write(STDOUT_FILENO, s_buffer, num_chars) < 0) {
        fprintf(stderr, "Error %d: Could not write buffer to server.\nMessage: %s\n", errno, strerror(errno));
        exit(1);
    }
    return 1;
}

void wait_for_input() {
    struct pollfd fds[2];
    int time;

    // Both polls wait for input or error events.
    fds[0].fd = STDIN_FILENO;
    fds[0].events = POLLIN | POLLHUP | POLLERR;
    fds[1].fd = sock;
    fds[1].events = POLLIN | POLLHUP | POLLERR;

    while (1) {
        if ((time = poll(fds, 2, 0)) < 0) {
            fprintf(stderr, "Error %d: Poll could not be executed.\nMessage: %s\n", errno, strerror(errno));
            exit(1);
        }
        if (time > 0) {
            if (fds[0].revents & POLLIN) {
                if (keyboard_input_event() == 0) {
                    break;
                }
            }
            if (fds[0].revents & (POLLHUP | POLLERR)) {
                break;
            }
            if (fds[1].revents & POLLIN) {
                if (socket_input_event() == 0) {
                    break;
                }
            }
            if (fds[1].revents & (POLLHUP | POLLERR)) {
                break;
            }
        }
    }

    // if (shutdown(sock, SHUT_RDWR) < 0) {
    //     fprintf(stderr, "Error %d: Could not shut down socket.\nMessage: %s\n", errno, strerror(errno));
    //     exit(1);
    // }
    close(sock);
    exit(0);
}

int main(int argc, char* argv[]) {
    // Get arguments.
    const struct option long_options[] = {
        {"port", required_argument, 0, 1},
        {"log", required_argument, 0, 2},
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
                log_flag = 1;
                log_arg = creat(optarg, 0666);
                if (log_arg < 0) {
                    fprintf(stderr, "Error %d: Log file could not be created.\nMessage: %s\n", errno, strerror(errno));
                    exit(1);
                }
                break;
            case 3:
                compress_flag = 1;
                from_server.zalloc = Z_NULL;
                from_server.zfree = Z_NULL;
                from_server.opaque = Z_NULL;
                if (inflateInit(&from_server) != Z_OK) {
                    fprintf(stderr, "Error %d: Client could not create decompression stream.\nMessage: %s\n", errno, strerror(errno));
                    exit(1);
                }
                to_server.zalloc = Z_NULL;
                to_server.zfree = Z_NULL;
                to_server.opaque = Z_NULL;
                if (deflateInit(&to_server, Z_DEFAULT_COMPRESSION) != Z_OK) {
                    fprintf(stderr, "Error %d: Client could not create compression stream.\nMessage: %s\n", errno, strerror(errno));
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

    // Set terminal mode.
    set_input_mode();

    struct sockaddr_in server_address;
    int address_length = sizeof(server_address);
    struct hostent* server_host;

    // Create socket and connect to server.
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        fprintf(stderr, "Error %d: Socket could not be created.\nMessage: %s\n", errno, strerror(errno));
        exit(1);
    }

    server_host = gethostbyname("localhost");
    if (server_host == 0) {
        fprintf(stderr, "Error %d: Could not get server host.\nMessage: %s\n", errno, strerror(errno));
        exit(1);
    }
    memset(&server_address, '0', address_length);
    memcpy((char *) &server_address.sin_addr.s_addr, (char *) server_host->h_addr, server_host->h_length);
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(atoi(port_arg));

    if (connect(sock, (struct sockaddr *) &server_address, address_length) < 0) {
        fprintf(stderr, "Error %d: Could not connect to server.\nMessage: %s\n", errno, strerror(errno));
        exit(1);
    }

    // Wait for input.
    wait_for_input();

}