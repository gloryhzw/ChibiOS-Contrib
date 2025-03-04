/******************************************************************************
 * @file     system_SN32F260.c
 * @brief    CMSIS Cortex-M0 Device Peripheral Access Layer Source File
 *           for the SONIX SN32F240B Devices
 * @version  V1.0.3
 * @date     2018/09/18
 *
 * @note
 * Copyright (C) 2014-2018 ARM Limited. All rights reserved.
 *
 * @par
 * ARM Limited (ARM) is supplying this software for use with Cortex-M
 * processor based microcontrollers.  This file can be freely distributed
 * within development tools that are supporting such ARM based processors.
 *
 * @par
 * THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 * OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 * ARM SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR
 * CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 *
 ******************************************************************************/


#include <stdint.h>
#include <SN32F260.h>
#include <mcuconf.h>



/*
//-------- <<< Use Configuration Wizard in Context Menu >>> ------------------
*/

/*--------------------- Clock Configuration ----------------------------------
//
//<e> System Clock Configuration
//		<o1>   SYSCLKSEL (SYS0_CLKCFG)
//					<0=> IHRC=48MHz
//					<1=> ILRC
//
//		<o2>   AHB Clock Prescaler Register  (SYS0_AHBCP)
//					<0=> SYSCLK/1
//					<1=> SYSCLK/2
//					<2=> SYSCLK/4
//					<3=> SYSCLK/8
//					<4=> SYSCLK/16
//					<5=> SYSCLK/32
//					<6=> SYSCLK/64
//					<7=> SYSCLK/128
//
//		<o3>  CLKOUT selection
//					<0=> Disable
//					<1=> ILRC
//					<4=> HCLK
//					<5=> IHRC
//
//		<o4>  CLKOUT Prescaler Register  (SYS1_APBCP1)
//					<0=> CLKOUT/1
//					<1=> CLKOUT/2
//					<2=> CLKOUT/4
//					<3=> CLKOUT/8
//					<4=> CLKOUT/16
//					<5=> CLKOUT/32
//					<6=> CLKOUT/64
//					<7=> CLKOUT/128
//</e>
*/
#ifndef SYS_CLOCK_SETUP
#define SYS_CLOCK_SETUP		1
#endif
#ifndef SYS0_CLKCFG_VAL
#define SYS0_CLKCFG_VAL		0
#endif
#ifndef AHB_PRESCALAR
#define AHB_PRESCALAR 		0x2
#endif
#ifndef CLKOUT_SEL_VAL
#define CLKOUT_SEL_VAL 		0x0
#endif
#ifndef CLKOUT_PRESCALAR
#define CLKOUT_PRESCALAR 	0x0
#endif

/*
//-------- <<< end of configuration section >>> ------------------------------
*/


/*----------------------------------------------------------------------------
  DEFINES
 *----------------------------------------------------------------------------*/
#ifndef IHRC48
#define	IHRC48						0
#endif
#ifndef ILRC
#define	ILRC			  			1
#endif

/*----------------------------------------------------------------------------
  Define clocks
 *----------------------------------------------------------------------------*/
#define __IHRC48_FREQ			(48000000UL)
#define __ILRC_FREQ				(32000UL)


/*----------------------------------------------------------------------------
  Clock Variable definitions
 *----------------------------------------------------------------------------*/
uint32_t SystemCoreClock;	/*!< System Clock Frequency (Core Clock)*/


/*----------------------------------------------------------------------------
  Clock functions
 *----------------------------------------------------------------------------*/
void SystemCoreClockUpdate (void)            /* Get Core Clock Frequency      */
{
	uint32_t AHB_prescaler = 0;
	uint32_t sys_clk = 0;

	switch (SN_SYS0->CLKCFG_b.SYSCLKST) {
		case 0:		//IHRC
			sys_clk = __IHRC48_FREQ;
			break;
		case 1:		//ILRC
			sys_clk = __ILRC_FREQ;
			break;
		default:
			break;
	}

	switch (SN_SYS0->AHBCP_b.AHBPRE) {
		case 0:	AHB_prescaler = 1;	break;
		case 1:	AHB_prescaler = 2;	break;
		case 2:	AHB_prescaler = 4;	break;
		case 3:	AHB_prescaler = 8;	break;
		case 4:	AHB_prescaler = 16;	break;
		case 5:	AHB_prescaler = 32;	break;
		case 6:	AHB_prescaler = 64;	break;
		case 7:	AHB_prescaler = 128;break;
		default: break;
	}

	SystemCoreClock = sys_clk / AHB_prescaler;

	return;
}

/**
 * Initialize the system
 *
 * @param  none
 * @return none
 *
 * @brief  Setup the microcontroller system.
 *         Initialize the System.
 */
void SystemInit (void)
{
#if (SYS_CLOCK_SETUP)

	// Setup pre-scaler so HCLK is always less than 24MHz, this way it's save to configure LPCTRL.
	// NOTE: This assumes it save to select >=24MHz mode while HCLK is <24MHz. LPCTRL most likely configures flash wait states, and in that case the assumption holds.
	// SYSCLK will be IHRC or ILRC, both are less then 24MHz when divided by 4.
	SN_SYS0->AHBCP = 2; // SYSCLK/4

	#if SYS0_CLKCFG_VAL == IHRC48			//IHRC=48MHz
		// Config flash based on HCLK
		#if (AHB_PRESCALAR == 0 | AHB_PRESCALAR == 1)
			SN_FLASH->LPCTRL = 0x5AFA0005; // >= 24MHz
		#else
			SN_FLASH->LPCTRL = 0x5AFA0000; // < 24MHz
		#endif

		// Start and make sure IHRC is running
        SN_SYS0->ANBCTRL = 0x1;
        while ((SN_SYS0->CSST & 0x1) != 0x1);

		// Switch to IHRC
        SN_SYS0->CLKCFG = 0x0;
        while ((SN_SYS0->CLKCFG & 0x70) != 0x0);

    #elif SYS0_CLKCFG_VAL == ILRC			//ILRC ON
		// // Config flash based on HCLK
		SN_FLASH->LPCTRL = 0x5AFA0000;

		// Switch to KLRC
        SN_SYS0->CLKCFG = 0x1;
        while ((SN_SYS0->CLKCFG & 0x70) != 0x10);
	#endif

	// Switch to the final prescaler.
	SN_SYS0->AHBCP = AHB_PRESCALAR;

	#if (CLKOUT_SEL_VAL > 0)			//CLKOUT
        SN_SYS1->AHBCLKEN_b.CLKOUTSEL 	= CLKOUT_SEL_VAL;
        SN_SYS1->APBCP1_b.CLKOUTPRE 	= CLKOUT_PRESCALAR;
	#endif
#endif //(SYS_CLOCK_SETUP)

}
