import math
import sys

#L = 3
if __name__ == '__main__':
    total_memory = 64
    meta = 3
    memory = total_memory-meta
    lst1 = []
    lst2 = []
    Y = 0
    for C3 in [28,29,30,31,32]:
        for C2 in [20,21,22,23,24]:
            for C1 in [12,13,14,15,16]:
                Y += memory - int(memory / C3) * C3
                Y += memory - int(memory / C2) * C2
                Y += memory - int(memory / C1) * C1
                lst1.append(Y)
                Y = 0
                lst2.append([C1,C2,C3])
    min_lst1 = min(lst1)
    index = 0
    for i,value in enumerate(lst1):
        if value == min_lst1:
            index = i
            break
    print(min_lst1)
    print(lst2[index])