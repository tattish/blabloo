
#include "operations.h"
#include "options.h"
#include "utils.h"
#include "local_gridfile.h"
#include <algorithm>
#include <string>
#include <cerrno>
#include <fcntl.h>
#include <stdio.h>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/tee.hpp>

#include <fstream>
#include <iostream>
#include <sstream>


#include <mongo/client/gridfs.h>
#include <mongo/client/connpool.h>

#ifdef __linux__
#include <sys/xattr.h>
#endif

using namespace std;
using namespace mongo;

std::map<string, LocalGridFile*> open_files;

unsigned int FH = 1;

std::ofstream file( "gridLog.out" );
//TODO: Currently only mocked: to impl. Real attrs on gridfs metadata field. use print_stat helper
int gridfs_getattr(const char *path, struct stat *stbuf)
{
  file << "___________________________________________________________________________________" << endl;
  file << "### getattr : \n\t path:[" << path << "] \n\t stat_:["<< print_stat(stbuf)<< "]\n" << endl;
  int res = 0;
  memset(stbuf, 0, sizeof(struct stat));

  if(strcmp(path, "/") == 0) {
    stbuf->st_mode = S_IFDIR | 0777;
    stbuf->st_nlink = 2;
    return 0;
  }

  // find the right gridfile
  path = fuse_to_mongo_path(path);

  map<string,LocalGridFile*>::const_iterator file_iter;
  file_iter = open_files.find(path);

  if(file_iter != open_files.end()) {
    stbuf->st_mode = S_IFREG | 0555;
    stbuf->st_nlink = 1;
    stbuf->st_ctime = time(NULL);
    stbuf->st_mtime = time(NULL);
    stbuf->st_size = file_iter->second->getLength();
    return 0;
  }

  // HACK: This is a protective measure to ensure we don't spin off into forever if a path without a period is requested.
  if(path_depth(path) > 10) {
    return -ENOENT;
  }

  // HACK: Assumes that if the last part of the path has a '.' in it, it's the leaf of the path, and if we haven't found a match by now,
  // give up and go home. This works just dandy as long as you avoid putting periods in your 'directory' names.
  if(!is_leaf(path)) {
    stbuf->st_mode = S_IFDIR | 0777;
    stbuf->st_nlink = 2;
    return 0;
  }

  ScopedDbConnection sdc(gridfs_options.host);
  GridFS gf(sdc.conn(), gridfs_options.db);
  GridFile file = gf.findFile(path);
  sdc.done();

  if(!file.exists()) {
    return -ENOENT;
  }

  stbuf->st_mode = S_IFREG | 0555;
  stbuf->st_nlink = 1;
  stbuf->st_size = file.getContentLength();

  time_t upload_time = mongo_time_to_unix_time(file.getUploadDate());
  stbuf->st_ctime = upload_time;
  stbuf->st_mtime = upload_time;

  return 0;
}
//TODO:
int gridfs_readlink(const char *path, char *buf, size_t size){
	file << "___________________________________________________________________________________" << endl;
	file << "### \nReadlink :\n\t path[" << path <<"],\n\t size["<< size <<"]\n" << endl;
	return -ENOTSUP;
}
//TODO
int gridfs_mknod(const char *path, mode_t mod, dev_t dev){
	file << "___________________________________________________________________________________" << endl;
	file << "\n ### mknod :\n\t path[" << path <<"], \n\t mod["<< mod <<"],\n\t dev["<< dev <<"]\n" << endl;
	return -ENOTSUP;
}
//TODO: Implement directory creation here:
int gridfs_mkdir(const char *path, mode_t mod){
	file << "___________________________________________________________________________________" << endl;
	file << "\n### mkdir : \n\t path[" << path <<"],\n\t mod["<< mod <<"]\n" << endl;
	return -ENOTSUP;
}

//TODO : Revisit if it is the right operation
int gridfs_unlink(const char *path) {
	file << "___________________________________________________________________________________" << endl;
	file << "\n### unlink : \n\t path[" << path <<"]\n" << endl;
  path = fuse_to_mongo_path(path);

  ScopedDbConnection sdc(gridfs_options.host);
  GridFS gf(sdc.conn(), gridfs_options.db);
  gf.removeFile(path);
  sdc.done();

  return 0;
}
//TODO : Implement director remove
int gridfs_rmdir(const char *path){
	file << "___________________________________________________________________________________" << endl;
	file << "\n### rmdir  :\n\t path[" << path <<"]\n" << endl;
	return -ENOTSUP;
}
//TODO
int gridfs_symlink(const char *paths, const char *pathl){
	file << "\nTry to run symlink"<< endl;
	return -ENOTSUP;
}

