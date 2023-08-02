#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <cmath>
#include "murmurhash.h"
#include "ap_int.h"

using namespace std;
#define b 1.08          //exponential decay parameter
#define BUCKET_NUM 6400 //the number of bucket 3200*256bits=100KB
#define KEY_SIZE 4      //size of key

#define THRESHOLD 750
#define ListLength 6400 // 6400*4 bytes=25KB

//frequently used value
#define FP_LEN1 8
#define FP_LEN2 12
#define FP_LEN3 16

#define FP_MASK1 0xFF
#define FP_MASK2 0xFFF
#define FP_MASK3 0xFFFF

#define CT_LEN1 7
#define CT_LEN2 10
#define CT_LEN3 15

#define CT_MASK1 0x7F
#define CT_MASK2 0x3FF
#define CT_MASK3 0x7FFF

#define CELL_LEN1 15
#define CELL_LEN2 22
#define CELL_LEN3 31

#define CELL_MASK1 0x7FFF
#define CELL_MASK2 0x3FFFFF
#define CELL_MASK3 0x7FFFFFFF

#define META_LENGTH 6
#define META_MASK 0x3F
#define BUCKET_CELLS_LEGTH 250

#define EXP_MODE_MASK 0x4000
//counters >= this threshold cannot be directly decayed
#define EXP_MODE_T 0x4001
//mask for reset the counting part of the two mode active counter
#define EXP_CT_MASK 0x3FF0
//mask to check if the counting part will overflow
#define EXP_CT_OVFL_MASK 0x7FF0

#define MAX_NUM_LV1 16
#define MAX_NUM_LV2 11
#define MAX_NUM_LV3 8

//value of counters that just Switch (not the represented value of counters, just the value directly read from counter)
#define MIN_C_LV_2 64
#define MIN_C_LV_3 4096

unsigned int packet_c = 0;
unsigned int *packet_p = &packet_c;
unsigned int rann = 0;
unsigned int *rann_p = &rann;

bool half_flag = false;

ap_uint<256> B[BUCKET_NUM];
uint8_t counterExceedThreshold[6000][4];
int numOfStoredCounter = 0;

inline uint32_t BKDRHash(const uint8_t *str, uint32_t len) {
#pragma HLS INTERFACE ap_bus port=*str

    uint32_t seed = 13131;
    uint32_t hash = 0;

    for (uint32_t i = 0; i < len; i++) {
        hash = hash * seed + str[i];
    }

    return (hash & 0x7FFFFFFF);
}

inline uint16_t finger_print(uint32_t hash) {
    hash ^= hash >> 16;
    hash *= 0x85ebca6b;
    hash ^= hash >> 13;
    hash *= 0xc2b2ae35;
    hash ^= hash >> 16;
    return hash & 65535;
}

