#include "operations.h"
#include "options.h"
#include "utils.h"
#include <cstring>

using namespace std;

int main(int argc, char *argv[])
{
  static struct fuse_operations gridfs_oper;

  //For the particular operations see the docs in the operations.h/.cpp
  //File and directory operations same order as in fuse.h

  gridfs_oper.getattr = gridfs_getattr;
  gridfs_oper.readlink = gridfs_readlink;
  gridfs_oper.mknod = gridfs_mknod;
  gridfs_oper.mkdir = gridfs_mkdir;
  gridfs_oper.unlink = gridfs_unlink;
  gridfs_oper.rmdir = gridfs_rmdir;
  gridfs_oper.symlink = gridfs_symlink;
  gridfs_oper.rename = gridfs_rename;
  gridfs_oper.link = gridfs_link;
  gridfs_oper.chmod = gridfs_chmod;
  gridfs_oper.chown = gridfs_chown;
  gridfs_oper.truncate = gridfs_truncate;
  gridfs_oper.open = gridfs_open;
  gridfs_oper.read = gridfs_read;
  gridfs_oper.write = gridfs_write;
  gridfs_oper.statfs = gridfs_statfs;
  gridfs_oper.flush = gridfs_flush;
  gridfs_oper.release = gridfs_release;
  gridfs_oper.fsync = gridfs_fsync;
  gridfs_oper.setxattr = gridfs_setxattr;
  gridfs_oper.getxattr = gridfs_getxattr;
  gridfs_oper.listxattr = gridfs_listxattr;
  gridfs_oper.removexattr = gridfs_removexattr;
  gridfs_oper.opendir = gridfs_opendir;
  gridfs_oper.readdir = gridfs_readdir;
  gridfs_oper.releasedir = gridfs_releasedir;
  gridfs_oper.fsyncdir = gridfs_fsyncdir;

  //Filesystem operations
  gridfs_oper.init = gridfs_init;
  gridfs_oper.destroy = gridfs_destroy;
  gridfs_oper.access = gridfs_access;
  gridfs_oper.create = gridfs_create;
  gridfs_oper.ftruncate = gridfs_ftruncate;
  gridfs_oper.fgetattr = gridfs_fgetattr;


  //TODO: think it over if we need lock or not.
  //Done, we need it for concurrent access.
  //probably we will need to implement a locking mechanism for this
  gridfs_oper.lock = gridfs_lock;

  gridfs_oper.utimens = gridfs_utimens;


  struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

  memset(&gridfs_options, 0, sizeof(struct gridfs_options));
  if(fuse_opt_parse(&args, &gridfs_options, gridfs_opts,
            gridfs_opt_proc) == -1)
  {
    return -1;
  }

  if(!gridfs_options.host) {
    gridfs_options.host = "localhost";
  }
  if(!gridfs_options.db) {
    gridfs_options.db = "test";
  }
  if(gridfs_options.mongoAuth) {
	  try{
		  if(!gridfs_options.mongoUser) {
			  throw 'If auth is activ the username is mendatory!';
		  }
		  if(!gridfs_options.mongoPassword) {
			  throw 'If auth is activ the password is mendatory!';
		  }
	  }catch(char* e){
		  print_exception(e);
		  return -1;
	  }
  }

  return fuse_main(args.argc, args.argv, &gridfs_oper, NULL);
}
