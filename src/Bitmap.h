#include<iostream>
#include<cstdio>
#include<cstdlib>
#include<vector>

class BitMap
{
public:
    BitMap(size_t num)
    {
        _v.resize((num >> 5) + 1); // 相当于num/32 + 1
    }

    void Set(size_t num) //set 1
    {
        size_t index = num >> 5; // 相当于num/32
        size_t pos = num % 32;
        _v[index] |= (1 << pos);
    }

    void ReSet(size_t num) //set 0
    {
        size_t index = num >> 5; // 相当于num/32
        size_t pos = num % 32;
        _v[index] &= ~(1 << pos);
    }

    bool HasExisted(size_t num)//check whether it exists
    {
        size_t index = num >> 5;
        size_t pos = num % 32;
        bool flag = false;
        if (_v[index] & (1 << pos))
            flag = true;
        return flag;

    }

private:
    std::vector<size_t> _v;
}; 
