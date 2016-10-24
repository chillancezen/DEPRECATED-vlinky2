#ifndef _FRONT_CONFIG
#define _FRONT_CONFIG


#define VLINK_VENDOR_ID 0xcccc
#define VLINK_DEVICE_ID 0x2222

#define MAX_LINKS 16
#define DEFAULT_MAX_CHANNEL 64


#define BAR0_OFFSET_COMMON_0_REG 0					/*RD&WR 32-bit wide*/
#define BAR0_OFFSET_COMMON_1_REG 4 					/*RD&WR 32-bit wide*/
#define BAR0_OFFSET_COMMON_2_REG 8 					/*RD&WR 32-bit wide*/
#define BAR0_OFFSET_COMMON_3_REG 12					/*RD&WR 32-bit wide*/
#define BAR0_OFFSET_INTERRUPT_ENABLE_REG 16			/*RD&WR*/
#define BAR0_OFFSET_COMMAND_NR_PAGES_REG 20			/*RD only*/
#define BAR0_OFFSET_COMMAND_HUGEPAGE_LO_ADDRESS_REG 24		/*RD only ,reg0 keep the hugepage index*/
#define BAR0_OFFSET_COMMAND_HUGEPAGE_HI_ADDRESS_REG 28
#define BAR0_OFFSET_COMMAND_LINK_ENABLED_REG 32				/*RD only ,reg0 keep the virtual link index*/
#define BAR0_OFFSET_COMMAMD_LINK_CTRL_CHANNEL_IDX_REG 36	/*RD only ,reg0 keep the virtual link index*/


#endif