int gridfs_rename(const char* old_path, const char* new_path)
{
  file << "\nTry to run rename with params :path[" << old_path <<"],newpath[" << new_path <<"]\n" << endl;
  old_path = fuse_to_mongo_path(old_path);
  new_path = fuse_to_mongo_path(new_path);

  ScopedDbConnection sdc(gridfs_options.host);
  DBClientBase &client = sdc.conn();

  string db_name = gridfs_options.db;

  BSONObj file_obj = client.findOne(db_name + ".fs.files",
                    BSON("filename" << old_path));

  if(file_obj.isEmpty()) {
    return -ENOENT;
  }

  BSONObjBuilder b;
  set<string> field_names;
  file_obj.getFieldNames(field_names);

  for(set<string>::iterator name = field_names.begin();
    name != field_names.end(); name++)
  {
    if(*name != "filename") {
      b.append(file_obj.getField(*name));
    }
  }

  b << "filename" << new_path;

  client.update(db_name + ".fs.files",
          BSON("_id" << file_obj.getField("_id")), b.obj());

  sdc.done();

  return 0;
}
//TODO
int gridfs_link(const char * path, const char* path_link){
	file << "\nLink" << endl;
	return -ENOTSUP;
}
//TODO
int gridfs_chmod(const char* path, mode_t mod){
	file << "\nChmod" << endl;
	return -ENOTSUP;
}
//TODO
int gridfs_chown(const char* path, uid_t uid, gid_t gid){
	file << "\nChown" << endl;
	return -ENOTSUP;
}
//TODO
int gridfs_truncate(const char* path, off_t offset){
	file << "\n truncate" << endl;
	return -ENOTSUP;
}

int gridfs_open(const char *path, struct fuse_file_info *fi)
{
	file << "\nOpen" << endl;
  path = fuse_to_mongo_path(path);

  if((fi->flags & O_ACCMODE) == O_RDONLY) {
    if(open_files.find(path) != open_files.end()) {
      return 0;
    }

    ScopedDbConnection sdc(gridfs_options.host);
    GridFS gf(sdc.conn(), gridfs_options.db);
    GridFile file = gf.findFile(path);
    sdc.done();

    if(file.exists()) {
      return 0;
    }

    return -ENOENT;
  } else {
    return -EACCES;
  }
}

int gridfs_read(const char *path, char *buf, size_t size, off_t offset,
        struct fuse_file_info *fi)
{ file << "\n read :  path:["<< path << "] size:[" <<
		size << "] offset:[" << offset << "] fileinfo:[" <<
		print_ffi(fi) << "]\n" << endl;
  path = fuse_to_mongo_path(path);
  size_t len = 0;

  map<string,LocalGridFile*>::const_iterator file_iter;
  file_iter = open_files.find(path);
  if(file_iter != open_files.end()) {
    LocalGridFile *lgf = file_iter->second;
    return lgf->read(buf, size, offset);
  }

  ScopedDbConnection sdc(gridfs_options.host);
  GridFS gf(sdc.conn(), gridfs_options.db);
  GridFile file = gf.findFile(path);

  if(!file.exists()) {
    sdc.done();
    return -EBADF;
  }

  int chunk_size = file.getChunkSize();
  int chunk_num = offset / chunk_size;

  while(len < size && chunk_num < file.getNumChunks()) {
    GridFSChunk chunk = file.getChunk(chunk_num);
    int to_read;
    int cl = chunk.len();

    const char *d = chunk.data(cl);

    if(len) {
      to_read = min((long unsigned)cl, (long unsigned)(size - len));
      memcpy(buf + len, d, to_read);
    } else {
      to_read = min((long unsigned)(cl - (offset % chunk_size)), (long unsigned)(size - len));
      memcpy(buf + len, d + (offset % chunk_size), to_read);
    }

    len += to_read;
    chunk_num++;
  }

  sdc.done();
  return len;
}

int gridfs_write(const char* path, const char* buf, size_t nbyte,
         off_t offset, struct fuse_file_info* ffi)
{ file << "\n write" << endl;
  path = fuse_to_mongo_path(path);

  if(open_files.find(path) == open_files.end()) {
    return -ENOENT;
  }

  LocalGridFile *lgf = open_files[path];

  return lgf->write(buf, nbyte, offset);
}

