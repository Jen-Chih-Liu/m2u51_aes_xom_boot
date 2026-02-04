#ifndef __XOM_VERIFY_H__
#define __XOM_VERIFY_H__

#include <stdint.h>


#define SB_PASS              (0)
#define SB_FAIL_SIGNATURE    (-1)
#define SB_FAIL_INIT         (-2)


typedef struct
{
    uint32_t *pInputBuf;      // ???? 256 bytes ? Buffer (4-byte Aligned)
    uint8_t  *pOutputTrash;   // ???? 256 bytes ? Buffer (4-byte Aligned)
} XOM_Context_T;

#define XOM_CMD_VERIFY   0
#define XOM_CMD_GET_VER  1

// ????????????
// pParam ???????,?? cmd ??
typedef int32_t (*pXomEntryFunc)(uint32_t cmd, void *pParam);
//int32_t XOM_SecureVerify_FMC(XOM_Context_T *ctx);

#endif