void metaCodeToData(uint8_t metaCode, int &num_lv_1, int &num_lv_2, int &num_lv_3) {
    switch (metaCode) {
        case 0:
            num_lv_1 = 16;
            num_lv_2 = 0;
            num_lv_3 = 0;
            break;
        case 1:
            num_lv_1 = 15;
            num_lv_2 = 1;
            num_lv_3 = 0;
            break;
        case 2:
            num_lv_1 = 13;
            num_lv_2 = 2;
            num_lv_3 = 0;
            break;
        case 3:
            num_lv_1 = 12;
            num_lv_2 = 3;
            num_lv_3 = 0;
            break;
        case 4:
            num_lv_1 = 10;
            num_lv_2 = 4;
            num_lv_3 = 0;
            break;
        case 5:
            num_lv_1 = 9;
            num_lv_2 = 5;
            num_lv_3 = 0;
            break;
        case 6:
            num_lv_1 = 7;
            num_lv_2 = 6;
            num_lv_3 = 0;
            break;
        case 7:
            num_lv_1 = 6;
            num_lv_2 = 7;
            num_lv_3 = 0;
            break;
        case 8:
            num_lv_1 = 4;
            num_lv_2 = 8;
            num_lv_3 = 0;
            break;
        case 9:
            num_lv_1 = 3;
            num_lv_2 = 9;
            num_lv_3 = 0;
            break;
        case 10:
            num_lv_1 = 2;
            num_lv_2 = 10;
            num_lv_3 = 0;
            break;
        case 11:
            num_lv_1 = 0;
            num_lv_2 = 11;
            num_lv_3 = 0;
            break;
        case 12:
            num_lv_1 = 14;
            num_lv_2 = 0;
            num_lv_3 = 1;
            break;
        case 13:
            num_lv_1 = 13;
            num_lv_2 = 1;
            num_lv_3 = 1;
            break;
        case 14:
            num_lv_1 = 11;
            num_lv_2 = 2;
            num_lv_3 = 1;
            break;
        case 15:
            num_lv_1 = 10;
            num_lv_2 = 3;
            num_lv_3 = 1;
            break;
        case 16:
            num_lv_1 = 8;
            num_lv_2 = 4;
            num_lv_3 = 1;
            break;
        case 17:
            num_lv_1 = 7;
            num_lv_2 = 5;
            num_lv_3 = 1;
            break;
        case 18:
            num_lv_1 = 5;
            num_lv_2 = 6;
            num_lv_3 = 1;
            break;
        case 19:
            num_lv_1 = 4;
            num_lv_2 = 7;
            num_lv_3 = 1;
            break;
        case 20:
            num_lv_1 = 2;
            num_lv_2 = 8;
            num_lv_3 = 1;
            break;
        case 21:
            num_lv_1 = 1;
            num_lv_2 = 9;
            num_lv_3 = 1;
            break;
        case 22:
            num_lv_1 = 12;
            num_lv_2 = 0;
            num_lv_3 = 2;
            break;
        case 23:
            num_lv_1 = 11;
            num_lv_2 = 1;
            num_lv_3 = 2;
            break;
        case 24:
            num_lv_1 = 9;
            num_lv_2 = 2;
            num_lv_3 = 2;
            break;
        case 25:
            num_lv_1 = 8;
            num_lv_2 = 3;
            num_lv_3 = 2;
            break;
        case 26:
            num_lv_1 = 6;
            num_lv_2 = 4;
            num_lv_3 = 2;
            break;
        case 27:
            num_lv_1 = 5;
            num_lv_2 = 5;
            num_lv_3 = 2;
            break;
        case 28:
            num_lv_1 = 3;
            num_lv_2 = 6;
            num_lv_3 = 2;
            break;
        case 29:
            num_lv_1 = 2;
            num_lv_2 = 7;
            num_lv_3 = 2;
            break;
        case 30:
            num_lv_1 = 0;
            num_lv_2 = 8;
            num_lv_3 = 2;
            break;
        case 31:
            num_lv_1 = 10;
            num_lv_2 = 0;
            num_lv_3 = 3;
            break;
        case 32:
            num_lv_1 = 9;
            num_lv_2 = 1;
            num_lv_3 = 3;
            break;
        case 33:
            num_lv_1 = 7;
            num_lv_2 = 2;
            num_lv_3 = 3;
            break;
        case 34:
            num_lv_1 = 6;
            num_lv_2 = 3;
            num_lv_3 = 3;
            break;
        case 35:
            num_lv_1 = 4;
            num_lv_2 = 4;
            num_lv_3 = 3;
            break;
        case 36:
            num_lv_1 = 3;
            num_lv_2 = 5;
            num_lv_3 = 3;
            break;
        case 37:
            num_lv_1 = 1;
            num_lv_2 = 6;
            num_lv_3 = 3;
            break;
        case 38:
            num_lv_1 = 0;
            num_lv_2 = 7;
            num_lv_3 = 3;
            break;
        case 39:
            num_lv_1 = 8;
            num_lv_2 = 0;
            num_lv_3 = 4;
            break;
        case 40:
            num_lv_1 = 6;
            num_lv_2 = 1;
            num_lv_3 = 4;
            break;
        case 41:
            num_lv_1 = 5;
            num_lv_2 = 2;
            num_lv_3 = 4;
            break;
        case 42:
            num_lv_1 = 4;
            num_lv_2 = 3;
            num_lv_3 = 4;
            break;
        case 43:
            num_lv_1 = 2;
            num_lv_2 = 4;
            num_lv_3 = 4;
            break;
        case 44:
            num_lv_1 = 1;
            num_lv_2 = 5;
            num_lv_3 = 4;
            break;
        case 45:
            num_lv_1 = 6;
            num_lv_2 = 0;
            num_lv_3 = 5;
            break;
        case 46:
            num_lv_1 = 4;
            num_lv_2 = 1;
            num_lv_3 = 5;
            break;
        case 47:
            num_lv_1 = 3;
            num_lv_2 = 2;
            num_lv_3 = 5;
            break;
        case 48:
            num_lv_1 = 1;
            num_lv_2 = 3;
            num_lv_3 = 5;
            break;
        case 49:
            num_lv_1 = 0;
            num_lv_2 = 4;
            num_lv_3 = 5;
            break;
        case 50:
            num_lv_1 = 4;
            num_lv_2 = 0;
            num_lv_3 = 6;
            break;
        case 51:
            num_lv_1 = 2;
            num_lv_2 = 1;
            num_lv_3 = 6;
            break;
        case 52:
            num_lv_1 = 1;
            num_lv_2 = 2;
            num_lv_3 = 6;
            break;
        case 53:
            num_lv_1 = 2;
            num_lv_2 = 0;
            num_lv_3 = 7;
            break;
        case 54:
            num_lv_1 = 0;
            num_lv_2 = 1;
            num_lv_3 = 7;
            break;
        case 55:
            num_lv_1 = 0;
            num_lv_2 = 0;
            num_lv_3 = 8;
            break;
        default:
            break;
    }
}

