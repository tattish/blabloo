#include "options.h"
#include <iostream>

using namespace std;

struct gridfs_options gridfs_options;

struct fuse_opt gridfs_opts[] =
{
  GRIDFS_OPT_KEY("--host=%s", host, 0),
  GRIDFS_OPT_KEY("--db=%s", db, 0),
  FUSE_OPT_KEY("-v", KEY_VERSION),
  FUSE_OPT_KEY("--version", KEY_VERSION),
  FUSE_OPT_KEY("-h", KEY_HELP),
  FUSE_OPT_KEY("--help", KEY_HELP),
  NULL
};

int gridfs_opt_proc(void* data, const char* arg, int key,
          struct fuse_args* outargs)
{
  if(key == KEY_HELP) {
    print_help();
    return -1;
  }

  if(key == KEY_VERSION) {
    cout << "gridfs-fuse version 0.5" << endl;
    return -1;
  }

  return 1;
}

void print_help()
{
  cout << "usage: ./mount_gridfs [options] mountpoint" << endl;
  cout << endl << "general options:" << endl;
  cout << "\t--db=[dbname]\t\twhich mongo database to use" << endl;
  cout << "\t--host=[hostname]\thostname of your mongodb server" << endl;
  cout << "\t-h, --help\t\tprint help" << endl;
  cout << "\t-v, --version\t\tprint version" << endl;
  cout << endl << "FUSE options: " << endl;
  cout << "\t-d, -o debug\t\tenable debug output (implies -f)" << endl;
  cout << "\t-f\t\t\tforeground operation" << endl;
  cout << "\t-s\t\t\tdisable multi-threaded operation" << endl;
}

void print_exception(char* e){
	cout << "An exception occurred. Exception : " << e << endl;
}
