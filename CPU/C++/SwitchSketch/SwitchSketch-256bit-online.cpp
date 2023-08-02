#include <vector>
#include <string.h>
#include <iostream>
#include <cmath>
#include <unordered_map>
#include <algorithm>
#include <fstream>
#include <iomanip>

using namespace std;
#define b 1.08          //exponential decay parameter

#define BUCKET_NUM 3200

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


uint64_t B[BUCKET_NUM][4]{0};//array of buckets
uint8_t **keys = (uint8_t **) malloc(sizeof(uint8_t * ) * 40000000); //array of keys
uint8_t savedKeys[ListLength][KEY_SIZE]{0};
uint32_t num_of_saved_keys = 0;

inline uint32_t BKDRHash(const uint8_t *str, uint32_t len) {
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

bool cmpPairFunc(pair<uint8_t *, int> p1, pair<uint8_t *, int> p2) {
    return p1.second > p2.second;
}

struct CmpFunc {
    bool operator()(const uint8_t *keyA, const uint8_t *keyB) const {
        return memcmp(keyA, keyB, KEY_SIZE) == 0;
    }
};

struct HashFunc {
    uint32_t operator()(const uint8_t *key) const {
        uint32_t hashValue = BKDRHash(key, KEY_SIZE);
        return hashValue;
    }
};

inline void metaCodeToData(uint8_t metaCode, int &num_lv_1, int &num_lv_2, int &num_lv_3) {
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
            break;
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
            break;
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
            break;
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
            break;
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
            break;
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
            break;
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
            break;
        case 7:
            switch (new_num_lv_2) {
                case 0:
                    return 53;
                case 1:
                    return 54;
                default:
                    return -1;
            }
            break;
        case 8:
            switch (new_num_lv_2) {
                case 0:
                    return 55;
                default:
                    return -1;
            }
            break;
    }
    cout << "encode error!";
    return -1;
}

void findMinCell(uint64_t *bucket, int level, const int &num_lv1, const int &num_lv2, const int &num_lv3,
                 uint16_t &min_counter, uint16_t &min_index) {
    switch (level) {
        case 1: {
            min_counter = -1;
            min_index = -1;
            uint32_t start_lv1 = META_LENGTH + num_lv3 * CELL_LEN3 + num_lv2 * CELL_LEN2;
            uint32_t end_lv1 = start_lv1 + num_lv1 * CELL_LEN1;

            uint16_t tmp_counter = 0;
            for (uint32_t j = start_lv1 + FP_LEN1; j < end_lv1; j += CELL_LEN1) {
                tmp_counter = ((*(uint32_t * )((uint8_t *) bucket + (j >> 3))) >> (j & 0x7)) & CT_MASK1;

                if (tmp_counter < min_counter) {
                    min_counter = tmp_counter;
                    min_index = j - FP_LEN1;
                }
            }
            return;
        }
        case 2: {
            min_counter = -1;
            min_index = -1;
            uint32_t start_lv2 = META_LENGTH + num_lv3 * CELL_LEN3;
            uint32_t end24 = start_lv2 + num_lv2 * CELL_LEN2;

            uint16_t tmp_counter = 0;
            for (uint32_t j = start_lv2 + FP_LEN2; j < end24; j += CELL_LEN2) {
                tmp_counter = ((*(uint32_t * )((uint8_t *) bucket + (j >> 3))) >> (j & 0x7)) & CT_MASK2;

                if (tmp_counter < min_counter) {
                    min_counter = tmp_counter;
                    min_index = j - FP_LEN2;
                }
            }
            return;
        }
        case 3://only find the counters in normal mode
        {
            min_counter = EXP_MODE_T;
            min_index = -1;
            uint16_t tmp_counter = 0;
            uint32_t start_lv2 = META_LENGTH + num_lv3 * CELL_LEN3;

            for (uint32_t j = META_LENGTH + FP_LEN3; j < start_lv2; j += CELL_LEN3) {
                tmp_counter = ((*(uint32_t * )((uint8_t *) bucket + (j >> 3))) >> (j & 0x7)) & CT_MASK3;

                if (tmp_counter < min_counter) {
                    min_counter = tmp_counter;
                    min_index = j - FP_LEN3;
                }
            }
            return;
        }
    }
}

