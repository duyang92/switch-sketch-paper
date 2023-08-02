#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <stdint.h>
#include <memory>
#include <iostream>
#include <cmath>
#include <math.h>
#include <time.h>
#include <string.h>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <string>
using namespace std;
//the bytes num of one bucket
#define landa_h 16
//the exponential decay parameter
#define b 1.08
//bytes num of key
#define KEY_SIZE 4

#define THRESHOLD 750
#define ListLength 6400 // 6400*4 bytes=25KB

uint8_t savedKeys[ListLength][KEY_SIZE]{0};
uint32_t numOfSavedKeys=0;

inline uint32_t BKDRHash(const uint8_t* str, uint32_t len)
{
    uint32_t seed = 13131;
    uint32_t hash = 0;

    for (uint32_t i = 0; i < len; i++) {
        hash = hash * seed + str[i];
    }

    return (hash & 0x7FFFFFFF);
}

//get the 16-bit fingerprint
inline uint16_t finger_print(uint32_t hash)
{
    hash ^= hash >> 16;
    hash *= 0x85ebca6b;
    hash ^= hash >> 13;
    hash *= 0xc2b2ae35;
    hash ^= hash >> 16;
    return hash & 65535;
}

class hg_node
{
public:
    uint8_t heavy[landa_h]{0};
    uint32_t usage;//num-/used-
    hg_node()
    {
        usage = (landa_h/2)<<24;
        //here we withdraw the num2 because it can be inferred
        //usage += 15;
        //all level 2 initially
        //usage += (2<<8);
        //usage += (1<<16);
    }
    void levelup(int level, int f,int oriCellIndex)
    {
        double ran = 1.0 * rand() / RAND_MAX;
        switch(level)
        {
            case 1:
            {
                int num2 = usage>>24;
                int usage2 = usage & 0xFF;
                int start = 0;
                int end = start + num2*2;
                if(usage2 < num2)//exist empty space
                {
                    for(int i = start;i<end;i+=2)
                    {
                        if(i >= landa_h)printf("error warning! levelup-1 find level-2 empty\n");
                        if(heavy[i+1] == 0)
                        {
                            heavy[i] = f&0xFF;
                            heavy[i+1] = 1;
                            usage += 1;
                            return;
                        }
                    }
                }
                else //no empty space
                {
                    //find weakest guardian
                    if(num2 == 0)return;
                    int min_f = -1;//index of the weakest cell
                    int min_fq = -1;// counter value of the weakest cell
                    for(int i = start;i<end;i+=2)
                    {
                        if(i >= landa_h)printf("error warning! levelup-1 find weakest level-2\n");
                        if(min_f == -1)
                        {
                            min_f = i;
                            min_fq = heavy[i+1];
                        }
                        else if(heavy[i+1] < min_fq)
                        {
                            min_f = i;
                            min_fq = heavy[i+1];
                        }
                    }
                    //exponential decay
                    if(min_f==-1 || min_fq < 0)printf("minus warning! levelup-1 decay level-2\n");
                    if (ran < pow(b, min_fq * -1))
                    {
                        heavy[min_f+1] -= 1;
                        if(heavy[min_f+1] <= 0)
                        {
                            heavy[min_f] = (f&0xFF);
                            heavy[min_f+1] = 1;
                        }
                    }
                }
                break;
            }
            case 2:
            {
                int num3 = (usage>>8) & 0xF;
                int num4 = (usage>>16) & 0xF;
                int num2 = usage>>24;
                int usage2 = usage & 0xFF;
                int usage3 = (usage>>12) & 0xF;

                if(num3 == usage3 && num2 >= 3)// no empty level-3 cell, use three level-2 cells to trade for two new level-3 cell
                {
                    //clear original cell
                    heavy[oriCellIndex] = 0;
                    heavy[oriCellIndex+1] = 0;
                    usage2-=1;

                    vector<int> weaks(3, -1);//store the counters of the 3 weakest level-2 cells
                    vector<int> widx(3, -1);//store the indexes of the 3 weakest level-2 cells
                    for(int i = 0; i< num2*2; i+=2)
                    {
                        if(i >= landa_h)printf("error warning! levelup-2 find weakest\n");
                        for(int j = 0; j<3; j++)
                        {
                            if(widx[j] == -1)
                            {
                                widx[j] = i;
                                weaks[j] = heavy[i+1];
                                break;
                            }
                            else if(heavy[i+1] < weaks[j])
                            {
                                //delete the cell stored in widx[rest-1]
                                for(int l = 2; l>j; l--)
                                {
                                    widx[l] = widx[l-1];
                                    weaks[l] = weaks[l-1];
                                }
                                widx[j] = i;
                                weaks[j] = heavy[i+1];
                                break;
                            }
                        }
                    }

                    int kill = 0;
                    sort(widx.begin(), widx.end());
                    for(int i = widx[kill]; i< num2*2; i+=2)
                    {
                        if(i >= landa_h)printf("error warning! levelup-2 remove weakest level-2 cells\n");
                        if(i == widx[kill])
                        {
                            kill++;
                            heavy[i] = 0;
                            heavy[i+1] = 0;
                            continue;
                        }
                        else if(kill)
                        {
                            heavy[i-kill*2] = heavy[i];
                            heavy[i+1-kill*2] = heavy[i+1];
                            heavy[i] = 0;
                            heavy[i+1] = 0;
                        }
                    }

                    int usage4 = (usage>>20) & 0xF;
                    num3 += 2;
                    num2 -= 3;
                    if(usage2-num2>=0){
                        usage2=num2;
                    }

                    usage = 0;
                    usage += usage2;
                    usage += (num3<<8);
                    usage += (usage3<<12);
                    usage += (num4<<16);
                    usage += (usage4<<20);
                    usage += (num2<<24);
                }

                int start = num2*2;
                int end = start + num3*3;
                if(usage3 < num3)//exist empty space
                {
                    for(int i = start;i<end;i+=3)
                    {
                        if(i >= landa_h)printf("error warning! levelup-2 find empty\n");
                        uint32_t c = ((uint32_t)(heavy[i + 1] & 15) << 8) + heavy[i + 2];
                        if(c == 0)
                        {
                            f &= 0xFFF;
                            heavy[i] = (f>>4);
                            heavy[i+1] = ((f&0xF)<<4) + 1;
                            heavy[i+2] = 0;

                            usage += (1<<12);
                            return;
                        }
                    }
                    cout<<"error warning! levelup-2 not find empty"<<endl;
                }
                else //no empty space
                {
                    //find weakest guardian
                    int min_f = -1;
                    int min_fq = -1;
                    for(int i = start;i<end;i+=3)
                    {
                        if(i >= landa_h)printf("error warning! levelup-2 find weakest to decay\n");
                        int freq = ((int)(heavy[i+1]&0xF)<<8)+heavy[i+2];
                        if(min_f == -1)
                        {
                            min_f = i;
                            min_fq = freq;
                        }
                        else if(freq < min_fq && freq>0)
                        {
                            min_f = i;
                            min_fq = freq;
                        }
                    }
                    //exponential decay
                    if(min_f==-1 || min_fq < 0)printf("minus warning! levelup-2\n");
                    if (ran < pow(b, min_fq * -1))
                    {
                        min_fq -= 1;
                        if(min_fq <= 255)
                        {
                            f &= 0xFFF;
                            heavy[min_f] = (f>>4);
                            heavy[min_f+1] = ((f&0xF)<<4) + 1;
                            heavy[min_f+2] = 0;
                        }
                        else
                        {
                            heavy[min_f+1] = (heavy[min_f+1]&0xF0)+(min_fq>>8);
                            heavy[min_f+2] = (min_fq&0xFF);
                        }
                    }
                }
                break;
            }
            case 3:
            {
                int num3 = (usage>>8) & 0xF;
                int num4 = (usage>>16) & 0xF;
                int num2 = usage>>24;
                int usage2 = usage & 0xFF;
                int usage4 = (usage>>20) & 0xF;

                if(num4 == usage4 && num2 >= 3)// no empty level-4 cell, if exists 3 level-2 cells, then use 3 level-2 cells to trade for a new level-4 cell
                {
                    int usage3 = (usage>>12) & 0xF;
                    //clear original cell
                    heavy[oriCellIndex] = 0;
                    heavy[oriCellIndex+1] = 0;
                    heavy[oriCellIndex+2] = 0;
                    usage3--;

                    int toDeleteNum=3;
                    vector<int> weaks(toDeleteNum, -1);
                    vector<int> widx(toDeleteNum, -1);
                    for(int i = 0; i< num2*2; i+=2)
                    {
                        if(i >= landa_h)printf("error warning! levelup-3 find weakest level-2 cells to remove\n");
                        for(int j = 0; j<toDeleteNum; j++)
                        {
                            if(widx[j] == -1)
                            {
                                widx[j] = i;
                                weaks[j] = heavy[i+1];
                                break;
                            }
                            else if(heavy[i+1] < weaks[j])
                            {
                                for(int l = toDeleteNum-1; l>j; l--)
                                {
                                    widx[l] = widx[l-1];
                                    weaks[l] = weaks[l-1];
                                }
                                widx[j] = i;
                                weaks[j] = heavy[i+1];
                                break;
                            }
                        }
                    }
                    //remove level-2 cells
                    int kill = 0;
                    sort(widx.begin(), widx.end());
                    for(int i = widx[kill]; i< num2*2; i+=2)
                    {
                        if(i >= landa_h)printf("error warning! levelup-3 remove weakest level-2 cells\n");
                        if(i == widx[kill])
                        {
                            kill++;
                            heavy[i] = 0;
                            heavy[i+1] = 0;
                            continue;
                        }
                        else if(kill)
                        {
                            heavy[i-kill*2] = heavy[i];
                            heavy[i+1-kill*2] = heavy[i+1];
                            heavy[i] = 0;
                            heavy[i+1] = 0;
                        }
                    }
                    //move level-3 cells
                    for(int i = num2*2; i<num2*2+num3*3; i+=3)
                    {
                        if(i >= landa_h)printf("error warning! levelup-3 move level-3 cells\n");
                        heavy[i-6] = heavy[i];
                        heavy[i+1-6] = heavy[i+1];
                        heavy[i+2-6] = heavy[i+2];
                        heavy[i] = 0;
                        heavy[i+1] = 0;
                        heavy[i+2] = 0;
                    }
                    //move level-4 cells, since three level-2 cells deallocate more bits than a level-4 cell
                    int startByteIdx = num2*2+num3*3;
                    int endByteIdx = startByteIdx + ((num4*36)>>3);
                    if((num4*36)&0x7!=0){
                        endByteIdx+=1;
                    }
                    for(int i = startByteIdx; i<endByteIdx; i+=1)
                    {
                        if(i >= landa_h)printf("error warning! levelup-3 move level-3 cells\n");
                        heavy[i-6] = heavy[i];
                        heavy[i] = 0;
                    }

                    num2-=3;
                    num4+=1;
                    if(usage2-num2>=0){
                        usage2=num2;
                    }

                    usage = 0;
                    usage += usage2;
                    usage += (num3<<8);
                    usage += (usage3<<12);
                    usage += (num4<<16);
                    usage += (usage4<<20);
                    usage += num2<<24;
                }else if(num4 == usage4 && num3 >= 3){// no empty level-4 cell, no 3 level-2 cells. if exists 3 level-3 cells, then use 3 level-3 cells to trade for 2 new level-4 cell
                    int usage3 = (usage>>12) & 0xF;
                    //clear original cell
                    heavy[oriCellIndex] = 0;
                    heavy[oriCellIndex+1] = 0;
                    heavy[oriCellIndex+2] = 0;
                    usage3--;

                    int toDeleteNum=3;
                    vector<int> weaks(toDeleteNum, -1);
                    vector<int> widx(toDeleteNum, -1);
                    for(int i = num2*2; i< num2*2+num3*3; i+=3)
                    {
                        if(i >= landa_h)printf("error warning! levelup-3 find weakest level-3 cells to remove\n");
                        for(int j = 0; j<toDeleteNum; j++)
                        {
                            int counter=((int)(heavy[i+1]&0xF)<<8)+heavy[i+2];
                            if(widx[j] == -1)
                            {
                                widx[j] = i;
                                weaks[j] = counter;
                                break;
                            }
                            else if(counter < weaks[j])
                            {
                                for(int l = toDeleteNum-1; l>j; l--)
                                {
                                    widx[l] = widx[l-1];
                                    weaks[l] = weaks[l-1];
                                }
                                widx[j] = i;
                                weaks[j] = counter;
                                break;
                            }
                        }
                    }
                    //remove level-3 cells
                    int kill = 0;
                    sort(widx.begin(), widx.end());
                    for(int i = widx[kill]; i< num2*2+num3*3; i+=3)
                    {
                        if(i >= landa_h)printf("error warning! levelup-3 remove weakest level-3 cells\n");
                        if(i == widx[kill])
                        {
                            kill++;
                            heavy[i] = 0;
                            heavy[i+1] = 0;
                            heavy[i+2] = 0;
                            continue;
                        }
                        else if(kill)
                        {
                            heavy[i-kill*3] = heavy[i];
                            heavy[i+1-kill*3] = heavy[i+1];
                            heavy[i+2-kill*3] = heavy[i+2];
                            heavy[i] = 0;
                            heavy[i+1] = 0;
                            heavy[i+2] = 0;
                        }
                    }

                    num3-=3;
                    num4+=2;
                    if(usage3-num3>=0){
                        usage3=num3;
                    }

                    usage = 0;
                    usage += usage2;
                    usage += (num3<<8);
                    usage += (usage3<<12);
                    usage += (num4<<16);
                    usage += (usage4<<20);
                    usage += num2<<24;
                }

                int startBitIdx = num2*16+num3*24;
                int endBitIdx = startBitIdx + num4*36;
                if(usage4 < num4)//exist empty space
                {
                    for(int i = startBitIdx;i<endBitIdx;i+=36)
                    {
                        uint32_t tmpCounter=0;
                        readLevel4Counter(i,tmpCounter);
                        if(i >= landa_h*8)printf("error warning! levelup-3 find empty\n");
                        if(tmpCounter == 0)
                        {
                            int bitIdx=i;
                            int extractedBitsNum=0;
                            while(extractedBitsNum<16){
                                int toWriteBitsNum=min(16-extractedBitsNum,8-(bitIdx&0x7));
                                extractedBitsNum+=toWriteBitsNum;
                                heavy[bitIdx>>3]&=~(((1<<(toWriteBitsNum))-1)<<(bitIdx&0x7));
                                heavy[bitIdx>>3]|=((f>>(16-extractedBitsNum))&((1<<(toWriteBitsNum))-1))<<(bitIdx&0x7);
                                bitIdx+=toWriteBitsNum;
                            }
                            extractedBitsNum=0;
                            while(extractedBitsNum<20){
                                int toWriteBitsNum=min(20-extractedBitsNum,8-(bitIdx&0x7));
                                extractedBitsNum+=toWriteBitsNum;
                                heavy[bitIdx>>3]&=~(((1<<(toWriteBitsNum))-1)<<(bitIdx&0x7));
                                heavy[bitIdx>>3]|=((4096>>(20-extractedBitsNum))&((1<<(toWriteBitsNum))-1))<<(bitIdx&0x7);
                                bitIdx+=toWriteBitsNum;
                            }
                            usage += (1<<20);
                            return;
                        }
                    }
                    cout<<"error warning!  levelup-3 not find empty"<<endl;
                }
                else //no empty space
                {
                    if(num4 == 0)return;
                    //find the weakest guardian
                    int min_f = -1;
                    int min_fq = -1;
                    for(int i = startBitIdx;i<endBitIdx;i+=36)
                    {
                        if(i >= landa_h*8)printf("error warning! levelup-3 find weakest cell to decay\n");
                        uint32_t freq=0;
                        readLevel4Counter(i,freq);

                        if(min_f == -1)
                        {
                            min_f = i;
                            min_fq = freq;
                        }
                        else if(freq < min_fq && freq>0)
                        {
                            min_f = i;
                            min_fq = freq;
                        }
                    }
                    //exponential decay
                    if(min_f==-1 || min_fq <0)printf("minus warning! levelup-3\n");
                    if (ran < pow(b, min_fq * -1))
                    {
                        min_fq -= 1;
                        if(min_fq <= 4095)
                        {
                            writeLevel4Cell(min_f,f,4096);
                        }
                        else
                        {
                            writeLevel4Counter(min_f,min_fq);
                        }
                    }
                }
                break;
            }
            default:break;
        }
        return;
    }

