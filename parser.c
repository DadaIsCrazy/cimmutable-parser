
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "parser.h"

typedef enum {
  STRUCT = 's'+'t',
  IMPLEM = 'i'+'m',
  TYPE   = 't'+'y',
  INIT   = 'i'+'n',
  BENCH  = 'b'+'e'
} section;

void die(char* str, ...) {
  va_list args;
  va_start(args, str);
  vprintf(str, args);
  va_end(args);
  exit(EXIT_FAILURE);
}

int get_section_size (FILE* fp, const char* section) {

  int position = ftell(fp);  
  char* line = NULL;
  size_t len  = 0;
  char flag = 0;
  int size = 0;
  
  while (getline(&line, &len, fp) != -1) {
    int i = 0;
    while (line[i] == ' ') i++;
    if (line[i] == '#' || line[i] == '\n') continue;
    if (line[i] == '[') {
      if (flag) break;
      flag = strstr(line, section) != NULL;
      continue;
    }
    
    size += flag;
  }

  fseek(fp, position, SEEK_SET);
  
  return size;
}


Prog* read_file(char* name) {

  
  FILE* fp = fopen(name, "r");
  if (fp == NULL) die("Unable to open file %s", name);

  char* line = NULL;
  unsigned int linum = 0;
  size_t len  = 0;
  int section = -1;
  int init_count  = 0;
  int bench_count = 0;
  int init_size = get_section_size(fp, "init");
  int bench_size = get_section_size(fp, "bench");
  char* delim = " ,=()";
  
  Prog* prog = malloc(sizeof *prog);
  prog->struc = 0;
  prog->data_type = prog->key_type = 0;
  prog->implem = 0;
  prog->init  = malloc(init_size * sizeof(*(prog->init)));
  prog->init_size  = init_size;
  prog->bench = malloc(bench_size * sizeof(*(prog->bench)));
  prog->bench_size = bench_size;
  

  while (getline(&line, &len, fp) != -1) {
    linum++;
    
    char* c = line;
    while (*c == ' ' || *c == '\t') c++; // skipping spaces.
    if (*c == '\n' || *c == '\0') continue; // skipping empty lines.
    if (*c == '#') continue; // skipping comments.

    if (*c == '[') {
      section = *(c+1) + *(c+2);
      continue;
    }
    
    switch (section) {
    case STRUCT:
      prog->struc = *c == 'v' ? VECTOR : MAP;
      break;
    case IMPLEM:
      prog->implem |= *c == 'A' ? AVL : *c == 'R' ? RRB : FINGER;
      break;
    case TYPE:
      if (prog->data_type == 0) prog->data_type = *c == 'i' ? INT : STRING;
      else prog->key_type = *c == 'i' ? INT : STRING;
      break;
    case INIT: case BENCH: ;
      command* cmd = malloc(sizeof *cmd);
      if (section == INIT) {
	prog->init[init_count++] = cmd;
      } else {
	prog->bench[bench_count++] = cmd;
      }
      cmd->is_assign = 0;
      cmd->index = 0;
      cmd->obj_in = cmd->obj_in_2 = cmd->obj_out =
	cmd->obj_out_2 = cmd->data.as_string = NULL;
      char* fun_name;
      if (strpbrk(c,"=") != NULL ) {
	cmd->is_assign = 1;
	cmd->obj_out = strdup(strtok(c, delim));
	fun_name = strtok(NULL, delim);
      } else {
	fun_name = strtok(c, delim);
      }

      if (strcmp(fun_name, "create") == 0) {
	cmd->type = CREATE;
      } else if (strcmp(fun_name, "update") == 0) {
	cmd->type = UPDATE;
	cmd->obj_in = strdup(strtok(NULL, delim));
	cmd->index = atoi(strtok(NULL, delim));
	if (prog->data_type == INT)
	  cmd->data.as_int = atoi(strtok(NULL, delim));
	else
	  cmd->data.as_string = strdup(strtok(NULL, delim));
      } else if (strcmp(fun_name, "merge") == 0) {
	cmd->type = MERGE;
	cmd->obj_in   = strdup(strtok(NULL, delim));
	cmd->obj_in_2 = strdup(strtok(NULL, delim));
      } else if (strcmp(fun_name, "push") == 0) { 
	cmd->obj_in = strdup(strtok(NULL, delim));
	cmd->type = PUSH;
	if (prog->data_type == INT)
	  cmd->data.as_int = atoi(strtok(NULL, delim));
	else
	  cmd->data.as_string = strdup(strtok(NULL, delim));
      } else if (strcmp(fun_name, "pop") == 0) {
	cmd->type = POP;
	cmd->obj_in = strdup(strtok(NULL, delim));
      } else if (strcmp(fun_name, "unref") == 0) {
	cmd->type = UNREF;
	cmd->obj_in = strdup(strtok(NULL, delim));
      } else if (strcmp(fun_name, "lookup") == 0) {
	cmd->type = LOOKUP;
	cmd->obj_in = strdup(strtok(NULL, delim));
	cmd->index = atoi(strtok(NULL, delim));
      } else if (strcmp(fun_name, "size") == 0) {
	cmd->type = SIZE;
	cmd->obj_in = strdup(strtok(NULL, delim));
      } else if (strcmp(fun_name, "dump") == 0) {
	cmd->type = DUMP;
	cmd->obj_in = strdup(strtok(NULL, delim));
      } else if (strcmp(fun_name, "split") == 0) {
	cmd->type = SPLIT;
	cmd->obj_in = strdup(strtok(NULL, delim));
	cmd->index  = atoi(strtok(NULL, delim));
	cmd->obj_out   = strdup(strtok(NULL, delim));
	cmd->obj_out_2 = strdup(strtok(NULL, delim));
      } else {
	die("Unknown operation: %s\n", line);
      }
      break;
    }
  }
  
  fclose(fp);

  return prog;
}

