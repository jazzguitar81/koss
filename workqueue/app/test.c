#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>

#define BUF_SIZE	10

const char *test_path = "/sys/kernel/debug/test_work/start";

int main(void)
{
	int ret;
	int fd;
	char buf[BUF_SIZE];

	fd = open(test_path, O_RDONLY);
	if (fd < 0) {
		printf("fail to open:%s\n", test_path);
		return 0;
	}

	ret = read(fd, buf, BUF_SIZE);
	if (ret < 0) {
		printf("fail to read:%s\n", test_path);
		return 0;
	}

	close(fd);

	return 0;
}
