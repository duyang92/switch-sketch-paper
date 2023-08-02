import mmh3
import utils.File_Util
import utils.Probability_Pick
import collections
import random
import decimal
import math
from matplotlib import pyplot as plt
from tqdm import tqdm
import sys

def Query(item,id,FP1,FP2,FP3):
    if len(Level[3][id]) > 0:
        for index,value in enumerate(Level[3][id]):
            if value != 0:
                if value[0] == FP3:
                    return [index,3]
    if len(Level[2][id]) > 0:
        for index, value in enumerate(Level[2][id]):
            if value != 0:
                if value[0] == FP2:
                    return [index, 2]
    if len(Level[1][id]) > 0:
        for index,value in enumerate(Level[1][id]):
            if value != 0:
                if value[0] == FP1:
                    return [index,1]
    return 0

def Get_C1(C3, C2):
    if W - meta - C3*(fp_bit_level3+counter_bit_level3)-C2*(fp_bit_level2+counter_bit_level2) < 0:
        return 99
    else:
        return int((W - meta - C3 * (fp_bit_level3 + counter_bit_level3) - C2 * (fp_bit_level2 + counter_bit_level2)) / (
                    fp_bit_level1 + counter_bit_level1))

def Adjust(C3_old,C2_old,C1_old,C3_new,C2_new,C1_new):
    #reconstruct a new l2
    if C3_new == C3_old and C2_new == C2_old + 1:
        Level[2][bucket_index1].append([FP_level2,2**(counter_bit_level1-1)])
        lst_index[bucket_index1][1] += 1

        clear_num = -C1_new-C2_new-C3_new+C1_old+C2_old+C3_old

        # clear the original record
        for index, item in enumerate(Level[1][bucket_index1]):
            if item != 0:
                if item[0] == FP_level1:
                    Level[1][bucket_index1].pop(index)
        Clear_l1(Level[1][bucket_index1], clear_num)

        # if [len(Level[1][bucket_index1]), len(Level[2][bucket_index1]), len(Level[3][bucket_index1])] == [11, 4, 0]:
        #     print(Level[1][bucket_index1])
        #     print(Level[2][bucket_index1])
        #     print(Level[3][bucket_index1])
        #     print("===============")
    # reconstruct a new l3
    else:
        Level[3][bucket_index1].append([FP_level3,(2**(counter_bit_level2))*4])
        clear_num_l1 = C1_old-C1_new
        clear_num_l2 = C2_old-C2_new
        #clear l1 cells only
        # none
        if clear_num_l2 == 0 and clear_num_l1 > 0:
            print(Level[1][bucket_index1])
            print(Level[2][bucket_index1])
            print(Level[3][bucket_index1])
            # lst_index[bucket_index1][0] += 1
            # Clear_l1(Level[1][bucket_index1], clear_num_l1)
            # #clear the original record
            # for index, item in enumerate(Level[2][bucket_index1]):
            #     if item != 0:
            #         if item[0] == FP_level2:
            #             Level[2][bucket_index1][index] = 0
            print("clear l1 cells only TAG")
            return

        #clear l2 cells only
        elif clear_num_l1 == 0 and clear_num_l2 > 0:
            lst_index[bucket_index1][0] += 1
            lst_index[bucket_index1][1] -= 1
            # clear the original record
            for index, item in enumerate(Level[2][bucket_index1]):
                if item != 0:
                    if item[0] == FP_level2:
                        Level[2][bucket_index1].pop(index)
            return

        # clear one l2 and clear one l1
        elif clear_num_l2 == 1 and clear_num_l1 == 1:
            for index, item in enumerate(Level[2][bucket_index1]):
                if item != 0:
                    if item[0] == FP_level2:
                        Level[2][bucket_index1].pop(index)
            Clear_l1(Level[1][bucket_index1], 1)
            lst_index[bucket_index1][0] += 1
            lst_index[bucket_index1][1] -= 1

        # clear two l2s and add one l1
        else:
            for index, item in enumerate(Level[2][bucket_index1]):
                if item != 0:
                    if item[0] == FP_level2:
                        Level[2][bucket_index1].pop(index)
            Clear_l2(Level[2][bucket_index1], 1)
            lst_index[bucket_index1][0] += 1
            lst_index[bucket_index1][1] -= 2
            Level[1][bucket_index1].append(0)
    return

