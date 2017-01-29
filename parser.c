
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

// memory leaks function.
#define remove_quotes(str)			\
  if (str[0] == '"') {				\
    str++;					\
    str[strlen(str)-1] = '\0';			\
  }

int get_section_size (FILE* fp, const char* section) {

  int position = ftell(fp);  
  char* line = NULL;
  size_t len  = 0;
  char flag = 0;
  int size = 0;
  
  while (getline(&line, &len, fp) != -1) {
    int i = 0;
    while (line[i] == ' ' || line[i] == '\t') i++;
    if (line[i] == '#' || line[i] == '\n') continue;
    if (strcmp(&line[i],"__END__\n") == 0) break;  
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

  /* Conversion of the variables names. */
  FILE* fp;
  char output[20];
  int nb_var;
  char* cmd = malloc((151+strlen(name))*sizeof(char));
  sprintf(cmd, "perl -pi -e 's{ (\\w+) (\\s*=) }{ ($c{$1}//=$i++).$2 }gex; s{ ([a-z]\\w*) (\\s*[,)]) }{ ($c{$1}//=$i++).$2 }gex; END { print 0+(sort values %%c)[-1] }' %s", name);
  fp = popen(cmd, "r");
  
  if (fp == NULL) die("Couldn't convert the names.");
  if (fgets(output, sizeof(output)-1, fp) != NULL) {
    nb_var = atoi(output)+1;
  } else {
    die("Convertion didn't return anything");
  }
  pclose(fp);

  
  fp = fopen(name, "r");
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
  prog->nb_var = nb_var;
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
    if (strcmp(c,"__END__\n") == 0) break; // skipping end block.

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
      cmd->obj_in = cmd->obj_out = cmd->obj_aux = -1;
      cmd->data.as_string = NULL;
      char* fun_name;
      if (strpbrk(c,"=") != NULL ) {
	cmd->is_assign = 1;
	cmd->obj_out = atoi(strtok(c, delim));
	fun_name = strtok(NULL, delim);
      } else {
	fun_name = strtok(c, delim);
      }

      if (strcmp(fun_name, "create") == 0) {
	cmd->type = CREATE;
      } else if (strcmp(fun_name, "update") == 0) {
	cmd->type = UPDATE;
	cmd->obj_in = atoi(strtok(NULL, delim));
	if (prog->struc == VECTOR) {
	  cmd->index = atoi(strtok(NULL, delim));
	} else {
	  if (prog->key_type == INT)
	    cmd->key.as_int = atoi(strtok(NULL, delim));
	  else {
	    cmd->key.as_string = strdup(strtok(NULL, delim));
	    remove_quotes(cmd->key.as_string);
	  }
	}
	if (prog->data_type == INT)
	  cmd->data.as_int = atoi(strtok(NULL, delim));
	else {
	  cmd->data.as_string = strdup(strtok(NULL, delim));
	  remove_quotes(cmd->data.as_string);
	}
      } else if (strcmp(fun_name, "merge") == 0) {
	cmd->type = MERGE;
	cmd->obj_in   = atoi(strtok(NULL, delim));
	cmd->obj_aux = atoi(strtok(NULL, delim));
      } else if (strcmp(fun_name, "push") == 0) { 
	cmd->obj_in = atoi(strtok(NULL, delim));
	cmd->type = PUSH;
	if (prog->data_type == INT)
	  cmd->data.as_int = atoi(strtok(NULL, delim));
	else {
	  cmd->data.as_string = strdup(strtok(NULL, delim));
	  remove_quotes(cmd->data.as_string);
	}
      } else if (strcmp(fun_name, "pop") == 0) {
	cmd->type = POP;
	cmd->obj_in = atoi(strtok(NULL, delim));
      } else if (strcmp(fun_name, "remove") == 0) {
	cmd->type = REMOVE;
	cmd->obj_in = atoi(strtok(NULL, delim));
	if (prog->key_type == INT)
	  cmd->key.as_int = atoi(strtok(NULL, delim));
	else {
	  cmd->key.as_string = strdup(strtok(NULL, delim));
	  remove_quotes(cmd->key.as_string);
	}
      } else if (strcmp(fun_name, "unref") == 0) {
	cmd->type = UNREF;
	cmd->obj_in = atoi(strtok(NULL, delim));
      } else if (strcmp(fun_name, "lookup") == 0) {
	cmd->type = LOOKUP;
	cmd->obj_in = atoi(strtok(NULL, delim));
	cmd->index = atoi(strtok(NULL, delim));
      } else if (strcmp(fun_name, "size") == 0) {
	cmd->type = SIZE;
	cmd->obj_in = atoi(strtok(NULL, delim));
      } else if (strcmp(fun_name, "dump") == 0) {
	cmd->type = DUMP;
	cmd->obj_in = atoi(strtok(NULL, delim));
      } else if (strcmp(fun_name, "split") == 0) {
	cmd->type = SPLIT;
	cmd->obj_in = atoi(strtok(NULL, delim));
	cmd->index  = atoi(strtok(NULL, delim));
	cmd->obj_out   = atoi(strtok(NULL, delim));
	cmd->obj_aux = atoi(strtok(NULL, delim));
      } else {
	die("Unknown operation: %s\n", line);
      }
      cmd->is_mutable = cmd->obj_in == cmd->obj_out;
      break;
    }
  }
  
  fclose(fp);

  return prog;
}

