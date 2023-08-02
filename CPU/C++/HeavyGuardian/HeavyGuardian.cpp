#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <string.h>
#include <stdint.h>
#include <random>
#include <string>
#include <memory>
#include <iostream>
#include <cmath>
#include <math.h>
#include<time.h>
#include<unordered_map>
#include<algorithm>
#include<fstream>

using namespace std;

//number of cells in heavy part
#define landa_h 8
//number of cells in light part,each cell is 4 bits
#define landa_l 64
#define b 1.08

#define keyListSize 6000
#define thresholdToSaveKeys 750

#define KEY_SIZE 4

char savedKeys[keyListSize][KEY_SIZE]{ 0 };
unsigned int numOfSavedKeys = 0;

inline unsigned int BKDRHash(const char* str, unsigned int len)
{
    unsigned int seed = 131;
    unsigned int hash = 0;

    for (unsigned int i = 0; i < len; i++) {
        hash = hash * seed + str[i];
    }

    return (hash & 0x7FFFFFFF);
}

inline unsigned short finger_print(unsigned int hash)
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
    pair<unsigned short, unsigned int> heavy[landa_h];
    unsigned char light[landa_l / 2];
    hg_node() :light{ 0 }
    {
    }
    void insert(unsigned short fp, int hash, char* key)
    {
        //printf("fingerprint:%d, hash value: %d\n", fp, hash);
        int heavy_empty = -1;
        int wg = -1;
        int wgf = -1;
        for (int i = 0; i < landa_h; i++)
        {
            if (heavy[i].first == fp)
            {
                heavy[i].second += 1;

                if (numOfSavedKeys < keyListSize && heavy[i].second == thresholdToSaveKeys) {
                    memcpy(savedKeys[numOfSavedKeys], key, KEY_SIZE);
                    numOfSavedKeys++;
                }
                return;
            }
            if (heavy[i].second == 0 && heavy_empty == -1)
            {
                heavy_empty = i;
            }
            else if (heavy[i].second)
            {
                if (wg == -1 || heavy[i].second > wgf)
                {
                    wgf = heavy[i].second;
                    wg = i;
                }
            }
        }
        if (heavy_empty >= 0)
        {
            heavy[heavy_empty].second = 1;
            heavy[heavy_empty].first = fp;
        }
        else
        {
            //exponential decay
            double ran = 1.0 * rand() / RAND_MAX;
            bool replace = 0;
            if (ran < pow(b, wgf * -1))
            {
                heavy[wg].second -= 1;
                if (heavy[wg].second == 0)
                {
                    heavy[wg].first = fp;
                    heavy[wg].second = 1;
                    replace = 1;
                }
            }
            if (replace != 1) //insert into the light part
            {
                int index = hash % (landa_l);
                if (index % 2)
                {
                    int bits = light[index / 2] & 15;
                    if (bits < 15)light[index / 2] = light[index / 2] + 1;
                    
                    if (numOfSavedKeys < keyListSize && bits+1 == thresholdToSaveKeys) {
                        memcpy(savedKeys[numOfSavedKeys], key, KEY_SIZE);
                        numOfSavedKeys++;
                    }
                }
                else
                {
                    int bits = (light[index / 2] & 240);
                    if (bits < 240)light[index / 2] = light[index / 2] + 16;
                    if (numOfSavedKeys < keyListSize && bits+1 == thresholdToSaveKeys) {
                        memcpy(savedKeys[numOfSavedKeys], key, KEY_SIZE);
                        numOfSavedKeys++;
                    }
                }
            }
        }
    }
    int query(unsigned short fp, int hash)
    {
        for (int i = 0; i < landa_h; i++)
        {
            if (heavy[i].first == fp)
            {
                return heavy[i].second;
            }
        }
        int index = hash % (landa_l);
        if (index % 2)
        {
            int bits = light[index / 2] & 15;
            return bits;
        }
        else
        {
            int bits = (light[index / 2] & 240);
            return bits >> 4;
        }
    }
};

unsigned int ReadInTraces(const char* traceFilePath, char** keys)
{
    FILE* fin = fopen(traceFilePath, "rb");

    char temp[KEY_SIZE]{ 0 };
    char temp2[9]{ 0 };
    unsigned int count = 0;
    char* key;
    while (fread(temp, 1, KEY_SIZE, fin) == KEY_SIZE) {//read source ip
        key = new char[KEY_SIZE] {0};
        memcpy(key, temp, KEY_SIZE);
        keys[count] = key;
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
    //prepare heavyGuardian
    cout << "prepare heavyGuardian" << endl;

    unsigned int totalMem = 100 * 1024 * 8;//the total number of bits
    unsigned int bucketMem = totalMem;

    unsigned int bucketNum = bucketMem / ((16 + 20) * landa_h + 4 * landa_l);//number of buckets
    vector<hg_node> hg(bucketNum);
    cout << "total Mem: " << totalMem / (8 * 1024) << "KB\tbucketMem: " << bucketMem / (8 * 1024) << "KB\t bucketNum: " << bucketNum << endl;
    cout << "*********************" << endl;


    //prepare dataset
    cout << "prepare dataset" << endl;
    unsigned int maxItemNum = 40000000;//max number of items
    char** keys = new char* [maxItemNum];//to store keys of all items
    unsigned int actualItemNum = ReadInTraces(R"(../data/20190117-130000-new.dat)", keys);
    cout << "number of items: " << actualItemNum << endl;
    cout << "*********************" << endl;


    //insert items
    cout << "insert items" << endl;
    cout << "threshold to save keys: " << thresholdToSaveKeys << " key list size " << keyListSize << endl;

    clock_t time1 = clock();
    for (unsigned int i = 0; i < actualItemNum; i++) {
        unsigned int hash = BKDRHash(keys[i], KEY_SIZE);
        hg[hash % bucketNum].insert(finger_print(hash), hash, keys[i]);
    }
    clock_t time2 = clock();

    double numOfSeconds = ((double)time2 - time1) / CLOCKS_PER_SEC;//the seconds using to insert items
    double throughput = (actualItemNum / 1000000.0) / numOfSeconds;

    cout << "use " << numOfSeconds << " seconds" << endl;
    cout << "throughput: " << throughput << " Mpps" << endl;
    cout << "*********************" << endl;
}

