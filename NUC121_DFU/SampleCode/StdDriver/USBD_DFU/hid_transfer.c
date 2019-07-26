/******************************************************************************//**
 * @file     hid_transfer.c
 * @version  V3.00
 * @brief    NUC121 series USBD HID transfer sample file
 *
 * @copyright (C) 2016 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/

/*!<Includes */
#include <stdio.h>
#include <string.h>
#include "NUC121.h"
#include "hid_transfer.h"
#include "dfu.h"
uint8_t dfuBuf[1024];
volatile uint8_t set_report_flag = 0;
volatile uint8_t get_set_report_flag = 0;
extern volatile uint32_t g_usbd_CtrlOutSize;
extern volatile uint32_t g_usbd_CtrlOutSizeLimit;
dfu_status_struct dfu_status;
uint16_t data_len;
uint16_t block_num;
void USBD_IRQHandler(void)
{
    uint32_t u32IntSts = USBD_GET_INT_FLAG();
    uint32_t u32State = USBD_GET_BUS_STATE();

    //------------------------------------------------------------------
    if (u32IntSts & USBD_INTSTS_FLDET) {
        // Floating detect
        USBD_CLR_INT_FLAG(USBD_INTSTS_FLDET);

        if (USBD_IS_ATTACHED()) {
            /* USB Plug In */
            USBD_ENABLE_USB();

            /*Enable HIRC tirm*/
            SYS->IRCTCTL = DEFAULT_HIRC_TRIM_SETTING;
        } else {
            /* USB Un-plug */
            USBD_DISABLE_USB();

            /*Disable HIRC tirm*/
            SYS->IRCTCTL = DEFAULT_HIRC_TRIM_SETTING & (~SYS_IRCTCTL_FREQSEL_Msk);
        }
    }

    //------------------------------------------------------------------
    if (u32IntSts & USBD_INTSTS_BUS) {
        /* Clear event flag */
        USBD_CLR_INT_FLAG(USBD_INTSTS_BUS);

        if (u32State & USBD_STATE_USBRST) {
            /* Bus reset */
            USBD_ENABLE_USB();
            USBD_SwReset();

            /*Enable HIRC tirm*/
            SYS->IRCTCTL = DEFAULT_HIRC_TRIM_SETTING;
        }

        if (u32State & USBD_STATE_SUSPEND) {
            /* Enable USB but disable PHY */
            USBD_DISABLE_PHY();

            /*Disable HIRC tirm*/
            SYS->IRCTCTL = DEFAULT_HIRC_TRIM_SETTING & (~SYS_IRCTCTL_FREQSEL_Msk);

        }

        if (u32State & USBD_STATE_RESUME) {
            /* Enable USB and enable PHY */
            USBD_ENABLE_USB();

            /*Enable HIRC tirm*/
            SYS->IRCTCTL = DEFAULT_HIRC_TRIM_SETTING;
        }




    }

    if (u32IntSts & USBD_INTSTS_NEVWKIF_Msk) {
        /*Clear no-event wake up interrupt */
        USBD_CLR_INT_FLAG(USBD_INTSTS_NEVWKIF_Msk);
        /*
           TODO: Implement the function that will be executed when device is woken by non-USB event.
        */
    }

    //------------------------------------------------------------------
    if (u32IntSts & USBD_INTSTS_USB) {
        // USB event
        if (u32IntSts & USBD_INTSTS_SETUP) {
            // Setup packet
            /* Clear event flag */
            USBD_CLR_INT_FLAG(USBD_INTSTS_SETUP);

            /* Clear the data IN/OUT ready flag of control end-points */
            USBD_STOP_TRANSACTION(EP0);
            USBD_STOP_TRANSACTION(EP1);

            USBD_ProcessSetupPacket();
        }

        // EP events
        if (u32IntSts & USBD_INTSTS_EP0) {
            /* Clear event flag */
            USBD_CLR_INT_FLAG(USBD_INTSTS_EP0);
            // control IN
            USBD_CtrlIn();
        }

        if (u32IntSts & USBD_INTSTS_EP1) {
            /* Clear event flag */
            USBD_CLR_INT_FLAG(USBD_INTSTS_EP1);

            // control OUT
            USBD_CtrlOut();
					
					/* Nuvoton */
					if((set_report_flag == 1) && (g_usbd_CtrlOutSize >= g_usbd_CtrlOutSizeLimit))
								get_set_report_flag = 1;

        }

        if (u32IntSts & USBD_INTSTS_EP2) {
            /* Clear event flag */
            USBD_CLR_INT_FLAG(USBD_INTSTS_EP2);
            // Interrupt IN
          // EP2_Handler();
        }

        if (u32IntSts & USBD_INTSTS_EP3) {
            /* Clear event flag */
            USBD_CLR_INT_FLAG(USBD_INTSTS_EP3);
            // Interrupt OUT
         //   EP3_Handler();
        }

        if (u32IntSts & USBD_INTSTS_EP4) {
            /* Clear event flag */
            USBD_CLR_INT_FLAG(USBD_INTSTS_EP4);
        }

        if (u32IntSts & USBD_INTSTS_EP5) {
            /* Clear event flag */
            USBD_CLR_INT_FLAG(USBD_INTSTS_EP5);
        }

        if (u32IntSts & USBD_INTSTS_EP6) {
            /* Clear event flag */
            USBD_CLR_INT_FLAG(USBD_INTSTS_EP6);
        }

        if (u32IntSts & USBD_INTSTS_EP7) {
            /* Clear event flag */
            USBD_CLR_INT_FLAG(USBD_INTSTS_EP7);
        }
    }
}



/*--------------------------------------------------------------------------*/
/**
  * @brief  USBD Endpoint Config.
  * @param  None.
  * @retval None.
  */
