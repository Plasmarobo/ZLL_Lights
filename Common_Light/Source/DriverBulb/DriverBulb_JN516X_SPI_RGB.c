/****************************************************************************
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * Copyright Austen Higgins-Cassidy 2018. All rights reserved
 ****************************************************************************/

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
/* Standard includes */
#include <string.h>
/* SDK includes */
#include <jendefs.h>
/* Hardware includes */
#include <AppHardwareApi.h>
#include <PeripheralRegs.h>
/* JenOS includes */
#include <dbg.h>
/* Application includes */
/* Device includes */
#include "DriverBulb.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
/* PWM timer configuration */
#define PERIPHERAL_CLOCK_FREQUENCY_HZ	16000000UL		/* System frequency 16MHz */
#define SPI_FREQUENCY_HZ				2000000UL
#define PWM_TIMER_PRESCALE				6     			/* Prescale value to use   */
#define N_LEDS 							26

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
PRIVATE void DriverBulb_vOutput(void);

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Global Variables                                               ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
PRIVATE bool_t  bIsOn      	 	= FALSE;
PRIVATE uint8   u8CurrLevel 	= 127;

PRIVATE uint8   u8CurrRed       = 64;
PRIVATE uint8   u8CurrGreen     = 64;
PRIVATE uint8   u8CurrBlue		= 64;


/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
/****************************************************************************
 *
 * NAME:       		DriverBulb_vInit
 *
 * DESCRIPTION:		Initializes the lamp drive system
 *
 * PARAMETERS:      Name     RW  Usage
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void DriverBulb_vInit(void)
{
	static bool_t bInit = FALSE;

	/* Not already initialized ? */
	if (bInit == FALSE)
	{
		uint8 divider = (PERIPHERAL_CLOCK_FREQUENCY_HZ / 2) / SPI_FREQUENCY_HZ;
		/* Enable SPI */
		vAHI_SpiConfigure(1, /* 1 Slave device */
						  FALSE, /* MSB FIRST */
						  FALSE, /* Normal Polarity */
						  FALSE, /* Leading edge latch */
						  divider, /* 2x8 = 16MHz, Divide 16MHz clk to get 1MHz spi clk */
						  TRUE, /* Enable Interrupts */
						  FALSE); /* Disable auto slave select */


		/* Note light is on */
		bIsOn = TRUE;

		/* Set outputs */
		DriverBulb_vOutput();

		/* Now initialized */
		bInit = TRUE;
	}
}

PUBLIC void DriverBulb_vSetOnOff(bool_t bOn)
{
	(bOn) ? DriverBulb_vOn() : DriverBulb_vOff();
}

/****************************************************************************
 *
 * NAME:       		DriverBulb_bReady
 *
 * DESCRIPTION:		Returns if lamp is ready to be operated
 *
 * PARAMETERS:      Name     RW  Usage
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC bool_t DriverBulb_bReady(void)
{
	return (TRUE);
}

/****************************************************************************
 *
 * NAME:       		DriverBulb_vSetLevel
 *
 * DESCRIPTION:		Updates the PWM via the thermal control loop
 *
 *
 * PARAMETERS:      Name     RW  Usage
 *         	        u8Level  R   Light level 0-LAMP_LEVEL_MAX
 *
 ****************************************************************************/
PUBLIC void DriverBulb_vSetLevel(uint32 u32Level)
{
	/* Different value ? */
	if (u8CurrLevel != (uint8) u32Level)
	{
		/* Note the new level */
		u8CurrLevel = (uint8) MAX(1, u32Level);
		/* Is the lamp on ? */
		if (bIsOn)
		{
			/* Set outputs */
			DriverBulb_vOutput();
		}
	}
}

/****************************************************************************
 *
 * NAME:       		DriverBulb_vSetColour
 *
 * DESCRIPTION:		Updates the PWM via the thermal control loop
 *
 *
 * PARAMETERS:      Name     RW  Usage
 *         	        u8Level  R   Light level 0-LAMP_LEVEL_MAX
 *
 ****************************************************************************/

PUBLIC void DriverBulb_vSetColour(uint32 u32Red, uint32 u32Green, uint32 u32Blue)
{
	/* Different value ? */
	if (u8CurrRed != (uint8) u32Red || u8CurrGreen != (uint8) u32Green || u8CurrBlue != (uint8) u32Blue)
	{
		/* Note the new values */
		u8CurrRed   = (uint8) u32Red;
		u8CurrGreen = (uint8) u32Green;
		u8CurrBlue  = (uint8) u32Blue;
		/* Is the lamp on ? */
		if (bIsOn)
		{
			/* Set outputs */
			DriverBulb_vOutput();
		}
	}
}

