#include <stdio.h>
 
#define SIGN_BIT    (0x80)      /* Sign bit for a A-law byte. */  
#define QUANT_MASK  (0xf)       /* Quantization field mask.   */  
#define NSEGS       (8)         /* Number of A-law segments.  */  
#define SEG_SHIFT   (4)         /* Left shift for segment number. */  
#define SEG_MASK    (0x70)      /* Segment field mask. */  
  
static short seg_end[8] = {0xFF, 0x1FF, 0x3FF, 0x7FF,  
                           0xFFF, 0x1FFF, 0x3FFF, 0x7FFF}; // Разделим на 8 нечетных сегментов, посчитаем отрицательные числа и всего 16 сегментов
  
/* copy from CCITT G.711 specifications */  
unsigned char _u2a[128] = { /* u- to A-law conversions */  
    1,  1,  2,  2,  3,  3,  4,  4,  
    5,  5,  6,  6,  7,  7,  8,  8,  
    9,  10, 11, 12, 13, 14, 15, 16,  
    17, 18, 19, 20, 21, 22, 23, 24,  
    25, 27, 29, 31, 33, 34, 35, 36,  
    37, 38, 39, 40, 41, 42, 43, 44,  
    46, 48, 49, 50, 51, 52, 53, 54,  
    55, 56, 57, 58, 59, 60, 61, 62,  
    64, 65, 66, 67, 68, 69, 70, 71,  
    72, 73, 74, 75, 76, 77, 78, 79,  
    81, 82, 83, 84, 85, 86, 87, 88,  
    89, 90, 91, 92, 93, 94, 95, 96,  
    97, 98, 99, 100,101,102,103,104,  
    105,106,107,108,109,110,111,112,  
    113,114,115,116,117,118,119,120,  
    121,122,123,124,125,126,127,128  
};  
  
unsigned char _a2u[128] = { /* A- to u-law conversions */  
    1,  3,  5,  7,  9,  11, 13, 15,  
    16, 17, 18, 19, 20, 21, 22, 23,  
    24, 25, 26, 27, 28, 29, 30, 31,  
    32, 32, 33, 33, 34, 34, 35, 35,  
    36, 37, 38, 39, 40, 41, 42, 43,  
    44, 45, 46, 47, 48, 48, 49, 49,  
    50, 51, 52, 53, 54, 55, 56, 57,  
    58, 59, 60, 61, 62, 63, 64, 64,  
    65, 66, 67, 68, 69, 70, 71, 72,  
    73, 74, 75, 76, 77, 78, 79, 79,  
    80, 81, 82, 83, 84, 85, 86, 87,  
    88, 89, 90, 91, 92, 93, 94, 95,  
    96, 97, 98, 99, 100,101,102,103,  
    104,105,106,107,108,109,110,111,  
    112,113,114,115,116,117,118,119,  
    120,121,122,123,124,125,126,127  
};  
  
static int search(int val,short *table,int size)  
{  
    int     i;  
    for (i = 0; i < size; i++) {  
        if (val <= *table++)  
            return (i);  
    }  
    return (size);  
}  
  
/********************************************************************* 
   * Диапазон входных параметров: -32768 ~ 32767
   * Возвращает 8-битное целое число без знака
 * linear2alaw() - Convert a 16-bit linear PCM value to 8-bit A-law 
 *   
 * linear2alaw() accepts an 16-bit integer and encodes it as A-law data. 
 * 
 *  Linear Input Code       Compressed Code 
 *  -----------------       ------------------ 
 *  0000000wxyza            000wxyz 
 *  0000001wxyza            001wxyz 
 *  000001wxyzab            010wxyz 
 *  00001wxyzabc            011wxyz 
 *  0001wxyzabcd            100wxyz 
 *  001wxyzabcde            101wxyz 
 *  01wxyzabcdef            110wxyz 
 *  1wxyzabcdefg            111wxyz 
 * 
 * For further information see John C. Bellamy's Digital Telephony, 1982, 
 * John Wiley & Sons, pps 98-111 and 472-476. 
 *********************************************************************/  
unsigned char linear2alaw(int pcm_val)  /* 2's complement (16-bit range) */  
{  
    int             mask;  
    int             seg;  
    unsigned char   aval;  
  
    if (pcm_val >= 0) {  
        mask = 0xD5;        /* sign (7th) bit = 1 */  
    } else {  
        mask = 0x55;        /* sign bit = 0 */  
        //pcm_val = -pcm_val - 8;  
        pcm_val = -pcm_val - 1;  
    }  
  
    /* Convert the scaled magnitude to segment number. */  
         seg = search (pcm_val, seg_end, 8); // возвращаемся к какому сегменту принадлежит pcm_val
  
    /* Combine the sign, segment, and quantization bits. */  
  
    if (seg >= 8)        /* out of range, return maximum value. */  
        return (0x7F ^ mask);  
    else {  
        aval = seg << SEG_SHIFT; // aval - это смещение каждого сегмента, данные после квантования сегмента должны добавить смещение (aval)
        // Квантование сегмента
        // Метод квантования: (значение сегмента pcm_val), а затем взять эффективные верхние 4 бита (кроме сегмента 0)
        // Например, pcm_val = 0x7000, тогда seg = 7, диапазон седьмого сегмента 0x4000 ~ 0x7FFF, а смещение сегмента aval = 7 << 4 = 0x7F
        // 0x7000-0x4000 = 0x3000, затем берем 4 эффективных старших бита, то есть сдвигаем вправо на 10 (сегмент + 3), 0x3000 >> 10 = 0xC
        // Предыдущий шаг эквивалентен: (0x7000 >> 10) & 0xF = 0xC. То есть: (pcm_val >> (seg + 3)) & QUANT_MASK
        // Затем добавляем смещение сегмента 0x7F (aval), сложение эквивалентно операции OR, то есть | aval 
        
        if (seg < 2)  
            aval |= (pcm_val >> 4) & QUANT_MASK; // Наклоны сегментов 0 и 1 одинаковы
        else  
            aval |= (pcm_val >> (seg + 3)) & QUANT_MASK; 
                 return (aval ^ mask); // Исключающее ИЛИ 0x55, цель состоит в том, чтобы попытаться избежать непрерывного 0 или непрерывного 1 для повышения надежности процесса передачи
    }  
}  
  
/********************************************************************* 
 *    alaw2linear() - Convert an A-law value to 16-bit linear PCM 
 *********************************************************************/  
int alaw2linear(unsigned char a_val)  
{  
    int     t;  
    int     seg;  
  
    a_val ^= 0x55;  
  
    t = (a_val & QUANT_MASK) << 4;  
    seg = ((unsigned)a_val & SEG_MASK) >> SEG_SHIFT;  
    switch (seg) {  
    case 0:  
        t += 8;  
        break;  
    case 1:  
        t += 0x108;  
        break;  
    default:  
        t += 0x108;  
        t <<= seg - 1;  
    }  
    return ((a_val & SIGN_BIT) ? t : -t);  
}  
 
