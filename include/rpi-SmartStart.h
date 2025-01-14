#ifndef _SMART_START_H_
#define _SMART_START_H_

#ifdef __cplusplus								// If we are including to a C++
extern "C" {									// Put extern C directive wrapper around
#endif

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++}
{																			}			
{       Filename: rpi-smartstart.h											}
{       Copyright(c): Leon de Boer(LdB) 2017, 2018							}
{       Version: 2.14														}
{																			}		
{***************[ THIS CODE IS FREEWARE UNDER CC Attribution]***************}
{																            }
{     This sourcecode is released for the purpose to promote programming    }
{  on the Raspberry Pi. You may redistribute it and/or modify with the      }
{  following disclaimer and condition.                                      }
{																            }
{      The SOURCE CODE is distributed "AS IS" WITHOUT WARRANTIES AS TO      }
{   PERFORMANCE OF MERCHANTABILITY WHETHER EXPRESSED OR IMPLIED.            }
{   Redistributions of source code must retain the copyright notices to     }
{   maintain the author credit (attribution) .								}
{																			}
{***************************************************************************}
{                                                                           }
{      This code provides a 32bit or 64bit C wrapper around the assembler   }
{  stub code. In AARCH32 that file is SmartStart32.S, while in AARCH64 the  }
{  file is SmartStart64.S.													}
{	   There file also provides access to the basic hardware of the PI.     }
{  It is also the easy point to insert a couple of important very useful    }
{  Macros that make C development much easier.		        				} 
{++++++++++++++++++++++++[ REVISIONS ]++++++++++++++++++++++++++++++++++++++}
{  2.08 Added setIrqFuncAddress & setFiqFuncAddress							}
{  2.09 Added Hard/Soft float compiler support								}
{  2.10 Context Switch support API calls added								}
{  2.11 MiniUart, PL011 Uart and console uart support added					}
{  2.12 New FIQ, DAIF flag support added									}
{  2.13	Graphics routines relocated to there own unit						}
{  2.14 Multicore task switcher support Added								}
{++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include <stdbool.h>		// C standard unit needed for bool and true/false
#include <stdint.h>			// C standard unit needed for uint8_t, uint32_t, etc
#include <stdarg.h>			// C standard unit needed for variadic functions

/***************************************************************************}
{		  PUBLIC MACROS MUCH AS WE HATE THEM SOMETIMES YOU NEED THEM        }
{***************************************************************************/

/* Most versions of C don't have _countof macro so provide it if not available */
#if !defined(_countof)
#define _countof(_Array) (sizeof(_Array) / sizeof(_Array[0])) 
#endif

/* As we are compiling for Raspberry Pi if winmain make it main */
#define WinMain(...) main(uint32_t r0, uint32_t r1, uint32_t atags)

/***************************************************************************}
{					     PUBLIC ENUMERATION CONSTANTS			            }
****************************************************************************/

/*--------------------------------------------------------------------------}
;{				  ENUMERATED AN ID FOR DIFFERENT PI MODELS					}
;{-------------------------------------------------------------------------*/
typedef enum {
	MODEL_1A		= 0x0,
	MODEL_1B		= 0x1,
	MODEL_1A_PLUS	= 0x2,
	MODEL_1B_PLUS	= 0x3,
	MODEL_2B		= 0x4,
	MODEL_ALPHA		= 0x5,
	MODEL_CM		= 0x6,		// Compute Module
	MODEL_2A        = 0x7,
	MODEL_PI3		= 0x8,
	MODEL_PI_ZERO	= 0x9,
	MODEL_CM3       = 0x0A,      // Compute module 3
	MODEL_PI_ZEROW	= 0x0C,
	MODEL_PI3B_PLUS = 0x0D,
	MODEL_PI3A_PLUS = 0x0E,
	MODEL_CM3_PLUS  = 0x10,     // Compute module 3a plus
} RPI_BOARD_TYPE;

/*--------------------------------------------------------------------------}
;{	      ENUMERATED FSEL REGISTERS ... BCM2835.PDF MANUAL see page 92		}
;{-------------------------------------------------------------------------*/
/* In binary so any error is obvious */
#define GPIO_INPUT		0b000							// 0
#define GPIO_OUTPUT		0b001							// 1
#define GPIO_ALTFUNC5	0b010							// 2
#define GPIO_ALTFUNC4	0b011							// 3
#define GPIO_ALTFUNC0	0b100							// 4
#define	GPIO_ALTFUNC1	0b101							// 5
#define	GPIO_ALTFUNC2	0b110							// 6
#define GPIO_ALTFUNC3	0b111							// 7
typedef uint8_t GPIOMODE;

/*--------------------------------------------------------------------------}
;{	    ENUMERATED GPIO FIX RESISTOR ... BCM2835.PDF MANUAL see page 101	}
;{-------------------------------------------------------------------------*/
/* In binary so any error is obvious */
#define NO_RESISTOR		0b00							// 0
#define PULLUP			0b01							// 1
#define PULLDOWN		0b10							// 2
typedef uint8_t GPIO_FIX_RESISTOR;

/*--------------------------------------------------------------------------}
{	ENUMERATED TIMER CONTROL PRESCALE ... BCM2835.PDF MANUAL see page 197	}
{--------------------------------------------------------------------------*/
/* In binary so any error is obvious */
typedef enum {
	Clkdiv1 = 0b00,										// 0
	Clkdiv16 = 0b01,									// 1
	Clkdiv256 = 0b10,									// 2
	Clkdiv_undefined = 0b11,							// 3 
} TIMER_PRESCALE;

/*--------------------------------------------------------------------------}
{	                  ENUMERATED MAILBOX CHANNELS							}
{		  https://github.com/raspberrypi/firmware/wiki/Mailboxes			}
{--------------------------------------------------------------------------*/
typedef enum {
	MB_CHANNEL_POWER = 0x0,								// Mailbox Channel 0: Power Management Interface 
	MB_CHANNEL_FB = 0x1,								// Mailbox Channel 1: Frame Buffer
	MB_CHANNEL_VUART = 0x2,								// Mailbox Channel 2: Virtual UART
	MB_CHANNEL_VCHIQ = 0x3,								// Mailbox Channel 3: VCHIQ Interface
	MB_CHANNEL_LEDS = 0x4,								// Mailbox Channel 4: LEDs Interface
	MB_CHANNEL_BUTTONS = 0x5,							// Mailbox Channel 5: Buttons Interface
	MB_CHANNEL_TOUCH = 0x6,								// Mailbox Channel 6: Touchscreen Interface
	MB_CHANNEL_COUNT = 0x7,								// Mailbox Channel 7: Counter
	MB_CHANNEL_TAGS = 0x8,								// Mailbox Channel 8: Tags (ARM to VC)
	MB_CHANNEL_GPU = 0x9,								// Mailbox Channel 9: GPU (VC to ARM)
} MAILBOX_CHANNEL;

/*--------------------------------------------------------------------------}
{	            ENUMERATED MAILBOX TAG CHANNEL COMMANDS						}
{  https://github.com/raspberrypi/firmware/wiki/Mailbox-property-interface  }
{--------------------------------------------------------------------------*/
typedef enum {
	/* Videocore info commands */
	MAILBOX_TAG_GET_VERSION					= 0x00000001,			// Get firmware revision

	/* Hardware info commands */
	MAILBOX_TAG_GET_BOARD_MODEL				= 0x00010001,			// Get board model
	MAILBOX_TAG_GET_BOARD_REVISION			= 0x00010002,			// Get board revision
	MAILBOX_TAG_GET_BOARD_MAC_ADDRESS		= 0x00010003,			// Get board MAC address
	MAILBOX_TAG_GET_BOARD_SERIAL			= 0x00010004,			// Get board serial
	MAILBOX_TAG_GET_ARM_MEMORY				= 0x00010005,			// Get ARM memory
	MAILBOX_TAG_GET_VC_MEMORY				= 0x00010006,			// Get VC memory
	MAILBOX_TAG_GET_CLOCKS					= 0x00010007,			// Get clocks

	/* Power commands */
	MAILBOX_TAG_GET_POWER_STATE				= 0x00020001,			// Get power state
	MAILBOX_TAG_GET_TIMING					= 0x00020002,			// Get timing
	MAILBOX_TAG_SET_POWER_STATE				= 0x00028001,			// Set power state

	/* GPIO commands */
	MAILBOX_TAG_GET_GET_GPIO_STATE			= 0x00030041,			// Get GPIO state
	MAILBOX_TAG_SET_GPIO_STATE				= 0x00038041,			// Set GPIO state

	/* Clock commands */
	MAILBOX_TAG_GET_CLOCK_STATE				= 0x00030001,			// Get clock state
	MAILBOX_TAG_GET_CLOCK_RATE				= 0x00030002,			// Get clock rate
	MAILBOX_TAG_GET_MAX_CLOCK_RATE			= 0x00030004,			// Get max clock rate
	MAILBOX_TAG_GET_MIN_CLOCK_RATE			= 0x00030007,			// Get min clock rate
	MAILBOX_TAG_GET_TURBO					= 0x00030009,			// Get turbo

	MAILBOX_TAG_SET_CLOCK_STATE				= 0x00038001,			// Set clock state
	MAILBOX_TAG_SET_CLOCK_RATE				= 0x00038002,			// Set clock rate
	MAILBOX_TAG_SET_TURBO					= 0x00038009,			// Set turbo

	/* Voltage commands */
	MAILBOX_TAG_GET_VOLTAGE					= 0x00030003,			// Get voltage
	MAILBOX_TAG_GET_MAX_VOLTAGE				= 0x00030005,			// Get max voltage
	MAILBOX_TAG_GET_MIN_VOLTAGE				= 0x00030008,			// Get min voltage

	MAILBOX_TAG_SET_VOLTAGE					= 0x00038003,			// Set voltage

	/* Temperature commands */
	MAILBOX_TAG_GET_TEMPERATURE				= 0x00030006,			// Get temperature
	MAILBOX_TAG_GET_MAX_TEMPERATURE			= 0x0003000A,			// Get max temperature

	/* Memory commands */
	MAILBOX_TAG_ALLOCATE_MEMORY				= 0x0003000C,			// Allocate Memory
	MAILBOX_TAG_LOCK_MEMORY					= 0x0003000D,			// Lock memory
	MAILBOX_TAG_UNLOCK_MEMORY				= 0x0003000E,			// Unlock memory
	MAILBOX_TAG_RELEASE_MEMORY				= 0x0003000F,			// Release Memory
																	
	/* Execute code commands */
	MAILBOX_TAG_EXECUTE_CODE				= 0x00030010,			// Execute code

	/* QPU control commands */
	MAILBOX_TAG_EXECUTE_QPU					= 0x00030011,			// Execute code on QPU
	MAILBOX_TAG_ENABLE_QPU					= 0x00030012,			// QPU enable

	/* Displaymax commands */
	MAILBOX_TAG_GET_DISPMANX_HANDLE			= 0x00030014,			// Get displaymax handle
	MAILBOX_TAG_GET_EDID_BLOCK				= 0x00030020,			// Get HDMI EDID block

	/* SD Card commands */
	MAILBOX_GET_SDHOST_CLOCK				= 0x00030042,			// Get SD Card EMCC clock
	MAILBOX_SET_SDHOST_CLOCK				= 0x00038042,			// Set SD Card EMCC clock

	/* Framebuffer commands */
	MAILBOX_TAG_ALLOCATE_FRAMEBUFFER		= 0x00040001,			// Allocate Framebuffer address
	MAILBOX_TAG_BLANK_SCREEN				= 0x00040002,			// Blank screen
	MAILBOX_TAG_GET_PHYSICAL_WIDTH_HEIGHT	= 0x00040003,			// Get physical screen width/height
	MAILBOX_TAG_GET_VIRTUAL_WIDTH_HEIGHT	= 0x00040004,			// Get virtual screen width/height
	MAILBOX_TAG_GET_COLOUR_DEPTH			= 0x00040005,			// Get screen colour depth
	MAILBOX_TAG_GET_PIXEL_ORDER				= 0x00040006,			// Get screen pixel order
	MAILBOX_TAG_GET_ALPHA_MODE				= 0x00040007,			// Get screen alpha mode
	MAILBOX_TAG_GET_PITCH					= 0x00040008,			// Get screen line to line pitch
	MAILBOX_TAG_GET_VIRTUAL_OFFSET			= 0x00040009,			// Get screen virtual offset
	MAILBOX_TAG_GET_OVERSCAN				= 0x0004000A,			// Get screen overscan value
	MAILBOX_TAG_GET_PALETTE					= 0x0004000B,			// Get screen palette

	MAILBOX_TAG_RELEASE_FRAMEBUFFER			= 0x00048001,			// Release Framebuffer address
	MAILBOX_TAG_SET_PHYSICAL_WIDTH_HEIGHT	= 0x00048003,			// Set physical screen width/heigh
	MAILBOX_TAG_SET_VIRTUAL_WIDTH_HEIGHT	= 0x00048004,			// Set virtual screen width/height
	MAILBOX_TAG_SET_COLOUR_DEPTH			= 0x00048005,			// Set screen colour depth
	MAILBOX_TAG_SET_PIXEL_ORDER				= 0x00048006,			// Set screen pixel order
	MAILBOX_TAG_SET_ALPHA_MODE				= 0x00048007,			// Set screen alpha mode
	MAILBOX_TAG_SET_VIRTUAL_OFFSET			= 0x00048009,			// Set screen virtual offset
	MAILBOX_TAG_SET_OVERSCAN				= 0x0004800A,			// Set screen overscan value
	MAILBOX_TAG_SET_PALETTE					= 0x0004800B,			// Set screen palette
	MAILBOX_TAG_SET_VSYNC					= 0x0004800E,			// Set screen VSync
	MAILBOX_TAG_SET_BACKLIGHT				= 0x0004800F,			// Set screen backlight

	/* VCHIQ commands */
	MAILBOX_TAG_VCHIQ_INIT					= 0x00048010,			// Enable VCHIQ

	/* Config commands */
	MAILBOX_TAG_GET_COMMAND_LINE			= 0x00050001,			// Get command line 

	/* Shared resource management commands */
	MAILBOX_TAG_GET_DMA_CHANNELS			= 0x00060001,			// Get DMA channels

	/* Cursor commands */
	MAILBOX_TAG_SET_CURSOR_INFO				= 0x00008010,			// Set cursor info
	MAILBOX_TAG_SET_CURSOR_STATE			= 0x00008011,			// Set cursor state
} TAG_CHANNEL_COMMAND;

/*--------------------------------------------------------------------------}
{					    ENUMERATED MAILBOX CLOCK ID							}
{		  https://github.com/raspberrypi/firmware/wiki/Mailboxes			}
{--------------------------------------------------------------------------*/
typedef enum {
	CLK_EMMC_ID		= 0x1,								// Mailbox Tag Channel EMMC clock ID 
	CLK_UART_ID		= 0x2,								// Mailbox Tag Channel uart clock ID
	CLK_ARM_ID		= 0x3,								// Mailbox Tag Channel ARM clock ID
	CLK_CORE_ID		= 0x4,								// Mailbox Tag Channel SOC core clock ID
	CLK_V3D_ID		= 0x5,								// Mailbox Tag Channel V3D clock ID
	CLK_H264_ID		= 0x6,								// Mailbox Tag Channel H264 clock ID
	CLK_ISP_ID		= 0x7,								// Mailbox Tag Channel ISP clock ID
	CLK_SDRAM_ID	= 0x8,								// Mailbox Tag Channel SDRAM clock ID
	CLK_PIXEL_ID	= 0x9,								// Mailbox Tag Channel PIXEL clock ID
	CLK_PWM_ID		= 0xA,								// Mailbox Tag Channel PWM clock ID
} MB_CLOCK_ID;


/*--------------------------------------------------------------------------}
{			      ENUMERATED MAILBOX POWER BLOCK ID							}
{		  https://github.com/raspberrypi/firmware/wiki/Mailboxes			}
{--------------------------------------------------------------------------*/
typedef enum {
	PB_SDCARD		= 0x0,								// Mailbox Tag Channel SD Card power block 
	PB_UART0		= 0x1,								// Mailbox Tag Channel UART0 power block 
	PB_UART1		= 0x2,								// Mailbox Tag Channel UART1 power block 
	PB_USBHCD		= 0x3,								// Mailbox Tag Channel USB_HCD power block 
	PB_I2C0			= 0x4,								// Mailbox Tag Channel I2C0 power block 
	PB_I2C1			= 0x5,								// Mailbox Tag Channel I2C1 power block 
	PB_I2C2			= 0x6,								// Mailbox Tag Channel I2C2 power block 
	PB_SPI			= 0x7,								// Mailbox Tag Channel SPI power block 
	PB_CCP2TX		= 0x8,								// Mailbox Tag Channel CCP2TX power block 
} MB_POWER_ID;

/*--------------------------------------------------------------------------}
;{	  ENUMERATED CODE TARGET ... WHICH ARM CPU THE CODE IS COMPILED FOR		}
;{-------------------------------------------------------------------------*/
typedef enum {
	ARM5_CODE = 5,										// ARM 5 CPU is targetted
	ARM6_CODE = 6,										// ARM 6 CPU is targetted
	ARM7_CODE = 7,										// ARM 7 CPU is targetted
	ARM8_CODE = 8,										// ARM 8 CPU is targetted
} ARM_CODE_TYPE;

/*--------------------------------------------------------------------------}
;{	 ENUMERATED AARCH TARGET ... WHICH AARCH TARGET CODE IS COMPILED FOR	}
;{-------------------------------------------------------------------------*/
typedef enum {
	AARCH32 = 0,										// AARCH32 - 32 bit
	AARCH64 = 1,										// AARCH64 - 64 bit
} AARCH_MODE;


/***************************************************************************}
{		 		        PUBLIC STRUCTURE DEFINITIONS			            }
****************************************************************************/

/*--------------------------------------------------------------------------}
{			     RegType_t DEFINES A REGISTER TYPE AND SIZE					}
{--------------------------------------------------------------------------*/
#if __aarch64__ == 1
typedef uint64_t RegType_t;											// Registers are 64bit in AARCH64
#else
typedef uint32_t RegType_t;											// Registers are 32bits in AARCH32
#endif

/*--------------------------------------------------------------------------}
{				 COMPILER TARGET SETTING STRUCTURE DEFINED					}
{--------------------------------------------------------------------------*/
typedef union 
{
	struct 
	{
		ARM_CODE_TYPE ArmCodeTarget : 4;							// @0		Compiler code target
		AARCH_MODE AArchMode : 1;									// @5		Code AARCH type compiler is producing
		unsigned CoresSupported : 3;								// @6		Cores the code is setup to support
		unsigned reserved : 23;										// @9-31	reserved
		unsigned HardFloats : 1;									// @31		Compiler code for hard floats
	};
	uint32_t Raw32;													// Union to access all 32 bits as a uint32_t
} CODE_TYPE;

/*--------------------------------------------------------------------------}
{						ARM CPU ID STRUCTURE DEFINED						}
{--------------------------------------------------------------------------*/
typedef union 
{
	struct 
	{
		unsigned Revision : 4;										// @0-3		CPU minor revision 
		unsigned PartNumber: 12;									// @4-15	Partnumber
		unsigned Architecture : 4;									// @16-19	Architecture
		unsigned Variant : 4;										// @20-23	Variant
		unsigned Implementer : 8;									// @24-31	reserved
	};
	uint32_t Raw32;													// Union to access all 32 bits as a uint32_t
} CPU_ID;

/*--------------------------------------------------------------------------}
{				SMARTSTART VERSION STRUCTURE DEFINED						}
{--------------------------------------------------------------------------*/
typedef union
{
	struct
	{
		unsigned LoVersion : 16;									// @0-15	SmartStart minor version 
		unsigned HiVersion : 8;										// @16-23	SmartStart major version
		unsigned _reserved : 8;										// @24-31	reserved
	};
	uint32_t Raw32;													// Union to access all 32 bits as a uint32_t
} SMARTSTART_VER;

/***************************************************************************}
{                      PUBLIC INTERFACE MEMORY VARIABLES                    }
{***************************************************************************/
extern uint32_t RPi_IO_Base_Addr;				// RPI IO base address auto-detected by SmartStartxx.S
extern uint32_t RPi_ARM_TO_GPU_Alias;			// RPI ARM_TO_GPU_Alias auto-detected by SmartStartxx.S
extern uint32_t RPi_BootAddr;					// RPI address processor booted from auto-detected by SmartStartxx.S
extern uint32_t RPi_CoresReady;					// RPI cpu cores made read for use by SmartStartxx.S
extern uint32_t RPi_CPUBootMode;				// RPI cpu mode it was in when it booted
extern CPU_ID RPi_CpuId;						// RPI CPU type auto-detected by SmartStartxx.S
extern CODE_TYPE RPi_CompileMode;				// RPI code type that compiler produced
extern uint32_t RPi_CPUCurrentMode;				// RPI cpu current operation mode
extern SMARTSTART_VER RPi_SmartStartVer;		// SmartStart version
extern void* RPi_coreCB_PTR[4];					// RPI pointer to the TCB block for each core .. needs to be set by task system	

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++}
{			MMU HELPER ROUTINES PROVIDE BY RPi-SmartStart API			    }
{++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/*-[ enable_mmu_tables ]----------------------------------------------------}
. NOTE: Public C interface only to code located in SmartsStartxx.S
. The given map1to1 TLB table and virtual map TLB table are enabled. The
. assumption is you have built valid TLB tables.
.--------------------------------------------------------------------------*/
void enable_mmu_tables (void* map1to1, void* virtualmap);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++}
{		VC4 GPU ADDRESS HELPER ROUTINES PROVIDE BY RPi-SmartStart API	    }
{++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/*-[ARMaddrToGPUaddr]-------------------------------------------------------}
. NOTE: Public C interface only to code located in SmartsStartxx.S
. Converts an ARM address to GPU address by using the GPU_alias offset
.--------------------------------------------------------------------------*/
uint32_t ARMaddrToGPUaddr (uint32_t ARMaddress);

/*-[GPUaddrToARMaddr]-------------------------------------------------------}
. NOTE: Public C interface only to code located in SmartsStartxx.S
. Converts a GPU address to an ARM address by using the GPU_alias offset
.--------------------------------------------------------------------------*/
uint32_t GPUaddrToARMaddr (uint32_t GPUaddress);

/*==========================================================================}
{		  PUBLIC PI MAILBOX ROUTINES PROVIDED BY RPi-SmartStart API			}
{==========================================================================*/

/*-[mailbox_write]----------------------------------------------------------}
. This will execute the sending of the given data block message thru the
. mailbox system on the given channel. It is normal for a response back so
. usually you need to follow the write up with a read.
. RETURN: True for success, False for failure.
.--------------------------------------------------------------------------*/
bool mailbox_write (MAILBOX_CHANNEL channel, uint32_t message);

/*-[mailbox_read]-----------------------------------------------------------}
. This will read any pending data on the mailbox system on the given channel.
. RETURN: The read value for success, 0xFEEDDEAD for failure.
.--------------------------------------------------------------------------*/
uint32_t mailbox_read (MAILBOX_CHANNEL channel);

/*-[mailbox_tag_message]----------------------------------------------------}
. This will post and execute the given variadic data onto the tags channel
. on the mailbox system. You must provide the correct number of response
. uint32_t variables and a pointer to the response buffer. You nominate the
. number of data uint32_t for the call and fill the variadic data in. If you
. do not want the response data back the use NULL for response_buffer.
. RETURN: True for success and the response data will be set with data
.         False for failure and the response buffer is untouched.
.--------------------------------------------------------------------------*/
bool mailbox_tag_message (uint32_t* response_buf,					// Pointer to response buffer (NULL = no response wanted)
						  uint8_t data_count,						// Number of uint32_t data to be set for call
						  ...);										// Variadic uint32_t values for call

#ifdef __cplusplus								// If we are including to a C++ file
}												// Close the extern C directive wrapper
#endif

#endif

