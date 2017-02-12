/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>
  Copyright (C) 2011       Sebastian Pipping <sebastian@pipping.org>

  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.
*/

/** @file
 * @tableofcontents
 *
 * fusexmp.c - FUSE: Filesystem in Userspace
 *
 * \section section_compile compiling this example
 *
 * gcc -Wall fusexmp.c `pkg-config fuse3 --cflags --libs` -o fusexmp
//gcc -Wall fusexmp.c `pkg-config fuse3 --cflags --libs` `pkg-config libcurl --cflags --libs` -o fusexmp
 *
 * \section section_source the complete source
 * \include fusexmp.c
 */

#define LOG_FILE_PATH "/data/local/tmp/log"
#define FUSE_USE_VERSION 30

int clientSocket = 0;

//defines for socket programming
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef linux
/* For pread()/pwrite()/utimensat() */
#define _XOPEN_SOURCE 700
#endif

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#ifdef HAVE_SETXATTR
#include <sys/xattr.h>
#endif

#include <stdarg.h>

char ABS_DIR[100];

void log1(const char * function_name, char * str,...)
{
	va_list args;
	va_start( args, str );
	int max_va_list_size = 4146;
	char* va_msg = (char*)malloc( strlen( str ) + max_va_list_size );
	strcpy(va_msg, function_name);
	int va_string_size = vsnprintf( &va_msg[strlen(function_name)], strlen( str ) + max_va_list_size, str, args );
	va_string_size = strlen(va_msg);
	va_msg[va_string_size]='\n';
	va_msg[va_string_size+1]='\0';
	int log = open( LOG_FILE_PATH, O_CREAT | O_APPEND | O_RDWR, 0664 );
	printf("\n%s\n", function_name);
	write( log, va_msg, strlen( va_msg ) );
	close(log);
}

static int xmp_getattr(const char *path, struct stat *stbuf)
{
	log1(__FUNCTION__,"path: %s \n", path);
	char mod_path[100];
	sprintf(mod_path, "%s%s", ABS_DIR,path);
	log1(__FUNCTION__,"path: %s modpath: %s \n", path, mod_path);
	
	int res = 0;
	
	write(clientSocket, __FUNCTION__, strlen(__FUNCTION__) + 1);
	write(clientSocket, path, strlen(path) + 1);
	read(clientSocket, stbuf, sizeof(struct stat));
	
	//log1(__FUNCTION__," size of struct = %d", sizeof(stbuf));

	//log1(__FUNCTION__,"\nst_dev=%d\nst_nlink=%d\n", stbuf->st_dev, stbuf->st_nlink);
	
	//res = lstat(mod_path, stbuf);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_access(const char *path, int mask)
{
	log1(__FUNCTION__,"path: %s \n", path);
	char mod_path[100];
	sprintf(mod_path, "%s%s", ABS_DIR,path);
	log1(__FUNCTION__,"path: %s modpath: %s \n", path, mod_path);

	int res;
	/*
	res = access(mod_path, mask);
	if (res == -1)
		return -errno;
	*/
	return 0;
}

static int xmp_readlink(const char *path, char *buf, size_t size)
{
	log1(__FUNCTION__,"path: %s \n", path);
	char mod_path[100];
	sprintf(mod_path, "%s%s", ABS_DIR,path);
	log1(__FUNCTION__,"path: %s modpath: %s \n", path, mod_path);
	
	int res;

	res = readlink(mod_path, buf, size - 1);
	if (res == -1)
		return -errno;

	buf[res] = '\0';
	return 0;
}


static int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		       off_t offset, struct fuse_file_info *fi)
{
	log1(__FUNCTION__,"path: %s offset: %ld\n", path, offset);
	char mod_path[100];
	sprintf(mod_path, "%s%s", ABS_DIR,path);
	log1(__FUNCTION__,"path: %s modpath: %s offset: %ld\n", path, mod_path, offset);
	/*
	DIR *dp;
	struct dirent *de;

	(void) offset;
	(void) fi;
	//(void) flags;
	
	dp = opendir(mod_path);
	if (dp == NULL)
		return -errno;
	
	while ((de = readdir(dp)) != NULL) {
		struct stat st;
		memset(&st, 0, sizeof(st));
		st.st_ino = de->d_ino;
		st.st_mode = de->d_type << 12;
		if (filler(buf, de->d_name, &st, 0))
			break;
		log1(__FUNCTION__, " ino: %d",(int)st.st_ino);
	}

	closedir(dp);
	*/	

	write(clientSocket, __FUNCTION__, strlen(__FUNCTION__) + 1);
	write(clientSocket, path, strlen(path) + 1);
	read(clientSocket, buf, 5000);
	return 0;
}