def Switch(k, C3, C2):
    C1 = Get_C1(C3, C2)
    if k == 1:
        result = Get_C1(C3, C2 + 1)
        if result != 99:
            C1_new = result
            Adjust(C3,C2,C1,C3,C2+1,C1_new)
        # l2 exponential decay
        else:
            if len(Level[2][bucket_index1]) == 0:
                return
            min_num = sys.maxsize
            min_pos = -1
            for i in range(len(Level[2][bucket_index1])):
                if Level[2][bucket_index1][i] != 0:
                    if isinstance(Level[2][bucket_index1][i][1], int):
                        if Level[2][bucket_index1][i][1] < min_num:
                            min_num = Level[2][bucket_index1][i][1]
                            min_pos = i
            if min_num == sys.maxsize:
                gailv_minus = 0
            else:
                gailv_minus = 1 / b ** math.log2(min_num)

            if random.random() < gailv_minus:
                if isinstance(Level[2][bucket_index1][min_pos][1], int):
                    if random.random() < 1/4:
                        Level[2][bucket_index1][min_pos][1] -= 1
                        if Level[2][bucket_index1][min_pos][1] < 2 ** (counter_bit_level1-1):
                            Level[2][bucket_index1][min_pos][0] = FP_level2
                            Level[2][bucket_index1][min_pos][1] = 2 ** (counter_bit_level1-1)
                            for index, item in enumerate(Level[1][bucket_index1]):
                                if item != 0:
                                    if item[0] == FP_level1 and item[1] == 2**counter_bit_level1-1:
                                        Level[1][bucket_index1][index] = 0
    else:
        flag = False
        for i in range(C2-1,-1,-1):
            result = Get_C1(C3 + 1, i)
            if result == 99:
                continue
            else:
                C1_new = result
                Adjust(C3, C2, C1, C3+1, i, C1_new)
                flag = True
                break
        # l3 exponential decay
        if not flag:
            if len(Level[3][bucket_index1]) == 0:
                return
            min_num = sys.maxsize
            min_pos = -1
            for i in range(len(Level[3][bucket_index1])):
                if Level[3][bucket_index1][i] != 0:
                    if isinstance(Level[3][bucket_index1][i][1], int):
                        if Level[3][bucket_index1][i][1] < min_num:
                            min_num = Level[3][bucket_index1][i][1]
                            min_pos = i
            if min_num == sys.maxsize:
                gailv_minus = 0
            else:
                gailv_minus = 1 / b ** math.log2(min_num)

            if random.random() < gailv_minus:
                if isinstance(Level[3][bucket_index1][min_pos][1], int):
                    Level[3][bucket_index1][min_pos][1] -= 1
                    if Level[3][bucket_index1][min_pos][1] < 2 ** (counter_bit_level2 + 2):
                        Level[3][bucket_index1][min_pos][0] = FP_level3
                        Level[3][bucket_index1][min_pos][1] = 2 ** (counter_bit_level2 + 2)
                        for index, item in enumerate(Level[2][bucket_index1]):
                            if item != 0:
                                if item[0] == FP_level2 and item[1] == 2 ** counter_bit_level2 - 1:
                                    Level[2][bucket_index1][index] = 0

def Clear_l2(lst,clear_num):
    for times in range(clear_num):
        Delete_Min_l2(lst)

def Delete_Min_l2(lst):
    min_num = sys.maxsize
    min_pos = -1
    for i in range(len(lst)):
        if lst[i] != 0:
            if lst[i][1] < min_num:
                min_num = lst[i][1]
                min_pos = i
        else:
            lst.pop(i)
            return lst

    lst.pop(min_pos)
    return lst