int getMetaCode(int new_num_lv_2, int new_num_lv_3) {
    switch (new_num_lv_3) {
        case 0:
            switch (new_num_lv_2) {
                case 0:
                    return 0;
                case 1:
                    return 1;
                case 2:
                    return 2;
                case 3:
                    return 3;
                case 4:
                    return 4;
                case 5:
                    return 5;
                case 6:
                    return 6;
                case 7:
                    return 7;
                case 8:
                    return 8;
                case 9:
                    return 9;
                case 10:
                    return 10;
                case 11:
                    return 11;
                default:
                    return -1;
            }
        case 1:
            switch (new_num_lv_2) {
                case 0:
                    return 12;
                case 1:
                    return 13;
                case 2:
                    return 14;
                case 3:
                    return 15;
                case 4:
                    return 16;
                case 5:
                    return 17;
                case 6:
                    return 18;
                case 7:
                    return 19;
                case 8:
                    return 20;
                case 9:
                    return 21;
                default:
                    return -1;
            }
        case 2:
            switch (new_num_lv_2) {
                case 0:
                    return 22;
                case 1:
                    return 23;
                case 2:
                    return 24;
                case 3:
                    return 25;
                case 4:
                    return 26;
                case 5:
                    return 27;
                case 6:
                    return 28;
                case 7:
                    return 29;
                case 8:
                    return 30;
                default:
                    return -1;
            }
        case 3:
            switch (new_num_lv_2) {
                case 0:
                    return 31;
                case 1:
                    return 32;
                case 2:
                    return 33;
                case 3:
                    return 34;
                case 4:
                    return 35;
                case 5:
                    return 36;
                case 6:
                    return 37;
                case 7:
                    return 38;
                default:
                    return -1;
            }
        case 4:
            switch (new_num_lv_2) {
                case 0:
                    return 39;
                case 1:
                    return 40;
                case 2:
                    return 41;
                case 3:
                    return 42;
                case 4:
                    return 43;
                case 5:
                    return 44;
                default:
                    return -1;
            }
        case 5:
            switch (new_num_lv_2) {
                case 0:
                    return 45;
                case 1:
                    return 46;
                case 2:
                    return 47;
                case 3:
                    return 48;
                case 4:
                    return 49;
                default:
                    return -1;
            }
        case 6:
            switch (new_num_lv_2) {
                case 0:
                    return 50;
                case 1:
                    return 51;
                case 2:
                    return 52;
                default:
                    return -1;
            }
        case 7:
            switch (new_num_lv_2) {
                case 0:
                    return 53;
                case 1:
                    return 54;
                default:
                    return -1;
            }
        case 8:
            switch (new_num_lv_2) {
                case 0:
                    return 55;
                default:
                    return -1;
            }
    }
    return -1;
}

