#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>

#define SH_TOK_BUFSIZE 64
#define SH_TOK_DELIM " \t\r\n\a"
#define SH_TOK_REDIR_DELIM "<>"

char *sh_read_line(void) {
  char *line = NULL;
  size_t bufsize = 0;

  if (getline(&line, &bufsize, stdin) == -1) {
    if (feof(stdin)) {
      fprintf(stderr, "EOF\n");
      exit(EXIT_SUCCESS);
    } else {
      fprintf(stderr, "Value of errno: %d\n", errno);
      exit(EXIT_FAILURE);
    }   
  }
  return line;
}

char **sh_split_line(char *line) {
  int bufsize = SH_TOK_BUFSIZE;
  int position = 0;
  char **tokens = malloc(bufsize * sizeof(char *));
  char *token, **tokens_backup;

  if (!tokens) {
    fprintf(stderr, "sh: allocation error\n");
    exit(EXIT_FAILURE);
  }

  token = strtok(line, SH_TOK_DELIM);
  while (token != NULL) {
    tokens[position] = token;
    position++;

    if (position >= bufsize) {
      bufsize += SH_TOK_BUFSIZE;
      tokens_backup = tokens;
      tokens = realloc(tokens, bufsize * sizeof(char *));
      if (!tokens) {
        free(tokens_backup);
        fprintf(stderr, "sh: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }

    token = strtok(NULL, SH_TOK_DELIM);
  }
  tokens[position] = NULL;
  return tokens;
}

int sh_launch(char **args) {
  if (fork() == 0) {
    execvp(args[0], args);
    fprintf(stderr, "exec %s failed\n", args[0]);
    return 0;
  }
  wait(NULL);
  return 1;
}

int sh_execute(char **args) {
  if (args[0] == NULL) {
    return 1;
  }

  return sh_launch(args);
}

void sh_loop(void) {
  char *line;
  char **args;
  int status;

  do {
    printf("143A$ ");
    line = sh_read_line();
    args = sh_split_line(line);
    status = sh_execute(args);

    free(line);
    free(args);
  } while (status);
}

int main(int argc, char **argv) {
  sh_loop();
  return EXIT_SUCCESS;
}
