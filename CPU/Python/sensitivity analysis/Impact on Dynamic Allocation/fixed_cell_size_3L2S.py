import os
import math
import mmh3
import random
import ipaddress
import sys
import time


def update_active_counter(counter): # str in, str out
    if counter[0] == "0": # normal mode
        now_value = int(counter, 2)
        new_value = now_value + 1
        output = str(bin(new_value))[2:]
        output = "0" * (16-len(output)) + output
    else: # exp mode
        alpha = int(counter[:12], 2)
        beta = int(counter[12:], 2)
        gamma = 4
        if random.random() < 1/(2**(beta+gamma)):
            alpha += 1
        
        if alpha == 2**12-1:
            beta += 1
            alpha_str = "100000000000"
            beta_str = str(bin(beta))[2:]
            beta_str = "0" * (4-len(beta_str)) + beta_str
            output = alpha_str + beta_str
        else:
            alpha_str = str(bin(alpha))[2:]
            beta_str = str(bin(beta))[2:]
            beta_str = "0" * (4-len(beta_str)) + beta_str
            output = alpha_str + beta_str
    return output

def read_active_counter(counter): # str in, str out
    if counter[0] == "0": # normal mode
        now_value = int(counter, 2)
        output = now_value
    else: # exp mode
        alpha = int(counter[:12], 2)
        beta = int(counter[12:], 2)
        gamma = 4
        output = alpha * (2**(beta+gamma))
    return output

def decrease_active_counter(counter): # str in, str out
    if counter[0] == "0": # normal mode
        now_value = int(counter, 2)
        new_value = now_value - 1
        output = str(bin(new_value))[2:]
        output = "0" * (16-len(output)) + output
    return output


def switchsketch_3L2S(file, memory_size):
    bucket_array = []
    bucket_num = (memory_size*8*1024)//128
    for i in range(bucket_num):
        bucket = [ [None, "0000000000000000"] for j in range(3) ]
        bucket += [ [None, 0] for j in range(2) ]
        bucket_array.append(bucket)
    est_label = set()
    
    with open(file,"r") as f:

        for line in f:

            src = line.split()[0]
            fingerprint1 = mmh3.hash(src, seed = 0, signed=False) % (2**16)
            fingerprint2 = fingerprint1 % (2**8)
            bucket_idx = mmh3.hash(src, seed = 1, signed=False) % bucket_num
            
            hit_flag = False
            empty_flag_b = False
            empty_idx_b = -1
            empty_flag_s = False
            empty_idx_s = -1
            Min_b = float('inf')
            min_idx_b = None
            Min_s = float('inf')
            min_idx_s = None
            
            # traverse the big cell
            for i in range(3):
                if bucket_array[bucket_idx][i][0] == fingerprint1:
                    hit_flag = True
                    # update counter
                    counter = bucket_array[bucket_idx][i][1]
                    bucket_array[bucket_idx][i][1] = update_active_counter(counter)
                    if read_active_counter(bucket_array[bucket_idx][i][1]) == 750:
                        est_label.add(src)
                    continue
                elif bucket_array[bucket_idx][i][0] == None or read_active_counter(bucket_array[bucket_idx][i][1]) == 0:
                    empty_flag_b = True
                    empty_idx_b = i
                else:
                    if read_active_counter(bucket_array[bucket_idx][i][1]) < Min_b:
                        Min_b = read_active_counter(bucket_array[bucket_idx][i][1])
                        min_idx_b = i
            
            # traverse the small cell
            if not hit_flag:
                for i in range(3,5):
                    if bucket_array[bucket_idx][i][0] == fingerprint2:
                        hit_flag = True
                        # update when no overflow
                        if bucket_array[bucket_idx][i][1] + 1 <= 2**8-1:                        
                            bucket_array[bucket_idx][i][1] += 1
                        # update when overflow
                        else:
                            # insert into an empty big cell
                            if empty_flag_b:
                                bucket_array[bucket_idx][empty_idx_b][0] = fingerprint1
                                output = str(bin(255))[2:]
                                bucket_array[bucket_idx][empty_idx_b][1] = "0" * (16-len(output)) + output
                                bucket_array[bucket_idx][i][1] = 0
                                bucket_array[bucket_idx][i][0] = None
                            # decrease the smallest cell when no empty cell
                            else:
                                # only decrease the cell in normal mode
                                if bucket_array[bucket_idx][min_idx_b][1][0] == "0":
                                    pr = 1.08**(-math.log(Min_b, 2))
                                    if random.random() < pr:
                                        counter = bucket_array[bucket_idx][min_idx_b][1]
                                        bucket_array[bucket_idx][min_idx_b][1] = decrease_active_counter(counter)
                                    # swap when meet a smaller cell
                                    if read_active_counter(bucket_array[bucket_idx][min_idx_b][1]) < bucket_array[bucket_idx][i][1]:
                                        bucket_array[bucket_idx][min_idx_b][0] = fingerprint1
                                        bucket_array[bucket_idx][i][0] = bucket_array[bucket_idx][min_idx_b][0] % (2**8)
                                        temp = read_active_counter(bucket_array[bucket_idx][min_idx_b][1])
                                        output = str(bin(255))[2:]
                                        bucket_array[bucket_idx][min_idx_b][1] = "0" * (16-len(output)) + output
                                        bucket_array[bucket_idx][i][1] = temp
                        continue
                    elif bucket_array[bucket_idx][i][0] == None or bucket_array[bucket_idx][i][1] == 0:
                        empty_flag_s = True
                        empty_idx_s = i
                    else:
                        if bucket_array[bucket_idx][i][1] < Min_s:
                            Min_s = bucket_array[bucket_idx][i][1]
                            min_idx_s = i
                        
            # fail to insert
            if not hit_flag:
                #  exist an empty cell, insert
                if empty_flag_s:
                    bucket_array[bucket_idx][empty_idx_s][1] = 1
                    bucket_array[bucket_idx][empty_idx_s][0] = fingerprint2
                    hit_flag = True
                elif empty_flag_b:
                    counter = bucket_array[bucket_idx][empty_idx_b][1]
                    bucket_array[bucket_idx][empty_idx_b][1] = update_active_counter(counter)
                    bucket_array[bucket_idx][empty_idx_b][0] = fingerprint1
                    hit_flag = True
                # exist no empty cell, decrease the smallest one
                else:
                    if Min_b > Min_s:
                        pr = 1.08**(-math.log(Min_s, 2))
                        if random.random() < pr:
                            bucket_array[bucket_idx][min_idx_s][1] -= 1
                            if bucket_array[bucket_idx][min_idx_s][1] == 0:
                                bucket_array[bucket_idx][min_idx_s][0] = fingerprint2
                                bucket_array[bucket_idx][min_idx_s][1] = 1
                    else:
                        if bucket_array[bucket_idx][min_idx_b][1][0] == "0":
                            pr = 1.08**(-math.log(Min_b, 2))
                            if random.random() < pr:
                                counter = bucket_array[bucket_idx][min_idx_b][1]
                                bucket_array[bucket_idx][min_idx_b][1] = decrease_active_counter(counter)
                                if read_active_counter(bucket_array[bucket_idx][min_idx_b][1]) == 0:
                                    bucket_array[bucket_idx][min_idx_b][0] = fingerprint1
                                    counter = bucket_array[bucket_idx][min_idx_b][1]
                                    bucket_array[bucket_idx][min_idx_b][1] = update_active_counter(counter)   

    return bucket_array, est_label

