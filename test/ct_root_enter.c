#include <unistd.h>
#include <libct.h>
#include <stdio.h>
#include <sys/mman.h>
#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>
#include "test.h"

#define FS_ROOT	"libct_test_root"
#define FS_DATA	"libct_test_string"
#define FS_FILE "file"

static int check_fs_enter_data(void *a)
{
	int fd;

	fd = open("/" FS_FILE, O_RDONLY);
	if (fd < 0)
		return 1;

	read(fd, a, sizeof(FS_DATA));
	close(fd);
	return 0;
}

static int check_fs_data(void *a)
{
	if (check_fs_enter_data(a))
		return 1;

	while (1)
		sleep(1);

	return 0;
}

int main(int argc, char **argv)
{
	int fd, pid;
	char *fs_data;
	libct_session_t s;
	ct_handler_t ct;

	mkdir(FS_ROOT);
	fd = open(FS_ROOT "/" FS_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0600);
	if (fd < 0)
		return err("Can't create file");

	write(fd, FS_DATA, sizeof(FS_DATA));
	close(fd);

	fs_data = mmap(NULL, 4096, PROT_READ | PROT_WRITE,
			MAP_SHARED | MAP_ANON, 0, 0);
	fs_data[0] = '\0';
	fs_data[1024] = '\0';

	libct_init();
	s = libct_session_open_local();
	ct = libct_container_create(s);
	libct_fs_set_root(ct, FS_ROOT);
	libct_container_spawn_cb(ct, check_fs_data, fs_data);
	pid = libct_container_enter(ct, check_fs_enter_data, fs_data + 1024);
	waitpid(pid, NULL, 0);
	libct_container_kill(ct);
	libct_container_join(ct);
	libct_container_destroy(ct);
	libct_session_close(s);
	libct_exit();

	unlink(FS_ROOT "/" FS_FILE);
	rmdir(FS_ROOT);

	if (strcmp(fs_data, FS_DATA))
		return fail("FS not accessed");

	if (strcmp(fs_data + 1024, FS_DATA))
		return fail("FS not entered");

	return pass("FS is created and entered");
}