void setCellFP(ap_uint<256> &bucket, int level, uint16_t fp16, uint32_t cell_start_bit_idx) {
    uint32_t fp_mask;

    switch (level) {
        case 1: {
            fp_mask = FP_MASK1;
            break;
        }
        case 2: {
            fp_mask = FP_MASK2;
            break;
        }
        case 3: {
            fp_mask = FP_MASK3;
            break;
        }
        default : {
            return;
        }
    }
    bucket &= ~(((ap_uint<256>) fp_mask) << (cell_start_bit_idx));
    bucket |= ((ap_uint < 256 > )(fp16 & fp_mask)) << (cell_start_bit_idx);
}

void setCellCounter(ap_uint<256> &bucket, int level, uint16_t newCounter, uint32_t cell_start_bit_idx) {
    uint32_t fp_len;
    uint32_t ct_mask;

    switch (level) {
        case 1: {
            fp_len = FP_LEN1;
            ct_mask = CT_MASK1;
            break;
        }
        case 2: {
            fp_len = FP_LEN2;
            ct_mask = CT_MASK2;
            break;
        }
        case 3: {
            fp_len = FP_LEN3;
            ct_mask = CT_MASK3;
            break;
        }
        default : {
            return;
        }
    }

    cell_start_bit_idx += fp_len;
    bucket &= ~(((ap_uint<256>) ct_mask) << (cell_start_bit_idx));
    bucket |= ((ap_uint < 256 > )(newCounter & ct_mask)) << (cell_start_bit_idx);
}

void setCell(ap_uint<256> &bucket, int level, uint16_t fp16, uint16_t newCounter, uint32_t cell_start_bit_idx) {
    uint32_t fp_len;
    uint32_t fp_mask;
    uint32_t ct_mask;
    uint32_t cell_mask;

    switch (level) {
        case 1: {
            fp_len = FP_LEN1;
            fp_mask = FP_MASK1;
            ct_mask = CT_MASK1;
            cell_mask = CELL_MASK1;
            break;
        }
        case 2: {
            fp_len = FP_LEN2;
            fp_mask = FP_MASK2;
            ct_mask = CT_MASK2;
            cell_mask = CELL_MASK2;
            break;
        }
        case 3: {
            fp_len = FP_LEN3;
            fp_mask = FP_MASK3;
            ct_mask = CT_MASK3;
            cell_mask = CELL_MASK3;
            break;
        }
        default : {
            return;
        }
    }

    bucket &= ~(((ap_uint<256>) cell_mask) << cell_start_bit_idx);
    bucket |= ((ap_uint < 256 > )((fp16 & fp_mask) | ((newCounter & ct_mask) << fp_len))) << (cell_start_bit_idx);
}

void clearCell(ap_uint<256> &bucket, int level, uint32_t cell_start_bit_idx) {
    uint32_t cell_mask;

    switch (level) {
        case 1: {
            cell_mask = CELL_MASK1;
            break;
        }
        case 2: {
            cell_mask = CELL_MASK2;
            break;
        }
        case 3: {
            return;
        }
        default : {
            return;
        }
    }
    bucket &= ~(((ap_uint<256>) cell_mask) << cell_start_bit_idx);
}