void debug_print_cmds (data_type type, command** cmds, int size);
void debug_print (Prog* prog) {
  fprintf(stderr, "struct... %s\n", prog->struc == VECTOR ? "vector" : "map");
  fprintf(stderr, "implem... ");
  if (prog->implem & AVL) fprintf(stderr, "AVL ");
  if (prog->implem & RRB) fprintf(stderr, "RRB ");
  if (prog->implem & FINGER) fprintf(stderr, "FINGER ");
  fprintf(stderr, "\n");
  fprintf(stderr, "type..... %s\n", prog->data_type == INT ? "int" : "string");
  fprintf(stderr, "init :\n");
  debug_print_cmds(prog->data_type, prog->init, prog->init_size);
  fprintf(stderr, "bench :\n");
  debug_print_cmds(prog->data_type, prog->bench, prog->bench_size);
}
void debug_print_cmds (data_type type, command** cmds, int size) {
  for (int i = 0; i < size; i++) {
    fprintf(stderr, "\t");
    command* cmd = cmds[i];
    if (cmd->is_assign) fprintf(stderr, "%s = ", cmd->obj_out);
    switch (cmd->type) {
    case CREATE: fprintf(stderr, "create()\n"); break;
    case UNREF:  fprintf(stderr, "unref(%s)\n", cmd->obj_in); break;
    case UPDATE:
      fprintf(stderr, "update(%s, %d, ", cmd->obj_in, cmd->index);
      if (type == INT) fprintf(stderr, "%d)\n",cmd->data.as_int);
      else fprintf(stderr, "%s)\n", cmd->data.as_string);
      break;
    case PUSH:
      fprintf(stderr, "push(%s, ", cmd->obj_in);
      if (type == INT) fprintf(stderr, "%d)\n",cmd->data.as_int);
      else fprintf(stderr, "%s)\n", cmd->data.as_string);
      break;
    case POP: fprintf(stderr, "pop(%s)\n", cmd->obj_in); break;
    case MERGE: fprintf(stderr, "merge(%s, %s)\n", cmd->obj_in, cmd->obj_in_2); break;
    case LOOKUP: fprintf(stderr, "lookup(%s, %d)\n", cmd->obj_in, cmd->index); break;
    case SIZE: fprintf(stderr, "size(%s)\n", cmd->obj_in); break;
    case DUMP: fprintf(stderr, "dump(%s)\n", cmd->obj_in); break;
    case SPLIT:
      fprintf(stderr, "split(%s, %d, %s, %s)\n",
	      cmd->obj_in, cmd->index, cmd->obj_out, cmd->obj_out_2);
      break;
			
    }
  }
}


int main () {

  Prog* prog = read_file("test.bench");
  debug_print(prog);
  
  return 0;
}
