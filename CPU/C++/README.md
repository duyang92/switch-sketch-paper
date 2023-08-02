# C++ Codes for Evaluation

## Platform

The codes is compiled using gcc version 7.5.0 (Ubuntu 7.5.0-3ubuntu1~18.04) and run on a server equipped with two six-core Intel Xeon E5-2643 v4 3.40GHz CPU and 256GB RAM.

## Descriptions

- `./DHS`: C++ codes for evaluation of DHS
- `./HeavyGuardian`: C++ codes for evaluation of HeavyGuardian
- `./SwitchSketch`: C++ codes for evaluation of SwitchSketch
- `./WavingSketch`:C++ codes for evaluation of WavingSketch
- `./data`: Dataset path

## Run

Before run those codes, you need to download the datasets from https://catalog.caida.org/details/dataset/passive_2016_pcap and https://catalog.caida.org/details/dataset/passive_2019_pcap , then move them to ./data . 

We preprocessed the datasets to remove all IPv6 packets to ensure that every 8 bytes correspond to 1 IP packet. The first 4 bytes are the source IP address and the last 4 bytes are the destination IP address.

Compile and run with the following commands：
```shell
g++ ./DHS/DHS.cpp -O3
```
```shell
.DHS/a.out
```
```shell
g++ ./HeavyGuardian/HeavyGuardian.cpp -O3 
```
```shell
.HeavyGuardian/a.out
```
```shell
g++ ./SwitchSketch/SwitchSketch.cpp -O3 
```
```shell
.SwitchSketch/a.out
```
```shell
g++ ./WavingSketch/WavingSketch.cpp -O3 
```
```shell
.WavingSketch/a.out
```