void findMinCell(ap_uint<256> &bucket, int level, const int &num_lv1, const int &num_lv2, const int &num_lv3,
                 uint16_t &min_counter, uint16_t &min_index) {
    uint32_t fp_len;
    uint32_t ct_mask;
    uint32_t cell_len;
    uint32_t start, end;


    switch (level) {
        case 1: {
            fp_len = FP_LEN1;
            ct_mask = CT_MASK1;
            cell_len = CELL_LEN1;

            min_counter = -1;
            min_index = -1;

            start = META_LENGTH + num_lv3 * CELL_LEN3 + num_lv2 * CELL_LEN2;
            end = start + num_lv1 * CELL_LEN1;
            break;
        }
        case 2: {
            fp_len = FP_LEN2;
            ct_mask = CT_MASK2;
            cell_len = CELL_LEN2;

            min_counter = -1;
            min_index = -1;

            start = META_LENGTH + num_lv3 * CELL_LEN3;
            end = start + num_lv2 * CELL_LEN2;
            break;
        }
        case 3://only find the counters in normal mode
        {
            fp_len = FP_LEN3;
            ct_mask = CT_MASK3;
            cell_len = CELL_LEN3;

            min_counter = EXP_MODE_T;
            min_index = -1;
            start = META_LENGTH;
            end = start + num_lv3 * CELL_LEN3;
            break;
        }
    }

    uint16_t tmp_counter = 0;
    for (uint32_t j = start; j < end; j += cell_len) {

        tmp_counter = (bucket >> (j + fp_len)) & ct_mask;

        if (tmp_counter < min_counter) {
            min_counter = tmp_counter;
            min_index = j;
        }
    }
}

