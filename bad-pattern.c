#include <stdio.h>
#include "util.h"

int main(int argc, char **argv)
{
#if 1
	const char * dev = argc > 1 ? argv[1] : "/dev/ttyACM2";
	int fd = serial_open(dev);
	if (fd < 0)
		die("%s: open failed\n", dev);
#endif

	static uint8_t bad_pattern[210*8*3+3] = 
	{
		0x2a,0x00,0x00
#include "bad-pattern.txt"
//#include "zeros.txt"
	};

	for (size_t i = 3+24 ; i < sizeof(bad_pattern) ; i++)
		bad_pattern[i] &= 0xFF;

	ssize_t rc = write_all(fd, bad_pattern, sizeof(bad_pattern));
	fprintf(stderr, "rc=%zu\n", rc);

	return 0;
}