    void readLevel4Cell(int cellStartBitIdx,uint16_t &fp,uint32_t &counter){
        fp=0;
        counter=0;

        int bitIdx=cellStartBitIdx;
        int extractedBitsNum=0;
        while(extractedBitsNum<16){
            int toReadBitsNum=min(16-extractedBitsNum,8-(bitIdx&0x7));
            extractedBitsNum+=toReadBitsNum;
            fp+=((heavy[bitIdx>>3]>>(bitIdx&0x7))&((1<<(toReadBitsNum))-1))<<(16-extractedBitsNum);
            bitIdx+=toReadBitsNum;
        }
        extractedBitsNum=0;
        while(extractedBitsNum<20){
            int toReadBitsNum=min(20-extractedBitsNum,8-(bitIdx&0x7));
            extractedBitsNum+=toReadBitsNum;
            counter+=((heavy[bitIdx>>3]>>(bitIdx&0x7))&((1<<(toReadBitsNum))-1))<<(20-extractedBitsNum);
            bitIdx+=toReadBitsNum;
        }
    }

    void readLevel4Counter(int cellStartBitIdx,uint32_t &counter){
        counter=0;
        int bitIdx=cellStartBitIdx+16;
        int extractedBitsNum=0;
        while(extractedBitsNum<20){
            int toReadBitsNum=min(20-extractedBitsNum,8-(bitIdx&0x7));
            extractedBitsNum+=toReadBitsNum;
            counter+=((heavy[bitIdx>>3]>>(bitIdx&0x7))&((1<<(toReadBitsNum))-1))<<(20-extractedBitsNum);
            bitIdx+=toReadBitsNum;
        }
    }
    void writeLevel4Cell(int cellStartBitIdx,uint16_t fp,uint32_t counter){
        int bitIdx=cellStartBitIdx;
        int extractedBitsNum=0;
        while(extractedBitsNum<16){
            int toWriteBitsNum=min(16-extractedBitsNum,8-(bitIdx&0x7));
            extractedBitsNum+=toWriteBitsNum;
            heavy[bitIdx>>3]&=~(((1<<(toWriteBitsNum))-1)<<(bitIdx&0x7));
            heavy[bitIdx>>3]|=((fp>>(16-extractedBitsNum))&((1<<(toWriteBitsNum))-1))<<(bitIdx&0x7);
            bitIdx+=toWriteBitsNum;
        }
        extractedBitsNum=0;
        while(extractedBitsNum<20){
            int toWriteBitsNum=min(20-extractedBitsNum,8-(bitIdx&0x7));
            extractedBitsNum+=toWriteBitsNum;
            heavy[bitIdx>>3]&=~(((1<<(toWriteBitsNum))-1)<<(bitIdx&0x7));
            heavy[bitIdx>>3]|=((counter>>(20-extractedBitsNum))&((1<<(toWriteBitsNum))-1))<<(bitIdx&0x7);
            bitIdx+=toWriteBitsNum;
        }
    }

