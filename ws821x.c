/*
 * PRU_mem1DTransfer.c
 *
 * Copyright (C) 2010 Texas Instruments Incorporated - http://www.ti.com/ 
 * 
 * 
 *  Redistribution and use in source and binary forms, with or without 
 *  modification, are permitted provided that the following conditions 
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright 
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the 
 *    documentation and/or other materials provided with the   
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
*/

/*
 * ============================================================================
 * Copyright (c) Texas Instruments Inc 2010-11
 *
 * Use of this software is controlled by the terms and conditions found in the
 * license agreement under which this software has been supplied or provided.
 * ============================================================================
 * 
 */

/*****************************************************************************
* PRU_mem1DTransfer.c
*
* The PRU executes a simple 1-D byte array system memory transfer. As an 
* initialization step, the ARM writes an array of random byte-sized values 
* into a source address in L3 memory. The ARM then stores the source address, 
* destination address, and number of bytes to transfer into PRU's DRAM. The 
* PRU then transfers the bytes from the source address to the destination 
* address. The ARM compares the values at the source and destination address 
* to verify the transfer was successful. 
*
*****************************************************************************/

/*****************************************************************************
* Include Files                                                              *
*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

// Header file
#include <prussdrv.h>
#include <pruss_intc_mapping.h>

/*****************************************************************************
* Explicit External Declarations                                             *
*****************************************************************************/

/*****************************************************************************
* Local Macro Declarations                                                   *
*****************************************************************************/
#define PRU_NUM  (0)

#define PIXEL_DATA_ADDR 0x80001000u

/*****************************************************************************
* Local Function Declarations                                                *
*****************************************************************************/

static int LOCAL_exampleInit ( unsigned short pruNum );


/*****************************************************************************
* Local Variable Definitions                                                 *
*****************************************************************************/ 


/*****************************************************************************
* Global Variable Definitions                                                *
*****************************************************************************/

static void *pruDataMem, *l3mem;

typedef struct
{
	const void * pixels;
	unsigned size; // in bytes of the entire pixel array
	volatile unsigned start; // write 1 to start, 0xFF to abort. will be cleared when done
} ws281x_command_t;

static ws281x_command_t * ws281x_command; // mapped to the PRU DRAM
static uint8_t * pixels; // mapped to the L3 shared with the PRU


/*****************************************************************************
* Global Function Definitions                                                *
*****************************************************************************/

int main (void)
{
    unsigned int ret;
    tpruss_intc_initdata pruss_intc_initdata = PRUSS_INTC_INITDATA;
	
    printf("\nINFO: Starting %s example.\r\n", __FILE__);
    /* Initialize the PRU */
    prussdrv_init ();		
	
    /* Open PRU Interrupt */
    ret = prussdrv_open(PRU_EVTOUT_0);
    if (ret)
    {
        printf("prussdrv_open open failed\n");
        return (ret);
    }
	
    /* Get the interrupt initialized */
    prussdrv_pruintc_init(&pruss_intc_initdata);

    /* Initialize example */
    printf("\tINFO: Initializing example.\r\n");
    LOCAL_exampleInit(PRU_NUM);

    /* Execute example on PRU */
    printf("\tINFO: Executing example.\r\n");
    prussdrv_exec_program (PRU_NUM, "./ws821x.bin");

    int i;
    for (i = 0 ; i < 16 ; i++)
    {
	printf("starting %d!\n", i);
	ws281x_command->start = 1;
	while (ws281x_command->start)
		;
	printf("done!\n");
    }

    // Signal a halt command
    ws281x_command->start = 0xFF;

    printf("\tINFO: Waiting for HALT command.\r\n");
    prussdrv_pru_wait_event (PRU_EVTOUT_0);
    printf("\tINFO: PRU completed transfer.\r\n");
    prussdrv_pru_clear_event (PRU0_ARM_INTERRUPT);

    /* Disable PRU and close memory mapping*/
    prussdrv_pru_disable(PRU_NUM); 
    prussdrv_exit ();

    return(0);
}

/*****************************************************************************
* Local Function Definitions                                                 *
*****************************************************************************/

static int LOCAL_exampleInit ( unsigned short pruNum )
{
    if (pruNum == 0)
    {
        prussdrv_map_prumem (PRUSS0_PRU0_DATARAM, &pruDataMem);
    }
    else if (pruNum == 1)
    {
        prussdrv_map_prumem (PRUSS0_PRU1_DATARAM, &pruDataMem);
    }

    /* open the device */
    int mem_fd = open("/dev/mem", O_RDWR);
    if (mem_fd < 0) {
        printf("Failed to open /dev/mem (%s)\n", strerror(errno));
        return -1;
    }	

//#define DDR_BASEADDR 0x80000000
#define DDR_BASEADDR 0x98580000
#define OFFSET_DDR	 0x00001000 
#define OFFSET_L3	 2048       //equivalent with 0x00002000

    /* map the memory */
    void * ddrMem = mmap(0, 0x0FFFFFFF, PROT_WRITE | PROT_READ, MAP_SHARED, mem_fd, DDR_BASEADDR);
    if (ddrMem == MAP_FAILED) {
        printf("Failed to map the device (%s)\n", strerror(errno));
        close(mem_fd);
        return -1;
    }
    
    ws281x_command = (void*) pruDataMem;
    ws281x_command->pixels = (void*) DDR_BASEADDR;
    ws281x_command->size = 256 * 32 * 3; // 256 pixels * 32 strips * 3 bytes/pixel
    ws281x_command->start = 0;

#if 0
    prussdrv_map_l3mem (&l3mem);	
    pixels = l3mem;
#else
    pixels = ddrMem;
#endif

    // Store values into source
    printf("data ram %p l3 ram %p: setting %zu bytes\n", ws281x_command, pixels,  ws281x_command->size);
    memset(pixels, 0, ws281x_command->size);

    return(0);
}