void HID_Init(void)
{
    /* Init setup packet buffer */
    /* Buffer range for setup packet -> [0 ~ 0x7] */
    USBD->STBUFSEG = SETUP_BUF_BASE;

    /*****************************************************/
    /* EP0 ==> control IN endpoint, address 0 */
    USBD_CONFIG_EP(EP0, USBD_CFG_CSTALL | USBD_CFG_EPMODE_IN | 0);
    /* Buffer range for EP0 */
    USBD_SET_EP_BUF_ADDR(EP0, EP0_BUF_BASE);

    /* EP1 ==> control OUT endpoint, address 0 */
    USBD_CONFIG_EP(EP1, USBD_CFG_CSTALL | USBD_CFG_EPMODE_OUT | 0);
    /* Buffer range for EP1 */
    USBD_SET_EP_BUF_ADDR(EP1, EP1_BUF_BASE);

    /*****************************************************/
    /* EP2 ==> Interrupt IN endpoint, address 1 */
    USBD_CONFIG_EP(EP2, USBD_CFG_EPMODE_IN | INT_IN_EP_NUM);
    /* Buffer range for EP2 */
    USBD_SET_EP_BUF_ADDR(EP2, EP2_BUF_BASE);

    /* EP3 ==> Interrupt OUT endpoint, address 2 */
    USBD_CONFIG_EP(EP3, USBD_CFG_EPMODE_OUT | INT_OUT_EP_NUM);
    /* Buffer range for EP3 */
    USBD_SET_EP_BUF_ADDR(EP3, EP3_BUF_BASE);
    /* trigger to receive OUT data */
    USBD_SET_PAYLOAD_LEN(EP3, EP3_MAX_PKT_SIZE);

}





void dfu_init (void)
{
    /* restore device default state */
    dfu_status.bStatus = STATUS_OK;
    dfu_status.bState = STATE_dfuIDLE;
    block_num = 0;
    data_len = 0;
}
unsigned int command_Count=0;

void HID_ClassRequest(void)
{
    uint8_t buf[8];
   uint32_t wValue,wLength;
    USBD_GetSetupPacket(buf);

	wValue=buf[3]<<8|buf[2];
	wLength=buf[7]<<8|buf[6];
	
	if(buf[0] & 0x80)    /* request data transfer direction */	
	 {
		 
	 switch(buf[1])
        {
					
					case DFU_GETSTATUS:
					{				
		USBD_PrepareCtrlIn((uint8_t *)&dfu_status.bStatus, 6);
             USBD_PrepareCtrlOut(0,0);
		break;
	}
						
					case DFU_GETSTATE:
					{	
					
					
						 
						USBD_PrepareCtrlIn((uint8_t *)&dfu_status.bState, 1);
             USBD_PrepareCtrlOut(0,0);
						break;
					}
					
					case DFU_UPLOAD:
					{	
						if(wLength <= 0) {
            dfu_status.bState = STATE_dfuIDLE;
            return;
            }
						
						
						
						 USBD_PrepareCtrlIn((uint8_t *)&dfu_status.bState, 1);
             USBD_PrepareCtrlOut(0,0);
						break;
					}
					
	 default:
            {
                // Stall
                /* Setup error, stall the device */
                USBD_SetStall(0);
                break;
            }
					}
   }
	else
	  {
		  switch(buf[1])
        {
					case DFU_DETACH:
					{
					switch (dfu_status.bState) 
						{
					case STATE_dfuIDLE:
					case STATE_dfuDNLOAD_SYNC:
					case STATE_dfuDNLOAD_IDLE:
					case STATE_dfuMANIFEST_SYNC:
					case STATE_dfuUPLOAD_IDLE:
								dfu_status.bStatus = STATUS_OK;
								dfu_status.bState = STATE_dfuIDLE;
								dfu_status.iString = 0; /* iString */
								block_num = 0;
								data_len = 0;
					      break;
					
					
					
    default:
        break;
    }

					}
					
					case DFU_DNLOAD:
					{
					switch (dfu_status.bState) {
             case STATE_dfuIDLE:
             case STATE_dfuDNLOAD_IDLE:
           if (wLength > 0) {
            /* update the global length and block number */
            block_num = wValue;
            data_len = wLength;
            dfu_status.bState = STATE_dfuDNLOAD_SYNC;

            /* enable EP0 prepare receive the buffer */
					  USBD_PrepareCtrlOut((uint8_t *)dfuBuf, wLength);
		        USBD_PrepareCtrlIn(0,0);
						 
				    set_report_flag = 1;
          }else{
					  USBD_PrepareCtrlOut((uint8_t *)dfuBuf, 0);
		        USBD_PrepareCtrlIn(0,0);
					
					} 

					break;
					
					}
					break;
				}
				

	case DFU_CLRSTATUS:
					{
        dfu_status.bStatus = STATUS_OK;
        dfu_status.bState = STATE_dfuIDLE;
         dfu_status.iString = 0; /* iString: index = 0 */
						break;
						
					}
				
				
				case DFU_ABORT:
					{
				 switch (dfu_status.bState){
    case STATE_dfuIDLE:
    case STATE_dfuDNLOAD_SYNC:
    case STATE_dfuDNLOAD_IDLE:
    case STATE_dfuMANIFEST_SYNC:
    case STATE_dfuUPLOAD_IDLE:
        dfu_status.bStatus = STATUS_OK;
        dfu_status.bState = STATE_dfuIDLE;
        dfu_status.iString = 0; /* iString: index = 0 */

        block_num = 0;
        data_len = 0;
        break;

    default:
        break;
						
	}
						
					}
				 
					
					 default:
            {
                /* Setup error, stall the device */
                USBD_SetStall(0);
                break;
            }
				
		}
	
	}
}