void Switch(uint64_t *bucket, int level, const int &num_lv_1, const int &num_lv_2, const int &num_lv_3, uint16_t fp16,
            uint32_t cell_start_bit_idx) {
    int new_num_lv_1 = 0, new_num_lv_2 = 0, new_num_lv_3 = 0; //new number of level_1,level_2,level_3 cells
    int start_lv2 = 0, start_lv1 = 0, end_lv1 = 0;            //start index in bit
    int new_start_lv2 = 0, new_start_lv1 = 0;      //new start index in bit


    uint8_t cell_lv_1[MAX_NUM_LV1][2] = {0}; //Temporary variable to store flow in the bucket
    uint16_t cell_lv_2[MAX_NUM_LV2][2] = {0};
    uint16_t cell_lv_3[MAX_NUM_LV3][2] = {0};

    int usage_lv_1 = 0, usage_lv_2 = 0, usage_lv_3 = 0;        //number of used level_1,level_2,level_3 cells
    int i = 0, j = 0;
    double ranf = 0; //random number for exponential decay
    uint16_t tmp_fp = 0, tmp_counter = 0;

    uint16_t min_counter = 0;
    uint16_t min_index = -1;

    switch (level) {
        case 2: {
            new_num_lv_2 = num_lv_2 + 1; //get new_num_lv_2
            new_num_lv_3 = num_lv_3;     //get new_num_lv_3
            int bits_remain = BUCKET_CELLS_LEGTH - CELL_LEN3 * new_num_lv_3 - CELL_LEN2 * new_num_lv_2;

            //levelup success
            if (bits_remain >= 0) {
                new_num_lv_1 = bits_remain / CELL_LEN1;

                start_lv2 = META_LENGTH + num_lv_3 * CELL_LEN3;
                start_lv1 = start_lv2 + num_lv_2 * CELL_LEN2;
                end_lv1 = start_lv1 + num_lv_1 * CELL_LEN1;

                //Temporary store level_3 flows
                for (usage_lv_3 = 0, j = META_LENGTH; j < start_lv2; j += CELL_LEN3) {
                    tmp_fp = ((*(uint32_t * )((uint8_t *) bucket + (j >> 3))) >> (j & 0x7)) & FP_MASK3;
                    tmp_counter =
                            ((*(uint32_t * )((uint8_t *) bucket + ((j + FP_LEN3) >> 3))) >> ((j + FP_LEN3) & 0x7)) &
                            CT_MASK3;

                    if (tmp_counter > 0) {
                        cell_lv_3[usage_lv_3][0] = tmp_fp;
                        cell_lv_3[usage_lv_3][1] = tmp_counter;
                        usage_lv_3++;
                    }
                }
                //Temporary store level_2 flows
                for (usage_lv_2 = 0, j = start_lv2; j < start_lv1; j += CELL_LEN2) {
                    tmp_fp = ((*(uint32_t * )((uint8_t *) bucket + (j >> 3))) >> (j & 0x7)) & FP_MASK2;
                    tmp_counter =
                            ((*(uint32_t * )((uint8_t *) bucket + ((j + FP_LEN2) >> 3))) >> ((j + FP_LEN2) & 0x7)) &
                            CT_MASK2;

                    if (tmp_counter > 0) {
                        cell_lv_2[usage_lv_2][0] = tmp_fp;
                        cell_lv_2[usage_lv_2][1] = tmp_counter;
                        usage_lv_2++;
                    }
                }
                //Temporary store  fp16
                cell_lv_2[usage_lv_2][0] = fp16 & FP_MASK2;
                cell_lv_2[usage_lv_2][1] = MIN_C_LV_2;
                usage_lv_2++;

                uint16_t min_f = 0; //Minimum flow
                //Temporary store level_1 flows except fp16 and min_f
                for (j = start_lv1; j < end_lv1; j += CELL_LEN1) {
                    tmp_fp = ((*(uint32_t * )((uint8_t *) bucket + (j >> 3))) >> (j & 0x7)) & FP_MASK1;
                    tmp_counter =
                            ((*(uint32_t * )((uint8_t *) bucket + ((j + FP_LEN1) >> 3))) >> ((j + FP_LEN1) & 0x7)) &
                            CT_MASK1;

                    if (tmp_counter && (tmp_fp != (fp16 & FP_MASK1))) {
                        if (!min_counter) {
                            min_f = tmp_fp;
                            min_counter = tmp_counter;
                        } else if (tmp_counter < min_counter) {
                            cell_lv_1[usage_lv_1][0] = min_f;
                            cell_lv_1[usage_lv_1][1] = min_counter;
                            min_f = tmp_fp;
                            min_counter = tmp_counter;
                            usage_lv_1++;
                        } else {
                            cell_lv_1[usage_lv_1][0] = tmp_fp;
                            cell_lv_1[usage_lv_1][1] = tmp_counter;
                            usage_lv_1++;
                        }
                    }
                }

                //if new level_1 cell is not full, store min_f
                if (min_counter > 0 && usage_lv_1 < new_num_lv_1) {
                    cell_lv_1[usage_lv_1][0] = min_f;
                    cell_lv_1[usage_lv_1][1] = min_counter;
                    usage_lv_1++;
                }
                //Temporary store finished

                //flush bucket
                //change when bucket structure changes
                bucket[0] = 0;
                bucket[1] = 0;
                bucket[2] = 0;
                bucket[3] = 0;

                new_start_lv2 = META_LENGTH + new_num_lv_3 * CELL_LEN3;
                new_start_lv1 = new_start_lv2 + new_num_lv_2 * CELL_LEN2;


                //insert stored level_3 flows
                for (j = META_LENGTH, i = 0; i < usage_lv_3; i++, j += CELL_LEN3) {
                    *(uint32_t * )((uint8_t *) bucket + (j >> 3)) |= ((uint32_t) cell_lv_3[i][0]) << (j & 0x7);
                    *(uint32_t * )((uint8_t *) bucket + ((j + FP_LEN3) >> 3)) |=
                            ((uint32_t) cell_lv_3[i][1]) << ((j + FP_LEN3) & 0x7);
                }
                //insert stored level_2 flows
                for (j = new_start_lv2, i = 0; i < usage_lv_2; i++, j += CELL_LEN2) {
                    *(uint32_t * )((uint8_t *) bucket + (j >> 3)) |= ((uint32_t) cell_lv_2[i][0]) << (j & 0x7);
                    *(uint32_t * )((uint8_t *) bucket + ((j + FP_LEN2) >> 3)) |=
                            ((uint32_t) cell_lv_2[i][1]) << ((j + FP_LEN2) & 0x7);
                }
                //insert stored level_1 flows
                for (j = new_start_lv1, i = 0; i < usage_lv_1; i++, j += CELL_LEN1) {
                    *(uint32_t * )((uint8_t *) bucket + (j >> 3)) |= ((uint32_t) cell_lv_1[i][0]) << (j & 0x7);
                    *(uint32_t * )((uint8_t *) bucket + ((j + FP_LEN1) >> 3)) |=
                            ((uint32_t) cell_lv_1[i][1]) << ((j + FP_LEN1) & 0x7);
                }

                int metaCode = getMetaCode(new_num_lv_2, new_num_lv_3);
                bucket[0] |= metaCode & META_MASK;
                return;
            } else//levelup not success,exponential decay, change when bucket structure changes
            {
                findMinCell(bucket, 2, num_lv_1, num_lv_2, num_lv_3, min_counter, min_index);
                //exponential decay
                ranf = 1.0 * rand() / RAND_MAX;
                if (ranf < 0.25 * pow(b, log2(min_counter * 4) * -1)) {
                    if (min_counter <= MIN_C_LV_2) {
                        *(uint32_t * )((uint8_t *) bucket + (cell_start_bit_idx >> 3)) &= ~(((uint32_t) CELL_MASK1)
                                << (cell_start_bit_idx & 0x7));//clear original cell

                        //replace fp
                        *(uint32_t * )((uint8_t *) bucket + (min_index >> 3)) &= ~(((uint32_t) FP_MASK2)
                                << (min_index & 0x7));
                        *(uint32_t * )((uint8_t *) bucket + (min_index >> 3)) |=
                                ((uint32_t) fp16 & FP_MASK2) << (min_index & 0x7);
                        *(uint32_t * )((uint8_t *) bucket + ((min_index + FP_LEN2) >> 3)) &= ~(((uint32_t) FP_MASK2)
                                << ((min_index + FP_LEN2) & 0x7));
                        *(uint32_t * )((uint8_t *) bucket + ((min_index + FP_LEN2) >> 3)) |=
                                ((uint32_t) MIN_C_LV_2) << ((min_index + FP_LEN2) & 0x7);
                        return;
                    } else//decay
                    {
                        *(uint32_t * )((uint8_t *) bucket + ((min_index + FP_LEN2) >> 3)) -=
                                ((uint32_t) 1) << ((min_index + FP_LEN2) & 0x7);
                        return;
                    }
                }
            }
            break;
        }
        case 3: {
            new_num_lv_2 = num_lv_2 - 1; //get new_num_lv_2
            new_num_lv_3 = num_lv_3 + 1;     //get new_num_lv_3
            int bits_remain = BUCKET_CELLS_LEGTH - CELL_LEN3 * new_num_lv_3 - CELL_LEN2 * new_num_lv_2;

            if (bits_remain >= 0) {//remove a CELL_LEN1-bit cell and the original CELL_LEN2-bit cell
                new_num_lv_1 = bits_remain / CELL_LEN1;

                start_lv2 = META_LENGTH + num_lv_3 * CELL_LEN3;
                start_lv1 = start_lv2 + num_lv_2 * CELL_LEN2;
                end_lv1 = start_lv1 + num_lv_1 * CELL_LEN1;

                for (usage_lv_3 = 0, j = META_LENGTH; j < start_lv2; j += CELL_LEN3) {

                    tmp_fp = ((*(uint32_t * )((uint8_t *) bucket + (j >> 3))) >> (j & 0x7)) & FP_MASK3;
                    tmp_counter =
                            ((*(uint32_t * )((uint8_t *) bucket + ((j + FP_LEN3) >> 3))) >> ((j + FP_LEN3) & 0x7)) &
                            CT_MASK3;

                    if (tmp_counter > 0) {
                        cell_lv_3[usage_lv_3][0] = tmp_fp;
                        cell_lv_3[usage_lv_3][1] = tmp_counter;
                        usage_lv_3++;
                    }
                }
                //store fp16
                cell_lv_3[usage_lv_3][0] = fp16;
                cell_lv_3[usage_lv_3][1] = MIN_C_LV_3;
                usage_lv_3++;

                //Temporary store level_2 flows except fp16
                for (usage_lv_2 = 0, j = start_lv2; j < start_lv1; j += CELL_LEN2) {
                    tmp_fp = ((*(uint32_t * )((uint8_t *) bucket + (j >> 3))) >> (j & 0x7)) & FP_MASK2;
                    tmp_counter =
                            ((*(uint32_t * )((uint8_t *) bucket + ((j + FP_LEN2) >> 3))) >> ((j + FP_LEN2) & 0x7)) &
                            CT_MASK2;

                    if (tmp_counter && (tmp_fp != (fp16 & FP_MASK2))) {
                        cell_lv_2[usage_lv_2][0] = tmp_fp;
                        cell_lv_2[usage_lv_2][1] = tmp_counter;
                        usage_lv_2++;
                    }
                }

                uint16_t min_f = 0; //Minimum flow
                //Temporary store level_1 flows except the min
                for (j = start_lv1; j < end_lv1; j += CELL_LEN1) {
                    tmp_fp = ((*(uint32_t * )((uint8_t *) bucket + (j >> 3))) >> (j & 0x7)) & FP_MASK1;
                    tmp_counter =
                            ((*(uint32_t * )((uint8_t *) bucket + ((j + FP_LEN1) >> 3))) >> ((j + FP_LEN1) & 0x7)) &
                            CT_MASK1;

                    if (tmp_counter) {
                        if (!min_counter) {
                            min_f = tmp_fp;
                            min_counter = tmp_counter;
                        } else if (tmp_counter < min_counter) {
                            cell_lv_1[usage_lv_1][0] = min_f;
                            cell_lv_1[usage_lv_1][1] = min_counter;
                            min_f = tmp_fp;
                            min_counter = tmp_counter;
                            usage_lv_1++;
                        } else {
                            cell_lv_1[usage_lv_1][0] = tmp_fp;
                            cell_lv_1[usage_lv_1][1] = tmp_counter;
                            usage_lv_1++;
                        }
                    }
                }
                //if new level_1 cell is not full, store min_f
                if (usage_lv_1 < new_num_lv_1) {
                    cell_lv_1[usage_lv_1][0] = min_f;
                    cell_lv_1[usage_lv_1][1] = min_counter;
                    usage_lv_1++;
                }
                //Temporary store finished

                //flush bucket
                //change when bucket structure changes
                bucket[0] = 0;
                bucket[1] = 0;
                bucket[2] = 0;
                bucket[3] = 0;

                new_start_lv2 = META_LENGTH + new_num_lv_3 * CELL_LEN3;
                new_start_lv1 = new_start_lv2 + new_num_lv_2 * CELL_LEN2;


                //insert stored level_3 flows
                for (j = META_LENGTH, i = 0; i < usage_lv_3; i++, j += CELL_LEN3) {
                    *(uint32_t * )((uint8_t *) bucket + (j >> 3)) |= ((uint32_t) cell_lv_3[i][0]) << (j & 0x7);
                    *(uint32_t * )((uint8_t *) bucket + ((j + FP_LEN3) >> 3)) |=
                            ((uint32_t) cell_lv_3[i][1]) << ((j + FP_LEN3) & 0x7);
                }
                //insert stored level_2 flows
                for (j = new_start_lv2, i = 0; i < usage_lv_2; i++, j += CELL_LEN2) {
                    *(uint32_t * )((uint8_t *) bucket + (j >> 3)) |= ((uint32_t) cell_lv_2[i][0]) << (j & 0x7);
                    *(uint32_t * )((uint8_t *) bucket + ((j + FP_LEN2) >> 3)) |=
                            ((uint32_t) cell_lv_2[i][1]) << ((j + FP_LEN2) & 0x7);
                }
                //insert stored level_1 flows
                for (j = new_start_lv1, i = 0; i < usage_lv_1; i++, j += CELL_LEN1) {
                    *(uint32_t * )((uint8_t *) bucket + (j >> 3)) |= ((uint32_t) cell_lv_1[i][0]) << (j & 0x7);
                    *(uint32_t * )((uint8_t *) bucket + ((j + FP_LEN1) >> 3)) |=
                            ((uint32_t) cell_lv_1[i][1]) << ((j + FP_LEN1) & 0x7);
                }
                int metaCode = getMetaCode(new_num_lv_2, new_num_lv_3);
                bucket[0] |= metaCode & META_MASK;
                return;
            }

            new_num_lv_2 = num_lv_2 - 2; //get new_num_lv_2
            new_num_lv_3 = num_lv_3 + 1;     //get new_num_lv_3
            bits_remain = BUCKET_CELLS_LEGTH - CELL_LEN3 * new_num_lv_3 - CELL_LEN2 * new_num_lv_2;

            if (bits_remain >= 0) {//remove another CELL_LEN2-bit cell and the original CELL_LEN2-bit cell
                new_num_lv_1 = bits_remain / CELL_LEN1;

                start_lv2 = META_LENGTH + num_lv_3 * CELL_LEN3;
                start_lv1 = start_lv2 + num_lv_2 * CELL_LEN2;
                end_lv1 = start_lv2 + num_lv_1 * CELL_LEN1;

                //Temporary store level_3 flows
                for (usage_lv_3 = 0, j = META_LENGTH; j < start_lv2; j += CELL_LEN3) {
                    tmp_fp = ((*(uint32_t * )((uint8_t *) bucket + (j >> 3))) >> (j & 0x7)) & FP_MASK3;
                    tmp_counter =
                            ((*(uint32_t * )((uint8_t *) bucket + ((j + FP_LEN3) >> 3))) >> ((j + FP_LEN3) & 0x7)) &
                            CT_MASK3;

                    if (tmp_counter > 0) {
                        cell_lv_3[usage_lv_3][0] = tmp_fp;
                        cell_lv_3[usage_lv_3][1] = tmp_counter;
                        usage_lv_3++;
                    }
                }
                //store fp16
                cell_lv_3[usage_lv_3][0] = fp16;
                cell_lv_3[usage_lv_3][1] = MIN_C_LV_3;
                usage_lv_3++;

                uint16_t min_f = 0; //Minimum flow
                //Temporary store level_2 flows except fp16 and min
                for (usage_lv_2 = 0, j = start_lv2; j < start_lv1; j += CELL_LEN2) {
                    tmp_fp = ((*(uint32_t * )((uint8_t *) bucket + (j >> 3))) >> (j & 0x7)) & FP_MASK2;
                    tmp_counter =
                            ((*(uint32_t * )((uint8_t *) bucket + ((j + FP_LEN2) >> 3))) >> ((j + FP_LEN2) & 0x7)) &
                            CT_MASK2;

                    if (tmp_counter && (tmp_fp != (fp16 & FP_MASK2))) {

                        if (!min_counter) {
                            min_f = tmp_fp;
                            min_counter = tmp_counter;
                        } else if (tmp_counter < min_counter) {
                            cell_lv_2[usage_lv_2][0] = min_f;
                            cell_lv_2[usage_lv_2][1] = min_counter;
                            min_f = tmp_fp;
                            min_counter = tmp_counter;
                            usage_lv_2++;
                        } else {
                            cell_lv_2[usage_lv_2][0] = tmp_fp;
                            cell_lv_2[usage_lv_2][1] = tmp_counter;
                            usage_lv_2++;
                        }
                    }
                }

                //if new level_2 cell is not full, store min_f
                if (min_counter > 0 && usage_lv_2 < new_num_lv_2) {
                    cell_lv_2[usage_lv_1][0] = min_f;
                    cell_lv_2[usage_lv_1][1] = min_counter;
                    usage_lv_2++;
                }

                //Temporary store level_1 flows except the min
                for (j = start_lv1; j < end_lv1; j += CELL_LEN1) {
                    tmp_fp = ((*(uint32_t * )((uint8_t *) bucket + (j >> 3))) >> (j & 0x7)) & FP_MASK1;
                    tmp_counter =
                            ((*(uint32_t * )((uint8_t *) bucket + ((j + FP_LEN1) >> 3))) >> ((j + FP_LEN1) & 0x7)) &
                            CT_MASK1;

                    if (tmp_counter) {
                        cell_lv_1[usage_lv_1][0] = tmp_fp;
                        cell_lv_1[usage_lv_1][1] = tmp_counter;
                        usage_lv_1++;
                    }
                }
                //Temporary store finished

                //flush bucket
                //change when bucket structure changes
                bucket[0] = 0;
                bucket[1] = 0;
                bucket[2] = 0;
                bucket[3] = 0;

                new_start_lv2 = META_LENGTH + new_num_lv_3 * CELL_LEN3;
                new_start_lv1 = new_start_lv2 + new_num_lv_2 * CELL_LEN2;


                //insert stored level_3 flows
                for (j = META_LENGTH, i = 0; i < usage_lv_3; i++, j += CELL_LEN3) {
                    *(uint32_t * )((uint8_t *) bucket + (j >> 3)) |= ((uint32_t) cell_lv_3[i][0]) << (j & 0x7);
                    *(uint32_t * )((uint8_t *) bucket + ((j + FP_LEN3) >> 3)) |=
                            ((uint32_t) cell_lv_3[i][1]) << ((j + FP_LEN3) & 0x7);
                }
                //insert stored level_2 flows
                for (j = new_start_lv2, i = 0; i < usage_lv_2; i++, j += CELL_LEN2) {
                    *(uint32_t * )((uint8_t *) bucket + (j >> 3)) |= ((uint32_t) cell_lv_2[i][0]) << (j & 0x7);
                    *(uint32_t * )((uint8_t *) bucket + ((j + FP_LEN2) >> 3)) |=
                            ((uint32_t) cell_lv_2[i][1]) << ((j + FP_LEN2) & 0x7);
                }
                //insert stored level_1 flows
                for (j = new_start_lv1, i = 0; i < usage_lv_1; i++, j += CELL_LEN1) {
                    *(uint32_t * )((uint8_t *) bucket + (j >> 3)) |= ((uint32_t) cell_lv_1[i][0]) << (j & 0x7);
                    *(uint32_t * )((uint8_t *) bucket + ((j + FP_LEN1) >> 3)) |=
                            ((uint32_t) cell_lv_1[i][1]) << ((j + FP_LEN1) & 0x7);
                }
                int metaCode = getMetaCode(new_num_lv_2, new_num_lv_3);
                bucket[0] |= metaCode & META_MASK;

                return;
            } else//levelup not success,exponential decay, change when bucket structure changes
            {
                findMinCell(bucket, 3, num_lv_1, num_lv_2, num_lv_3, min_counter, min_index);
                if (min_counter > EXP_MODE_MASK) {
                    return;
                }

                //exponential decay
                ranf = 1.0 * rand() / RAND_MAX;
                if (ranf < pow(b, log2(min_counter) * -1)) {
                    if (min_counter == MIN_C_LV_3)//only replace fingerprints
                    {
                        *(uint32_t * )((uint8_t *) bucket + (cell_start_bit_idx >> 3)) &= ~(((uint32_t) CELL_MASK2)
                                << (cell_start_bit_idx & 0x7));//clear original cell

                        //replace fp
                        *(uint32_t * )((uint8_t *) bucket + (min_index >> 3)) &= ~(((uint32_t) FP_MASK3)
                                << (min_index & 0x7));
                        *(uint32_t * )((uint8_t *) bucket + (min_index >> 3)) |=
                                ((uint32_t) fp16 & FP_MASK3) << (min_index & 0x7);
                        return;
                    } else//decay
                    {
                        *(uint32_t * )((uint8_t *) bucket + ((min_index + FP_LEN3) >> 3)) -=
                                ((uint32_t) 1) << ((min_index + FP_LEN3) & 0x7);
                        return;
                    }
                }
            }
        }
    }
}