    void writeLevel4Counter(int cellStartBitIdx,uint32_t counter){
        int bitIdx=cellStartBitIdx+16;
        int extractedBitsNum=0;
        while(extractedBitsNum<20){
            int toWriteBitsNum=min(20-extractedBitsNum,8-(bitIdx&0x7));
            extractedBitsNum+=toWriteBitsNum;
            heavy[bitIdx>>3]&=~(((1<<(toWriteBitsNum))-1)<<(bitIdx&0x7));
            heavy[bitIdx>>3]|=((counter>>(20-extractedBitsNum))&((1<<(toWriteBitsNum))-1))<<(bitIdx&0x7);
            bitIdx+=toWriteBitsNum;
        }
    }
    //level-3 cell: 12+12 ; level-2 cell: 8+8; level-1: not match
    void insert(uint16_t f, uint8_t* key)
    {
        //if exist a flow
        int num3 = (usage>>8) & 0xF;
        int num4 = (usage>>16) & 0xF;
        int num2 = usage>>24;//
        //level 4
        int startBitIdx = num2*16+num3*24;
        int endBitIdx = startBitIdx + num4*36;
        for(int i = startBitIdx;i<endBitIdx;i+=36)
        {
            if(i >= landa_h*8)printf("error warning! level-4 search\n");

            uint16_t tmpFp=0;
            uint32_t tmpCounter=0;
            readLevel4Cell(i,tmpFp,tmpCounter);
            if(tmpFp==f)
            {
                if(tmpCounter!=0xFFFFFF){
                    writeLevel4Counter(i,tmpCounter+1);
                }
                else
                {
                    levelup(4, f,i);
                }
                return;
            }
        }
        //level 3
        int start = num2*2;
        int end = start + num3*3;
        for(int i = start;i<end;i+=3)
        {
            if(i >= landa_h)printf("error warning! level-3 search\n");
            uint16_t e = ((uint16_t)heavy[i]<<4)+(heavy[i+1]>>4);
            uint32_t counter=((uint32_t)(heavy[i+1]&0xF)<<8)+heavy[i+2];
            if(e==(f&0xFFF) && counter>0)
            {
                if(heavy[i+2]<255){
                    heavy[i+2]++;
                }
                else if((heavy[i+1] & 0xF)!= 15)
                {
                    heavy[i+1]++;
                    heavy[i+2] = 0;
                }
                else
                {
                    levelup(3, f,i);
                    return;
                }

                if (numOfSavedKeys < ListLength && counter == THRESHOLD) {
                    memcpy(savedKeys[numOfSavedKeys],key,KEY_SIZE);
                    numOfSavedKeys++;
                }
                return;
            }
        }
        //level 2
        start = 0;
        end = start + num2*2;
        for(int i = start;i<end;i+=2)
        {
            if(i >= landa_h){
                printf("error warning! level-2 search\n");
            }
            uint16_t e = heavy[i];
            if(e==(f&0xFF) && heavy[i+1]>0)
            {
                if(heavy[i+1]<255)heavy[i+1]++;
                else
                {
                    levelup(2, f,i);
                }
                return;
            }
        }

        //no existing flow
        levelup(1, f,-1);
    }
    uint32_t query(uint16_t f)
    {
        int num3 = (usage>>8) & 0xF;
        int num4 = (usage>>16) & 0xF;
        int num2 = usage>>24;
        //level 4
        int startBitIdx = num2*16+num3*24;
        int endBitIdx = startBitIdx + num4*36;
        for(int i = startBitIdx;i<endBitIdx;i+=36)
        {

            uint16_t tmpFp=0;
            uint32_t tmpCounter=0;

            readLevel4Cell(i,tmpFp,tmpCounter);
            if(tmpFp==f && tmpCounter>0)
            {
                return tmpCounter;
            }
        }
        //level 3
        int start = num2*2;
        int end = start + num3*3;
        for(int i = start;i<end;i+=3)
        {
            uint16_t e = ((uint16_t)heavy[i]<<4)+(heavy[i+1]>>4);
            uint32_t counter=((uint32_t)(heavy[i+1]&0xF)<<8)+heavy[i+2];
            if(e==(f&0xFFF) && counter>0)
            {
                return counter;
            }
        }
        //level 2
        start = 0;
        end = start + num2*2;
        for(int i = start;i<end;i+=2)
        {
            uint16_t e = heavy[i];
            if(e==(f&0xFF) && heavy[i+1]>0)
            {
                return heavy[i+1];
            }
        }

        //no existing flow
        return 0;
    }
};