void debug_print_cmds (Prog* prog, command** cmds, int size);
void debug_print (Prog* prog) {
  fprintf(stderr, "nb var... %d\n", prog->nb_var);
  fprintf(stderr, "struct... %s\n", prog->struc == VECTOR ? "vector" : "map");
  fprintf(stderr, "implem... ");
  if (prog->implem & AVL) fprintf(stderr, "AVL ");
  if (prog->implem & RRB) fprintf(stderr, "RRB ");
  if (prog->implem & FINGER) fprintf(stderr, "FINGER ");
  fprintf(stderr, "\n");
  fprintf(stderr, "type..... %s\n", prog->data_type == INT ? "int" : "string");
  fprintf(stderr, "init :\n");
  debug_print_cmds(prog, prog->init, prog->init_size);
  fprintf(stderr, "bench :\n");
  debug_print_cmds(prog, prog->bench, prog->bench_size);
}
void debug_print_cmds (Prog* prog, command** cmds, int size) {
  for (int i = 0; i < size; i++) {
    fprintf(stderr, "\t");
    command* cmd = cmds[i];
    if (cmd->is_assign) fprintf(stderr, "%d = ", cmd->obj_out);
    switch (cmd->type) {
    case CREATE: fprintf(stderr, "create()\n"); break;
    case UNREF:  fprintf(stderr, "unref(%d)\n", cmd->obj_in); break;
    case UPDATE:
      fprintf(stderr, "update(%d, ", cmd->obj_in);
      if (prog->struc == VECTOR) fprintf(stderr, "%d, ", cmd->index);
      else
	if (prog->key_type == INT) fprintf(stderr, "%d, ", cmd->key.as_int);
	else fprintf(stderr, "%s, ", cmd->key.as_string);
      if (prog->data_type == INT) fprintf(stderr, "%d)\n",cmd->data.as_int);
      else fprintf(stderr, "%s)\n", cmd->data.as_string);
      break;
    case PUSH:
      fprintf(stderr, "push(%d, ", cmd->obj_in);
      if (prog->data_type == INT) fprintf(stderr, "%d)\n",cmd->data.as_int);
      else fprintf(stderr, "%s)\n", cmd->data.as_string);
      break;
    case POP: fprintf(stderr, "pop(%d)\n", cmd->obj_in); break;
    case REMOVE:
      if (prog->key_type == INT) fprintf(stderr, "remove(%d)\n", cmd->key.as_int);
      else fprintf(stderr, "remove(%s)\n", cmd->key.as_string);
    case MERGE: fprintf(stderr, "merge(%d, %d)\n", cmd->obj_in, cmd->obj_aux); break;
    case LOOKUP: fprintf(stderr, "lookup(%d, %d)\n", cmd->obj_in, cmd->index); break;
    case SIZE: fprintf(stderr, "size(%d)\n", cmd->obj_in); break;
    case DUMP: fprintf(stderr, "dump(%d)\n", cmd->obj_in); break;
    case SPLIT:
      fprintf(stderr, "split(%d, %d, %d, %d)\n",
	      cmd->obj_in, cmd->index, cmd->obj_out, cmd->obj_aux);
      break;
			
    }
  }
}

