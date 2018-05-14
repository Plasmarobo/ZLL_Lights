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
/* JenOS includes */
#include <dbg.h>
/* Application includes */
/* Device includes */
#include "DriverBulb.h"

#ifdef DEBUG_LIGHT_TASK
#define TRACE_LIGHT_TASK  TRUE
#else
#define TRACE_LIGHT_TASK FALSE
#endif

#ifdef DEBUG_PATH
#define TRACE_PATH  TRUE
#else
#define TRACE_PATH  FALSE
#endif

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
/* SPI configuration */
#define PERIPHERAL_CLOCK_FREQUENCY_HZ	16000000UL		/* System frequency 16MHz */
#define SPI_FREQUENCY_HZ				1000000UL
#define N_LEDS 							26
#define CLEAR_BYTES						1 /* N_LEDS/32 (min 1) */

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
PRIVATE uint8   u32CurrLevel 	= 127;

PRIVATE uint8   u32CurrRed       = 64;
PRIVATE uint8   u32CurrGreen     = 64;
PRIVATE uint8   u32CurrBlue		= 64;

PRIVATE uint8   u8GammaTable[256] = {
		128, 128, 128, 128, 128, 128, 128, 128,
		128, 128, 128, 128, 128, 128, 128, 128,
		128, 128, 128, 128, 128, 128, 128, 128,
		128, 128, 128, 128, 129, 129, 129, 129,
		129, 129, 129, 129, 129, 129, 129, 129,
		129, 129, 129, 129, 130, 130, 130, 130,
		130, 130, 130, 130, 130, 131, 131, 131,
		131, 131, 131, 131, 131, 132, 132, 132,
		132, 132, 132, 132, 133, 133, 133, 133,
		133, 134, 134, 134, 134, 134, 135, 135,
		135, 135, 135, 136, 136, 136, 136, 137,
		137, 137, 137, 138, 138, 138, 138, 139,
		139, 139, 140, 140, 140, 141, 141, 141,
		141, 142, 142, 142, 143, 143, 144, 144,
		144, 145, 145, 145, 146, 146, 146, 147,
		147, 148, 148, 149, 149, 149, 150, 150,
		151, 151, 152, 152, 152, 153, 153, 154,
		154, 155, 155, 156, 156, 157, 157, 158,
		158, 159, 160, 160, 161, 161, 162, 162,
		163, 163, 164, 165, 165, 166, 166, 167,
		168, 168, 169, 169, 170, 171, 171, 172,
		173, 173, 174, 175, 175, 176, 177, 178,
		178, 179, 180, 180, 181, 182, 183, 183,
		184, 185, 186, 186, 187, 188, 189, 190,
		190, 191, 192, 193, 194, 195, 195, 196,
		197, 198, 199, 200, 201, 202, 202, 203,
		204, 205, 206, 207, 208, 209, 210, 211,
		212, 213, 214, 215, 216, 217, 218, 219,
		220, 221, 222, 223, 224, 225, 226, 227,
		228, 229, 230, 232, 233, 234, 235, 236,
		237, 238, 239, 241, 242, 243, 244, 245,
		246, 248, 249, 250, 251, 253, 254, 255
};


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
		DBG_vPrintf(TRUE, "\nSetting SPI Frequency divider %d\n", divider);
		u32AHI_Init();
		vAHI_SpiConfigure(4, /* All Slave devices */
						  FALSE, /* MSB FIRST */
						  FALSE, /* Normal Polarity */
						  FALSE, /* Leading edge latch */
						  divider, /* 2x8 = 16MHz, Divide 16MHz clk to get 1MHz spi clk */
						  FALSE, /* Disable Interrupts */
						  FALSE); /* Disable auto slave select */

		vAHI_SpiSelSetLocation(1, FALSE);
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
 * DESCRIPTION:		Updates the color
 *
 *
 * PARAMETERS:      Name     RW  Usage
 *         	        u8Level  R   Light level 0-LAMP_LEVEL_MAX
 *
 ****************************************************************************/
PUBLIC void DriverBulb_vSetLevel(uint32 u32Level)
{
	/* Different value ? */
	if (u32CurrLevel != u32Level)
	{
		/* Note the new level */
		u32CurrLevel = MAX(1, u32Level);
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
 * DESCRIPTION:		Updates the color
 *
 *
 * PARAMETERS:      Name     RW  Usage
 *         	        u8Level  R   Light level 0-LAMP_LEVEL_MAX
 *
 ****************************************************************************/

PUBLIC void DriverBulb_vSetColour(uint32 u32Red, uint32 u32Green, uint32 u32Blue)
{
	/* Different value ? */
	if (u32CurrRed != u32Red || u32CurrGreen != u32Green || u32CurrBlue != u32Blue)
	{
		/* Note the new values */
		u32CurrRed   = u32Red;
		u32CurrGreen = u32Green;
		u32CurrBlue  = u32Blue;
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
	uint32  u32Color = 0;
	uint8 i;
	vAHI_SpiSelect(1);
	vAHI_SpiWaitBusy();
	DBG_vPrintf(TRACE_LIGHT_TASK, "Clearing SPI\n");
	for(i = 0; i < CLEAR_BYTES; i = i + 1) {
		vAHI_SpiWaitBusy();
		vAHI_SpiStartTransfer(7, 0);
	}

	/* Is bulb on ? */
	if (bIsOn)
	{
		/* Scale color for brightness level */
		/* Strip format is GRB. SPI is right aligned */
		u32Color |= u8GammaTable[(u32CurrBlue * u32CurrLevel) / 255] << 16;
		u32Color |= u8GammaTable[(u32CurrRed * u32CurrLevel) / 255] << 8;
		u32Color |= u8GammaTable[(u32CurrGreen * u32CurrLevel) / 255];
	}

	DBG_vPrintf(TRACE_LIGHT_TASK, "COLOR TO USE: 0x%6x\n", u32Color);
	for(i = 0; i < N_LEDS; i = i + 1) {
		vAHI_SpiStartTransfer(23, u32Color);
		vAHI_SpiWaitBusy();
	}
	vAHI_SpiSelect(0);
}

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
