
#define FUSE_USE_VERSION 30

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#define _FILE_OFFSET_BITS 64
#ifdef linux
/* For pread()/pwrite()/utimensat() */
#define _XOPEN_SOURCE 700
#endif

#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdarg.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#ifdef HAVE_SETXATTR
#include <sys/xattr.h>
#endif

#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

#define SIZEOF_BUFFER 500000
int DEBUG_LEVEL = 0;
void error(const char *msg)
{
    perror(msg);
    exit(1);
}

void DEBUG_LOG(const char * function_name, const char * str,...)
{
	if(DEBUG_LEVEL > 0) return;
	va_list args;
	va_start( args, str );
	int max_va_list_size = 4146;
	char* va_msg = (char*)malloc( strlen( str ) + max_va_list_size );
	strcpy(va_msg, function_name);
	int va_string_size = vsnprintf( &va_msg[strlen(function_name)], strlen( str ) + max_va_list_size, str, args );
	va_string_size = strlen(va_msg);
	va_msg[va_string_size]='\n';
	va_msg[va_string_size+1]='\0';
	printf("%s", va_msg);
}

/*Read a file give its path, size, */
int xmp_read(int newsockfd)
{
	//static int xmp_read(const char *path, char *buf, size_t size, off_t offset,
//		    struct fuse_file_info *fi)
	char buffer[SIZEOF_BUFFER], buf[SIZEOF_BUFFER];
	char size_string[] = "4096";// buffer
	size_t size_to_read_t;
	
	bzero(buffer,SIZEOF_BUFFER); //Init with \0
	bzero(buf,SIZEOF_BUFFER);
	bzero(size_string,SIZEOF_BUFFER);
	int n;
	off_t offset;
	char mod_path[100];

	n = read(newsockfd,buffer,SIZEOF_BUFFER); //Reads file absolute path
	//printf("Bytes read: %d\n",n);
	if (n < 0) 
		error("ERROR reading from socket");
	if(n == 0)
		DEBUG_LOG("read()", "Empty msg.");
	DEBUG_LOG("xmp_read()", "::%s::",buffer);
	DEBUG_LOG("xmp_read()", "Waiting for file size for %s",buffer);
	/*n = read(newsockfd,size_string,SIZEOF_BUFFER); //Reads file size needed
	if (n < 0) 
		error("ERROR reading from socket");
	if(n == 0)
		DEBUG_LOG("read()", "Empty msg.");*/
	DEBUG_LOG("xmp_read()", "Done reading for file size.");
	DEBUG_LOG(__FUNCTION__,"path: %s offset: %ld, %s\n", buffer, offset, size_string);
	int sizet = 4096;//atoi(size_string);
	DEBUG_LOG(__FUNCTION__,"String to int %d\n",sizet);
	size_to_read_t = (size_t)sizet;
	int fd;
	int res;

//	(void) fi;
	fd = open(buffer, O_RDONLY);
	if (fd == -1)
		return -errno;

	res = pread(fd, buf, size_to_read_t, offset);
	if (res == -1)
		res = -errno;
	DEBUG_LOG(__FUNCTION__, "REad content: %s", buf);
	n = write(newsockfd,buf,4096); //Return buf having stat
	if (n < 0) 
		error("ERROR sending from socket");
		
	DEBUG_LOG(__FUNCTION__, "Sent content: %s", buf);
	close(fd);
	return res;

}


/*When the command received is xmp_getattr 
This method will take file absolute path which is the parameter for fstat */
int get_attrs(int newsockfd) 
{
	char buffer[SIZEOF_BUFFER];// buffer
	bzero(buffer,SIZEOF_BUFFER); //Init with \0
	int n;
	//n = read(newsockfd,buffer,SIZEOF_BUFFER);
	n = read(newsockfd,buffer,SIZEOF_BUFFER); //Reads file absolute path
	//printf("Bytes read: %d\n",n);
	if (n < 0) 
		error("ERROR reading from socket");
	if(n == 0)
		DEBUG_LOG("get_attrs()", "Empty msg.");
	DEBUG_LOG("get_attrs()", "::%s::",buffer);
	struct stat buf;
	lstat(buffer, &buf);
	DEBUG_LOG("get_attrs()", "::%d, %d::", buf.st_dev, buf.st_nlink);
	DEBUG_LOG("get_attrs()", "Size of lstat obj::%d::", sizeof(buf));
	n = write(newsockfd,&buf,sizeof(buf)); //Return buf having stat
	if (n < 0) 
		error("ERROR sending from socket");
	return 1;
}

