#define _GNU_SOURCE
#include <sys/uio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define BUF_LEN (16*1024*1024)

static char* needle;
static char* replace;


void write_mem(pid_t pid, void *ptr, void* buf, size_t size)
{
    ssize_t nwrite;

    struct iovec local[1];
    local[0].iov_base = buf;
    local[0].iov_len = size;

    struct iovec remote[1];
    remote[0].iov_base = ptr;
    remote[0].iov_len = size;

	nwrite = process_vm_writev(pid, local, 1, remote, 1, 0);
	if (nwrite < 0) {
		printf("pid %d write vm error %d\n", pid, errno);
		return;
	}
	printf("pid %d write %ld bytes at %p ok\n", pid, nwrite, ptr);
}

void read_mem(pid_t pid, void *ptr, size_t size)
{
    ssize_t nread;
    void* found;

    struct iovec local[1];
    local[0].iov_base = calloc(size, sizeof(char));
    local[0].iov_len = size;

    struct iovec remote[1];
    remote[0].iov_base = ptr;
    remote[0].iov_len = size;

	nread = process_vm_readv(pid, local, 1, remote, 1, 0);
	if (nread < 0) {
		printf("pid %d read vm error %d\n", pid, errno);
		free(local[0].iov_base);
		return;
	}
	printf("pid %d read %ld bytes\n", pid, nread);

	void* base = local[0].iov_base;
	void* last = base;

	found = memmem(last, nread, needle, strlen(needle));

	if (found) {
		size_t offset = found - base;
		printf("needle '%s' found at offset %ld\n", needle, offset);;
		write_mem(pid, ptr + offset, replace, strlen(replace));
		last = found + 1;
	} else {
		printf("needle '%s' not found\n", needle);
	}

	free(local[0].iov_base);
}

int main(int argc, char* argv[])
{
	if (argc < 4) {
		printf("usage: mem2 <pid> <needle> <replace>\n");
		return 1;
	}

    pid_t pid = atoi(argv[1]);
    ssize_t nread;
    char maps[250];
    FILE* fp;

   	void* alo;
   	void* ahi;
   	char perms[10];
   	size_t offset;
   	char dev[10];
   	int inode;
   	char path[260];

	needle = argv[2];
	replace = argv[3];

	if (strlen(needle) != strlen(replace)) {
		printf("error: '%s' length not equal to '%s' length\n", needle, replace);
		return 1;
	}

    sprintf(maps, "/proc/%d/maps", pid);
    fp = fopen(maps, "rt");
    if (fp == NULL) {
    	printf("%s not found\n", maps);
    	return 1;
    }

	// 564cc816d000-564cc816e000 r-xp 00000000 08:01 540914 /home/vv/knu/mem1
	// address           		 perms offset  dev   inode  pathname

    while (1)
    {
	    alo = ahi = NULL;
	    memset(path, 0, sizeof(path));
	    nread = fscanf(fp, "%p-%p %s %lx %s %d %s\n",
	    	&alo, &ahi, perms, &offset, dev, &inode, path);
	    if (nread < 7)
	    	break;
	    if (perms[1] != 'w')
	    	continue;

	    printf("%p-%p %s %lx %s %d %s\n",
	    	alo, ahi, perms, offset, dev, inode, path);

	    read_mem(pid, alo, ahi-alo);
    }
    fclose(fp);

	return 0;
}