int gridfs_statfs(const char *path, struct statvfs *statvfs){
	//TODO
	file << "\nstatFS" << endl;
	return -ENOTSUP;
}

int gridfs_flush(const char* path, struct fuse_file_info *ffi)
{
	file << "___________________________________________________________________________________" << endl;
	file << "\n flush : path:["<< path << "] fileinfo:["<< print_ffi(ffi) << "]\n" << endl;
  path = fuse_to_mongo_path(path);

  if(!ffi->fh) {
    return 0;
  }

  map<string,LocalGridFile*>::iterator file_iter;
  file_iter = open_files.find(path);
  if(file_iter == open_files.end()) {
    return -ENOENT;
  }

  LocalGridFile *lgf = file_iter->second;

  if(!lgf->dirty()) {
    return 0;
  }

  ScopedDbConnection sdc(gridfs_options.host);
  DBClientBase &client = sdc.conn();
  GridFS gf(sdc.conn(), gridfs_options.db);

  if(gf.findFile(path).exists()) {
    gf.removeFile(path);
  }

  size_t len = lgf->getLength();
  char *buf = new char[len];
  lgf->read(buf, len, 0);

  gf.storeFile(buf, len, path);

  sdc.done();

  lgf->flushed();

  return 0;
}

int gridfs_release(const char* path, struct fuse_file_info* ffi)
{
  file << "___________________________________________________________________________________" << endl;
  file << "\n release : path:["<< path << "] fileinfo:["<< print_ffi(ffi) << "]\n"  << endl;
  path = fuse_to_mongo_path(path);

  if(!ffi->fh) {
    // fh is not set if file is opened read only
    // Would check ffi->flags for O_RDONLY instead but MacFuse doesn't
    // seem to properly pass flags into release
    return 0;
  }

  delete open_files[path];
  open_files.erase(path);

  return 0;
}

int gridfs_fsync(const char* path, int datasync, struct fuse_file_info* ffi){
	//TODO
	file << "\n fsync" << endl;
	return -ENOTSUP;
}

int gridfs_setxattr(const char* path, const char* name, const char* value,
          size_t size, int flags)
{ file << "\n setxattr" << endl;
  return -ENOTSUP;
}

int gridfs_getxattr(const char* path, const char* name, char* value, size_t size)
{ file << "\n getxattr :path["<< path <<"] name:[" << name << "] value:["<< value << "] size:[" << size << "]\n" << endl;
  if(strcmp(path, "/") == 0) {
    return -ENOATTR;
  }

  path = fuse_to_mongo_path(path);
  const char* attr_name = unnamespace_xattr(name);
  if(!attr_name) {
    return -ENOATTR;
  }

  if(open_files.find(path) != open_files.end()) {
    return -ENOATTR;
  }

  ScopedDbConnection sdc(gridfs_options.host);
  GridFS gf(sdc.conn(), gridfs_options.db);
  GridFile file = gf.findFile(path);
  sdc.done();

  if(!file.exists()) {
    return -ENOENT;
  }

  BSONObj metadata = file.getMetadata();
  if(metadata.isEmpty()) {
    return -ENOATTR;
  }

  BSONElement field = metadata[attr_name];
  if(field.eoo()) {
    return -ENOATTR;
  }

  string field_str = field.toString();
  int len = field_str.size() + 1;
  if(size == 0) {
    return len;
  } else if(size < len) {
    return -ERANGE;
  }

  memcpy(value, field_str.c_str(), len);

  return len;
}

int gridfs_listxattr(const char* path, char* list, size_t size)
{ file << "\n listxattr" << endl;
  path = fuse_to_mongo_path(path);

  if(open_files.find(path) != open_files.end()) {
    return 0;
  }

  ScopedDbConnection sdc(gridfs_options.host);
  GridFS gf(sdc.conn(), gridfs_options.db);
  GridFile file = gf.findFile(path);
  sdc.done();

  if(!file.exists()) {
    return -ENOENT;
  }

  int len = 0;
  BSONObj metadata = file.getMetadata();
  set<string> field_set;
  metadata.getFieldNames(field_set);
  for(set<string>::const_iterator s = field_set.begin(); s != field_set.end(); s++) {
    string attr_name = namespace_xattr(*s);
    int field_len = attr_name.size() + 1;
    len += field_len;
    if(size >= len) {
      memcpy(list, attr_name.c_str(), field_len);
      list += field_len;
    }
  }

  if(size == 0) {
    return len;
  } else if(size < len) {
    return -ERANGE;
  }

  return len;
}

