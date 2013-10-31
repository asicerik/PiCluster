#include "stdint.h"
#include "stdio.h"
#include "stdarg.h"
#ifdef WIN32
#include "Windows.h"
#include <conio.h>


#else
#include "bcm2835.h"
#include "uart0.h"
#include "eemc.h"

uint32_t* eemc_arg2			= (uint32_t*)BCM2835_EEMC_ARG2;
uint32_t* eemc_blksizecnt	= (uint32_t*)BCM2835_EEMC_BLKSIZECNT;
uint32_t* eemc_arg1			= (uint32_t*)BCM2835_EEMC_ARG1;
uint32_t* eemc_cmdtm		= (uint32_t*)BCM2835_EEMC_CMDTM;
uint32_t* eemc_resp0		= (uint32_t*)BCM2835_EEMC_RESP0;
uint32_t* eemc_resp1		= (uint32_t*)BCM2835_EEMC_RESP1;
uint32_t* eemc_resp2		= (uint32_t*)BCM2835_EEMC_RESP2;
uint32_t* eemc_resp3		= (uint32_t*)BCM2835_EEMC_RESP3;
uint32_t* eemc_data			= (uint32_t*)BCM2835_EEMC_DATA;
uint32_t* eemc_status		= (uint32_t*)BCM2835_EEMC_STATUS;
uint32_t* eemc_control0		= (uint32_t*)BCM2835_EEMC_CONTROL0;
uint32_t* eemc_control1		= (uint32_t*)BCM2835_EEMC_CONTROL1;
uint32_t* eemc_interrupt	= (uint32_t*)BCM2835_EEMC_INTERRUPT;
uint32_t* eemc_irpt_mask	= (uint32_t*)BCM2835_EEMC_IRPT_MASK;
uint32_t* eemc_irpt_en		= (uint32_t*)BCM2835_EEMC_IRPT_EN;
uint32_t* eemc_control2		= (uint32_t*)BCM2835_EEMC_CONTROL2;
uint32_t* eemc_force_irpt	= (uint32_t*)BCM2835_EEMC_FORCE_IRPT;
uint32_t* eemc_boot_timeout	= (uint32_t*)BCM2835_EEMC_BOOT_TIMEOUT;
uint32_t* eemc_dbg_sel		= (uint32_t*)BCM2835_EEMC_DGB_SEL;
uint32_t* eemc_exrdfifo_cfg	= (uint32_t*)BCM2835_EEMC_EXRDFIFO_CFG;
uint32_t* eemc_exrdfifo_en	= (uint32_t*)BCM2835_EEMC_EXRDFIFO_EN;
uint32_t* eemc_tune_step	= (uint32_t*)BCM2835_EEMC_TUNE_STEP;
uint32_t* eemc_tune_steps_std = (uint32_t*)BCM2835_EEMC_TUNE_STEPS_STD;
uint32_t* eemc_tune_steps_ddr = (uint32_t*)BCM2835_EEMC_TUNE_STEPS_DDR;
uint32_t* eemc_spi_int_spt	= (uint32_t*)BCM2835_EEMC_SPI_INT_SPT;
uint32_t* eemc_slotisr_ver	= (uint32_t*)BCM2835_EEMC_SLOTISR_VER;

bool eemcInit()
{
}

void eemcDump()
{
	UartPrintf("eemc_arg2			%p\n", eemc_arg2);
	UartPrintf("eemc_blksizecnt		%p\n", eemc_blksizecnt);
	UartPrintf("eemc_arg1			%p\n", eemc_arg1);
	UartPrintf("eemc_cmdtm			%p\n", eemc_cmdtm);
	UartPrintf("eemc_resp0			%p\n", eemc_resp0);
	UartPrintf("eemc_resp1			%p\n", eemc_resp1);
	UartPrintf("eemc_resp2			%p\n", eemc_resp2);
	UartPrintf("eemc_resp3			%p\n", eemc_resp3);
	UartPrintf("eemc_data			%p\n", eemc_data);
	UartPrintf("eemc_status			%p\n", eemc_status);
	UartPrintf("eemc_control0		%p\n", eemc_control0);
	UartPrintf("eemc_control1		%p\n", eemc_control1);
	UartPrintf("eemc_interrupt		%p\n", eemc_interrupt);
	UartPrintf("eemc_irpt_mask		%p\n", eemc_irpt_mask);
	UartPrintf("eemc_irpt_en		%p\n", eemc_irpt_en);
	UartPrintf("eemc_control2		%p\n", eemc_control2);
	UartPrintf("eemc_force_irpt		%p\n", eemc_force_irpt);
	UartPrintf("eemc_boot_timeout	%p\n", eemc_boot_timeout);
	UartPrintf("eemc_dbg_sel		%p\n", eemc_dbg_sel);
	UartPrintf("eemc_exrdfifo_cfg	%p\n", eemc_exrdfifo_cfg);
	UartPrintf("eemc_exrdfifo_en	%p\n", eemc_exrdfifo_en);
	UartPrintf("eemc_tune_step		%p\n", eemc_tune_step);
	UartPrintf("eemc_tune_steps_std	%p\n", eemc_tune_steps_std);
	UartPrintf("eemc_tune_steps_ddr	%p\n", eemc_tune_steps_ddr);
	UartPrintf("eemc_spi_int_spt	%p\n", eemc_spi_int_spt);
	UartPrintf("eemc_slotisr_ver	%p\n", eemc_slotisr_ver);
}

#endif
