def Get_C1(C3, C2):
    if W - meta - C3*(fp_bit_level3+counter_bit_level3)-C2*(fp_bit_level2+counter_bit_level2) < 0:
        return 99
    else:
        return int((W - meta - C3 * (fp_bit_level3 + counter_bit_level3) - C2 * (fp_bit_level2 + counter_bit_level2)) / (
                    fp_bit_level1 + counter_bit_level1))

if __name__ == '__main__':
    for memorysize in [100]:
        counter_bit_level1 = 4
        fp_bit_level1 = 8

        counter_bit_level2 = 8
        fp_bit_level2 = 12

        counter_bit_level3 = 14
        fp_bit_level3 = 16

        # bits of one bucket
        W = 64
        meta = 3

        lst = []
        for C3 in range(9):
            for C2 in range(12):
                if Get_C1(C3,C2) != 99:
                    lst.append([Get_C1(C3,C2),C2,C3])

        print(lst)
        print(len(lst))






