/******************************************************************************//**
 * @file     main.c
 * @version  V3.00
 * @brief
 *           Demonstrate how to transfer data between USB device and PC through USB HID interface.
 *           A windows tool is also included in this sample code to connect with USB device.
 *
 * @copyright (C) 2016 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/
#include <stdio.h>
#include "NUC121.h"
#include "hid_transfer.h"
#include "dfu.h"

/*--------------------------------------------------------------------------*/
void SYS_Init(void)
{
    /* Init System Clock                                                                                       */
    /*---------------------------------------------------------------------------------------------------------*/
    /* Enable Internal HIRC 48 MHz clock */
    CLK->PWRCTL |= CLK_PWRCTL_HIRCEN;

    /* Waiting for Internal RC clock ready */
    while (!(CLK->STATUS & CLK_STATUS_HIRCSTB_Msk));

    /* Switch HCLK clock source to Internal HIRC and HCLK source divide 1 */
    CLK->CLKSEL0 = (CLK->CLKSEL0 & (~CLK_CLKSEL0_HCLKSEL_Msk)) | CLK_CLKSEL0_HCLKSEL_HIRC;
    CLK->CLKDIV0 = (CLK->CLKDIV0 & (~CLK_CLKDIV0_HCLKDIV_Msk)) | CLK_CLKDIV0_HCLK(1);
    /* Enable module clock */
    CLK->APBCLK0 |= CLK_APBCLK0_USBDCKEN_Msk;
    /* Enable IP clock */
    CLK_SetModuleClock(USBD_MODULE, CLK_CLKSEL3_USBDSEL_HIRC, CLK_CLKDIV0_USB(1));

}



extern void dfu_init (void);
extern volatile uint8_t set_report_flag;
extern volatile uint8_t get_set_report_flag;
extern uint8_t dfuBuf[1024];
extern uint16_t data_len;
extern uint16_t block_num;
extern volatile uint32_t g_usbd_CtrlOutSize;
extern dfu_status_struct dfu_status;
int FMC_Write_User(unsigned int u32Addr, unsigned int u32Data)
{
    unsigned int Reg;
    FMC->ISPCMD = FMC_ISPCMD_PROGRAM;
    FMC->ISPADDR = u32Addr;
    FMC->ISPDAT = u32Data;
    FMC->ISPTRG = 0x1;
    __ISB();
    Reg = FMC->ISPCTL;

    if (Reg & FMC_ISPCTL_ISPFF_Msk)
    {
        FMC->ISPCTL = Reg;
        return -1;
    }

    return 0;
}

int FMC_Erase_User(unsigned int u32Addr)
{
    unsigned int Reg;
    FMC->ISPCMD = FMC_ISPCMD_PAGE_ERASE;
    FMC->ISPADDR = u32Addr;
    FMC->ISPTRG = 0x1;
    __ISB();
    Reg = FMC->ISPCTL;

    if (Reg & FMC_ISPCTL_ISPFF_Msk)
    {
        FMC->ISPCTL = Reg;
        return -1;
    }

    return 0;
}
unsigned int temp_cnt;
unsigned int temp_buffer;
int FlashWrite_SECTOR(uint32_t addr, uint32_t size, uint32_t* buffer)
{
	unsigned int cnt;
	unsigned int buffer_cnt;
	buffer_cnt=0;
	for(cnt=addr;cnt<addr+size;cnt=cnt+4)
	{
		temp_cnt=cnt;
	if (cnt%512==0)
	{
		//printf("er\n\r");
		if(FMC_Erase_User(cnt)!=0)
			return -1;
	}
	temp_buffer=buffer[buffer_cnt];
//	printf("t %x\n\r",temp_buffer);
	if(FMC_Write_User(cnt,buffer[buffer_cnt])!=0)
		 return -1;
	buffer_cnt=buffer_cnt+1;
	}
	return 0;
}
/*---------------------------------------------------------------------------------------------------------*/
/*  Main Function                                                                                          */
/*---------------------------------------------------------------------------------------------------------*/
int32_t main(void)
{
    /*
        This sample code demonstrate how to use HID interface to transfer data
        between PC and USB device.
        A demo window tool are also included in "WindowsTool" directory with this
        sample code. User can use it to test data transfer with this sample code.

    */

    /* Unlock write-protected registers */
    SYS_UnlockReg();

    /* Init system and multi-funcition I/O */
    SYS_Init();
FMC->ISPCTL |= FMC_ISPCTL_ISPEN_Msk;   

    /* Open USB controller */
    USBD_Open(&gsInfo, HID_ClassRequest, NULL);

    /*Init Endpoint configuration for HID */
    HID_Init();

    /* Start USB device */
    USBD_Start();

    /* Enable USB device interrupt */
    NVIC_EnableIRQ(USBD_IRQn);
		dfu_init();
    while(1)
		{
				if((set_report_flag == 1) && (get_set_report_flag == 1))
        {
					  dfu_status.bState = STATE_dfuDNLOAD_SYNC;
						/* clear set report flag */
						set_report_flag = 0;
						get_set_report_flag = 0;   
					//  printf("len=%d\n\r",data_len);
				//	  printf("block_num=%d\n\r",block_num);
					  if(FlashWrite_SECTOR(block_num*1024,data_len,(unsigned int*)dfuBuf)!=0)
					     dfu_status.bState = STATE_dfuERROR;
					  else
               dfu_status.bState = STATE_dfuDNLOAD_IDLE;
				}	
		}
}



/*** (C) COPYRIGHT 2016 Nuvoton Technology Corp. ***/

