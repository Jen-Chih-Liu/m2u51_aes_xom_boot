/***************************************************************************//**
 * @file     main.c
 * @brief    ISP tool main function
 * @version  0x32
 * @date     14, June, 2017
 *
 * @note
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (C) 2017-2018 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/
#include <stdio.h>
#include "targetdev.h"
#include "uart_transfer.h"
int32_t FMC_SetVectorAddr(uint32_t u32PageAddr);
#include "xom_verify.h" // ?? include header
#define aprom_size 64*1024

#ifdef __ICCARM__
    #pragma data_alignment=4
    uint32_t g_XomInputBuf[256 / 4];
    #pragma data_alignment=4
    uint8_t  g_XomOutputTrash[256];
#else
    __attribute__((aligned(4))) uint32_t g_XomInputBuf[256 / 4];
    __attribute__((aligned(4))) uint8_t  g_XomOutputTrash[256];
#endif
void SYS_Init(void)
{
    /*---------------------------------------------------------------------------------------------------------*/
    /* Init System Clock                                                                                       */
    /*---------------------------------------------------------------------------------------------------------*/
    /* Unlock protected registers */
    SYS_UnlockReg();
    /* Enable HIRC clock (Internal RC 16MHz) */
    CLK->PWRCTL |= CLK_PWRCTL_HIRCEN_Msk;

    /* Wait for HIRC clock ready */
    while (!(CLK->STATUS & CLK_STATUS_HIRCSTB_Msk));

    /* Select HCLK clock source as HIRC and HCLK clock divider as 1 */
    CLK->CLKSEL0 = (CLK->CLKSEL0 & (~CLK_CLKSEL0_HCLKSEL_Msk)) | CLK_CLKSEL0_HCLKSEL_HIRC;
    CLK->HCLKDIV = (CLK->HCLKDIV & (~CLK_HCLKDIV_HCLKDIV_Msk)) | CLK_HCLKDIV_HCLK(1);

    /* Enable UART module clock */
    CLK->APBCLK0 |= CLK_APBCLK0_UART0CKEN_Msk;
    /* Select UART module clock source as HIRC and UART module clock divider as 1 */
    CLK->CLKSEL1 = (CLK->CLKSEL2 & (~CLK_CLKSEL2_UART0SEL_Msk)) | CLK_CLKSEL2_UART0SEL_HIRC;
    CLK->CLKDIV = (CLK->CLKDIV & (~CLK_CLKDIV_UART0DIV_Msk)) | CLK_CLKDIV_UART0(1);
    /*---------------------------------------------------------------------------------------------------------*/
    /* Init I/O Multi-function                                                                                 */
    /*---------------------------------------------------------------------------------------------------------*/
    /* Set PB multi-function pins for UART0 RXD=PB.12 and TXD=PB.13 */
    SYS->GPB_MFPH &= ~(SYS_GPB_MFPH_PB12MFP_Msk | SYS_GPB_MFPH_PB13MFP_Msk);
    SYS->GPB_MFPH |= (SYS_GPB_MFPH_PB12MFP_UART0_RXD | SYS_GPB_MFPH_PB13MFP_UART0_TXD);
}
#define XOM_BASE_ADDR    0x0002000

typedef struct
{
    int32_t (*pVerify)(XOM_Context_T *);
    int32_t (*pGetVer)(void);
} XOM_API_T;
/*---------------------------------------------------------------------------------------------------------*/
/*  Main Function                                                                                          */
/*---------------------------------------------------------------------------------------------------------*/
int32_t main(void)
{
    /* Init System, peripheral clock and multi-function I/O */
    SYS_Init();
    /* Init UART to 115200-8n1 */
    UART_Init();

    CLK->AHBCLK0 |= CLK_AHBCLK0_ISPCKEN_Msk;
    FMC->ISPCTL |= (FMC_ISPCTL_ISPEN_Msk | FMC_ISPCTL_APUEN_Msk);
    g_apromSize = aprom_size;//GetApromSize();
    GetDataFlashInfo(&g_dataFlashAddr, &g_dataFlashSize);
    SysTick->LOAD = 300000 * CyclesPerUs;
    SysTick->VAL   = (0x00);
    SysTick->CTRL = SysTick->CTRL | SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk;

    while (1)
    {
        if ((bufhead >= 4) || (bUartDataReady == TRUE))
        {
            uint32_t lcmd;
            lcmd = inpw(uart_rcvbuf);

            if (lcmd == CMD_CONNECT)
            {
                goto _ISP;
            }
            else
            {
                bUartDataReady = FALSE;
                bufhead = 0;
            }
        }

        if (SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk)
        {
			
					
            XOM_Context_T ctx;
            ctx.pInputBuf = g_XomInputBuf;
            ctx.pOutputTrash = g_XomOutputTrash;
            pXomEntryFunc XOM_Call = (pXomEntryFunc)(XOM_BASE_ADDR + 1);
            int32_t ret = XOM_Call(XOM_CMD_VERIFY, &ctx);

            if (ret == SB_PASS)
            {
                //printf("Verify OK. Booting...\n");
                goto _APROM;
            }
            else
            {
                goto _ISP;
            }
		
        }
    }

_ISP:

    while (1)
    {
        if (bUartDataReady == TRUE)
        {
            bUartDataReady = FALSE;
            ParseCmd(uart_rcvbuf, 64);
            PutString();
        }
    }

_APROM:
		FMC->ISPCTL = FMC_ISPCTL_ISPEN_Msk;
		__disable_irq();
NVIC->ICPR[0] = 0xFFFFFFFF;

    FMC_SetVectorAddr(FMC_APROM_BASE);
	//	 SCB->AIRCR = (V6M_AIRCR_VECTKEY_DATA | V6M_AIRCR_SYSRESETREQ);
    //SYS_ResetCPU();
		    SYS->IPRST0 |= SYS_IPRST0_CPURST_Msk;

    /* Trap the CPU */
    while (1);
}