int gridfs_removexattr(const char* path, const char* name){
	//TODO
	file << "\n removexattr" << endl;
	return -ENOTSUP;
}

int gridfs_opendir(const char* path, struct fuse_file_info* ffi){
	//TODO
	file << "\n opendir : path:["<< path << "] fileinfo:["<< print_ffi(ffi) << "]\n" << endl;
	return -ENOTSUP;
}

int gridfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
           off_t offset, struct fuse_file_info *fi)
{ file << "\n readdir" << endl;
  if(strcmp(path, "/") != 0)
    return -ENOENT;

  filler(buf, ".", NULL, 0);
  filler(buf, "..", NULL, 0);

  ScopedDbConnection sdc(gridfs_options.host);
  GridFS gf(sdc.conn(), gridfs_options.db);

  auto_ptr<DBClientCursor> cursor = gf.list();
  while(cursor->more()) {
    BSONObj f = cursor->next();
    filler(buf, f.getStringField("filename") , NULL , 0);
  }
  sdc.done();

  for(map<string,LocalGridFile*>::const_iterator i = open_files.begin();
    i != open_files.end(); i++)
  {
    filler(buf, i->first.c_str(), NULL, 0);
  }

  return 0;
}

int gridfs_releasedir(const char *path, struct fuse_file_info *ffi){
	//TODO
	file << "\n releasedir" << endl;
	return -ENOTSUP;
}

int gridfs_fsyncdir(const char *path, int mode, struct fuse_file_info *ffi){
	//TODO
	file << "\n fsyncdir" << endl;
	return -ENOTSUP;
}

//Filesystem operations:

void* gridfs_init(struct fuse_conn_info *conn){
	//TODO

	file << "\n init conn:[" << print_conninfo(conn) << "]" << endl;
}

void gridfs_destroy(void *){
	//TODO
	file << "\n destroy" << endl;
}

int gridfs_access(const char *path, int mode){
	//TODO
	file << "___________________________________________________________________________________" << endl;
	file << "\n access : path:[" << path << "] mode:["<< mode << "]\n" << endl;
	file << "\n\t\t" << decimal_to_binary(mode) << "< BIN type :mode: DEC type >" << mode;
	return -ENOTSUP;
}

//:TODO update right
int gridfs_create(const char* path, mode_t mode, struct fuse_file_info* ffi)
{

  file << "\n ##### Create: PATH:["<< path << "]" << print_mode(mode) << print_ffi(ffi) << endl;
  path = fuse_to_mongo_path(path);

  open_files[path] = new LocalGridFile(DEFAULT_CHUNK_SIZE);

  ffi->fh = FH++;

  return 0;
}

int gridfs_ftruncate(const char *path, off_t offset, struct fuse_file_info *ffi){
	//TODO
	file << "\n ftruncate" << endl;
	return -ENOTSUP;
}

int gridfs_fgetattr(const char *path, struct stat *stat, struct fuse_file_info *ffi){
	//TODO
	file << "___________________________________________________________________________________" << endl;
	file << "\n fgetattr:path["<< path << "] fileinfo:["<< print_ffi(ffi) << "] stat:[" << print_stat(stat) << "]\n" << endl;
	return -ENOTSUP;
}


//TODO:
int gridfs_lock(const char *path, struct fuse_file_info *ffi, int cmd,
		     struct flock *filelock){
	//TODO
	file << "___________________________________________________________________________________" << endl;
	file << "\n lock :path["<< path << "] fileinfo:["<< print_ffi(ffi) << "] cmd:["<< cmd << "] filelock:["<< print_flock(filelock) << "]" << endl;
	return -ENOTSUP;
}

int gridfs_utimens(const char *path, const struct timespec tv[2]){
	//TODO
	file << "\n utimens" << endl;
	return -ENOTSUP;
}

int gridfs_bmap(const char *path, size_t blocksize, uint64_t *idx){
	//TODO
	file << "\n bmap" << endl;
	return -ENOTSUP;
}