def Clear_l1(lst8_7, clear_num):
    if clear_num <= 0:
        return
    for times in range(clear_num):
        Delete_Min_l1(lst8_7)

def Delete_Min_l1(lst8_7):
    flag = 0
    min_num = sys.maxsize
    min_pos_8_7 = -1

    for i in range(len(lst8_7)):
        if lst8_7[i] != 0:
            if lst8_7[i][1]*2 < min_num:
                min_num = lst8_7[i][1]*2
                min_pos_8_7 = i
                flag = 7
        else:
            lst8_7.pop(i)
            return

    if flag == 7:
        lst8_7.pop(min_pos_8_7)
    return


if __name__ == '__main__':
    # read file
    df = utils.File_Util.read_single_file(19, True, unique_tag=False)
    F = df['tag1']
    print('The numbers of packets:{}'.format(len(F)))
    l = collections.Counter(F)

    for memorysize in [100]:
        lst_PR = []
        lst_RR = []
        for top in [1000]:
            alpha = 0.5
            bottom = top * alpha
            T = int((top + bottom) / 2)
            topK = 1024
            count_levelup = [0,0,0]
            b = 1.08
            k = 0.6

            counter_bit_level1 = 7
            fp_bit_level1 = 8

            counter_bit_level2 = 10
            fp_bit_level2 = 12

            counter_bit_level3 = 15
            fp_bit_level3 = 16

            #length of the two-mode active counter's exponent part
            gamma = 4

            #bits of one bucket
            W = 256

            meta = 6

            init_l1_num = 16

            BUCKET_NUM = int(memorysize*1024*8/W)

            print('Memory Size:{}KB'.format(memorysize))
            print('W:{}bits'.format(W))
            print('T:{}'.format(T))
            print('alpha:{}'.format(alpha))
            print('bottom:{},top:{}'.format(bottom, top))
            # print('The number of buckets:{}'.format(BUCKET_NUM))
            # print('The number of initial level-1 cell in each bucket:{}'.format(cell_num))

            #15 k,v = 8,7
            Level1_1 = [[0 for j in range(init_l1_num)] for i in range(BUCKET_NUM)]
            #22 k,v = 11,11
            Level2 = [[] for i in range(BUCKET_NUM)]
            #31 k,v = 16,15
            Level3 = [[] for i in range(BUCKET_NUM)]

            Level = [[],Level1_1,Level2,Level3]
            #[C3,C2] in each bucket
            lst_index = [[0,0] for i in range(BUCKET_NUM)]
            # auxiliary set
            set_HH = set()

            SEED = [2132,315,1651,3165,4651]

            count_l1_overflow = 0
            count_l1_switch = 0
            count_l2_overflow = 0
            count_l2_switch = 0


            # SEED = [0, 1, 2, 3, 4, 5]
            for item in tqdm(F):
                bucket_index1 = mmh3.hash(item, SEED[0]) % BUCKET_NUM

                FP_level1 = mmh3.hash(item,SEED[1]) % (2 ** fp_bit_level1)
                FP_level2 = mmh3.hash(item,SEED[3]) % (2 ** fp_bit_level2)
                FP_level3 = mmh3.hash(item,SEED[4]) % (2 ** fp_bit_level3)
                query_result = Query(item, bucket_index1, FP_level1, FP_level2, FP_level3)

                #query_result = [index,x] type-x index-th
                # if query_result:
                #     if query_result[1] == 2:
                #         if Level[query_result[1]][bucket_index1][query_result[0]][1]*4 >= T
                #             set_HH.add(item)

                if query_result != 0:
                    query_index = query_result[0]
                    query_level = query_result[1]
                    if query_level == 3:
                        # until 1000000000000000 = 2 ** (counter_bit_level3 - 1)
                        if isinstance(Level[3][bucket_index1][query_index][1],int):
                            if Level[3][bucket_index1][query_index][1] == T:
                                set_HH.add(item)
                            if Level[3][bucket_index1][query_index][1] < 2 ** (counter_bit_level3 - 1) - 1:
                                Level[3][bucket_index1][query_index][1] += 1
                            elif Level[3][bucket_index1][query_index][1] == 2 ** (counter_bit_level3 - 1) - 1:
                                # [exponent part, coefficient part]
                                Level[3][bucket_index1][query_index][1] = [0, 2 ** (counter_bit_level3 - 1 - gamma)]
                        else:
                            exponent_part = Level[3][bucket_index1][query_index][1][0]
                            coefficient_part = Level[3][bucket_index1][query_index][1][1]
                            if random.random() < 1/(2**(exponent_part + gamma)):
                                Level[3][bucket_index1][query_index][1][1] += 1
                                if Level[3][bucket_index1][query_index][1][1] >= 2 ** (counter_bit_level3 - gamma) - 1:
                                    Level[3][bucket_index1][query_index][1][0] += 1
                                    Level[3][bucket_index1][query_index][1][1] = 2 ** (counter_bit_level3 - 1 - gamma)

                    elif query_level == 2:
                        if Level[2][bucket_index1][query_index][1] < 2**counter_bit_level2-1:
                            if random.random() < 1 / 4:
                                if Level[query_result[1]][bucket_index1][query_result[0]][1] == T // 4:
                                    set_HH.add(item)
                                Level[2][bucket_index1][query_index][1] += 1
                        else:
                            count_l2_overflow += 1
                            C2 = len(Level[2][bucket_index1])
                            C3 = len(Level[3][bucket_index1])
                            flag = False
                            for i in range(C2 - 1, -1, -1):
                                result = Get_C1(C3 + 1, i)
                                if result == 99:
                                    continue
                                else:
                                    flag = True
                                    break
                            if flag:
                                count_l2_switch += 1
                            Switch(2, lst_index[bucket_index1][0], lst_index[bucket_index1][1])

                    elif query_level == 1:
                        if Level[1][bucket_index1][query_index][1] < 2**counter_bit_level1-1:
                            if random.random() < 1/2:
                                Level[1][bucket_index1][query_index][1] += 1
                        else:
                            count_l1_overflow += 1
                            if Get_C1(len(Level[3][bucket_index1]), len(Level[2][bucket_index1]) + 1) != 99:
                                count_l1_switch += 1
                            Switch(1, lst_index[bucket_index1][0], lst_index[bucket_index1][1])

                #not recorded
                else:
                    if len(Level[1][bucket_index1]) > 0:
                        noEmpty = True
                        for index, item in enumerate(Level[1][bucket_index1]):
                            if item == 0:
                                if random.random() < 1 / 2:
                                    Level[1][bucket_index1][index] = [FP_level1, 1]
                                noEmpty = False
                                break

                        if noEmpty:
                            flag = 0
                            min_num_level1 = sys.maxsize
                            min_pos_level1_87 = -1
                            for i in range(len(Level[1][bucket_index1])):
                                if Level[1][bucket_index1] != 0:
                                    if Level[1][bucket_index1][i][1] * 2 < min_num_level1:
                                        min_num_level1 = Level[1][bucket_index1][i][1] * 2
                                        min_pos_level1_87 = i
                                        flag = 7

                            if min_num_level1 == sys.maxsize:
                                gailv_minus = 0
                            else:
                                gailv_minus = 1 / (b ** (math.log2(min_num_level1)))


                            if random.random() < gailv_minus:
                                if random.random() < 1 / 2:

                                    Level[1][bucket_index1][min_pos_level1_87][1] -= 1

                                if Level[1][bucket_index1][min_pos_level1_87][1] <= 0:
                                    Level[1][bucket_index1][min_pos_level1_87][0] = FP_level1
                                    Level[1][bucket_index1][min_pos_level1_87][1] = 1


                    elif len(Level[2][bucket_index1]) > 0:
                        noEmpty = True
                        for index, item in enumerate(Level[2][bucket_index1]):
                            if item == 0:
                                if random.random() < 1 / 4:
                                    Level[2][bucket_index1][index] = [FP_level2, 1]
                                noEmpty = False
                                break
                        if noEmpty:
                            min_num_level2 = sys.maxsize
                            min_pos_level2 = -1
                            for i in range(len(Level[2][bucket_index1])):
                                if Level[2][bucket_index1] != 0:
                                    if Level[2][bucket_index1][i][1] < min_num_level2:
                                        min_num_level2 = Level[2][bucket_index1][i][1]
                                        min_pos_level2 = i
                            # print(min_num_level1)

                            if min_num_level2 == sys.maxsize:
                                gailv_minus = 0
                            else:
                                gailv_minus = 1 / (b ** (math.log2(min_num_level2)))

                            if min_num_level2 < 256:
                                if random.random() < gailv_minus:
                                    if random.random() < 1 / 4:
                                        Level[2][bucket_index1][min_pos_level2][1] -= 1

                                    if Level[2][bucket_index1][min_pos_level2][1] <= 0:
                                        Level[2][bucket_index1][min_pos_level2][0] = FP_level2
                                        Level[2][bucket_index1][min_pos_level2][1] = 1
                            else:
                                if random.random() < gailv_minus:
                                    if random.random() < 1 / 4:
                                        Level[2][bucket_index1][min_pos_level2][1] -= 1

                                    if Level[2][bucket_index1][min_pos_level2][1] <= 255:
                                        Level[2][bucket_index1][min_pos_level2][0] = FP_level2
                                        Level[2][bucket_index1][min_pos_level2][1] = 256

                    else:
                        noEmpty = True
                        for index, item in enumerate(Level[3][bucket_index1]):
                            if item == 0:
                                Level[3][bucket_index1][index] = [FP_level3, 1]
                                noEmpty = False
                                break
                        if noEmpty:
                            min_num_level3 = sys.maxsize
                            min_pos_level3 = -1
                            for i in range(len(Level[3][bucket_index1])):
                                if Level[3][bucket_index1] != 0:
                                    if isinstance(Level[3][bucket_index1][i][1], int):
                                        if Level[3][bucket_index1][i][1] < min_num_level3:
                                            min_num_level3 = Level[3][bucket_index1][i][1]
                                            min_pos_level3 = i
                                    else:
                                        v = Level[3][bucket_index1][i][1][1] * (2 ** (gamma + Level[3][bucket_index1][i][1][0]))
                                        if v < min_num_level3:
                                            min_num_level3 = v
                                            min_pos_level3 = i

                            if min_num_level3 == sys.maxsize:
                                gailv_minus = 0
                            else:
                                gailv_minus = 1 / (b ** (math.log2(min_num_level3)))

                            if min_num_level3 < 4096:
                                if random.random() < gailv_minus:
                                    Level[3][bucket_index1][min_pos_level3][1] -= 1

                                    if Level[3][bucket_index1][min_pos_level3][1] <= 0:
                                        Level[3][bucket_index1][min_pos_level3][0] = FP_level3
                                        Level[3][bucket_index1][min_pos_level3][1] = 1
                            else:
                                if random.random() < gailv_minus:
                                    if isinstance(Level[3][bucket_index1][min_pos_level3][1], int):
                                        Level[3][bucket_index1][min_pos_level3][1] -= 1

                                        if Level[3][bucket_index1][min_pos_level3][1] <= 4095:
                                            Level[3][bucket_index1][min_pos_level3][0] = FP_level3
                                            Level[3][bucket_index1][min_pos_level3][1] = 4096

            print(count_l1_overflow,count_l1_switch,count_l2_overflow,count_l2_switch)