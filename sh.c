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
#include <glob.h>

#define SH_TOK_BUFSIZE 64
#define SH_TOK_DELIM " \t\r\n\a"
#define MAX_HISTORY_SIZE 1000

static char *cmd_history[MAX_HISTORY_SIZE];
static unsigned history_count = 0;

void add_to_history(const char *cmd) {
  if (history_count < MAX_HISTORY_SIZE)
      cmd_history[history_count++] = strdup(cmd);
  else {
    free(cmd_history[0]);
    unsigned i;
    for (i = 1; i < MAX_HISTORY_SIZE; i++)
        cmd_history[i-1] = cmd_history[i];
    cmd_history[MAX_HISTORY_SIZE-1] = strdup(cmd);
  }
}

void print_history(void) {
  unsigned i;
  for (i = 0; i < history_count; i++) {
    printf("   %d  %s", i+1, cmd_history[i]);
  }
}

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
    exit(EXIT_FAILURE);
  }
  wait(NULL);
  return 1;
}

void sh_redir( char* file, int mode, int fd) {
  close(fd);
  if (open(file, mode) < 0) {
    fprintf(stderr, "open %s failed\n", file);
    exit(EXIT_FAILURE);
  }
}


int sh_execute(char **args) {

  if (args[0] == NULL) {
    return 1;
  }

  char **a = args;
  int pd[2];


  for(; *a; a++) {

    switch (**a) {
      case '<':
        *a = NULL;
        sh_redir(*(a+1), O_RDONLY, 0);
        break;
      case '>':
        *a = NULL;
        sh_redir(*(a+1), O_WRONLY|O_CREAT, 1);
        break;
      case '|': {
        *a = NULL;

        pipe(pd);

        if (fork() == 0) {
          close(1);
          dup(pd[1]);
          close(pd[0]);
          close(pd[1]);
          execvp(args[0], args);
          fprintf(stderr, "exec %s failed\n", args[0]);
          exit(EXIT_FAILURE); 
        }
        if (fork() == 0) {
          close(0);
          dup(pd[0]);
          close(pd[0]);
          close(pd[1]);
          return sh_execute(a+1);
        }

        close(pd[0]);
        close(pd[1]);
        wait(NULL);
        wait(NULL);
 
       
        return 1;
      }
        break;
      case '&':
        if (*(a+1) == NULL) {
          *a = NULL;
          int id = fork();
          int status;
          if (id == 0) {
            execvp(args[0], args);
            fprintf(stderr, "exec %s failed\n", args[0]);
            exit(EXIT_FAILURE);
          }
          waitpid(-1, &status, WNOHANG);
          return 1;
        }
        break;
    } 
   
    if (strchr(*a, '*') || strchr(*a, '?')) {
      glob_t paths;
      int retval;

      paths.gl_pathc = 0;
      paths.gl_pathv = NULL;
      paths.gl_offs = 0;

      retval = glob(*a, GLOB_NOCHECK, NULL, &paths);

      if (retval == 0) {
        int i;
        for (i = 0; i < paths.gl_pathc; i++) {
            *a = paths.gl_pathv[i];
            sh_execute(args);
        }
        globfree(&paths);
        return 1;
      }
    }
  }


  return sh_launch(args);
}

void sh_loop(void) {
  char *line;
  char **args;
  int status;

  int stdin_save = dup(0);
  int stdout_save = dup(1);

  do {
    printf("143A$ ");
    line = sh_read_line();

    add_to_history(line);

    if (line[0] == 'c' && line[1] == 'd' && line[2] == ' ') {
      line[strlen(line)-1] = 0;
      if (chdir(line+3) < 0)
          fprintf(stderr, "cannot cd %s\n", line+3);
      continue;
    }
    
    args = sh_split_line(line);

    if (strcmp(args[0], "history") == 0 && args[1] == NULL) {
      print_history();
      status = 1;
      continue;
    }

    status = sh_execute(args);

    free(line);
    free(args);

    dup2(stdin_save,0);
    dup2(stdout_save,1);
  } while (status);
}

int main(int argc, char **argv) {
  sh_loop();
  return EXIT_SUCCESS;
}
