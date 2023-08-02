# SwitchSketch

## Introduction

Heavy hitter detection is a fundamental task in network traffic measurement and security. Existing work faces the dilemma of suffering dynamic and imbalanced traffic characteristics or lowering the detection efficiency and flexibility. In this paper, we propose a flexible sketch called SwitchSketch that embraces dynamic and skewed traffic for efficient and accurate heavy-hitter detection. The key idea of SwitchSketch is allowing the sketch to dynamically switch among different modes and take full use of each bit of the memory. We present an encoding-based switching scheme together with a flexible bucket structure to jointly achieve this goal by using a combination of design features, including variable-length cells, counter shrink, embedded metadata, and transferable modes. We further implement SwitchSketch on the NetFPGA-1G-CML board. Experimental results based on real Internet traces show that SwitchSketch achieves a high $\text{F}_\beta$-Score of threshold-𝑡 detection (consistently higher than 0.938) and over 99\% precision rate of top-𝑘 detection under a tight memory size. Besides, it outperforms the state-of-the-art by reducing the ARE by 30.77\%~99.96\%.

## Descriptions

### Implementation: 

The hardware and software versions of SwitchSketch are implemented on FPGA and CPU platforms respectively. 

### Dataset: 

The datasets we use are two 1-minute traces downloaded from CAIDA-2016(https://catalog.caida.org/details/dataset/passive_2016_pcap) and CAIDA-2019(https://catalog.caida.org/details/dataset/passive_2019_pcap). In the traces, the data items are IP packets, and each flow’s key is identified by the source IP address. The 1-minute CAIDA-2016 trace contains over 31M items belonging to 0.58M flows, and the 1-minute CAIDA-2019 trace contains over 36M items belonging to 0.37M flows.
