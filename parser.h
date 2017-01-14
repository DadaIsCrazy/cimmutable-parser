#pragma once

typedef struct _prog Prog;
typedef struct _command command;

Prog* read_file(char* name);
void debug_print (Prog* prog);

  
typedef enum {
  VECTOR = 1,
  MAP = 2
} struct_type;

typedef enum {
  AVL = 1,
  RRB = 2,
  FINGER = 4
} implem_type;

typedef enum {
  INT = 1,
  STRING = 2
} data_type;

typedef enum {
  CREATE, UNREF, UPDATE, PUSH, POP, LOOKUP, MERGE, SPLIT, SIZE, DUMP
} cmd_type;

/* note: some fields can be empty */
/* examples: 
  - obj_out = create()
  - obj_out = update(obj_in, index, data)
  - obj_out = push(obj_in, data)
  - obj_out = pop(obj_in)
  - obj_out = merge(obj_in, obj_in_2)
  - unref(obj_in)
  - lookup(obj_in, index)
  - size(obj_in)
  - dump(obj_in)
  - split(obj_in, index, obj_out, obj_out_2) */
struct _command {
  char is_assign;
  cmd_type type;
  char* obj_in;
  char* obj_in_2; /* needed for merge */
  char* obj_out;
  char* obj_out_2; /* needed for split */
  int index; 
  union { /* type should be deduced from prog->data_type */
    int as_int;
    char* as_string;
  } data;
};

struct _prog {
  struct_type struc;
  int implem;
  data_type data_type;
  data_type key_type;
  int init_size;
  command** init;
  int bench_size;
  command** bench;
};