void insert(int packet_cnt) {
    uint32_t hash;
    uint16_t bucket_index, fp16;
    uint16_t tmp_fp, tmp_counter;

    uint8_t meta;
    int num_lv_1, num_lv_2, num_lv_3;
    int start_lv2, start_lv1, end_lv1;

    int i, j;
    uint32_t ran;
    double ranf;

    bool half_flag = false;
    uint8_t quarter_flag = 0;

    for (i = 0; i < packet_cnt; i++) {
        hash = BKDRHash(keys[i], KEY_SIZE);
        bucket_index = hash % BUCKET_NUM;         //get bucket index
        fp16 = finger_print(hash);                //get fp
        uint64_t *bucket = B[bucket_index];

        meta = bucket[0] & META_MASK;
        metaCodeToData(meta, num_lv_1, num_lv_2, num_lv_3);

        //each 256-bit bucket is composed of 4 64-bit words, all cells are stored from the lower addresses. In each cell, lower bits are the fingerprint, and higher bits are the counter.
        start_lv2 = META_LENGTH + num_lv_3 * CELL_LEN3;
        start_lv1 = start_lv2 + num_lv_2 * CELL_LEN2;
        end_lv1 = start_lv1 + num_lv_1 * CELL_LEN1;

        //if exists a flow in level_3
        for (j = META_LENGTH; j < start_lv2; j += CELL_LEN3) {
            tmp_fp = ((*(uint32_t * )((uint8_t *) bucket + (j >> 3))) >> (j & 0x7)) & FP_MASK3;
            tmp_counter =
                    ((*(uint32_t * )((uint8_t *) bucket + ((j + FP_LEN3) >> 3))) >> ((j + FP_LEN3) & 0x7)) & CT_MASK3;
            if (tmp_fp == fp16 && tmp_counter > 0) {
                if (!(tmp_counter & EXP_MODE_MASK))//normal mode
                {
                    *(uint32_t * )((uint8_t *) bucket + ((j + FP_LEN3) >> 3)) +=
                            ((uint32_t) 1) << ((j + FP_LEN3) & 0x7);
                    if (tmp_counter + 1 == THRESHOLD) {
                        memcpy(savedKeys[num_of_saved_keys], keys[i], KEY_SIZE);
                        num_of_saved_keys++;
                    }
                } else//exponential mode
                {
                    ran = rand() & 0x7fff;
                    if (ran <= (1 << (15 - 4 - (tmp_counter & 0xF)))) {
                        if (tmp_counter < EXP_CT_OVFL_MASK) {
                            *(uint32_t * )((uint8_t *) bucket + ((j + FP_LEN3) >> 3)) +=
                                    ((uint32_t) 0x10) << ((j + FP_LEN3) & 0x7);
                        } else//coefficient part is overflow
                        {
                            *(uint32_t * )((uint8_t *) bucket + ((j + FP_LEN3) >> 3)) &= ~(((uint32_t) EXP_CT_MASK)
                                    << ((j + FP_LEN3) & 0x7));
                            *(uint32_t * )((uint8_t *) bucket + ((j + FP_LEN3) >> 3)) +=
                                    ((uint32_t) 1) << ((j + FP_LEN3) & 0x7);
                        }
                    }
                }
                goto pkt_done;
            }
        }

        //change when bucket structure changes
        //if existing flow in level_2
        for (j = start_lv2; j < start_lv1; j += CELL_LEN2) {
            tmp_fp = ((*(uint32_t * )((uint8_t *) bucket + (j >> 3))) >> (j & 0x7)) & FP_MASK2;
            tmp_counter =
                    ((*(uint32_t * )((uint8_t *) bucket + ((j + FP_LEN2) >> 3))) >> ((j + FP_LEN2) & 0x7)) & CT_MASK2;

            if (tmp_fp == (fp16 & FP_MASK2) && tmp_counter > 0) {
                if (quarter_flag == 3) {
                    if (tmp_counter != CT_MASK2) {
                        *(uint32_t * )((uint8_t *) bucket + ((j + FP_LEN2) >> 3)) +=
                                ((uint32_t) 1) << ((j + FP_LEN2) & 0x7);
                        if (tmp_counter == 187) {//THRESHOLD=750/4=187.5
                            memcpy(savedKeys[num_of_saved_keys], keys[i], KEY_SIZE);
                            num_of_saved_keys++;
                        }
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
            tmp_fp = ((*(uint32_t * )((uint8_t *) bucket + (j >> 3))) >> (j & 0x7)) & FP_MASK1;
            tmp_counter =
                    ((*(uint32_t * )((uint8_t *) bucket + ((j + FP_LEN1) >> 3))) >> ((j + FP_LEN1) & 0x7)) & CT_MASK1;

            if (tmp_fp == (fp16 & FP_MASK1) && tmp_counter > 0) {
                half_flag = !half_flag;
                if (half_flag) {//0.5 prob to update
                    if (tmp_counter != 0x7F) {
                        *(uint32_t * )((uint8_t *) bucket + ((j + FP_LEN1) >> 3)) +=
                                ((uint32_t) 1) << ((j + FP_LEN1) & 0x7);
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
                    *(uint32_t * )((uint8_t *) bucket + (min_index >> 3)) |=
                            ((uint32_t) fp16 & FP_MASK1) << (min_index & 0x7);
                    *(uint32_t * )((uint8_t *) bucket + ((min_index + FP_LEN1) >> 3)) |=
                            ((uint32_t) 1) << ((min_index + FP_LEN1) & 0x7);
                }

            } else {//exp decay
                ranf = 1.0 * rand() / RAND_MAX;
                if (ranf < 0.5 * pow(b, log2(min_counter * 2) * -1)) {
                    if (min_counter == 1) {//replace fp
                        *(uint32_t * )((uint8_t *) bucket + (min_index >> 3)) &= ~(((uint32_t) FP_MASK1)
                                << (min_index & 0x7));
                        *(uint32_t * )((uint8_t *) bucket + (min_index >> 3)) |=
                                ((uint32_t) fp16 & FP_MASK1) << (min_index & 0x7);
                    } else {//decay counter
                        *(uint32_t * )((uint8_t *) bucket + ((min_index + FP_LEN1) >> 3)) -=
                                ((uint32_t) 1) << ((min_index + FP_LEN1) & 0x7);
                    }
                }
            }
        } else if (num_lv_2 > 0) {
            uint16_t min_counter = -1;
            uint16_t min_index = -1;
            findMinCell(bucket, 2, num_lv_1, num_lv_2, num_lv_3, min_counter, min_index);
            if (min_counter == 0) {//find empty cell
                if (quarter_flag == 3) {
                    *(uint32_t * )((uint8_t *) bucket + (min_index >> 3)) |=
                            ((uint32_t) fp16 & FP_MASK2) << (min_index & 0x7);
                    *(uint32_t * )((uint8_t *) bucket + ((min_index + FP_LEN2) >> 3)) |=
                            ((uint32_t) 1) << ((min_index + FP_LEN2) & 0x7);
                    quarter_flag = 0;
                } else {
                    quarter_flag++;
                }
            } else {//exp decay
                ranf = 1.0 * rand() / RAND_MAX;
                if (ranf < 0.25 * pow(b, log2(min_counter * 4) * -1)) {
                    if (min_counter == 1) {//replace fp
                        *(uint32_t * )((uint8_t *) bucket + (min_index >> 3)) &= ~(((uint32_t) FP_MASK2)
                                << (min_index & 0x7));
                        *(uint32_t * )((uint8_t *) bucket + (min_index >> 3)) |=
                                ((uint32_t) fp16 & FP_MASK2) << (min_index & 0x7);
                    } else {//decay counter
                        *(uint32_t * )((uint8_t *) bucket + ((min_index + FP_LEN2) >> 3)) -=
                                ((uint32_t) 1) << ((min_index + FP_LEN2) & 0x7);
                    }
                }
            }
        } else {
            uint16_t min_counter = EXP_MODE_T;
            uint16_t min_index = -1;
            findMinCell(bucket, 3, num_lv_1, num_lv_2, num_lv_3, min_counter, min_index);
            if (min_counter == 0) {//find empty cell
                *(uint32_t * )((uint8_t *) bucket + (min_index >> 3)) |=
                        ((uint32_t) fp16 & FP_MASK3) << (min_index & 0x7);
                *(uint32_t * )((uint8_t *) bucket + ((min_index + FP_LEN3) >> 3)) |=
                        ((uint32_t) 1) << ((min_index + FP_LEN3) & 0x7);
            } else if (min_counter < EXP_MODE_T) {//exp decay
                ranf = 1.0 * rand() / RAND_MAX;
                if (ranf < pow(b, log2(min_counter) * -1)) {
                    if (min_counter == 1) {//replace fp
                        *(uint32_t * )((uint8_t *) bucket + (min_index >> 3)) &= ~(((uint32_t) FP_MASK3)
                                << (min_index & 0x7));
                        *(uint32_t * )((uint8_t *) bucket + (min_index >> 3)) |=
                                ((uint32_t) fp16 & FP_MASK3) << (min_index & 0x7);
                    } else {//decay counter
                        *(uint32_t * )((uint8_t *) bucket + ((min_index + FP_LEN3) >> 3)) -=
                                ((uint32_t) 1) << ((min_index + FP_LEN3) & 0x7);
                    }
                }
            }
        }
        pkt_done:;
    }
}

uint32_t query(uint8_t *key) {
    uint16_t tmp_fp, tmp_counter;

    int num_lv_1, num_lv_2, num_lv_3;
    int start_lv2, start_lv1, end_lv1;

    int j;

    uint32_t hash = BKDRHash(key, KEY_SIZE);
    uint16_t bucket_index = hash % BUCKET_NUM;         //get bucket index
    uint16_t fp16 = finger_print(hash);                //get fp
    uint64_t *bucket = B[bucket_index];

    uint8_t meta = bucket[0] & META_MASK;
    metaCodeToData(meta, num_lv_1, num_lv_2, num_lv_3);

    //each 256-bit bucket is composed of 4 64-bit words, all cells are stored from the lower addresses. In each cell, lower bits are the fingerprint, and higher bits are the counter.
    start_lv2 = META_LENGTH + num_lv_3 * CELL_LEN3;
    start_lv1 = start_lv2 + num_lv_2 * CELL_LEN2;
    end_lv1 = start_lv1 + num_lv_1 * CELL_LEN1;

    //if exists a flow in level_3
    for (j = META_LENGTH; j < start_lv2; j += CELL_LEN3) {
        tmp_fp = ((*(uint32_t * )((uint8_t *) bucket + (j >> 3))) >> (j & 0x7)) & FP_MASK3;
        tmp_counter = ((*(uint32_t * )((uint8_t *) bucket + ((j + FP_LEN3) >> 3))) >> ((j + FP_LEN3) & 0x7)) & CT_MASK3;

        if (tmp_fp == fp16 && tmp_counter > 0) {
            if (tmp_counter <= EXP_MODE_MASK) {
                return tmp_counter;
            } else//exponential mode
            {
                return ((uint32_t) tmp_counter >> 4) * ((uint32_t) 1 << (4 + (tmp_counter & 0xF)));
            }
        }
    }
    //change when bucket structure changes
    //if existing flow in level_2
    for (j = start_lv2; j < start_lv1; j += CELL_LEN2) {
        tmp_fp = ((*(uint32_t * )((uint8_t *) bucket + (j >> 3))) >> (j & 0x7)) & FP_MASK2;
        tmp_counter = ((*(uint32_t * )((uint8_t *) bucket + ((j + FP_LEN2) >> 3))) >> ((j + FP_LEN2) & 0x7)) & CT_MASK2;

        if (tmp_fp == (fp16 & FP_MASK2) && tmp_counter > 0) {
            return tmp_counter * 4;
        }
    }

    //if existing flow in level_1
    for (j = start_lv1; j < end_lv1; j += CELL_LEN1) {
        tmp_fp = ((*(uint16_t * )((uint8_t *) bucket + (j >> 3))) >> (j & 0x7)) & FP_MASK1;
        tmp_counter = ((*(uint16_t * )((uint8_t *) bucket + ((j + FP_LEN1) >> 3))) >> ((j + FP_LEN1) & 0x7)) & CT_MASK1;

        if (tmp_fp == (fp16 & FP_MASK1) && tmp_counter > 0) {
            return ((uint32_t) tmp_counter) * 2;
        }
    }

    return 0;
}

void getEstimatedFlowSizes(unordered_map<uint8_t *, unsigned int, HashFunc, CmpFunc> &estimatedFlowSizes) {
    for (uint32_t i = 0; i < num_of_saved_keys; i++) {
        uint8_t *key = savedKeys[i];
        uint32_t flowSize = query(key);
        estimatedFlowSizes[key] = flowSize;
    }
}

uint32_t ReadInTraces(const char *traceFilePath, unordered_map<uint8_t *, uint32_t, HashFunc, CmpFunc> &actualFlowSizes) {
    FILE *fin = fopen(traceFilePath, "rb");

    uint8_t temp[KEY_SIZE]{0};
    uint8_t temp2[9]{0};
    uint32_t count = 0;
    uint8_t *key;
    while (fread(temp, 1, KEY_SIZE, fin) == KEY_SIZE) {
        key = (uint8_t *) malloc(KEY_SIZE);
        memcpy(key, temp, KEY_SIZE);
        keys[count] = key;
        if (actualFlowSizes.find(key) == actualFlowSizes.end()) {
            actualFlowSizes[key] = 1;
        } else {
            actualFlowSizes[key] += 1;
        }
        count++;

        if (count % 5000000 == 0) {
            printf("Successfully read in %s, %u items\n", traceFilePath, count);

        }
        if (fread(temp2, 1, 9, fin) != 9) {
            break;
        }
    }
    printf("Successfully read in %s, %u items\n", traceFilePath, count);
    fclose(fin);
    return count;
}

int main() {
    srand(4524524);
    long double totalPR = 0;
    long double totalRecall = 0;
    long double totalARE = 0;
    long double totalAAE = 0;
    long double totalTh = 0;

    unordered_map < uint8_t * , unsigned int, HashFunc, CmpFunc > actualFlowSizes;//count the actual flow sizes
    uint32_t packet_cnt = ReadInTraces("../data/20190117-130000-new.dat", actualFlowSizes);
    const int testNum = 10;
    for (int i = 0; i < testNum; i++)
    {
        timespec time1, time2;
        __uint128_t resns;

        //change when bucket structure changes
        for (int j = 0; j < BUCKET_NUM; j++)//reset buckets
        {
            B[j][0] = 0;
            B[j][1] = 0;
            B[j][2] = 0;
            B[j][3] = 0;
        }
        for (int j = 0; j < ListLength; j++) {//reset the list saving keys
            *((uint32_t *) savedKeys[i]) = 0;
        }
        num_of_saved_keys = 0;

        clock_gettime(CLOCK_MONOTONIC, &time1);
        insert(packet_cnt);
        clock_gettime(CLOCK_MONOTONIC, &time2);

        resns = (__uint128_t) (time2.tv_sec - time1.tv_sec) * 1000000000LL +
                (time2.tv_nsec - time1.tv_nsec);

        long double throughput = (double) 1000.0 * packet_cnt / resns;

        vector <pair<uint8_t *, unsigned int>> actualFlowSizesVector;
        for (auto iter = actualFlowSizes.begin(); iter != actualFlowSizes.end(); iter++) {
            actualFlowSizesVector.push_back(make_pair(iter->first, iter->second));
        }
        sort(actualFlowSizesVector.begin(), actualFlowSizesVector.end(), cmpPairFunc);

        vector <pair<uint8_t *, unsigned int>> estimatedFlowSizesVector;
        unordered_map < uint8_t * , unsigned int, HashFunc, CmpFunc > estimatedFlowSizes;
        getEstimatedFlowSizes(estimatedFlowSizes);

        for (auto iter = estimatedFlowSizes.begin(); iter != estimatedFlowSizes.end(); iter++) {
            estimatedFlowSizesVector.push_back(make_pair(iter->first, iter->second));
        }
        sort(estimatedFlowSizesVector.begin(), estimatedFlowSizesVector.end(), cmpPairFunc);

        //get acutal heavyHitters
        unordered_map < uint8_t * , unsigned int, HashFunc, CmpFunc > actualHeavyHitterFlowSizes;
        for (unsigned int idx = 0; idx < actualFlowSizesVector.size(); idx++) {
            unsigned int actualSize = actualFlowSizesVector[idx].second;
            if (actualSize >= THRESHOLD) {
                actualHeavyHitterFlowSizes[actualFlowSizesVector[idx].first] = actualSize;
            } else {
                break;
            }
        }
        unsigned int TPNum = 0;//True positive num
        unsigned int FPNum = 0;

        for (unsigned int idx = 0; idx < estimatedFlowSizesVector.size(); idx++) {
            uint8_t *key = estimatedFlowSizesVector[idx].first;
            if (actualHeavyHitterFlowSizes.find(key) != actualHeavyHitterFlowSizes.end()) {
                TPNum++;
            } else {
                FPNum++;
            }
        }

        ofstream report("./switchsketch256-zby -Report.txt");
        double AAEOfHH = 0;
        double AREOfHH = 0;
        for (unsigned int idx = 0; idx < estimatedFlowSizesVector.size(); idx++) {

            uint8_t *key = estimatedFlowSizesVector[idx].first;
            unsigned int estimatedSize = estimatedFlowSizesVector[idx].second;
            unsigned int actualSize = 0;
            if (actualFlowSizes.find(key) != actualFlowSizes.end()) {
                actualSize = actualFlowSizes[key];
            }

            AAEOfHH += abs((double) estimatedSize - actualSize);
            double rE = abs((double) estimatedSize - actualSize) / actualSize;
            AREOfHH += rE;

            if (actualSize < THRESHOLD) {
                report << "++++";
            }
            report << "Rep-" << idx << "\t" << estimatedSize << "\t" << actualSize << "\t"
                   << setiosflags(ios::fixed | ios::right) << setprecision(6) << AAEOfHH / (idx + 1) << "\t"
                   << AREOfHH / (idx + 1) << endl;
        }
        report.close();

        AAEOfHH = AAEOfHH / (TPNum + FPNum);
        AREOfHH = AREOfHH / (TPNum + FPNum);
        double precision = (double) TPNum / (TPNum + FPNum);

        uint32_t FNNum = actualHeavyHitterFlowSizes.size() - TPNum;
        double recall = (double) TPNum / actualHeavyHitterFlowSizes.size();

        ofstream out("./switchsketch256-zby -m64 -O.txt", ios::app);
        out << "****************************-----test " << i << endl;
        out << "collect " << num_of_saved_keys << " flow keys" << endl;
        out << "get " << estimatedFlowSizes.size() << " flows" << endl;
        out << "TP=" << TPNum << "\nFP=" << FPNum << "\nFN=" << FNNum << endl;

        out << "Recall=" << TPNum << "/" << actualHeavyHitterFlowSizes.size() << "=" << recall << endl;
        out << "PR=" << TPNum << "/" << (TPNum + FPNum) << "=" << precision << endl;
        out << "ARE=" << AREOfHH << "\nAAE=" << AAEOfHH << endl;
        out << "Th=" << throughput << " mps\n\n" << endl;
        out.close();

        totalPR += precision;
        totalARE += AREOfHH;
        totalAAE += AAEOfHH;
        totalTh += throughput;
        totalRecall += recall;


        ofstream real("./switchsketch256-zby -Real.txt");
        AAEOfHH = 0;
        AREOfHH = 0;
        for (unsigned int idx = 0; idx < actualFlowSizesVector.size(); idx++) {
            uint8_t *key = actualFlowSizesVector[idx].first;
            unsigned int actualSize = actualFlowSizesVector[idx].second;
            if (actualSize >= THRESHOLD) {
                unsigned int estimatedSize = query(key);

                AAEOfHH += abs((double) estimatedSize - actualSize);
                double rE = abs((double) estimatedSize - actualSize) / actualSize;
                AREOfHH += rE;
                if (estimatedSize < THRESHOLD) {
                    real << "----";
                }
                real << "Real-" << idx << "\t" << estimatedSize << "\t" << actualSize << "\t"
                     << setiosflags(ios::fixed | ios::right) << setprecision(6) << AAEOfHH / (idx + 1) << "\t"
                     << AREOfHH / (idx + 1) << endl;
            } else {
                break;
            }
        }
        real.close();
    }

    ofstream out("./switchsketch256-zby -m64 -O.txt", ios::app);
    out << "################################" << endl;
    out << "avg PR: " << totalPR / testNum << endl;
    out << "avg Recall: " << totalRecall / testNum << endl;
    out << "avg ARE: " << totalARE / testNum << endl;
    out << "avg AAE: " << totalAAE / testNum << endl;
    out << "avg Th: " << totalTh / testNum << endl;
    out.close();

    /* free memory */
    for (int k = 0; k < packet_cnt; k++)
        free(keys[k]);
    free(keys);
}