def query(bucket_array, per_flow_size, memory_size, est_label):
    bucket_num = (memory_size*8*1024)//128
    
    real_size = []
    est_size = []
    aae = 0
    
    for src in est_label:
        fingerprint1 = mmh3.hash(src, seed = 0, signed=False) % (2**16)
        fingerprint2 = mmh3.hash(src, seed = 0, signed=False) % (2**8)
        bucket_idx = mmh3.hash(src, seed = 1, signed=False) % bucket_num
        
        for i in range(5):
            if i < 3:
                if bucket_array[bucket_idx][i][0] == fingerprint1:
                    counter = bucket_array[bucket_idx][i][1]
                    est_size.append( read_active_counter(counter) )
                    real_size.append( per_flow_size[src] )
                    aae += abs( read_active_counter(counter) - per_flow_size[src] )
                    continue
            else:
                if bucket_array[bucket_idx][i][0] == fingerprint2:
                    est_size.append( bucket_array[bucket_idx][i][1] )
                    real_size.append( per_flow_size[src] )
                    aae += abs( bucket_array[bucket_idx][i][1] - per_flow_size[src] )
                    continue
    
    #################################################################################
    # the instances reported
    res1 = len(est_label)
    
    #  true heavy hitter instances reported among all the instances reported
    res2 = 0
    for label in est_label:
        if per_flow_size[label] > 750:
            res2 += 1

    PR = res2/res1
    
    # all the heavy hitters
    res3 = 0
    for value in per_flow_size.values():
        if value > 750:
            res3 += 1
    
    RR = res2/res3
    
    return PR, RR


if __name__ == "__main__":
    File = ["../../data/network_traffic/processed/49.txt"]

    per_flow_size = {}
    with open(File,"r") as f:
        for line in f:
            src = line.split()[0]
            if src in per_flow_size:
                per_flow_size[src] += 1
            else:
                per_flow_size[src] = 1
    f.close()

    memory_size = 100
    bucket_array, est_label = switchsketch_3L2S(File, memory_size)
    PR, RR = query(bucket_array, per_flow_size, memory_size, est_label)