/*
*/
int read_dir(int newsockfd)
{
	char buffer[SIZEOF_BUFFER];// buffer
	bzero(buffer,SIZEOF_BUFFER); //Init with \0
	int n;
	//n = read(newsockfd,buffer,SIZEOF_BUFFER);
	n = read(newsockfd,buffer,SIZEOF_BUFFER); //Reads file absolute path
	//printf("Bytes read: %d\n",n);
	if (n < 0) 
		error("ERROR reading from socket");
	if(n == 0)
		DEBUG_LOG("get_attrs()", "Empty msg.");
	DEBUG_LOG("read_dir()", "::%s::",buffer); //File absolute path
	


	//Critical code	
	char *path = buffer; //Absolute path
	void *buf; //Return buffer
	fuse_fill_dir_t filler; //Output filler
	off_t offset; //Offset defaults to 0
	struct fuse_file_info *fi; //Fuse file info 
	enum fuse_readdir_flags flags; 
	DIR *dp;
	struct dirent *de;

	(void) offset;
	(void) fi;
	(void) flags;

	dp = opendir(path);
	if (dp == NULL)
		return -errno;

	while ((de = readdir(dp)) != NULL) {
		struct stat st;
		memset(&st, 0, sizeof(st));
		st.st_ino = de->d_ino;
		st.st_mode = de->d_type << 12;
		DEBUG_LOG(__FUNCTION__," de->name = %s , de->d_type = %d, de->d_ino = %d ",de->d_name, de->d_type, de->d_ino);
		if (filler(buf, de->d_name, &st, 0, 0))
			break;
	}

	closedir(dp);

}

int getCommand(int newsockfd)
{
	int sizeof_bufer, n;
	char buffer[SIZEOF_BUFFER];
	bzero(buffer,SIZEOF_BUFFER);
	n = read(newsockfd,buffer,SIZEOF_BUFFER); // Reads a command like getattr, read
	if (n < 0) 
		error("ERROR reading from socket");
	if(n == 0)
		return 1;
	DEBUG_LOG("getCommand() ","::%s::", buffer);	//Loggig the command received
	if(strcmp(buffer, "exit") == 0) //Exit if the command is exit
		return -1;
	else if(strcmp(buffer, "xmp_getattr") == 0) //This is the case when xmp_getattr is called
	{
		get_attrs(newsockfd);
	}
	else if(strcmp(buffer, "xmp_readdir") == 0)
	{
		read_dir(newsockfd);
	}
	else if(strcmp(buffer, "xmp_read") == 0)
	{
		xmp_read(newsockfd);
	}
	else
	{
		DEBUG_LOG(__FUNCTION__, ".");
	}
//	else if(
	return 1;
}

int main(int argc, char *argv[])
{
DEBUG_LOG(__FUNCTION__, "ENTRY POINT");
     int sockfd, newsockfd, portno;
     socklen_t clilen;
     char buffer[256];
struct stat buf;
const char *path = "/home/jeffry/server/file1";
     struct sockaddr_in serv_addr, cli_addr;
     int n;
     if (argc < 2) {
         fprintf(stderr,"ERROR, no port provided\n");
         exit(1);
     }
     sockfd = socket(AF_INET, SOCK_STREAM, 0);
     if (sockfd < 0) 
        error("ERROR opening socket");
     bzero((char *) &serv_addr, sizeof(serv_addr));
     portno = atoi(argv[1]);
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY;
     serv_addr.sin_port = htons(portno);
     if (bind(sockfd, (struct sockaddr *) &serv_addr,
              sizeof(serv_addr)) < 0) 
              error("ERROR on binding");
     listen(sockfd,5);
     clilen = sizeof(cli_addr);
     newsockfd = accept(sockfd, 
                 (struct sockaddr *) &cli_addr, 
                 &clilen);
     if (newsockfd < 0) 
          error("ERROR on accept");
	printf("Client connected\n");
	int ret = 0;
	while(1)
	{
		ret = getCommand(newsockfd);
		if(ret == -1) break;
	}	
	DEBUG_LOG("main", "Exiting..");
//     n = write(newsockfd,&buf,sizeof(buf));
//     if (n < 0) error("ERROR writing to socket");
     close(newsockfd);
     close(sockfd);
     return 0; 
}
