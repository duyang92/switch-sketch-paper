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


def switchsketch_4L0S(file, memory_size):
    bucket_array = []
    bucket_num = (memory_size*8*1024)//128
    for i in range(bucket_num):
        bucket_array.append([ [None, "0000000000000000"] for j in range(4) ])
    est_label = set()
    count = 0
    with open(file, "r") as f:

        for line in f:

            src = line.split()[0]
            fingerprint = mmh3.hash(src, seed = 0, signed=False) % (2**16)
            bucket_idx = mmh3.hash(src, seed = 1, signed=False) % bucket_num
            
            empty_flag = False
            empty_idx = -1
            hit_flag = False
            Min = float('inf')
            min_idx = None
            for i in range(4):
                if bucket_array[bucket_idx][i][0] == fingerprint:
                    hit_flag = True
                    # update active counter
                    counter = bucket_array[bucket_idx][i][1]
                    bucket_array[bucket_idx][i][1] = update_active_counter(counter)
                    if read_active_counter(bucket_array[bucket_idx][i][1]) == 750:
                        est_label.add(src)
                    continue
                elif bucket_array[bucket_idx][i][0] == None or read_active_counter(bucket_array[bucket_idx][i][1]) == 0:
                    empty_flag = True
                    empty_idx = i
                else:
                    if read_active_counter(bucket_array[bucket_idx][i][1]) < Min:
                        Min = read_active_counter(bucket_array[bucket_idx][i][1])
                        min_idx = i
            
            # fail to insert
            if not hit_flag:
                #  exist an empty cell, insert
                if empty_flag:
                    counter = bucket_array[bucket_idx][empty_idx][1]
                    bucket_array[bucket_idx][empty_idx][1] = update_active_counter(counter)
                    bucket_array[bucket_idx][empty_idx][0] = fingerprint
                    continue
                # exist no empty cell, decrease the smallest one
                else:
                    if bucket_array[bucket_idx][min_idx][1][0] == "0":
                        pr = 1.08**(-math.log(Min, 2))
                        if random.random() < pr:
                            counter = bucket_array[bucket_idx][min_idx][1]
                            bucket_array[bucket_idx][min_idx][1] = decrease_active_counter(counter)
                            # insert when a cell decrease to 0
                            if read_active_counter(bucket_array[bucket_idx][min_idx][1]) == 0:
                                bucket_array[bucket_idx][min_idx][0] = fingerprint
                                counter = bucket_array[bucket_idx][min_idx][1]
                                bucket_array[bucket_idx][min_idx][1] = update_active_counter(counter)                                

    
    return bucket_array, est_label

def query(bucket_array, per_flow_size, memory_size, est_label):
    bucket_num = (memory_size*8*1024)//128
    
    real_size = []
    est_size = []
    aae = 0
    
    for src in est_label:
        fingerprint = mmh3.hash(src, seed = 0, signed=False) % (2**16)
        bucket_idx = mmh3.hash(src, seed = 1, signed=False) % bucket_num
        
        for i in range(4):
            if bucket_array[bucket_idx][i][0] == fingerprint:
                counter = bucket_array[bucket_idx][i][1]
                est_size.append( read_active_counter(counter) )
                real_size.append( per_flow_size[src] )
                aae += abs( read_active_counter(counter) - per_flow_size[src] )
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
    bucket_array, est_label = switchsketch_4L0S(File, memory_size)
    PR, RR = query(bucket_array, per_flow_size, memory_size, est_label)
