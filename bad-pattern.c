#include <stdio.h>
#include "util.h"

int main(void)
{
	int fd = serial_open("/dev/ttyACM1");
	if (fd < 0)
		die("open failed");

	static const uint8_t bad_pattern[] = 
	{
#include "bad-pattern.txt"
	};

	write_all(fd, bad_pattern, sizeof(bad_pattern));

	return 0;
}
