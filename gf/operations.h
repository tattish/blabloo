
using namespace std;
#ifndef __OPERATIONS_H
#define __OPERATIONS_H

#define _FILE_OFFSET_BITS 64
#define FUSE_USE_VERSION 26

// ENOATTR is not blessed by POSIX. Darwin uses 93.
#ifndef ENOATTR
#define ENOATTR 93
#endif
#include <fuse/fuse.h>
#include <string>

int gridfs_getattr(const char *path, struct stat *stbuf);

int gridfs_readlink(const char *path, char *buf, size_t size);

int gridfs_mknod (const char *path, mode_t mod, dev_t dev);

int gridfs_mkdir (const char *path, mode_t mod);

/////////////
int gridfs_unlink(const char* path);

int gridfs_rmdir(const char* path);

int gridfs_symlink(const char* paths, const char* path_link);

/////////////
int gridfs_rename(const char* old_path, const char* new_path);

int gridfs_link(const char * path, const char* path_link);

int gridfs_chmod(const char* path, mode_t mod);

int gridfs_chown(const char* path, uid_t uid, gid_t gid);

int gridfs_truncate(const char* path, off_t offset);

/////////////
int gridfs_open(const char *path, struct fuse_file_info *fi);

/////////////
int gridfs_read(const char *path, char *buf, size_t size, off_t offset,
        struct fuse_file_info *fi);

/////////////
int gridfs_write(const char* path, const char* buf, size_t nbyte,
         off_t offset, struct fuse_file_info* ffi);

int gridfs_statfs(const char *path, struct statvfs *statvfs);

/////////////
int gridfs_flush(const char* path, struct fuse_file_info* ffi);

/////////////
int gridfs_release(const char* path, struct fuse_file_info* ffi);

int gridfs_fsync(const char* path, int datasync, struct fuse_file_info* ffi);

////////////
int gridfs_setxattr(const char* path, const char* name, const char* value,
          size_t size, int flags);
////////////
int gridfs_getxattr(const char* path, const char* name, char* value, size_t size);
////////////
int gridfs_listxattr(const char* path, char* list, size_t size);

int gridfs_removexattr(const char* path, const char* name);

int gridfs_opendir(const char* path, struct fuse_file_info* ffi);
////////////
int gridfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
           off_t offset, struct fuse_file_info *fi);

int gridfs_releasedir(const char *path, struct fuse_file_info *ffi);

int gridfs_fsyncdir(const char *path, int mode, struct fuse_file_info *ffi);

//Filesystem operations:

void* gridfs_init(struct fuse_conn_info *conn);

void gridfs_destroy(void *);

int gridfs_access(const char *path, int mode);

////////////
int gridfs_create(const char *path, mode_t mode, struct fuse_file_info *ffi);

int gridfs_ftruncate(const char *path, off_t offset, struct fuse_file_info *ffi);

int gridfs_fgetattr(const char *path, struct stat *stat, struct fuse_file_info *ffi);


//TODO: think it over if we need lock or not.
//Done, we need it for concurrent access.
//probably we will need to implement a locking mechanism for this
int gridfs_lock(const char *path, struct fuse_file_info *ffi, int cmd,
		     struct flock *filelock);

int gridfs_utimens(const char *path, const struct timespec tv[2]);

int gridfs_bmap(const char *path, size_t blocksize, uint64_t *idx);

//printer helpers
std::string print_ffi(struct fuse_file_info *ffi);
std::string print_conninfo(struct fuse_conn_info *conn);
std::string print_stat(struct stat *stat);
std::string print_mode(mode_t mode);
std::string print_ffiflags(int flags);
std::string print_flock(flock *filelock);
std::string print_fh(file_handle *fh);
char *decimal_to_binary(int n);

#endif
