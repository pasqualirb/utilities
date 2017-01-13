/*
 * Copyright (C) 2017  Ricardo Biehl Pasquali <rbpoficial@gmail.com>
 * under the terms of the GNU General Public License (see LICENSE file)
 *
 * that's the craziest thing you have ever seen: two functions, one gets the
 * line in a file based on a given offset, other gets first offset where pattern
 * is matched -- plus a demo function which gets cpuset mountpoint -- I would
 * use this for ipmic performance but I found too complex
 */

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <string.h>
#include <stdio.h>


#define BUF_SIZE 1024

/*
 * put line which offset is pointing into retstr
 * *we assume fd is a valid and opened file descriptor
 */
static int
get_line_of(char *retstr, size_t retstr_size, off_t offset, int fd)
{
	off_t save_offset = lseek(fd, 0, SEEK_CUR);

	off_t line_begin = 0;
	off_t line_end = 0;
	int backward; /* used to get line_begin value */
	char buf[BUF_SIZE];
	int len;
	int i;

	int ret = 0;

	/*
	 * What we do:
	 *
	 * ..........\n...........................\n..............\0
	 *             ^            ^             ^
	 *        line begin      offset       line end
	 *
	 * 1st  read up to BUF_SIZE bytes from offset until find a \n (newline)
	 *      or reach end of file -- this is line_end offset
	 * 2nd  read up to BUF_SIZE bytes in opposite direction from offset
	 *      until at most (offset - BUF_SIZE) -- this is line_begin offset
	 * 3rd  put the content between line_begin and line_end in retstr
	 */

	/* get line end offset */
	lseek(fd, offset, SEEK_SET);
	if ( (len = read(fd, (void*) buf, BUF_SIZE)) == -1 ) {
		ret = -1;
		goto _go_end;
	}
	for (i = 0; i < len; i++) {
		if (buf[i] == '\n' || buf[i] == EOF) {
			line_end = offset + i;
			break;
		}
	}
	if (! line_end) {
		ret = -1;
		goto _go_end;
	}

	/* get line begin offset */
	backward = (offset < BUF_SIZE ? offset : BUF_SIZE);
	lseek(fd, offset - backward, SEEK_SET);
	if ( (len = read(fd, (void*) buf, backward)) == -1 ) {
		ret = -1;
		goto _go_end;
	}
	for (i = backward - 1; i >= 0; i--) {
		if (buf[i] == '\n') {
			line_begin = (offset - backward) + i + 1;
			break;
		}
	}

	if (line_end - line_begin >= retstr_size) {
		ret = -1;
		goto _go_end;
	}

	/* get the line itself */
	lseek(fd, line_begin, SEEK_SET);
	read(fd, retstr, line_end - line_begin);
	retstr[line_end - line_begin] = '\0';

_go_end:
	/* restore offset */
	lseek(fd, save_offset, SEEK_SET);

	return ret;
}

/*
 * get offset of pattern in file
 * *we assume fd is a valid and opened file descriptor
 */
static int
get_offset_of(off_t *retoff, char *pattern, int fd)
{
	off_t save_offset = lseek(fd, 0, SEEK_CUR);

	int pattern_idx = 0;
	char buf[BUF_SIZE];
	int len;
	int i;

	int ret = 0;


	/* set file offset to zero before begin scanning */
	*retoff = lseek(fd, 0, SEEK_SET);
	if (*retoff == -1)
		return -1;

	while (1) {
		len = read(fd, (void*) buf, BUF_SIZE);
		if (len == -1 || len == 0) {
			/*
			 * -1 or 0 means file can't be read anymore -- and if we
			 * hasn't found pattern until here, we should end :-)
			 */
			ret = -1;
			goto _go_end;
		}

		for (i = 0; i < len; i++) {
			if (pattern[pattern_idx] == buf[i]) {
				/* check if we are ready */
				if (pattern[pattern_idx + 1] == '\0') {
					/*
					 * look at the end of 'while (1)' loop:
					 *   *retoff += len
					 * Each read() we don't find the pattern
					 * we increment *retoff -- and now to
					 * get final result we need only to do
					 * this:
					 */
					*retoff += i - pattern_idx;
					goto _go_end;
				}
				pattern_idx++;
			} else if (pattern_idx != 0)
				/* if pattern was not matched completely */
				pattern_idx = 0;
		}

		/* we didn't find pattern yet, let's increment our return offset
		 */
		*retoff += len;
	}

_go_end:
	/* restore offset */
	lseek(fd, save_offset, SEEK_SET);

	return ret;
}

/*
 * a demo function:  try to find cpuset mountpoint in /proc/mounts
 */
static int
cpuset_get_mountpoint(char *ret_mountpoint, int ret_size)
{
	int fd;
	off_t tmpoff;
	char buf[BUF_SIZE];
	char *tmp_mountpoint;

	int ret = 0;

	fd = open("/proc/mounts", O_RDONLY);
	if (fd == -1)
		return -1;

	if ( get_offset_of(&tmpoff, "cpuset", fd) ||
	     get_line_of(buf, BUF_SIZE, tmpoff, fd) ) {
		ret = -1;
		goto _go_close;
	}

	strtok(buf, " ");
	tmp_mountpoint = strtok(NULL, " ");
	if (tmp_mountpoint == NULL) {
		ret = -1;
		goto _go_close;
	}

	strncpy(ret_mountpoint, tmp_mountpoint, ret_size);
	ret_mountpoint[ret_size - 1] = '\0';

_go_close:
	close(fd);

	return ret;
}

/*
 * usage cmd [blank for cpuset demo | <file> <pattern>]
 */
int
main(int argc, char **argv)
{
	int fd;
	char buf[BUF_SIZE];
	ssize_t len;
	off_t offset;

	if (argc < 3) {
		if (cpuset_get_mountpoint(buf, BUF_SIZE)) {
			printf("maybe cpuset is not mounted!\n");
			return 1;
		}
		printf("cpuset mountpoint = %s\n", buf);
		return 0;
	}

	if ( (fd = open(argv[1], O_RDONLY)) == -1)
		return 1;

	if ( get_offset_of(&offset, argv[2], fd) ||
	     get_line_of(buf, BUF_SIZE, offset, fd) )
		return 1;

	printf("buf = %s\n", buf);

	close(fd);

	return 0;
}
