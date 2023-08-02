import mmh3
import utils.File_Util
import utils.Probability_Pick
import collections
import random
import decimal
import sys

def Insert():
    isFull = True
    isRecorded = False
    min_value = sys.maxsize
    min_pos = -1
    pos_recorded = -1
    pos_empty = -1
    for index,value in enumerate(Heavypart[bucket_index]):
        if value[0] == FP:
            pos_recorded = index
            isRecorded = True
        if value == [0,0]:
            pos_empty = index
            isFull = False
        else:
            if value[1] < min_value:
                min_value = value[1]
                min_pos = index

    if isRecorded:
        if Heavypart[bucket_index][pos_recorded][1] == T:
            set_overT.add(item)

        Heavypart[bucket_index][pos_recorded][1] += 1
        return

    if not isFull:
        Heavypart[bucket_index][pos_empty][0] = FP
        Heavypart[bucket_index][pos_empty][1] = 1
        return

    if min_value > 9000:
        gailv_minus = 0
    else:
        gailv_minus = decimal.Decimal(1) / (decimal.Decimal(b) ** decimal.Decimal(min_value))
    if random.random() < gailv_minus:
        Heavypart[bucket_index][min_pos][1] -= 1
        if Heavypart[bucket_index][min_pos][1] <= 0:
            Heavypart[bucket_index][min_pos][0] = FP
            Heavypart[bucket_index][min_pos][1] = 1
        else:
            if Lightpart[bucket_index][index_in_counter] < 2 ** cell_light_Countersize:
                Lightpart[bucket_index][index_in_counter] += 1
    return


if __name__ == '__main__':
    # read file
    df = utils.File_Util.read_single_file(49, True, unique_tag=False)
    F = df['tag1']
    print('The numbers of packets:{}'.format(len(F)))
    l = collections.Counter(F)

    for upper in [1000]:
        lst_PR = []
        lst_RR = []
        for memorysize in [100]:
            alpha = 0.5
            bottom = upper * alpha
            T = (upper + bottom) / 2
            topK = 1024
            cell_heavy_num = 8
            cell_heavy_FPsize = 16
            cell_heavy_Countersize = 20
            cell_light_Countersize = 4
            cell_light_Counter_num = int((cell_heavy_FPsize+cell_heavy_Countersize)*cell_heavy_num/cell_light_Countersize)
            b = 1.08
            BUCKET_NUM = int(memorysize*1024*8/(cell_light_Countersize*cell_light_Counter_num*2))
            print('Memory Size:{}KB'.format(memorysize))
            print('The number of buckets:{}'.format(BUCKET_NUM))
            print('The number of heavy cells in each bucket:{}'.format(cell_heavy_num))
            print('The number of light counter in each bucket:{}'.format(cell_light_Counter_num))

            Heavypart = [[[0,0] for i in range(cell_heavy_num)] for j in range(BUCKET_NUM)]
            Lightpart = [[0 for i in range(cell_light_Counter_num)] for j in range(BUCKET_NUM)]
            set_overT = set()

            for item in F:
                bucket_index = mmh3.hash(item,0) % BUCKET_NUM
                FP = mmh3.hash(item,1) % (2 ** cell_heavy_FPsize)
                index_in_counter = mmh3.hash(item, 2) % cell_light_Counter_num
                Insert()

            real = {}
            result_all = {}
            result_overT = {}

            count_overt = 0
            real = {}

            real_less_than_bottom = {}
            real_larger_than_upper = {}

            for i in l.items():
                if i[1] > T:
                    real.update({i[0]: i[1]})
                if i[1] < bottom:
                    real_less_than_bottom.update({i[0]: i[1]})
                if i[1] > upper:
                    real_larger_than_upper.update({i[0]: i[1]})

            aae_overT = 0
            aae_all = 0
            are_all = 0
            are_overT = 0

            count_bottom = 0
            count_upper_all = len(real_larger_than_upper)

            for key in set_overT:
                if key in real:
                    count_overt += 1
                if l[key] < bottom:
                    count_bottom += 1
                if key in real_larger_than_upper:
                    del  real_larger_than_upper[key]

                ID = mmh3.hash(key, 0) % BUCKET_NUM
                FP_i = mmh3.hash(key, 1) % (2 ** cell_heavy_FPsize)
                ID_in_light = mmh3.hash(key, 2) % cell_light_Counter_num
                flag = False
                for item in Heavypart[ID]:
                    if FP_i == item[0]:
                        flag = True
                        result_all.update({key: item[1]})
                        if item[1] > T:
                            result_overT.update({key: item[1]})
                        break
                if not flag:
                    result_all.update({key: max(1, Lightpart[ID][ID_in_light])})

                aae_overT += abs(result_overT[key] - l[key])
                are_overT += abs(result_overT[key] - l[key]) / l[key]

            count_upper_mis = len(real_larger_than_upper)

            l1 = l.most_common(topK)
            count_topk = 0
            dic_l1 = dict(l1)
            aae_topk = 0
            are_topk = 0
            dic = sorted(result_overT.items(), key=lambda x: x[1], reverse=True)[:topK]
            for item in dic:
                aae_topk += abs(l[item[0]]-item[1])
                are_topk += abs(l[item[0]]-item[1])/l[item[0]]
                if item[0] in dic_l1:
                    count_topk += 1
                    del dic_l1[item[0]]

            FPR = count_bottom / len(real_less_than_bottom)
            FNR = count_upper_mis / count_upper_all
            print('Top-theta aae:{},Top-theta are:{}'.format(aae_topk / topK, are_topk / topK))
            print('OverT aae:{},OverT are:{}'.format(aae_overT / len(result_overT), are_overT / len(result_overT)))
            print(count_bottom, len(real_less_than_bottom), count_upper_mis, count_upper_all)
            print('FPR:{},FNR:{}'.format(FPR, FNR))
            print(count_overt, len(real), len(result_overT))
            print('Find top-{}:{}'.format(topK, count_topk))
            print('Threshold={},PR:{}'.format(T, count_overt / len(result_overT)))
            print('Threshold={},RR:{}'.format(T, count_overt / len(real)))
            lst_PR.append(count_overt / len(result_overT))
            lst_RR.append(count_overt / len(real))
            print('PR', lst_PR)
            print('RR', lst_RR)