/****************************************************************************
 *
 * NAME:            DriverBulb_vOn
 *
 * DESCRIPTION:     Turns the lamp on, over-driving if user deep-dimmed
 *                  before turning off otherwise ignition failures occur
 *
 ****************************************************************************/
PUBLIC void DriverBulb_vOn(void)
{
	/* Lamp is not on ? */
	if (bIsOn != TRUE)
	{
		/* Note light is on */
		bIsOn = TRUE;
		/* Set outputs */
		DriverBulb_vOutput();
	}
}

/****************************************************************************
 *
 * NAME:            DriverBulb_vOff
 *
 * DESCRIPTION:     Turns the lamp off
 *
 *
 * PARAMETERS:      Name     RW  Usage
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void DriverBulb_vOff(void)
{
	/* Lamp is on ? */
	if (bIsOn == TRUE)
	{
		/* Note light is off */
		bIsOn = FALSE;
		/* Set outputs */
		DriverBulb_vOutput();
	}
}

/****************************************************************************
 *
 * NAMES:           DriverBulb_bOn, u16ReadBusVoltage, u16ReadChipTemperature
 *
 * DESCRIPTION:		Access functions for Monitored Lamp Parameters
 *
 *
 * PARAMETERS:      Name     RW  Usage
 *
 * RETURNS:
 * Lamp state, Bus Voltage (Volts), Chip Temperature (Degrees C)
 *
 ****************************************************************************/
PUBLIC bool_t DriverBulb_bOn(void)
{
	return (bIsOn);
}

/****************************************************************************
 *
 * NAMES:           DriverBulb_vTick
 *
 * DESCRIPTION:		Hook for 10 ms Ticks from higher layer for timing
 *                  This currently restores very low dim levels after
 *                  we've started up with sufficient DCI level to ensure
 *                  ignition
 *
 ****************************************************************************/
PUBLIC void DriverBulb_vTick(void)
{
}

/****************************************************************************
 *
 * NAME:			DriverBulb_i16Analogue
 *
 * DESCRIPTION:     ADC based measurement of bus voltage is not support in
 * 					this driver.
 *
 * PARAMETERS:      Name	     RW      Usage
 *                  u8Adc        R       ADC Channel
 *                  u16AdcRead   R       Raw ADC Value
 *
 * RETURNS:         Bus voltage
 *
 ****************************************************************************/
PUBLIC int16 DriverBulb_i16Analogue(uint8 u8Adc, uint16 u16AdcRead)
{
	return 0;
}

/****************************************************************************
 *
 * NAME:			DriverBulb_bFailed
 *
 * DESCRIPTION:     Access function for Failed bulb state
 *
 *
 * RETURNS:         bulb state
 *
 ****************************************************************************/
PUBLIC bool_t DriverBulb_bFailed(void)
{
	return (FALSE);
}

/****************************************************************************
 *
 * NAME:			DriverBulb_vOutput
 *
 * DESCRIPTION:
 *
 * RETURNS:         void
 *
 ****************************************************************************/
PRIVATE void DriverBulb_vOutput(void)
{

	uint32   u32Red;
	uint32   u32Green;
	uint32   u32Blue;
	uint32  u32Color;
	uint8 i;

	/* LED Strip based on LPD8806 only supports Color values from 128->255 */

	/* Is bulb on ? */
	if (bIsOn)
	{
		/* Scale colour for brightness level */
		u32Red   = 128 + (uint8)(((uint32)u8CurrRed   * (uint32)u8CurrLevel) / (uint32)255);
		u32Green = 128 + (uint8)(((uint32)u8CurrGreen * (uint32)u8CurrLevel) / (uint32)255);
		u32Blue  = 128 + (uint8)(((uint32)u8CurrBlue  * (uint32)u8CurrLevel) / (uint32)255);

		/* Don't allow fully off */
		if (u32Red   == 128) u32Red   = 129;
		if (u32Green == 128) u32Green = 129;
		if (u32Blue  == 128) u32Blue  = 129;
	}
	else /* Turn off */
	{
		u32Red   = 128;
		u32Green = 128;
		u32Blue  = 128;
	}

	/* Strip format is GRB */
	u32Color = (u32Green << 16) | (u32Red << 8) | u32Blue;

	/* Clear Strip */
	for(i = 0; i < N_LEDS/32; i = i + 1) {
		vAHI_SpiStartTransfer(8, 0);
		vAHI_SpiWaitBusy();
	}

	/* Set RGB channel levels */
	for(i = 0; i < N_LEDS; i = i + 1) {
		vAHI_SpiStartTransfer(24, u32Color);
		vAHI_SpiWaitBusy();
	}

}

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/

