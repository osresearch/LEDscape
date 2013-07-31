/** \file
 * Userspace interface to the BeagleBone PRU.
 *
 * Wraps the prussdrv library in a sane interface.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <prussdrv.h>
#include <pruss_intc_mapping.h>
#include "pru.h"


static unsigned int
proc_read(
	const char * const fname
)
{
	FILE * const f = fopen(fname, "r");
	if (!f)
		die("%s: Unable to open: %s", fname, strerror(errno));
	unsigned int x;
	fscanf(f, "%x", &x);
	fclose(f);
	return x;
}


pru_t *
pru_init(
	const unsigned short pru_num
)
{
	prussdrv_init();		

	int ret = prussdrv_open(PRU_EVTOUT_0);
	if (ret)
		die("prussdrv_open open failed\n");

	tpruss_intc_initdata pruss_intc_initdata = PRUSS_INTC_INITDATA;
	prussdrv_pruintc_init(&pruss_intc_initdata);

	void * pru_data_mem;
	prussdrv_map_prumem(
		pru_num == 0 ? PRUSS0_PRU0_DATARAM :PRUSS0_PRU1_DATARAM,
		&pru_data_mem
	);

	const int mem_fd = open("/dev/mem", O_RDWR);
	if (mem_fd < 0)
		die("Failed to open /dev/mem: %s\n", strerror(errno));

	const uintptr_t ddr_addr = proc_read("/sys/class/uio/uio0/maps/map1/addr");
	const uintptr_t ddr_size = proc_read("/sys/class/uio/uio0/maps/map1/size");

	const uintptr_t ddr_start = 0x10000000;
	const uintptr_t ddr_offset = ddr_addr - ddr_start;
	const size_t ddr_filelen = ddr_size + ddr_start;

	/* map the memory */
	uint8_t * const ddr_mem = mmap(
		0,
		ddr_filelen,
		PROT_WRITE | PROT_READ,
		MAP_SHARED,
		mem_fd,
		ddr_offset
	);
	if (ddr_mem == MAP_FAILED)
		die("Failed to mmap offset %"PRIxPTR" @ %zu bytes: %s\n",
			ddr_offset,
			ddr_filelen,
			strerror(errno)
		);

	close(mem_fd);

	pru_t * const pru = calloc(1, sizeof(*pru));
	if (!pru)
		die("calloc failed: %s", strerror(errno));

	*pru = (pru_t) {
		.pru_num	= pru_num,
		.data_ram	= pru_data_mem,
		.data_ram_size	= 8192, // how to determine?
		.ddr_addr	= ddr_addr,
		.ddr		= (void*)(ddr_mem + ddr_start),
		.ddr_size	= ddr_size,
	};
    
	printf("%s: PRU %d: data %p @ %zu bytes,  DMA %p / %"PRIxPTR" @ %zu bytes\n",
		__func__,
		pru_num,
		pru->data_ram,
		pru->data_ram_size,
		pru->ddr,
		pru->ddr_addr,
		pru->ddr_size
	);

	return pru;
}


void
pru_exec(
	pru_t * const pru,
	const char * const program
)
{
	char * program_unconst = (char*)(uintptr_t) program;
	if (prussdrv_exec_program(pru->pru_num, program_unconst) < 0)
		die("%s failed", program);
}


void
pru_close(
	pru_t * const pru
)
{
	// \todo unmap memory
	prussdrv_pru_wait_event(PRU_EVTOUT_0);
	prussdrv_pru_clear_event(PRU0_ARM_INTERRUPT);
	prussdrv_pru_disable(pru->pru_num); 
	prussdrv_exit();
}


int
pru_gpio(
	const unsigned gpio,
	const unsigned pin,
	const unsigned direction,
	const unsigned initial_value
)
{
	const unsigned pin_num = gpio * 32 + pin;
	const char * export_name = "/sys/class/gpio/export";
	FILE * const export = fopen(export_name, "w");
	if (!export)
		die("%s: Unable to open? %s\n",
			export_name,
			strerror(errno)
		);

	fprintf(export, "%d\n", pin_num);
	fclose(export);

	char value_name[64];
	snprintf(value_name, sizeof(value_name),
		"/sys/class/gpio/gpio%u/value",
		pin_num
	);

	FILE * const value = fopen(value_name, "w");
	if (!value)
		die("%s: Unable to open? %s\n",
			value_name,
			strerror(errno)
		);

	fprintf(value, "%d\n", initial_value);
	fclose(value);

	char dir_name[64];
	snprintf(dir_name, sizeof(dir_name),
		"/sys/class/gpio/gpio%u/direction",
		pin_num
	);

	FILE * const dir = fopen(dir_name, "w");
	if (!dir)
		die("%s: Unable to open? %s\n",
			dir_name,
			strerror(errno)
		);

	fprintf(dir, "%s\n", direction ? "out" : "in");
	fclose(dir);

	return 0;
}