string print_ffi(struct fuse_file_info *ffi){
	std::ostringstream oss;
	oss << "\n flags:" <<             	ffi->flags;
	//flags :
	oss << print_ffiflags(ffi->flags);
	oss << "\n fh_old:" <<             	ffi->fh_old;
	oss << "\n writepage:" <<             ffi->writepage;
	oss << "\n direct_io:" <<             ffi->direct_io;
	oss << "\n keep_cache:" <<            ffi->keep_cache;
	oss << "\n flush:" <<             	ffi->flush;
	oss << "\n nonseekable:" <<           ffi->nonseekable;
	oss << "\n fh:" <<             		ffi->fh;
	oss << "\n padding:" <<            	ffi->padding;
	return oss.str();
}

string print_conninfo(struct fuse_conn_info *conn){
	std::ostringstream oss;
	string buffer;
	oss << " \n proto_major: " <<           conn->proto_major;
	oss << " \n proto_minor: " <<           conn->proto_minor;
	oss << " \n async_read: " <<            conn->async_read;
	oss << " \n max_write: " <<             conn->max_write;
	oss << " \n max_readahead: " <<         conn->max_readahead;
//	 * Capability bits for 'fuse_conn_info.capable' and 'fuse_conn_info.want'
//	 *
//	 * FUSE_CAP_ASYNC_READ: filesystem supports asynchronous read requests
//	 * FUSE_CAP_POSIX_LOCKS: filesystem supports "remote" locking
//	 * FUSE_CAP_ATOMIC_O_TRUNC: filesystem handles the O_TRUNC open flag
//	 * FUSE_CAP_EXPORT_SUPPORT: filesystem handles lookups of "." and ".."
//	 * FUSE_CAP_BIG_WRITES: filesystem can handle write size larger than 4kB
//	 * FUSE_CAP_DONT_MASK: don't apply umask to file mode on create operations
	oss << " \n capable: " <<             	conn->capable;
	oss << "Binary: " << decimal_to_binary(conn->capable);
	oss << "\n \t\ŧ FUSE_CAP_ASYNC_READ: filesystem supports asynchronous read requests :";
		if(conn->capable & FUSE_CAP_ASYNC_READ) oss << "Y"; else oss << "N";
	oss << "\n \t\ŧ FUSE_CAP_POSIX_LOCKS: filesystem supports \"remote\" locking :";
		if(conn->capable & FUSE_CAP_POSIX_LOCKS) oss << "Y"; else oss << "N";
	oss << "\n \t\ŧ FUSE_CAP_ATOMIC_O_TRUNC: filesystem handles the O_TRUNC open flag :";
		if(conn->capable & FUSE_CAP_ATOMIC_O_TRUNC) oss << "Y"; else oss << "N";
	oss << "\n \t\ŧ FUSE_CAP_EXPORT_SUPPORT: filesystem handles lookups of . and .. :";
		if(conn->capable & FUSE_CAP_EXPORT_SUPPORT) oss << "Y"; else oss << "N";
	oss << "\n \t\ŧ FUSE_CAP_BIG_WRITES: filesystem can handle write size larger than 4kB :";
		if(conn->capable & FUSE_CAP_BIG_WRITES) oss << "Y"; else oss << "N";
	oss << "\n \t\ŧ FUSE_CAP_DONT_MASK: don't apply umask to file mode on create operations :";
		if(conn->capable & FUSE_CAP_DONT_MASK) oss << "Y"; else oss << "N";
	oss << " \n want: " <<             		conn->want;
	return oss.str();
}

string print_stat(struct stat *stat){
	std::ostringstream oss;
	oss << "\n device: " <<              					stat->st_dev;
	oss << "\n inode: " <<              					stat->st_ino;
	oss << "\n protection: " <<              				stat->st_mode;
	oss << print_mode(stat->st_mode);
	oss << "\n number of hard links: " <<              		stat->st_nlink;
	oss << "\n  user ID of owner: " <<              		stat->st_uid;
	oss << "\n group ID of owner: " <<              		stat->st_uid;
	oss << "\n device type (if inode device): " <<          stat->st_rdev;
	oss << "\n  total size, in bytes: " <<              	stat->st_size;
	oss << "\n blocksize for filesystem I/O: " <<           stat->st_blksize;
	oss << "\n number of blocks allocated: " <<             stat->st_blocks;
	oss << "\n time of last access: " <<              		ctime(&stat->st_atime);
	oss << "\n time of last modification: " <<          	ctime(&stat->st_mtime);
	oss << "\n time of last change: " <<              		ctime(&stat->st_ctime);
	 return oss.str();
}

