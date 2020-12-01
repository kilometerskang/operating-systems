//NAME: Miles Kang
//EMAIL: milesjkang@gmail.com
//ID: 405106565

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <getopt.h>
#include <fcntl.h>
#include <string.h>

void seg_force() {
  char* x = NULL;
  *x = 'x';
  return;
}

void seg_handler() {
  int tmp = 0;
  tmp = errno;
  fprintf(stderr, "Error %d: Caught segmentation fault.\n", tmp);
  fprintf(stderr, "Message: %s\n", strerror(tmp));
  exit(4);
}

void redirect_input(char* newfile) {
  int ifd = open(newfile, O_RDONLY);
  if (ifd >= 0) {
    close(0);
    dup(ifd);
    close(ifd);
  }
  else {
    int tmp = 0;
    tmp = errno;
    fprintf(stderr, "Error %d: The input argument could not be opened.\n", tmp);
    fprintf(stderr, "Message: %s\n", strerror(tmp));
    exit(2);
  }
}

void redirect_output(char* newfile) {
  int ofd = creat(newfile, 0666);
  if (ofd >= 0) {
    close(1);
    dup(ofd);
    close(ofd);
  }
  else {
    int tmp = 0;
    tmp = errno;
    fprintf(stderr, "Error %d: The output file could not be opened/created.\n", tmp);
    fprintf(stderr, "Message: %s\n", strerror(tmp));
    exit(3);
  }
}

int main(int argc, char* argv[]) {
  // parse arguments.
  int seg_flag = 0;
  char *input = NULL;
  char *output = NULL;

  struct option long_options[] = {
    {"input", required_argument, 0, 0},
    {"output", required_argument, 0, 1},
    {"segfault", no_argument, 0, 2},
    {"catch", no_argument, 0, 3}
  };

  int tmp = 0;
  while(1) {
    int c = getopt_long(argc, argv, "", long_options, NULL);
    if (c == -1) break;

    switch (c) {
    case 0:
      input = optarg;
      break;
    case 1:
      output = optarg;
      break;
    case 2:
      seg_flag = 1;
      break;
    case 3:
      signal(SIGSEGV, seg_handler);
      break;
    default:
      tmp = errno;
      fprintf(stderr, "Error %d: Unrecognized argument was given.\n", tmp);
      fprintf(stderr, "Message: %s\n", strerror(tmp));
      exit(1);
    }
  }

  if (input != NULL) {
    redirect_input(input);
  }
  if (output != NULL) {
    redirect_output(output);
  }
  if (seg_flag == 1) {
    seg_force();
  }
 
  // read from stdin and write to stdout.
  char buffer;
  while (read(0, &buffer, 1) > 0) {
    write(1, &buffer, 1);
  }

  exit(0);
}