bool cmpPairFunc(pair<uint8_t*, int>p1, pair<uint8_t*, int>p2)
{
    return p1.second > p2.second;
}

//for recording information in the unordered_map
struct CmpFunc {
    bool operator()(const uint8_t* keyA, const uint8_t* keyB) const {
        return memcmp(keyA, keyB, KEY_SIZE) == 0;
    }
};

struct HashFunc {
    uint32_t operator()(const uint8_t* key) const {
        uint32_t hashValue = BKDRHash(key, KEY_SIZE);
        return hashValue;
    }
};
uint32_t ReadInTraces(const char* traceFilePath, uint8_t** keys, unordered_map<uint8_t*, uint32_t, HashFunc, CmpFunc>& actualFlowSizes)
{
   FILE* fin = fopen(traceFilePath, "rb");

   uint8_t temp[KEY_SIZE]{ 0 };
   uint8_t temp2[9]{ 0 };
   uint32_t count = 0;
   uint8_t* key;
   while (fread(temp, 1, KEY_SIZE, fin) == KEY_SIZE) {
       key = new uint8_t[KEY_SIZE] {0};
       memcpy(key, temp, KEY_SIZE);
       keys[count] = key;
       if (actualFlowSizes.find(key) == actualFlowSizes.end()) {
           actualFlowSizes[key] = 1;
       }
       else {
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

int main()
{
    //DHS-offline version: retrieve all flow labels to get estimated results
    //Here, we do not consider how to get the flow labels

    //prepare DHS
    cout << "prepare DHS" << endl;
    uint32_t singleMetaDataBitsNum = 32;
    uint32_t bucketMem = 200 * 1024 * 8;
    uint32_t bucketNum = bucketMem / (landa_h * 8 + singleMetaDataBitsNum);
    vector<hg_node> hg(bucketNum);
    cout << "bucketMem: " << bucketMem / (8 * 1024) << "KB\t metaDataBitsNum: " << singleMetaDataBitsNum << "\t bucketNum: " << bucketNum << endl;
    cout << "*********************" << endl;

    //prepare dataset
    cout << "prepare dataset" << endl;
    uint32_t maxItemNum = 40000000;//max number of items
    uint8_t** keys = new uint8_t* [maxItemNum];//to store keys of all items
    unordered_map<uint8_t*, uint32_t, HashFunc, CmpFunc> actualFlowSizes;//count the actual flow sizes
//     uint32_t actualItemNum = ReadInTraces(R"(G:\networkTrafficData\2016\equinix-chicago.dirA\processed_onlyIpv4Num_Char8BinaryFormat\49.dat)", keys, actualFlowSizes);
//     uint32_t actualItemNum = ReadInTraces(R"(../data/49.dat)", keys, actualFlowSizes);
   uint32_t actualItemNum = ReadInTraces(R"(../data/20190117-130000-new.dat)", keys, actualFlowSizes);
    cout << "number of items: " << actualItemNum << endl;
    cout << "number of flows: " << actualFlowSizes.size() << endl;
    cout << "*********************" << endl;


    //insert items
    cout << "insert items" << endl;
    numOfSavedKeys = 0;

    clock_t time1 = clock();
    for (uint32_t i = 0; i < actualItemNum; i++) {
    	uint32_t hash = BKDRHash(keys[i], KEY_SIZE);
    	hg[hash % bucketNum].insert(finger_print(hash),keys[i]);
    }
    clock_t time2 = clock();

    double numOfSeconds = (double)(time2 - time1) / CLOCKS_PER_SEC;//the seconds using to insert items
    double throughput = (actualItemNum / 1000000.0) / numOfSeconds;
    cout << "*********************" << endl;


    ////calculate metrics
    //get sorted acutal flow sizes and sorted estimated flow sizes
    vector<pair<uint8_t*, uint32_t>> actualFlowSizesVector;
    for (auto iter = actualFlowSizes.begin(); iter != actualFlowSizes.end(); iter++) {
        actualFlowSizesVector.push_back(make_pair(iter->first, iter->second));
    }
    sort(actualFlowSizesVector.begin(), actualFlowSizesVector.end(), cmpPairFunc);

    vector<pair<uint8_t*, uint32_t>> estimatedFlowSizesVector;
    unordered_map<uint8_t*, uint32_t, HashFunc, CmpFunc> estimatedFlowSizes;
    for (int i = 0; i < numOfSavedKeys; i++) {
    	uint8_t* key = savedKeys[i];

    	uint32_t hash = BKDRHash(key, KEY_SIZE);
    	uint32_t estimatedSize = hg[hash % bucketNum].query(finger_print(hash));
    	if (estimatedFlowSizes.find(key) == estimatedFlowSizes.end()) {
    		estimatedFlowSizesVector.push_back(make_pair(key, estimatedSize));
    		estimatedFlowSizes[key] = estimatedSize;
    	}
    }
    sort(estimatedFlowSizesVector.begin(), estimatedFlowSizesVector.end(), cmpPairFunc);

    //get acutal heavyHitters
    unordered_map<uint8_t*, uint32_t, HashFunc, CmpFunc> actualHeavyHitterFlowSizes;
    for (uint32_t idx = 0; idx < actualFlowSizesVector.size(); idx++) {
        uint32_t actualSize = actualFlowSizesVector[idx].second;
        if(actualSize>=THRESHOLD){
            actualHeavyHitterFlowSizes[actualFlowSizesVector[idx].first] = actualSize;
        }else{
            break;
        }
    }
    uint32_t TPNum = 0;//True positive num
    uint32_t FPNum=0;

    for (uint32_t idx = 0; idx < estimatedFlowSizesVector.size(); idx++) {
        uint8_t* key = estimatedFlowSizesVector[idx].first;
        if (actualHeavyHitterFlowSizes.find(key) != actualHeavyHitterFlowSizes.end()) {
            TPNum++;
        }else{
            FPNum++;
        }
    }

    double AAEOfHH = 0;
    double AREOfHH = 0;
    ofstream report("./DHS-YJK -Report.txt");  //打开输出文件
    for (uint32_t idx = 0; idx < estimatedFlowSizesVector.size(); idx++) {

        uint8_t* key = estimatedFlowSizesVector[idx].first;
        uint32_t estimatedSize=estimatedFlowSizesVector[idx].second;
        uint32_t actualSize = 0;
        if (actualFlowSizes.find(key) != actualFlowSizes.end()) {
            actualSize = actualFlowSizes[key];
        }

        AAEOfHH += abs((double)estimatedSize - actualSize);
        double rE = abs((double)estimatedSize - actualSize) / actualSize;
        AREOfHH += rE;

        if(actualSize<THRESHOLD){
            report<<"++++";
        }
        report << "Rep-" << idx << "\t" << estimatedSize << "\t" << actualSize << "\t" << setiosflags(ios::fixed | ios::right) << setprecision(6) << AAEOfHH / (idx + 1) << "\t" << AREOfHH / (idx + 1)  << endl;
    }
    report.close();

    AAEOfHH = AAEOfHH / (TPNum+FPNum);
    AREOfHH=AREOfHH / (TPNum+FPNum);
    double precision=(double)TPNum/(TPNum+FPNum);

    uint32_t FNNum=actualHeavyHitterFlowSizes.size()-TPNum;
    double recall=(double)TPNum/actualHeavyHitterFlowSizes.size();

    ofstream out("./DHS-YJK.txt",ios::app);  //打开输出文件
    out<<"****************************"<<endl;
    out << "threshold to save keys: " << THRESHOLD << ", key list size " << ListLength << endl;
    out<<"collect "<<numOfSavedKeys<<" flow keys"<<endl;
    out<<"get "<<estimatedFlowSizes.size()<<" flows"<<endl;
    out<<"TP="<<TPNum<<"\nFP="<<FPNum<<"\nFN="<<FNNum<<endl;

    out<<"Recall="<<TPNum<<"/"<<actualHeavyHitterFlowSizes.size()<<"="<<recall<<endl;
    out<<"PR="<<TPNum<<"/"<<(TPNum+FPNum)<<"="<<precision<<endl;
    out<<"ARE="<<AREOfHH <<"\nAAE="<<AAEOfHH<< endl;
    out <<"Th="<< throughput <<" mps\n\n"<< endl;
    out.close();


    ofstream real("./DHS-YJK -Real.txt");  //打开输出文件
    AAEOfHH = 0;
    AREOfHH = 0;
    for (uint32_t idx = 0; idx < actualFlowSizesVector.size(); idx++) {
        uint8_t *key = actualFlowSizesVector[idx].first;
        uint32_t actualSize = actualFlowSizesVector[idx].second;
        if(actualSize>=THRESHOLD){
            uint32_t hash = BKDRHash(key, KEY_SIZE);
            uint32_t estimatedSize = hg[hash % bucketNum].query(finger_print(hash));

            AAEOfHH += abs((double)estimatedSize - actualSize);
            double rE = abs((double)estimatedSize - actualSize) / actualSize;
            AREOfHH += rE;
            if(estimatedSize<THRESHOLD){
                real<<"----";
            }
            real <<"Real-" << idx << "\t" << estimatedSize << "\t" << actualSize << "\t" << setiosflags(ios::fixed | ios::right) << setprecision(6) << AAEOfHH / (idx + 1) << "\t" << AREOfHH / (idx + 1)  << endl;
        }else{
            break;
        }
    }
    real.close();

    /* free memory */
    for (int k = 0; k < actualItemNum; k++)
        free(keys[k]);

    return 0;
}