string print_mode(mode_t mode){

	//	S_ISUID	04000	Set user ID on execution
	//	S_ISGID	02000	Set group ID on execution
	//	S_ISVTX	01000	Sticky bit
	//	S_IRUSR, S_IREAD	00400	Read by owner
	//	S_IWUSR, S_IWRITE	00200	Write by owner
	//	S_IXUSR, S_IEXEC	00100	Execute/search by owner
	//	S_IRGRP	00040	Read by group
	//	S_IWGRP	00020	Write by group
	//	S_IXGRP	00010	Execute/search by group
	//	S_IROTH	00004	Read by others
	//	S_IWOTH	00002	Write by others
	//	S_IXOTH	00001	Execute/search by others
	std::ostringstream oss;
	oss << "\n \t MODE in bin:" << decimal_to_binary(mode);
	switch (mode & S_IFMT) {
		    case S_IFBLK:  oss << "\n \t\t block device\n";            break;
		    case S_IFCHR:  oss << "\n \t\t character device\n";        break;
		    case S_IFDIR:  oss << "\n \t\t directory\n";               break;
		    case S_IFIFO:  oss << "\n \t\t FIFO/pipe\n";               break;
		    case S_IFLNK:  oss << "\n \t\t symlink\n";                 break;
		    case S_IFREG:  oss << "\n \t\t regular file\n";            break;
		    case S_IFSOCK: oss << "\n \t\t socket\n";                  break;
		    default:       oss << "\n \t\t unknown?\n";                break;
		    }
		oss << "\n \t Permissions ";
		oss << "\n \t\ŧ Run with UID :";
		if(mode & S_ISUID) oss << "Y"; else oss << "N";
		oss << "\n \t\ŧ Run with GUID :";
		if(mode & S_ISGID) oss << "Y"; else oss << "N";
		oss << "\n \t\ŧ Sticky bit :";
		if(mode & S_ISVTX) oss << "Y"; else oss << "N";
		oss << "\n \t\ŧ psm :";
		if(mode & S_IREAD) oss << "r"; else oss << "-";
		if(mode & S_IWRITE) oss << "w"; else oss << "-";
		if(mode & S_IEXEC) oss << "x"; else oss << "-";

		if(mode & S_IRGRP) oss << "r"; else oss << "-";
		if(mode & S_IWGRP) oss << "w"; else oss << "-";
		if(mode & S_IXGRP) oss << "x"; else oss << "-";

		if(mode & S_IROTH) oss << "r"; else oss << "-";
		if(mode & S_IWOTH) oss << "w"; else oss << "-";
		if(mode & S_IXOTH) oss << "x"; else oss << "-";
	return oss.str();
}

