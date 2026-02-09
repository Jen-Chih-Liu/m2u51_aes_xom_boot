#include "NuMicro.h"
#include "xom_verify.h"
#include <string.h>




static uint32_t XOM_GetVersion(void) { return 0x26260203; }
// ================= ??? =================
#define APROM_START_OFFSET  0x4000      
#define TOTAL_SIZE          (64 * 1024)     // 64 KB
#define SIGNATURE_SIZE      16
#define VERIFY_LEN          (TOTAL_SIZE - SIGNATURE_SIZE)
#define CHUNK_SIZE          256 


static const uint32_t au32XomAESKey[8] = {
    0x41414141, 0x41414141, 0x41414141, 0x41414141, 
    0x00000000, 0x00000000, 0x00000000, 0x00000000 
};
static uint32_t FMC_Read_xom(uint32_t u32Addr)
{
    int32_t i32TimeOutCnt;


    FMC->ISPCMD = FMC_ISPCMD_READ;
    FMC->ISPADDR = u32Addr;
    FMC->ISPTRG = FMC_ISPTRG_ISPGO_Msk;

    i32TimeOutCnt = FMC_TIMEOUT_READ;
    while (FMC->ISPTRG & FMC_ISPTRG_ISPGO_Msk)
    {
        if( i32TimeOutCnt-- <= 0)
        {
            //g_FMC_i32ErrCode = -1;
            return 0xFFFFFFFF;
        }
    }

    return FMC->ISPDAT;
}

void AES_Open(CRPT_T *crpt, uint32_t u32Channel, uint32_t u32EncDec,
              uint32_t u32OpMode, uint32_t u32KeySize, uint32_t u32SwapType)
{
    (void)u32Channel;

    crpt->AES_CTL = (u32EncDec << CRPT_AES_CTL_ENCRPT_Pos) |
                    (u32OpMode << CRPT_AES_CTL_OPMODE_Pos) |
                    (u32KeySize << CRPT_AES_CTL_KEYSZ_Pos) |
                    (u32SwapType << CRPT_AES_CTL_OUTSWAP_Pos);

}

void AES_SetKey(CRPT_T *crpt, uint32_t u32Channel, uint32_t au32Keys[], uint32_t u32KeySize)
{
    uint32_t  i, wcnt, key_reg_addr;

    (void) u32Channel;

    key_reg_addr = (uint32_t)&crpt->AES_KEY[0];
    wcnt = 4UL + u32KeySize * 2UL;

    for(i = 0U; i < wcnt; i++)
    {
        outpw(key_reg_addr, au32Keys[i]);
        key_reg_addr += 4UL;
    }
}

void AES_SetInitVect(CRPT_T *crpt, uint32_t u32Channel, uint32_t au32IV[])
{
    uint32_t  i, key_reg_addr;

    (void) u32Channel;

    key_reg_addr = (uint32_t)&crpt->AES_IV[0];

    for(i = 0U; i < 4U; i++)
    {
        outpw(key_reg_addr, au32IV[i]);
        key_reg_addr += 4UL;
    }
}
void AES_SetDMATransfer(CRPT_T *crpt, uint32_t u32Channel, uint32_t u32SrcAddr,
                        uint32_t u32DstAddr, uint32_t u32TransCnt)
{
    (void) u32Channel;

    crpt->AES_SADDR = u32SrcAddr;
    crpt->AES_DADDR = u32DstAddr;
    crpt->AES_CNT   = u32TransCnt;

}
void AES_Start(CRPT_T *crpt, int32_t u32Channel, uint32_t u32DMAMode)
{
    (void)u32Channel;

    crpt->AES_CTL |= CRPT_AES_CTL_START_Msk | (u32DMAMode << CRPT_AES_CTL_DMALAST_Pos);
}
int32_t XOM_SecureVerify_FMC(XOM_Context_T *ctx)
{
    uint32_t u32CurrAddr = APROM_START_OFFSET;
    uint32_t u32RemLen = VERIFY_LEN;
    uint32_t u32ChunkBytes;
    uint32_t au32IV[4] = {0}; 


    if ((ctx == NULL) || (ctx->pInputBuf == NULL) || (ctx->pOutputTrash == NULL)) {
        return SB_FAIL_INIT;
    }


    SYS_UnlockReg();
    CLK_EnableModuleClock(ISP_MODULE);  // FMC ?
    CLK_EnableModuleClock(CRPT_MODULE); // AES ?
    

    FMC->ISPCTL |= (FMC_ISPCTL_ISPEN_Msk | FMC_ISPCTL_APUEN_Msk);

    AES_Open(CRPT, 0, 1, AES_MODE_CBC, AES_KEY_SIZE_128, AES_IN_OUT_SWAP);

    AES_SetKey(CRPT, 0, (uint32_t*)au32XomAESKey, AES_KEY_SIZE_128);
    AES_SetInitVect(CRPT, 0, au32IV);
    

    AES_CLR_INT_FLAG(CRPT);


    while(u32RemLen > 0)
    {
        if(u32RemLen > CHUNK_SIZE) {
            u32ChunkBytes = CHUNK_SIZE;
        } else {
            u32ChunkBytes = u32RemLen;
        }

 
        for(int i = 0; i < (u32ChunkBytes / 4); i++)
        {
            ctx->pInputBuf[i] = FMC_Read_xom(u32CurrAddr + (i * 4));
        }

  
        AES_SetDMATransfer(CRPT, 0, (uint32_t)ctx->pInputBuf, (uint32_t)ctx->pOutputTrash, u32ChunkBytes);
        

        if (u32RemLen > CHUNK_SIZE) {
            AES_Start(CRPT, 0, CRYPTO_DMA_CONTINUE);
        } else {
            AES_Start(CRPT, 0, CRYPTO_DMA_ONE_SHOT);
        }


        while (AES_GET_INT_FLAG(CRPT) == 0);
        

        AES_CLR_INT_FLAG(CRPT);
        
        // F. ????
        u32RemLen -= u32ChunkBytes;
        u32CurrAddr += u32ChunkBytes;
    }

    // 5. ???????? (?? 16 bytes)
    uint8_t *pCalcSig = &ctx->pOutputTrash[u32ChunkBytes - 16];

    // 6. ?? Flash ???? (?? FMC)
    uint32_t u32SigAddr = APROM_START_OFFSET + VERIFY_LEN;
    uint32_t au32StoredSig[4];
    
    for(int i=0; i<4; i++) {
        au32StoredSig[i] = FMC_Read_xom(u32SigAddr + (i*4));
    }
    uint8_t *pStoredSig = (uint8_t *)au32StoredSig;

    int result = 0;
    for(int i=0; i<16; i++) {
        result |= (pCalcSig[i] ^ pStoredSig[i]);
    }

    if (result == 0) {
        return SB_PASS;
    } else {
        return SB_FAIL_SIGNATURE;
    }
}

__attribute__((section("XOM_ENTRY"), used))
int32_t XOM_Entry(uint32_t cmd, void *pParam)
{
    switch(cmd)
    {
        case XOM_CMD_VERIFY:
            // ? void* ?? Context ??
            if (pParam == NULL) return -1;
            return XOM_SecureVerify_FMC((XOM_Context_T *)pParam);
            
        case XOM_CMD_GET_VER:
            return XOM_GetVersion();
            
        default:
            return -999; // ????
    }
}