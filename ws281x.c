/** \file
 * Userspace interface to the WS281x LED strip driver.
 *
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

#define PRU_NUM  (0)

// Should get this from /sys/class/uio/uio0/maps/map1/addr
//#define DDR_BASEADDR 0x80000000
#define DDR_BASEADDR 0x99400000
#define OFFSET_DDR	 0x00001000 
#define OFFSET_L3	 2048       //equivalent with 0x00002000

#define die(fmt, ...) \
	do { \
		fprintf(stderr, fmt, ## __VA_ARGS__); \
		exit(EXIT_FAILURE); \
	} while (0)

/** Command structure shared with the PRU.
 * This is mapped into the PRU data RAM and points to the
 * frame buffer in the shared DDR segment.
 */
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
} __attribute__((__packed__)) ws281x_command_t;

// data is laid out with BRGA format, since that is how it will
// be translated during the clock out from the PRU.
typedef struct {
	uint8_t b;
	uint8_t r;
	uint8_t g;
	uint8_t a;
} __attribute__((__packed__)) pixel_t;

// All 32 strips worth of data for each pixel are stored adjacent
// This makes it easier to clock out while reading from the DDR
// in a burst mode..
typedef struct {
	pixel_t strip[32];
} __attribute__((__packed__)) pixel_slice_t;

// mapped to the L3 shared with the PRU
static pixel_slice_t * pixels;

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


static ws281x_command_t *
ws281_init(
	const unsigned short pruNum,
	const unsigned num_pixels
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
    
    	ws281x_command_t * const cmd = (void*) pruDataMem;
	cmd->pixels = (void*) ddr_addr;
	cmd->size = num_pixels;
	cmd->command = 0;
	cmd->response = 0;

	const size_t pixel_size = num_pixels * 32 * 4;

	if (pixel_size > ddr_size)
		die("Pixel data needs at least %zu, only %zu in DDR\n",
			pixel_size,
			ddr_size
		);

#if 0
	prussdrv_map_l3mem (&l3mem);	
	pixels = l3mem;
#else
	pixels = (void*)(ddr_mem + ddr_start);
#endif

	// Store values into source
	printf("data ram %p l3 ram %p: setting %zu bytes\n",
		cmd,
		pixels,
		pixel_size
	);


	return cmd;
}


static void
fill_color(
	int num_pixels,
	uint8_t r,
	uint8_t g,
	uint8_t b
)
{
	size_t i;
	for(i=0 ; i < num_pixels ; i++)
	{
		int strip;
		for(strip=0 ; strip < 32; strip++)
		{
			pixel_t * const p = &pixels[i].strip[strip];
			p->r = r;
			p->g = g;
			p->b = b;
			p->a = 0xFF;
		}
	}
}


int main (void)
{
    prussdrv_init();		

    int ret = prussdrv_open(PRU_EVTOUT_0);
    if (ret)
        die("prussdrv_open open failed\n");

    tpruss_intc_initdata pruss_intc_initdata = PRUSS_INTC_INITDATA;
    prussdrv_pruintc_init(&pruss_intc_initdata);

    const int num_pixels = 256;
    ws281x_command_t * cmd = ws281_init(PRU_NUM, num_pixels);

    prussdrv_exec_program (PRU_NUM, "./ws281x.bin");

    unsigned i = 0;
    while (1)
    {
	printf(" starting %d!\n", ++i);
	uint8_t val = i >> 1;
	fill_color(num_pixels, val, 0, val);
	pixels[0].strip[0].r = 0xFF;
	pixels[0].strip[1].g = 0xFF;
	pixels[0].strip[2].b = 0xFF;
	cmd->response = 0;
	cmd->command = 1;
	while (!cmd->response)
		;
	const uint32_t * next = (uint32_t*)(cmd + 1);
	printf("done! %08x %08x", cmd->response, *next);
	//if (cmd->response > 0x2900) break;
    }

    // Signal a halt command
    cmd->command = 0xFF;

    prussdrv_pru_wait_event(PRU_EVTOUT_0);
    prussdrv_pru_clear_event(PRU0_ARM_INTERRUPT);
    prussdrv_pru_disable(PRU_NUM); 
    prussdrv_exit();

    return EXIT_SUCCESS;
}