string print_ffiflags(int flags){
	std::ostringstream oss;
//	 The  parameter  flags must include one of the following access modes: O_RDONLY, O_WRONLY, or O_RDWR.
//	 These request opening the file read-only, write-only,  or read/write, respectively.
//	 In addition, zero or more file creation flags and file status flags can be bit-wise-or'd in flags.
//	 The file creation flags are O_CREAT, O_EXCL, O_NOCTTY, and O_TRUNC. The file status flags are
//	 all of the remaining flags listed below. The distinction between these two groups of flags is
//	 that the file status flags can be retrieved and (in some cases) modified using fcntl(2).  The
//	 full list of file creation flags and file status flags is as follows:

// http://sourceforge.net/apps/mediawiki/fuse/index.php?title=Fuse_file_info
//#define O_ACCMODE	   0003
//#define O_RDONLY	     00
//#define O_WRONLY	     01
//#define O_RDWR		     02
//#define O_CREAT		   0100	/* not fcntl */
//#define O_EXCL		   0200	/* not fcntl */
//#define O_NOCTTY	   0400	/* not fcntl */
//#define O_TRUNC		  01000	/* not fcntl */
//#define O_APPEND	  02000
//#define O_NONBLOCK	  04000
//#define O_NDELAY	O_NONBLOCK
//#define O_SYNC	       04010000
//#define O_FSYNC		 O_SYNC
//#define O_ASYNC		 020000
//
//#ifdef __USE_XOPEN2K8
//# define O_DIRECTORY	0200000	/* Must be a directory.	 */
//# define O_NOFOLLOW	0400000	/* Do not follow links.	 */
//# define O_CLOEXEC     02000000 /* Set close_on_exec.  */
//#endif
//#ifdef __USE_GNU
//# define O_DIRECT	 040000	/* Direct disk access.	*/
//# define O_NOATIME     01000000 /* Do not set atime.  */
//# define O_PATH	      010000000 /* Resolve pathname but do not open file.  */
//#endif
//#if defined __USE_POSIX199309 || defined __USE_UNIX98
//# define O_DSYNC	010000	/* Synchronize data.  */
//# define O_RSYNC	O_SYNC	/* Synchronize read operations.	 */
//#endif
//
//#ifdef __USE_LARGEFILE64
//# if __WORDSIZE == 64
//#  define O_LARGEFILE	0
//# else
//#  define O_LARGEFILE	0100000
//# endif
//#endif
	oss << "\n \t\t" << decimal_to_binary(flags) << " : FLAGS";
	oss << "\n \t The file is opened in ACCESS MODE::::";

	switch (flags & O_ACCMODE) {
		    case O_RDONLY:  oss << "\t RDONLY";            break;
		    case O_WRONLY:  oss << "\t WRONLY";        break;
		    case O_RDWR:  oss << "\t RDWR";               break;
		    default:       oss << "\t UNKNOWN";                break;
		    }
	//The file is opened in append mode.
	oss << "\n \t\t"<< decimal_to_binary(O_APPEND) <<" O_APPEND:";
		if(flags & O_APPEND) oss << "Y"; else oss << "N";
	//Enable signal-driven I/O
	oss << "\n \t\t"<< decimal_to_binary(O_ASYNC) <<" O_ASYNC:";
		if(flags & O_ASYNC) oss << "Y"; else oss << "N";
	//If the file does not exist it will be created
	oss << "\n \t\t"<< decimal_to_binary(O_CREAT) <<" O_CREAT:";
		if(flags & O_CREAT) oss << "Y"; else oss << "N";
	//Try to minimize cache effects of the I/O to and from this file
	oss << "\n \t\t"<< decimal_to_binary(O_DIRECT) <<" O_DIRECT:";
		if(flags & O_DIRECT) oss << "Y"; else oss << "N";
	//If pathname is not a directory, cause the open to fail
	oss << "\n \t\t"<< decimal_to_binary(O_DIRECTORY) <<" O_DIRECTORY:";
		if(flags & O_DIRECTORY) oss << "Y"; else oss << "N";
	//When used with O_CREAT, if the file already exists it is an error and the open() will fail:
	oss << "\n \t\t"<< decimal_to_binary(O_EXCL) <<" O_EXCL:";
		if(flags & O_EXCL) oss << "Y"; else oss << "N";
	//(LFS) Allow files whose sizes cannot be repr in an off_t (but can in an off64_t) to be opened.
	oss << "\n \t\t"<< decimal_to_binary(O_LARGEFILE) <<" O_LARGEFILE:";
		if(flags & O_LARGEFILE) oss << "Y"; else oss << "N";
	//(Since Linux 2.6.8) Do not update the file last access time (st_atime in the inode) when the file is read(2)
	oss << "\n \t\t"<< decimal_to_binary(O_NOATIME) <<" O_NOATIME:";
		if(flags & O_NOATIME) oss << "Y"; else oss << "N";
	//If pathname refers to a terminal device it will not \n \t\t
	//become the process's controlling terminal even if the process does not have one.
	oss << "\n \t\t"<< decimal_to_binary(O_NOCTTY) <<" O_NOCTTY:";
		if(flags & O_NOCTTY) oss << "Y"; else oss << "N";
	//If pathname is a symbolic link, then the open fails
	oss << "\n \t\t"<< decimal_to_binary(O_NOFOLLOW) <<" O_NOFOLLOW:";
		if(flags & O_NOFOLLOW) oss << "Y"; else oss << "N";
	//When possible or O_NDELAY, the file is opened in non-blocking mode
	oss << "\n \t\t"<< decimal_to_binary(O_NONBLOCK) <<" O_NONBLOCK:";
		if(flags & O_NONBLOCK) oss << "Y"; else oss << "N";
	//The file is opened for synchronous I/O
	oss << "\n \t\t"<< decimal_to_binary(O_SYNC) <<" O_SYNC:";
		if(flags & O_SYNC) oss << "Y"; else oss << "N";
	//if regular and open mode allows writing (i.e., is O_RDWR or O_WRONLY) it will be truncated to length 0
	oss << "\n \t\t"<< decimal_to_binary(O_TRUNC) <<" O_TRUNC:";
		if(flags & O_TRUNC) oss << "Y"; else oss << "N";


	oss << "\n \t\t"<< decimal_to_binary(O_CLOEXEC) <<" O_CLOEXEC:";
		if(flags & O_CLOEXEC) oss << "Y"; else oss << "N";
	oss << "\n \t\t"<< decimal_to_binary(O_PATH) <<" O_PATH:";
		if(flags & O_PATH) oss << "Y"; else oss << "N";
	oss << "\n \t\t"<< decimal_to_binary(O_DSYNC) <<" O_DSYNC:";
		if(flags & O_DSYNC) oss << "Y"; else oss << "N";

return oss.str();
}

