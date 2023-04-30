//-----------------------------------------------------------------------------
#include "hanlib.h"
//-----------------------------------------------------------------------------
//	①초성의 벌수 결정 : [중성종류][종성유무] */
//-----------------------------------------------------------------------------
static byte _F1_by_F2F3[22][2] = {    /* 벌수는 0 ~ 7까지 8벌 */
    {0, 0},     /* [ 0]    */
    {0, 5},     /* [ 1] ㅏ */
    {0, 5},     /* [ 2] ㅐ */
    {0, 5},     /* [ 3] ㅑ */
    {0, 5},     /* [ 4] ㅒ */
    {0, 5},     /* [ 5] ㅓ */
    {0, 5},     /* [ 6] ㅔ */
    {0, 5},     /* [ 7] ㅕ */
    {0, 5},     /* [ 8] ㅖ */
    {1, 6},     /* [ 9] ㅗ */
    {3, 7},     /* [10] ㅘ */
    {3, 7},     /* [11] ㅙ */
    {3, 7},     /* [12] ㅚ */
    {1, 6},     /* [13] ㅛ */
    {2, 6},     /* [14] ㅜ */
    {4, 7},     /* [15] ㅝ */
    {4, 7},     /* [16] ㅞ */
    {4, 7},     /* [17] ㅟ */
    {2, 6},     /* [18] ㅠ */
    {1, 6},     /* [19] ㅡ */
    {3, 7},     /* [20] ㅢ */
    {0, 5}      /* [21] ㅣ */
};
//-----------------------------------------------------------------------------
//	②중성의 벌수 결정 : [초성종류][종성유뮤] */
//-----------------------------------------------------------------------------
static byte _F2_by_F1F3[20][2] = {    /* 벌수는 0 ~ 3까지 4벌 */
    {1, 3},     /* [ 0]   */
    {0, 2},     /* [ 1] ㄱ*/
    {1, 3},     /* [ 2] ㄲ*/
    {1, 3},     /* [ 3] ㄴ*/
    {1, 3},     /* [ 4] ㄷ*/
    {1, 3},     /* [ 5] ㄸ*/
    {1, 3},     /* [ 6] ㄹ*/
    {1, 3},     /* [ 7] ㅁ*/
    {1, 3},     /* [ 8] ㅂ*/
    {1, 3},     /* [ 9] ㅃ*/
    {1, 3},     /* [10] ㅅ*/
    {1, 3},     /* [11] ㅆ*/
    {1, 3},     /* [12] ㅇ*/
    {1, 3},     /* [13] ㅈ*/
    {1, 3},     /* [14] ㅉ*/
    {1, 3},     /* [15] ㅊ*/
    {0, 2},     /* [16] ㅋ*/
    {1, 3},     /* [17] ㅌ*/
    {1, 3},     /* [18] ㅍ*/
    {1, 3}      /* [19] ㅎ*/
};
//-----------------------------------------------------------------------------
//	③종성의 벌수 결정 : [중성종류] */
//-----------------------------------------------------------------------------
static byte _F3_by_F2[22] = {   /* 벌수는 0 ~ 3까지 4벌 */
    0,  /* [ 0]    */
    0,  /* [ 1] ㅏ */
    2,  /* [ 2] ㅐ */
    0,  /* [ 3] ㅑ */
    2,  /* [ 4] ㅒ */
    1,  /* [ 5] ㅓ */
    2,  /* [ 6] ㅔ */
    1,  /* [ 7] ㅕ */
    2,  /* [ 8] ㅖ */
    3,  /* [ 9] ㅗ */
    0,  /* [10] ㅘ */
    2,  /* [11] ㅙ */
    1,  /* [12] ㅚ */
    3,  /* [13] ㅛ */
    3,  /* [14] ㅜ */
    1,  /* [15] ㅝ */
    2,  /* [16] ㅞ */
    1,  /* [17] ㅟ */
    3,  /* [18] ㅠ */
    3,  /* [19] ㅡ */
    1,  /* [20] ㅢ */
    1   /* [21] ㅣ */
};
//-----------------------------------------------------------------------------
/* ①초성의 벌수 결정 : [중성종류][종성유무] */
/* ②중성의 벌수 결정 : [초성종류][종성유뮤] */
/* ③종성의 벌수 결정 : [중성종류] */

byte *_F1B_8x4x4 = &_F1_by_F2F3[0][false];
byte *_F2B_8x4x4 = &_F2_by_F1F3[0][false];
byte *_F3B_8x4x4 = &_F3_by_F2[0];
//-----------------------------------------------------------------------------
