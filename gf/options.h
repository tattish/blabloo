#ifndef __OPTIONS_H
#define __OPTIONS_H

#include <fuse/fuse_opt.h>
#include <cstddef>

struct gridfs_options {
  const char* host;
  const char* db;
  const char* mongoAuth;
  const char* mongoUser;
  const char* mongoPassword;
};

extern gridfs_options gridfs_options;

#define GRIDFS_OPT_KEY(t, p, v) { t, offsetof(struct gridfs_options, p), v }

enum {
  KEY_VERSION,
  KEY_HELP
};

extern struct fuse_opt gridfs_opts[];

int gridfs_opt_proc(void* data, const char* arg, int key,
          struct fuse_args* outargs);

void print_help();
void print_exception(char* e);
#endif