string print_flock(flock *filelock){
//	/* For F_[GET|SET]FD.  */
//	#define FD_CLOEXEC	1	/* actually anything with low bit set goes */
//
//	/* For posix fcntl() and `l_type' field of a `struct flock' for lockf().  */
//	#define F_RDLCK		0	/* Read lock.  */
//	#define F_WRLCK		1	/* Write lock.	*/
//	#define F_UNLCK		2	/* Remove lock.	 */
//
//	/* For old implementation of bsd flock().  */
//	#define F_EXLCK		4	/* or 3 */
//	#define F_SHLCK		8	/* or 4 */
//
//	#ifdef __USE_BSD
//	/* Operations for bsd flock(), also used by the kernel implementation.	*/
//	# define LOCK_SH	1	/* shared lock */
//	# define LOCK_EX	2	/* exclusive lock */
//	# define LOCK_NB	4	/* or'd with one of the above to prevent
//					   blocking */
//	# define LOCK_UN	8	/* remove lock */
//	#endif
//
//	#ifdef __USE_GNU
//	# define LOCK_MAND	32	/* This is a mandatory flock:	*/
//	# define LOCK_READ	64	/* ... which allows concurrent read operations.	 */
//	# define LOCK_WRITE	128	/* ... which allows concurrent write operations.  */
//	# define LOCK_RW	192	/* ... Which allows concurrent read & write operations.	 */
//	#endif
	std::ostringstream oss;
	oss << "\n\t File LOCK: \n\t\t" << decimal_to_binary(filelock->l_type) << "< BIN type :: DEC type >" << filelock->l_type;
	oss << "\n\t\t File whence:" <<  filelock->l_whence;
	oss << "\n\t\t start:" <<  filelock->l_start;
	oss << "\n\t\t length:" <<  filelock->l_len;
	oss << "\n\t\t Lock owner PID:" <<  filelock->l_pid;
//	struct flock
//	  {
//	    short int l_type;	/* Type of lock: F_RDLCK, F_WRLCK, or F_UNLCK.	*/
//	    short int l_whence;	/* Where `l_start' is relative to (like `lseek').  */
//	#ifndef __USE_FILE_OFFSET64
//	    __off_t l_start;	/* Offset where the lock begins.  */
//	    __off_t l_len;	/* Size of the locked area; zero means until EOF.  */
//	#else
//	    __off64_t l_start;	/* Offset where the lock begins.  */
//	    __off64_t l_len;	/* Size of the locked area; zero means until EOF.  */
//	#endif
//	    __pid_t l_pid;	/* Process holding the lock.  */
//	  };
	return oss.str();
}

string print_fh(file_handle *fh){
	std::ostringstream oss;
	oss << "\n\t File HANDLE: \n\t\t" << decimal_to_binary(fh->handle_bytes) << "< BIN type :handle_bytes: DEC type >" << fh->handle_bytes;
	oss << "\n\t\t" << decimal_to_binary(fh->handle_type) << "< BIN type :handle_type: DEC type >" << fh->handle_type;
	oss << "\n\t\t length:" <<  fh->f_handle;
//	struct file_handle
//	{
//	  unsigned int handle_bytes;
//	  int handle_type;
//	  /* File identifier.  */
//	  unsigned char f_handle[0];
//	};
	return oss.str();
}


char *decimal_to_binary(int n)
{
   int c, d, count;
   char *pointer;

   count = 0;
   pointer = (char*)malloc(32+1);

   if ( pointer == NULL )
      exit(EXIT_FAILURE);

   for ( c = 31 ; c >= 0 ; c-- )
   {
      d = n >> c;

      if ( d & 1 )
         *(pointer+count) = 1 + '0';
      else
         *(pointer+count) = 0 + '0';

      count++;
   }
   *(pointer+count) = '\0';

   return  pointer;
}
