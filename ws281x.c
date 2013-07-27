/** \file
 * Userspace interface to the WS281x LED strip driver.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <prussdrv.h>
#include <pruss_intc_mapping.h>

#define PRU_NUM  (0)

// Should get this from /sys/class/uio/uio0/maps/map1/addr
//#define DDR_BASEADDR 0x80000000
#define DDR_BASEADDR 0x97c80000
#define OFFSET_DDR	 0x00001000 
#define OFFSET_L3	 2048       //equivalent with 0x00002000


typedef struct
{
	// in the DDR shared with the PRU
	const void * pixels;

	// in bytes of the entire pixel array.
	// Should be Num pixels * Num strips * 3
	unsigned size;

	// write 1 to start, 0xFF to abort. will be cleared when started
	volatile unsigned command;

	// will have a non-zero response written when done
	volatile unsigned response;
} ws281x_command_t;

//static ws281x_command_t * ws281x_command; // mapped to the PRU DRAM
static uint8_t * pixels; // mapped to the L3 shared with the PRU


static ws281x_command_t *
ws281_init(
	unsigned short pruNum,
	unsigned num_leds
)
{
	void * pruDataMem;
	prussdrv_map_prumem(
		pruNum == 0 ? PRUSS0_PRU0_DATARAM :PRUSS0_PRU1_DATARAM,
		&pruDataMem
	);

	const int mem_fd = open("/dev/mem", O_RDWR);
	if (mem_fd < 0)
		die("Failed to open /dev/mem: %s\n", strerror(errno));

	/* map the memory */
	uint8_t * ddrMem = mmap(
		0,
		0x0FFFFFFF,
		PROT_WRITE | PROT_READ,
		MAP_SHARED,
		mem_fd,
		DDR_BASEADDR - 0x10000000
	);
	if (ddrMem == MAP_FAILED)
		die("Failed to mmap: %s\n", strerror(errno));
    
    	ws281x_command_t * const cmd = (void*) pruDataMem;
	cmd->pixels = (void*) DDR_BASEADDR;
	cmd->size = num_leds * 32 * 3; // pixels * 32 strips * 3 bytes/pixel
	cmd->command = 0;
	cmd->response = 0;

#if 0
	prussdrv_map_l3mem (&l3mem);	
	pixels = l3mem;
#else
	pixels = ddrMem + 0x10000000;
#endif

	// Store values into source
	printf("data ram %p l3 ram %p: setting %zu bytes\n",
		cmd,
		pixels,
		cmd->size
	);

	memset(pixels, 0xAB, ws281x_command->size);

	return(0);
}


int main (void)
{
    prussdrv_init();		

    int ret = prussdrv_open(PRU_EVTOUT_0);
    if (ret)
        die("prussdrv_open open failed\n");

    tpruss_intc_initdata pruss_intc_initdata = PRUSS_INTC_INITDATA;
    prussdrv_pruintc_init(&pruss_intc_initdata);

    ws281x_command_t * cmd = ws281_init(PRU_NUM, 256);

    prussdrv_exec_program (PRU_NUM, "./ws281x.bin");

    int i;
    for (i = 0 ; i < 16 ; i++)
    {
	printf("starting %d!\n", i);
	cmd->response = 0;
	cmd->command = 1;
	while (cmd->response)
		;
	const uint32_t * next = (uint32_t*)(cmd + 1);
	printf("done! %08x %08x\n", cmd->response, *next);
    }

    // Signal a halt command
    cmd->command = 0xFF;

    prussdrv_pru_wait_event(PRU_EVTOUT_0);
    prussdrv_pru_clear_event(PRU0_ARM_INTERRUPT);
    prussdrv_pru_disable(PRU_NUM); 
    prussdrv_exit();

    return EXIT_SUCCESS;
}

