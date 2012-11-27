#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<netdb.h>
#include<sys/socket.h>
#include<sys/errno.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<glib.h>
#include<regex.h>
#include "queue.h"
#define PAGEMAX 500000
#define FILENAMEMAX 100
char page_buff[PAGEMAX]; 

char * generate_file_name(const char *hostname, char *filename, size_t size) {
	regex_t regex;
	regmatch_t regmatchs[1];
	int res;
	res = regcomp(&regex, "[\\?/:*|<>\"]", 0);
	if(res) {
		fprintf(stderr, "Could not complie regex\n");
	}
	strncpy(filename, hostname, size);
	while(!(res = regexec(&regex, filename, 1, regmatchs, 0))) {
		memset(filename, '_', regmatchs[0].rm_so);
	}
	regfree(&regex);
	return filename;
}

int parse_web_page(queue *queue_ptr, GHashTable *hashtable_ptr) {

}

int get_web_page(char *hostname, char *page_buff, size_t size) {
	int sockfd;
	char buffer[1024];
	char sendBuff[200];
	struct sockaddr_in server_addr;
	struct hostent *host;
	int portnumber = 80, nbytes;
	if((host = (struct hostent*)gethostbyname(hostname)) == NULL) {
		fprintf(stderr,"Gethostname error\n");
		return 1; /*get host name error*/
	}
	if((sockfd=socket(AF_INET, SOCK_STREAM, 0))==-1)
	{
		fprintf(stderr, "Socket Error:%s\a\n", strerror(errno));
		return 2; /*socket error */
	}
	bzero(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(portnumber);
	server_addr.sin_addr = *((struct in_addr *)host->h_addr_list[0]);
	if(connect(sockfd, (struct sockaddr *)(&server_addr), sizeof(struct sockaddr)) == -1) {
		fprintf(stderr, "Connection Error:%s\a\n", strerror(errno));
		return 3; //connection error
	}
	sprintf(sendBuff, "GET / HTTP/1.0\r\nHost: %s\r\nUser-Agent: Mozilla/4.0 (compatible; MSIE 10.0; Windows NT 6.1; Win64; x64; Trident/4.0)\r\n\r\n", hostname);
	if(send(sockfd, sendBuff, strlen(sendBuff), 0) < 0) {
		fprintf(stderr, "Send Error:%s\a\n", strerror(errno));
		return 4; //send error
	}
	int cursor = 0;
	while(1) {
		nbytes = recv(sockfd, buffer, 1024, 0);
		if(nbytes == -1) {
			fprintf(stderr,"Read Error: %s\n", strerror(errno));
			break;
		}
		if(nbytes == 0) {
			fprintf(stderr, "Read Info:%s\n", strerror(errno));
			break;
		}
		/*
		if(nbytes < 1024)
			buffer[nbytes] = '\0';
		*/
		if(cursor + nbytes > size) break;
		if(snprintf(page_buff+cursor, nbytes, buffer) == -1) {
			fprintf(stderr, "Snprintf Error:%s\n", strerror(errno));
			break;
		}
		printf("cursor:%d\n", cursor);
		cursor += nbytes;
	}
	close(sockfd);
	return cursor;
}

int main(int argc, char *argv[]) {
	int fd;
	char *seed;
	queue *queue_ptr;
	GHashTable *hashtable_ptr;
	if(argc!=2) {
		fprintf(stderr, "Usage: %s seed\a\n", argv[0]);
		exit(1);
	}
	seed = argv[1];
	queue_ptr = queue_init();
	queue_add_last(queue_ptr, seed);
       	hashtable_ptr = g_hash_table_new(g_str_hash, g_str_equal);
	while(queue_ptr->size > 0) {
		int length = queue_get_first_size(queue_ptr);
		char hostname[length + 1];
		queue_remove_first(queue_ptr, hostname, length);
		hostname[length] = '\0';
		get_web_page(hostname, page_buff, PAGEMAX - 1);
		char filename[FILENAMEMAX];
		generate_file_name(hostname, filename, FILENAMEMAX - 1);
		filename[length] = '\0';
		if((fd = open(filename, O_CREAT|O_EXCL|O_WRONLY, 0644)) < 0) {
			fprintf(stderr, "Open Error:%s\n", strerror(errno));
			break;
		}
		if(write(fd, page_buff, strlen(page_buff)) == -1) {
			fprintf(stderr, "Write Error:%s\n", strerror(errno));
			break;
		}
		parse_web_page(queue_ptr, hashtable_ptr);
	}
	close(fd);
	queue_destroy(queue_ptr);
	g_hash_table_destroy(hashtable_ptr);
	exit(0);
}
