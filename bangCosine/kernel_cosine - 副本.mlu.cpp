#include "mlu.h"
#include "marco.h"
#define EPS 0.00001
#define M 256
#define N 256
#define ONELINE 256 //ONELINE* 64
#define ITER_NUM 50
#define INPUTSIZE 65536

__mlu_entry__ void CosineKernel(half *inputX, half *inputY, half *output)
{
    __nram__ half inputX_nram[INPUTSIZE];
    __nram__ half inputY_nram[INPUTSIZE];

    __nram__ half tans_nram[INPUTSIZE];

    __nram__ half calX_nram[ONELINE];
    __nram__ half calY_nram[ONELINE];
    __nram__ half temp1_nram[ONELINE];
    __nram__ half temp2_nram[ONELINE];
    __nram__ half temp3_nram[ONELINE];
    __nram__ half tempXY_nram[ONELINE];
    __nram__ half tempXX_nram[ONELINE];
    __nram__ half tempYY_nram[ONELINE];
    __nram__ half const_nram[ONELINE];
    __nram__ half esp_nram[ONELINE];

    __nramset_half(const_nram, ONELINE, 1);
    __nramset_half(esp_nram, ONELINE, EPS);
    __nramset_half(temp1_nram, ONELINE, 0.0);
    __nramset_half(temp2_nram, ONELINE, 0.0);
    __nramset_half(temp3_nram, ONELINE, 0.0);
    __nramset_half(tempXY_nram, ONELINE, 0.0);
    __nramset_half(tempXX_nram, ONELINE, 0.0);
    __nramset_half(tempYY_nram, ONELINE, 0.0);

    for (int32_t i = 0; i < ONELINE; i++)
    {
        __memcpy(inputX_nram + i * ONELINE, inputX + i * ONELINE, ONELINE * sizeof(half), GDRAM2NRAM);
        __memcpy(inputY_nram + i * ONELINE, inputY + i * ONELINE, ONELINE * sizeof(half), GDRAM2NRAM);
    }


    // calculate sum (Xi * Yi)
    for (int32_t i = 0; i < M; i++)
    {
        // GET LINE DATA INTO CAL NRAM
        __memcpy(calX_nram, inputX_nram + i * ONELINE, ONELINE * sizeof(half), NRAM2NRAM);
        __memcpy(calY_nram, inputY_nram + i * ONELINE, ONELINE * sizeof(half), NRAM2NRAM);

        // __bang_mul_const(calX_nram, calX_nram, 0.005, ONELINE);
        // __bang_mul_const(calY_nram, calY_nram, 0.005, ONELINE);

        // XY
        __bang_mul(temp1_nram, inputX_nram, inputY_nram, ONELINE);     // x*y
        __bang_add(tempXY_nram, temp1_nram, tempXY_nram, ONELINE); // sum +=  x*y

        // XX
        __bang_mul(temp2_nram, inputX_nram, inputX_nram, ONELINE);     // x*x
        __bang_add(tempXX_nram, temp2_nram, tempXX_nram, ONELINE); // sum +=  x*x

        // YY
        __bang_mul(temp3_nram, inputY_nram, inputY_nram, ONELINE);     // y*y
        __bang_add(tempYY_nram, temp3_nram, tempYY_nram, ONELINE); // sum +=  y*y
    }

    // XX=1/(power1/2,Xx)
    __bang_add(tempXX_nram, tempXX_nram, esp_nram, ONELINE);
    __bang_active_sqrt(tempXX_nram, tempXX_nram, ONELINE);
    __bang_active_recip(tempXX_nram, tempXX_nram, ONELINE);
    // YY=1/(power1/2,YY)
    __bang_add(tempYY_nram, tempYY_nram, esp_nram, ONELINE);
    __bang_active_sqrt(tempYY_nram, tempYY_nram, ONELINE);
    __bang_active_recip(tempYY_nram, tempYY_nram, ONELINE);

    __bang_mul(temp2_nram, tempYY_nram, tempXX_nram, ONELINE); // 1 / XX * YY

    __bang_mul(temp3_nram, tempXY_nram, temp2_nram, ONELINE); // XY / XX*


    for (int32_t i = 0; i < ONELINE; i++)
    {
        __memcpy(output + i * ONELINE, temp3_nram, ONELINE * sizeof(half), NRAM2GDRAM);
    }
}