void
Switch(ap_uint<256> &bucket, int level, const int &num_lv_1, const int &num_lv_2, const int &num_lv_3, uint16_t fp16,
       uint32_t cell_start_bit_idx) {
    int new_num_lv_1 = 0, new_num_lv_2 = 0, new_num_lv_3 = 0; //new number of level_1,level_2,level_3 cells
    int start_lv2 = 0, start_lv1 = 0, end_lv1 = 0;            //start index in bit

    ap_uint<256> temp_bucket = 0;

    int usage_lv_1 = 0, usage_lv_2 = 0;
    int i = 0, j = 0;
    double ranf = 0; //random number for exponential decay
    uint16_t tmp_fp = 0, tmp_counter = 0;
    int metaCode = 0;
    uint16_t min_counter = 0;
    uint16_t min_index = -1;
    uint16_t min_f = 0; //Minimum flow
    int bits_remain = 0;
    if (level == 2) {
        new_num_lv_2 = num_lv_2 + 1; //get new_num_lv_2
        new_num_lv_3 = num_lv_3;     //get new_num_lv_3
        bits_remain = BUCKET_CELLS_LEGTH - CELL_LEN3 * new_num_lv_3 - CELL_LEN2 * new_num_lv_2;
        if (bits_remain < 0) {
            goto levelup_fail;
        }
    } else {
        new_num_lv_2 = num_lv_2 - 1; //get new_num_lv_2
        new_num_lv_3 = num_lv_3 + 1;     //get new_num_lv_3
        bits_remain = BUCKET_CELLS_LEGTH - CELL_LEN3 * new_num_lv_3 - CELL_LEN2 * new_num_lv_2;
        if (bits_remain < 0) {
            new_num_lv_2 = num_lv_2 - 2; //get new_num_lv_2
            new_num_lv_3 = num_lv_3 + 1;     //get new_num_lv_3
            bits_remain = BUCKET_CELLS_LEGTH - CELL_LEN3 * new_num_lv_3 - CELL_LEN2 * new_num_lv_2;
            if (bits_remain < 0) {
                goto levelup_fail;
            }
        }
    }
    new_num_lv_1 = bits_remain / CELL_LEN1;

    start_lv2 = META_LENGTH + num_lv_3 * CELL_LEN3;
    start_lv1 = start_lv2 + num_lv_2 * CELL_LEN2;
    end_lv1 = start_lv2 + num_lv_1 * CELL_LEN1;

    //Temporary store level_3 flows
    for (j = META_LENGTH; j < start_lv2; j += CELL_LEN3) {
        temp_bucket |= (bucket) & (((ap_uint < 256 > )(CELL_MASK3)) << j);
    }
    i = start_lv2;
    //store fp16 when level==3
    if (level == 3) {
        temp_bucket |= (((ap_uint<256>) MIN_C_LV_3 << FP_LEN3) | (fp16 & FP_MASK3)) << i;
        i += CELL_LEN3;
    }

    //store level_2 flows
    for (usage_lv_2 = 0, j = start_lv2; j < start_lv1; j += CELL_LEN2) {
        tmp_fp = (bucket >> j) & FP_MASK2;
        tmp_counter = (bucket >> (j + FP_LEN1)) & CT_MASK2;
        if (tmp_counter && (tmp_fp != (fp16 & FP_MASK2))) {

            if (!min_counter) {
                min_f = tmp_fp;
                min_counter = tmp_counter;
            } else if (tmp_counter < min_counter) {
                temp_bucket |= (((ap_uint<256>) min_counter << FP_LEN2) | (min_f)) << i;
                i += CELL_LEN2;
                min_f = tmp_fp;
                min_counter = tmp_counter;
                usage_lv_2++;
            } else {
                temp_bucket |= (((ap_uint<256>) tmp_counter << FP_LEN2) | (tmp_fp)) << i;
                i += CELL_LEN2;
                usage_lv_2++;
            }
        }
    }
    //if new level_2 cell is not full, store min_f,
    if (min_counter > 0 && usage_lv_2 < new_num_lv_2) {
        temp_bucket |= (((ap_uint<256>) min_counter << FP_LEN2) | (min_f)) << i;
        i += CELL_LEN2;
    }

    //store fp16 when level==2
    if (level == 2) {
        temp_bucket |= (((ap_uint<256>) MIN_C_LV_2 << FP_LEN2) | (fp16 & FP_MASK2)) << i;
        i = i + CELL_LEN2;
    }

    //store level_1 flows
    min_counter = 0;
    for (j = start_lv1; j < end_lv1; j += CELL_LEN1) {
        tmp_fp = (bucket >> j) & FP_MASK1;
        tmp_counter = (bucket >> (j + FP_LEN1)) & CT_MASK1;
        if (tmp_counter && (tmp_fp != (fp16 & FP_MASK1))) {
            if (!min_counter) {
                min_f = tmp_fp;
                min_counter = tmp_counter;
            } else if (tmp_counter < min_counter) {
                temp_bucket |= (((ap_uint<256>) min_counter << FP_LEN1) | (min_f)) << i;
                i += CELL_LEN1;
                min_f = tmp_fp;
                min_counter = tmp_counter;
                usage_lv_1++;
            } else {
                temp_bucket |= (((ap_uint<256>) tmp_counter << FP_LEN1) | (tmp_fp)) << i;
                i += CELL_LEN1;
                usage_lv_1++;
            }
        }
    }
    //if new level_1 cell is not full, store min_f
    if (min_counter > 0 && usage_lv_1 < new_num_lv_1) {
        temp_bucket |= (((ap_uint<256>) min_counter << FP_LEN1) | (min_f)) << i;
    }
    metaCode = getMetaCode(new_num_lv_2, new_num_lv_3);
    temp_bucket |= metaCode & META_MASK;
    bucket = temp_bucket;
    return;
    levelup_fail:
    int min_c_lv = 0;
    int cell_mask = 0;
    int fp_mask = 0;
    int fp_len = 0;
    if (level == 2) {
        min_c_lv = MIN_C_LV_2;
        cell_mask = CELL_MASK1;
        fp_mask = FP_MASK2;
        fp_len = FP_LEN2;
    } else {
        min_c_lv = MIN_C_LV_3;
        cell_mask = CELL_MASK2;
        fp_mask = FP_MASK3;
        fp_len = FP_LEN3;
    }
    findMinCell(bucket, level, num_lv_1, num_lv_2, num_lv_3, min_counter, min_index);
    if (min_counter > EXP_MODE_MASK) {
        return;
    }
    //exponential decay
    MurmurHash3_x86_32(packet_p, 133, rann_p);
    rann = rann / 0x7fffffff;
    if (ranf < pow(b, log2(min_counter) * -1)) {
        if (min_counter == min_c_lv) {
            clearCell(bucket, level - 1, cell_start_bit_idx);
            setCellFP(bucket, level, fp16, min_index);//replace fp
            return;
        } else//decay
        {
            setCellCounter(bucket, level, min_counter - 1, min_index);
            return;
        }
    }
}