static int xmp_mknod(const char *path, mode_t mode, dev_t rdev)
{
	int res;
	log1(__FUNCTION__,"path: %s \n", path);
	char mod_path[100];
	sprintf(mod_path, "%s%s", ABS_DIR,path);
	log1(__FUNCTION__,"path: %s modpath: %s \n", path, mod_path);

	/* On Linux this could just be 'mknod(path, mode, rdev)' but this
	   is more portable */
	if (S_ISREG(mode)) {
		res = open(path, O_CREAT | O_EXCL | O_WRONLY, mode);
		if (res >= 0)
			res = close(res);
	} else if (S_ISFIFO(mode))
		res = mkfifo(mod_path, mode);
	else
		res = mknod(mod_path, mode, rdev);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_mkdir(const char *path, mode_t mode)
{
	log1(__FUNCTION__,"path: %s \n", path);
	char mod_path[100];
	sprintf(mod_path, "%s%s", ABS_DIR,path);
	log1(__FUNCTION__,"path: %s modpath: %s \n", path, mod_path);

	int res;

	res = mkdir(mod_path, mode);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_unlink(const char *path)
{
	log1(__FUNCTION__,"path: %s \n", path);
	char mod_path[100];
	sprintf(mod_path, "%s%s", ABS_DIR,path);
	log1(__FUNCTION__,"path: %s modpath: %s \n", path, mod_path);
	int res;

	res = unlink(mod_path);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_rmdir(const char *path)
{
	log1(__FUNCTION__,"path: %s \n", path);
	char mod_path[100];
	sprintf(mod_path, "%s%s", ABS_DIR,path);
	log1(__FUNCTION__,"path: %s modpath: %s \n", path, mod_path);
	int res;

	res = rmdir(mod_path);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_symlink(const char *path, const char *to)
{
	log1(__FUNCTION__,"path: %s \n", path);
	char mod_path[100];
	sprintf(mod_path, "%s%s", ABS_DIR,path);
	log1(__FUNCTION__,"path: %s modpath: %s \n", path, mod_path);
	int res;

	res = symlink(mod_path, to);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_rename(const char *path, const char *to)
{
	log1(__FUNCTION__,"path: %s \n", path);
	char mod_path[100];
	sprintf(mod_path, "%s%s", ABS_DIR,path);
	log1(__FUNCTION__,"path: %s modpath: %s \n", path, mod_path);
	int res;

	/*if (flags)
		return -EINVAL;*/

	res = rename(mod_path, to);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_link(const char *path, const char *to)
{
	log1(__FUNCTION__,"path: %s \n", path);
	char mod_path[100];
	sprintf(mod_path, "%s%s", ABS_DIR,path);
	log1(__FUNCTION__,"path: %s modpath: %s \n", path, mod_path);
	int res;

	res = link(mod_path, to);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_chmod(const char *path, mode_t mode)
{
	log1(__FUNCTION__,"path: %s \n", path);
	char mod_path[100];
	sprintf(mod_path, "%s%s", ABS_DIR,path);
	log1(__FUNCTION__,"path: %s modpath: %s \n", path, mod_path);
	int res;

	res = chmod(mod_path, mode);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_chown(const char *path, uid_t uid, gid_t gid)
{
	log1(__FUNCTION__,"path: %s \n", path);
	char mod_path[100];
	sprintf(mod_path, "%s%s", ABS_DIR,path);
	log1(__FUNCTION__,"path: %s modpath: %s \n", path, mod_path);
	int res;

	res = lchown(mod_path, uid, gid);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_truncate(const char *path, off_t size)
{
	log1(__FUNCTION__,"path: %s \n", path);
	char mod_path[100];
	sprintf(mod_path, "%s%s", ABS_DIR,path);
	log1(__FUNCTION__,"path: %s modpath: %s \n", path, mod_path);
	int res;

	res = truncate(mod_path, size);
	if (res == -1)
		return -errno;

	return 0;
}

#ifdef HAVE_UTIMENSAT
static int xmp_utimens(const char *path, const struct timespec ts[2])
{
	log1(__FUNCTION__,"path: %s \n", path);
	char mod_path[100];
	sprintf(mod_path, "%s%s", ABS_DIR,path);
	log1(__FUNCTION__,"path: %s modpath: %s \n", path, mod_path);
	int res;

	/* don't use utime/utimes since they follow symlinks */
	res = utimensat(0, mod_path, ts, AT_SYMLINK_NOFOLLOW);
	if (res == -1)
		return -errno;

	return 0;
}
#endif

static int xmp_open(const char *path, struct fuse_file_info *fi)
{
	log1(__FUNCTION__,"path: %s\n", path);
	char mod_path[100];
	sprintf(mod_path, "%s%s", ABS_DIR,path);
	log1(__FUNCTION__,"path: %s modpath: %s\n", path, mod_path);
		
	int res;
			
	res = open(path, fi->flags);
	//if (res == -1)
	//	return -errno;

	close(res);
	return 0;
}

static int xmp_read(const char *path, char *buf, size_t size, off_t offset,
		    struct fuse_file_info *fi)
{
	log1(__FUNCTION__,"path: %s offset: %ld\n", path, offset);
	char mod_path[100];
	sprintf(mod_path, "%s%s", ABS_DIR,path);
	log1(__FUNCTION__,"path: %s modpath: %s size: %ld\n", path, mod_path, size);
	/*
	int fd;
	int res;

	(void) fi;
	fd = open(mod_path, O_RDONLY);
	if (fd == -1)
		return -errno;

	res = pread(fd, buf, size, offset);
	if (res == -1)
		res = -errno;
	log1(__FUNCTION__,"Data: %s, Size: %d\n", buf, size);
	close(fd);
	*/
	int res = 0;
	write(clientSocket, __FUNCTION__, strlen(__FUNCTION__) + 1);	
	write(clientSocket, path, strlen(path) + 1);
	//char str[50];
	//sprintf(str, "%d", (int)size);
	//sleep(1000);
	//log1(__FUNCTION__," before write of size");
	//write(clientSocket, str, strlen(str) + 1);
	//log1(__FUNCTION__, "After write of file size");
	read(clientSocket, buf, 2000000);
	return 4096;
}

static int xmp_write(const char *path, const char *buf, size_t size,
		     off_t offset, struct fuse_file_info *fi)
{
	log1(__FUNCTION__,"@XMP_WRITE path: %s \n", path);
	char mod_path[100];
	sprintf(mod_path, "%s%s", ABS_DIR,path);
	log1(__FUNCTION__,"path: %s modpath: %s \n", path, mod_path);

	//	TWEAK
	char super_mod_path[200];
	sprintf(super_mod_path, "%s%s", mod_path,".super");
	log1(__FUNCTION__," Super_mod_path: %s \n", super_mod_path);
	int super_fd = open(super_mod_path, O_WRONLY);
	log1(__FUNCTION__," Super_file_descriptor: %d \n", super_fd);
	int res1 = pwrite(super_fd, buf, size, offset);
	if(res1 == -1)
		log1(__FUNCTION__,"Super write failed\n");
	close(super_fd);		
//	TWEAK


	int fd;
	int res;

	(void) fi;
	fd = open(mod_path, O_WRONLY);
	log1(__FUNCTION__," File descriptor: %d \n", fd);
	if (fd == -1)
		return -errno;
	log1(__FUNCTION__," File content: %s \n", buf);
	res = pwrite(fd, buf, size, offset);
	
	if (res == -1)
		res = -errno;

	close(fd);
	return res;
}

static int xmp_statfs(const char *path, struct statvfs *stbuf)
{
	log1(__FUNCTION__,"path: %s \n", path);
	char mod_path[100];
	sprintf(mod_path, "%s%s", ABS_DIR,path);
	log1(__FUNCTION__,"path: %s modpath: %s \n", path, mod_path);
	int res;

	//res = statvfs(mod_path, stbuf);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_release(const char *path, struct fuse_file_info *fi)
{
	log1(__FUNCTION__,"path: %s \n", path);
	char mod_path[100];
	sprintf(mod_path, "%s%s", ABS_DIR,path);
	log1(__FUNCTION__,"path: %s modpath: %s \n", path, mod_path);
	/* Just a stub.	 This method is optional and can safely be left
	   unimplemented */

	(void) mod_path;
	(void) fi;
	return 0;
}

static int xmp_fsync(const char *path, int isdatasync,
		     struct fuse_file_info *fi)
{
	log1(__FUNCTION__,"path: %s \n", path);
	char mod_path[100];
	sprintf(mod_path, "%s%s", ABS_DIR,path);
	log1(__FUNCTION__,"path: %s modpath: %s \n", path, mod_path);
	/* Just a stub.	 This method is optional and can safely be left
	   unimplemented */

	(void) mod_path;
	(void) isdatasync;
	(void) fi;
	return 0;
}

#ifdef HAVE_POSIX_FALLOCATE
static int xmp_fallocate(const char *path, int mode,
			off_t offset, off_t length, struct fuse_file_info *fi)
{
	log1(__FUNCTION__,"path: %s \n", path);
	char mod_path[100];
	sprintf(mod_path, "%s%s", ABS_DIR,path);
	log1(__FUNCTION__,"path: %s modpath: %s \n", path, mod_path);
	int fd;
	int res;

	(void) fi;

	if (mode)
		return -EOPNOTSUPP;

	fd = open(mod_path, O_WRONLY);
	if (fd == -1)
		return -errno;

	res = -posix_fallocate(fd, offset, length);

	close(fd);
	return res;
}
#endif

#ifdef HAVE_SETXATTR
/* xattr operations are optional and can safely be left unimplemented */
static int xmp_setxattr(const char *path, const char *name, const char *value,
			size_t size, int flags)
{
	log1(__FUNCTION__,"path: %s \n", path);
	char mod_path[100];
	sprintf(mod_path, "%s%s", ABS_DIR,path);
	log1(__FUNCTION__,"path: %s modpath: %s \n", path, mod_path);

	int res = lsetxattr(mod_path, name, value, size, flags);
	if (res == -1)
		return -errno;
	return 0;
}

static int xmp_getxattr(const char *path, const char *name, char *value,
			size_t size)
{
	log1(__FUNCTION__,"path: %s offset: %ld\n", path, offset);
	char mod_path[100];
	sprintf(mod_path, "%s%s", ABS_DIR,path);
	log1(__FUNCTION__,"path: %s modpath: %s offset: %ld\n", path, mod_path, offset);

	int res = lgetxattr(mod_path, name, value, size);
	if (res == -1)
		return -errno;
	return res;
}

static int xmp_listxattr(const char *path, char *list, size_t size)
{
	log1(__FUNCTION__,"path: %s offset: %ld\n", path, offset);
	char mod_path[100];
	sprintf(mod_path, "%s%s", ABS_DIR,path);
	log1(__FUNCTION__,"path: %s modpath: %s offset: %ld\n", path, mod_path, offset);

	int res = llistxattr(mod_path, list, size);
	if (res == -1)
		return -errno;
	return res;
}

static int xmp_removexattr(const char *path, const char *name)
{
	log1(__FUNCTION__,"path: %s \n", path);
	char mod_path[100];
	sprintf(mod_path, "%s%s", ABS_DIR,path);
	log1(__FUNCTION__,"path: %s modpath: %s \n", path, mod_path);
	int res = lremovexattr(mod_path, name);
	if (res == -1)
		return -errno;
	return 0;
}
#endif /* HAVE_SETXATTR */

static struct fuse_operations xmp_oper = {
	.getattr	= xmp_getattr,
	.access		= xmp_access,
	.readlink	= xmp_readlink,
	.readdir	= xmp_readdir,
	.mknod		= xmp_mknod,
	.mkdir		= xmp_mkdir,
	.symlink	= xmp_symlink,
	.unlink		= xmp_unlink,
	.rmdir		= xmp_rmdir,
	.rename		= xmp_rename,
	.link		= xmp_link,
	.chmod		= xmp_chmod,
	.chown		= xmp_chown,
	.truncate	= xmp_truncate,
#ifdef HAVE_UTIMENSAT
	.utimens	= xmp_utimens,
#endif
	.open		= xmp_open,
	.read		= xmp_read,
	.write		= xmp_write,
	.statfs		= xmp_statfs,
	.release	= xmp_release,
	.fsync		= xmp_fsync,
#ifdef HAVE_POSIX_FALLOCATE
	.fallocate	= xmp_fallocate,
#endif
#ifdef HAVE_SETXATTR
	.setxattr	= xmp_setxattr,
	.getxattr	= xmp_getxattr,
	.listxattr	= xmp_listxattr,
	.removexattr	= xmp_removexattr,
#endif
};

/**********************************************Network functions**********************************************/
void error(const char *msg)
{
    perror(msg);
    exit(0);
}

void initializeSocket(char * serverHostName, int serverPortNo)
{
    int  n;
    struct sockaddr_in serv_addr;
    struct hostent *server;    
    printf("host name - %s \n, serverPortNo - %d\n", serverHostName, serverPortNo);
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0) 
        error("ERROR opening socket");
    server = gethostbyname(serverHostName);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(serverPortNo);
    if (connect(clientSocket,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
        error("ERROR connecting");
}


/***************************************************************************************************************/
int main(int argc, char *argv[])
{
	umask(0);
	printf("\nPrint works.");
	log1(__FUNCTION__,"Logging 1st statement.");
	printf("\nAfter 1st log");
	printf("Hostname is %s, port no is %d\n", argv[2],atoi(argv[3]));
	initializeSocket(argv[2],atoi(argv[3]));
	printf("Socket successfully initialized\n");
	char * fuseArgs[2];
	fuseArgs[0] = argv[0];
	fuseArgs[1] = argv[1];
	//strcpy(ABS_DIR,argv[2]);
	//strcpy(LOG_FILE_PATH,"/home/jeffry/Desktop/fuse-src/libfuse-master/example/tweak/log.txt");
	return fuse_main(argc-2, fuseArgs, &xmp_oper, NULL);	
}
