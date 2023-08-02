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
#include<algorithm>
#include<fstream>
#include <bitset>

using namespace std;
#define landa_d 16
#define landa_l 64
#define KEY_SIZE 4

static const int COUNT[2] = { 1, -1 };
static const int factor = 1;

inline unsigned int BKDRHash(const char* str,unsigned int len)
{
	unsigned int seed = 131;
	unsigned int hash = 0;
	for (int i = 0; i < len; i++)
	{
		hash = hash * seed + (*str++);
	}

	return (hash & 0x7FFFFFFF);
}

class hg_node
{
public:
	char items[landa_d][KEY_SIZE];//32*landa_d bits
	unsigned int counters[landa_d];//20*landa_d, though we employ 32-bits counters, we actually only use 20 bits in each counter
	bitset<landa_d>real; //landa_d bits
	unsigned int incast; //20 bits,though we employ a 32-bits counter, we actually only use 20 bits in each counter

	hg_node():items{0},counters{0}
	{
		incast = 0;
		real.reset();
	}
	void insert(char* key)
	{
		unsigned int choice = key[KEY_SIZE-1] & 1;
		unsigned min_num = UINT32_MAX;
		unsigned int min_pos = -1;

		for (unsigned int i = 0; i < landa_d; ++i) {
			if (counters[i] == 0) {
				memcpy(items[i], key, KEY_SIZE);
				counters[i] = 1;
				real[i] = 1;
				return;
			}
			else if (memcmp(items[i],key,KEY_SIZE)==0) {
				if (real[i])
					counters[i]++;
				else {
					counters[i]++;
					incast += COUNT[choice];
				}
				return;
			}

			if (counters[i] < min_num) {
				min_num = counters[i];
				min_pos = i;
			}
		}

		if (incast * COUNT[choice] >= int(min_num * factor)) {
			//count_type pre_incast = incast;
			if (real[min_pos]) {
				unsigned int min_choice = items[min_pos][KEY_SIZE-1] & 1;
				incast += COUNT[min_choice] * counters[min_pos];
			}
			memcpy(items[min_pos], key, KEY_SIZE);
			counters[min_pos] += 1;
			real[min_pos] = 0;
		}
		incast += COUNT[choice];
	}

	int query(char* key)
	{
		for (unsigned int i = 0; i < landa_d; ++i) {
			if (memcmp(items[i], key, KEY_SIZE) == 0) {
				return counters[i];
			}
		}
		return 0;
	}
};


bool cmpPairFunc(pair<char*, int>p1, pair<char*, int>p2)
{
	return p1.second > p2.second;
}

//for recording information in the unordered_map
struct CmpFunc {
	bool operator()(const char* keyA, const char* keyB) const {
		return memcmp(keyA, keyB, KEY_SIZE) == 0;
	}
};

struct HashFunc {
	unsigned int operator()(const char* key) const {
		unsigned int hashValue = BKDRHash(key, KEY_SIZE);
		return hashValue;
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

bool cmp1(pair<int, int>p1, pair<int, int>p2)
{
	return p1.first > p2.first;
}

bool cmp2(pair<int, int>p1, pair<int, int>p2)
{
	return p1.second > p2.second;
}

int main()
{
	srand((unsigned int)time(0));
	cout << "prepare waving sketch" << endl;
	unsigned int totalMem =200 * 1024 * 8;//the total number of bits
	unsigned int bucketMem = totalMem;
	unsigned int bucketNum = bucketMem / (landa_d+20+(20+32)* landa_d);//number of buckets
	vector<hg_node> hg(bucketNum);
	cout << "total Mem: " << totalMem / (8 * 1024) << "KB\tbucketMem: " << bucketMem / (8 * 1024) << "KB\tbucketNum: " << bucketNum << endl;
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
	clock_t time1 = clock();
	for (unsigned int i = 0; i < actualItemNum; i++) {
		unsigned int hash = BKDRHash(keys[i],KEY_SIZE);
		hg[hash % bucketNum].insert(keys[i]);
	}
	clock_t time2 = clock();

	double numOfSeconds = ((double)time2 - time1) / CLOCKS_PER_SEC;//the seconds using to insert items
	double throughput = (actualItemNum / 1000000.0) / numOfSeconds;

	cout << "use " << numOfSeconds << " seconds" << endl;
	cout << "throughput: " << throughput << " Mpps" << endl;
	cout << "*********************" << endl;

	return 0;
}