void insert(uint8_t *key) {

    uint32_t hash;
    uint16_t bucket_index, fp16;
    uint16_t tmp_fp, tmp_counter;

    uint8_t meta;
    int num_lv_1, num_lv_2, num_lv_3;
    int start_lv2, start_lv1, end_lv1;

    int j;
    uint32_t ran;
    double ranf;

    packet_c++;

    uint8_t quarter_flag = 0;

    hash = BKDRHash(key, KEY_SIZE);
    bucket_index = hash % BUCKET_NUM;         //get bucket index
    fp16 = finger_print(hash);                //get fp
    ap_uint<256> &bucket = B[bucket_index];

    meta = bucket & META_MASK;
    metaCodeToData(meta, num_lv_1, num_lv_2, num_lv_3);

    //each 256-bit bucket is composed of 4 64-bit words, all cells are stored from the lower addresses. In each cell, lower bits are the fingerprint, and higher bits are the counter.
    start_lv2 = META_LENGTH + num_lv_3 * CELL_LEN3;
    start_lv1 = start_lv2 + num_lv_2 * CELL_LEN2;
    end_lv1 = start_lv1 + num_lv_1 * CELL_LEN1;

    //if exists a flow in level_3
    for (j = META_LENGTH; j < start_lv2; j += CELL_LEN3) {
        tmp_fp = (bucket >> j) & FP_MASK3;
        tmp_counter = (bucket >> (j + FP_LEN3)) & CT_MASK3;

        if (tmp_fp == fp16 && tmp_counter > 0) {
            if (!(tmp_counter & EXP_MODE_MASK))//normal mode
            {
                if (tmp_counter == THRESHOLD) {
                    counterExceedThreshold[numOfStoredCounter][0] = key[0];
                    counterExceedThreshold[numOfStoredCounter][0] = key[1];
                    counterExceedThreshold[numOfStoredCounter][0] = key[2];
                    counterExceedThreshold[numOfStoredCounter][0] = key[3];
                    numOfStoredCounter++;
                }
                setCellCounter(bucket, 3, tmp_counter + 1, j);
            } else//exponential mode
            {
                MurmurHash3_x86_32(packet_p, 133, rann_p);
                rann = rann / 0x7fffffff;
                if (ran <= (1 << (31 - 4 - (tmp_counter & 0xF)))) {
                    if (tmp_counter < EXP_CT_OVFL_MASK) {
                        uint32_t newCounter = tmp_counter + 0x10;
                        setCellCounter(bucket, 3, newCounter, j);
                    } else//coefficient part is overflow
                    {
                        uint32_t newCounter = (tmp_counter & (~(((uint32_t) EXP_CT_MASK)))) + 1;
                        setCellCounter(bucket, 3, newCounter, j);
                    }
                }
            }
            goto pkt_done;
        }
    }

    //change when bucket structure changes
    //if existing flow in level_2
    for (j = start_lv2; j < start_lv1; j += CELL_LEN2) {
        tmp_fp = (bucket >> j) & FP_MASK2;
        tmp_counter = (bucket >> (j + FP_LEN2)) & CT_MASK2;

        if (tmp_fp == (fp16 & FP_MASK2) && tmp_counter > 0) {
            if (quarter_flag == 3) {
                if (tmp_counter != CT_MASK2) {
                    if (tmp_counter == int(THRESHOLD / 4)) {
                        counterExceedThreshold[numOfStoredCounter][0] = key[0];
                        counterExceedThreshold[numOfStoredCounter][0] = key[1];
                        counterExceedThreshold[numOfStoredCounter][0] = key[2];
                        counterExceedThreshold[numOfStoredCounter][0] = key[3];
                        numOfStoredCounter++;
                    }
                    setCellCounter(bucket, 2, tmp_counter + 1, j);
                } else {
                    Switch(bucket, 3, num_lv_1, num_lv_2, num_lv_3, fp16, j);
                }
                quarter_flag = 0;
            } else {
                quarter_flag++;
            }
            goto pkt_done;
        }
    }
    //if existing flow in level_1
    for (j = start_lv1; j < end_lv1; j += CELL_LEN1) {
        tmp_fp = (bucket >> j) & FP_MASK1;
        tmp_counter = (bucket >> (j + FP_LEN1)) & CT_MASK1;

        if (tmp_fp == (fp16 & FP_MASK1) && tmp_counter > 0) {
            half_flag = !half_flag;
            if (half_flag) {//0.5 prob to update
                if (tmp_counter != 0x7F) {
                    setCellCounter(bucket, 1, tmp_counter + 1, j);
                } else {
                    Switch(bucket, 2, num_lv_1, num_lv_2, num_lv_3, fp16, j);
                }
            }
            goto pkt_done;
        }
    }

    if (num_lv_1 > 0) {
        uint16_t min_counter = -1;
        uint16_t min_index = -1;
        findMinCell(bucket, 1, num_lv_1, num_lv_2, num_lv_3, min_counter, min_index);
        if (min_counter == 0) {//find empty cell
            half_flag = !half_flag;
            if (half_flag) {
                setCell(bucket, 1, fp16, 1, min_index);
            }

        } else {//exp decay
            MurmurHash3_x86_32(packet_p, 133, rann_p);
            ranf = 1.0 * rann / 0x7fffffff;
            if (ranf < 0.5 * pow(b, log2(min_counter * 2) * -1)) {
                if (min_counter == 1) {//replace fp
                    setCellFP(bucket, 1, fp16, min_index);
                } else {//decay counter
                    setCellCounter(bucket, 1, min_counter - 1, min_index);
                }
            }
        }
    } else if (num_lv_2 > 0) {
        uint16_t min_counter = -1;
        uint16_t min_index = -1;
        findMinCell(bucket, 2, num_lv_1, num_lv_2, num_lv_3, min_counter, min_index);
        if (min_counter == 0) {//find empty cell
            if (quarter_flag == 3) {
                setCell(bucket, 2, fp16, 1, min_index);
                quarter_flag = 0;
            } else {
                quarter_flag++;
            }
        } else {//exp decay
            MurmurHash3_x86_32(packet_p, 133, rann_p);
            ranf = 1.0 * rann / 0x7fffffff;
            if (ranf < 0.25 * pow(b, log2(min_counter * 4) * -1)) {
                if (min_counter == 1) {//replace fp
                    setCellFP(bucket, 2, fp16, min_index);
                } else {//decay counter
                    setCellCounter(bucket, 2, min_counter - 1, min_index);
                }
            }
        }
    } else {
        uint16_t min_counter = EXP_MODE_T;
        uint16_t min_index = -1;
        findMinCell(bucket, 3, num_lv_1, num_lv_2, num_lv_3, min_counter, min_index);
        if (min_counter == 0) {//find empty cell
            setCell(bucket, 3, fp16, 1, min_index);
        } else if (min_counter < EXP_MODE_T) {//exp decay
            MurmurHash3_x86_32(packet_p, 133, rann_p);
            ranf = 1.0 * rann / 0x7fffffff;
            if (ranf < pow(b, log2(min_counter) * -1)) {
                if (min_counter == 1) {//replace fp
                    setCellFP(bucket, 3, fp16, min_index);
                } else {//decay counter
                    setCellCounter(bucket, 3, min_counter - 1, min_index);
                }
            }
        }
    }
    pkt_done:;
}
