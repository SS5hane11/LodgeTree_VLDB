#pragma GCC optimize (3)

//
// Created by ss5hane1 on 2021/3/17.
//

#ifndef LODGE_H
#define LODGE_H


#include <cassert>
#include <climits>
#include <fstream>
#include <future>
#include <iostream>
#include <typeinfo>
#include <libpmemobj.h>
#include <math.h>
#include <mutex>
#include <stdint.h>
#include <shared_mutex>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <vector>
#include <set>
#include <algorithm>
#include <memory>
#include <unordered_map>



#include <mutex>
#include <shared_mutex>

#include <x86intrin.h>

#include <thread>
#include "nvm-common.h"
#include "util.h"
#include "status.h"

long long hot_update_dram = 0;
long long hot_update_pm = 0;
long long hot_update_lodge = 0;
long long log_time = 0;
long long last_level_nodes = 0;
int node_t = 3;


const int SLOTS = 27;
const int ISLOTS = 29;

long long move_times = 0;
long long find_time = 0;
long long binary_number = 0;
long long fast_number = 0;
long long insert_shift = 0;
long long internal_leftmost_shift_num = 0;

using entry_key_t = int64_t;

namespace lodge {

class btree;
class InterNode;
class LeafNode;
class plog;

struct insert_ret{
    entry_key_t split_key;
    char *value;
    uint8_t sibling_num;

    insert_ret(){

    }

    insert_ret(const insert_ret& p) {
        split_key = p.split_key;
        value = p.value;
        sibling_num = p.sibling_num;
    }

    insert_ret(insert_ret&& p) {
        split_key = p.split_key;
        value = p.value;
        sibling_num = p.sibling_num;
    }

    insert_ret& operator=(insert_ret&& p) {
        split_key = p.split_key;
        value = p.value;
        sibling_num = p.sibling_num;

        return *this;
    }

    insert_ret& operator=(const insert_ret& p) {
        split_key = p.split_key;
        value = p.value;
        sibling_num = p.sibling_num;

        return *this;
    }

};


std::vector<entry_key_t> vec_fail;

static LeafNode *new_leaf_node();
static plog *new_plog();


class btree{

friend class InterNode;
friend class LeafNode;

private:
    LeafNode *root;
    // TOID(LeafNode) root;
    InterNode *v_root;
    InterNode *left_root;

    int invalid[50];
    
    // LeafNode *root;

public:
    plog* log;
    btree();
    void constructor();
    friend class LeafNode;
    void bitree_insert(entry_key_t key, int tid);

    // * modified by wsz in 2022/10/13
    void lodge_buffer_insert(entry_key_t key, int tid);
    void bitree_buffer_update(entry_key_t key, int tid);
    entry_key_t bitree_buffer_search(entry_key_t key);
    entry_key_t bitree_search(entry_key_t key);
    entry_key_t bitree_search_range(entry_key_t key, int size);
    void bitree_insert_internal(entry_key_t key, char *value, uint32_t depth, bool is_pln);
    // void bitree_insert_buffer_internal(entry_key_t key, char *value, int depth, bool is_pln);
    void prepare_nodes();
    void plog_add(entry_key_t key);
    void bitree_print_all();
    void bitree_find_through_all(std::vector<entry_key_t>);
    void setNewRoot(LeafNode *left, entry_key_t key, LeafNode *right, uint32_t depth);
    void setNewRoot(LeafNode *left, entry_key_t key, LeafNode *right, uint32_t depth, uint8_t num0, uint8_t num1);
    void setNewRoot(LeafNode* left, entry_key_t key, LeafNode *right, uint32_t depth, entry_key_t l_key, LeafNode *middle);

    void search_fail_kv(entry_key_t key);
};

struct alignas(8) InterHeader{
    friend class LeafNode;
    friend class btree;
    friend class InterNode;

private:
    InterNode *next;
    uint32_t depth;
    uint8_t leaf_count[ISLOTS];
    uint8_t bitmap[ISLOTS];
    uint8_t versions[ISLOTS];
    char *leftmost_ptr;
    bool is_pln;
    bool has_uninsert;
    uint8_t capacity;
    uint8_t switch_counter;
    int16_t last_index;
    uint16_t buffer_start_loc;
    entry_key_t last_split_key;
    entry_key_t previous_node_split_key;
    
    
    std::shared_mutex sLock;

public:
    InterHeader(){
        next = NULL;
        capacity = 0;
        depth = 0;
        leftmost_ptr = NULL;
        is_pln = false;
        switch_counter = 0;
        last_index = 0;
        buffer_start_loc = 15;
        last_split_key = -1;
        previous_node_split_key = -1;
        has_uninsert = false;
        for(int i = 0; i < ISLOTS; i++){
            bitmap[i] = 0;
            leaf_count[i] = 0;
            versions[i] = 0;
        }
    }

};

class Header{
    friend class LeafNode;
    friend class SortedTree;
    friend class btree;


private:
    LeafNode* next[2];
    // LeafNode* next1;
    int min_pos;
    uint32_t depth;
    int8_t usen;
    uint8_t rest_slots;
    std::shared_mutex sLock;


public:
    void constructor(){
        leftmost_ptr = NULL;
        next[0] = NULL;
        next[1] = NULL;
        usen = 0;
        depth = 0;
        min_pos = -1;
        rest_slots = SLOTS;
        
    }

    Header(){
        leftmost_ptr = NULL;
        next[0] = NULL;
        next[1] = NULL;
        usen = 0;
        depth = 0;
        min_pos = -1;
        rest_slots = SLOTS;
    }

    char *leftmost_ptr;
};

// const int space = PAGE_SIZE - sizeof(Header);


struct alignas(8) InterEntry{

friend class LeafNode;
friend class InterNode;
friend class btree;

private:
    entry_key_t key;
    char *ptr;

public:
    InterEntry(){
        ptr = NULL;
        key = -1;
    }

    InterEntry(entry_key_t key, char* ptr) {
        this->key = key;
        this->ptr = ptr;
    }

    InterEntry(InterEntry&& p) {
        this->key = p.key;
        this->ptr = p.ptr;
    }

    InterEntry(const InterEntry& p) {
        this->key = p.key;
        this->ptr = p.ptr;
    }

    InterEntry& operator=(const InterEntry& p) {
        this->key = p.key;
        this->ptr = p.ptr;

        return *this;
    }

    InterEntry& operator=(InterEntry&& p) {
        this->key = p.key;
        this->ptr = p.ptr;

        return *this;
    }

    
};

class LogEntry{
    entry_key_t key;
    char* ptr;
    int valid;
    int type;

    friend class LeafNode;
    friend class btree;
    friend class plog;
    public:
        void constructor(){
            ptr = NULL;
            key = -1;
            valid = 0;
            type = 0;
        }
};

class entry{
    entry_key_t key;
    char* ptr;

    friend class LeafNode;
    friend class btree;
    friend class plog;
public:
    void constructor(){
        ptr = nullptr;
        key = -1;
    }

    entry() {
        key = -1;
        ptr = nullptr;
    }

    entry(const entry& p) {
        key = p.key;
        ptr = p.ptr;
    }

    entry(entry&& p) {
        key = p.key;
        ptr = p.ptr;
    }

    entry& operator=(const entry& p) {
        this->key = p.key;
        this->ptr = p.ptr;

        return *this;
    }

    entry& operator=(entry&& p) {
        this->key = p.key;
        this->ptr = p.ptr;

        return *this;
    }

    
};


class plog{

public:
    LogEntry log_entry[CROSS_NUM][50][100005];

    int pointer[CROSS_NUM];
    int version[CROSS_NUM];
    int global;

    plog(){
        for(int i = 0; i < CROSS_NUM; i++){
            pointer[i] = 0;
            version[i] = 0;
        }
        global = 0;
    }



    void constructor(){
        for(int i = 0; i < CROSS_NUM; i++){
            pointer[i] = 0;
            version[i] = 0;
        }
    }

    int add_log(entry_key_t key, int tid){
        
        log_entry[tid][version[tid]][pointer[tid]].key = key;
        log_entry[tid][version[tid]][pointer[tid]].ptr = (char *)key;
        log_entry[tid][version[tid]][pointer[tid]].valid = 1;
        
        
        // clflush_then_sfence(&log_entry[tid][version[tid]][pointer[tid]]);

        pointer[tid]++;
        
        int ver = version[tid];
        if(pointer[tid] == 100000){
            pointer[tid] = 0;
            version[tid]++;
            if(version[tid] > 49)
                version[tid] = 0;
            if(global < version[tid]){
                global = version[tid];
            }
        }

        return ver;

    }

};

// class plog{

// public:
//     LogEntry log_entry[CROSS_NUM][3000005];

//     int pointer[CROSS_NUM];
//     int version[CROSS_NUM];
//     int npointer[CROSS_NUM];
//     int invalid[500];
//     int nnp;
//     int global;

//     plog(){
//         for(int i = 0; i < CROSS_NUM; i++){
//             pointer[i] = 0;
//             npointer[i] = 0;
//             version[i] = 0;
//         }
//         nnp=0;
//         global = 0;
//         // npointer = 0;
//     }



//     void constructor(){
//         for(int i = 0; i < CROSS_NUM; i++){
//             pointer[i] = 0;
//             npointer[i] = 0;
//         }
//         nnp=0;
//     }

//     int add_log(entry_key_t key, int tid){
        
//         log_entry[tid][pointer[tid]].key = key;
//         log_entry[tid][pointer[tid]].ptr = (char *)key;
//         log_entry[tid][pointer[tid]].valid = 1;
        
        
//         clflush_then_sfence(&log_entry[tid][pointer[tid]]);

//         pointer[tid]++;
        
//         // int ver = version[tid];
//         if(pointer[tid] == 13000000){
//             pointer[tid] = 0;
//         }

//         return 0;

//     }

// };

//const int SLOTS = 14;

class LeafNode{


private:
    Header header;
    uint8_t bitmap[SLOTS];
    uint8_t fingerprint[SLOTS];
    entry records[SLOTS];
    friend class btree;

public:

    void constructor(uint32_t depth = 0){
        header.constructor();
        // bits = new BitMap(SLOTS);

        for(int i=0; i < SLOTS; i++){
            records[i].key = -1;
            records[i].ptr = NULL;
            bitmap[i] = 0;
            fingerprint[i] = 0;
            // bits->ReSet(i);
        }
        


        header.depth = depth;
        records[0].ptr = NULL;
    }

    void constructor(PMEMobjpool *pop, uint32_t depth){
        header.constructor();
        // bits = new BitMap(SLOTS);
        for(int i=0; i < SLOTS; i++){
            records[i].key = -1;
            records[i].ptr = NULL;
            fingerprint[i] = 0;
            // bits->ReSet(i);
        }
        header.depth = 0;
        records[0].ptr = NULL;

        //  pmemobj_persist(pop, this, sizeof(LeafNode));
        clflush_then_sfence(this);
    }
    
    void constructor(PMEMobjpool *pop, char *left, entry_key_t key, char *right, uint32_t depth){
        header.constructor();
        // bits = new BitMap(SLOTS);
        for(int i=0; i < SLOTS; i++){
            records[i].key = -1;
            records[i].ptr = NULL;
            // bits->ReSet(i);
            fingerprint[i] = 0;
        }
        header.leftmost_ptr = left;
        header.depth = depth;
        records[0].key = key;
        records[0].ptr = right;
        
        records[1].ptr = NULL;
        // pmemobj_persist(pop, this, sizeof(LeafNode));
        clflush_then_sfence(this);
    }


    entry_key_t update_leaf_key_test(entry_key_t key){
        for(int i = 0; i < SLOTS; i++){
            if(bitmap[i] == 1)
            if(records[i].key == key){  
                records[i].ptr = (char*)key;
                clwb(&records[i]);
                return records[i].key;
            }
        }
        return 0;
    }

    entry_key_t find_internal_leaf_key_test(entry_key_t key){
        for(int i = 0; i < SLOTS; i++){
            if(bitmap[i] == 1)
            if(records[i].key == key){  
                return records[i].key;
            }
        }
        return 0;
    }
// 
    entry_key_t find_internal_leaf_key_fingerprints(entry_key_t key){
        unsigned char fp = hashcode1B(key);
        for(int i = 0; i < SLOTS; i++){
            if(bitmap[i] == 1)
            if(fingerprint[i] == fp){  
                if(records[i].key == key)
                    return records[i].key;
            }
        }
        return 0;
    }


    void new_buffer_insert(const std::vector<InterEntry>& buffer, btree *bt, int size, int worker_id, insert_ret &ret){

        entry_key_t split_key;

        int rest_num = 0;
        std::vector<int> rests;

        for (int i = 0; i < SLOTS; i++) {
            if (bitmap[i] == 0) {
                rests.push_back(i);
                rest_num++;

                if (rest_num > size)
                    break;
            }
        }

        if (size < rest_num) {
            // * enough free slots
            auto rest_it = 0;
            for (auto i = 0; i <= size-1; i++) {
                auto t = rests[rest_it];
                rest_it++;
                records[t].key = buffer[i].key;
                records[t].ptr = buffer[i].ptr;
                bitmap[t] = 1;
                unsigned char fp = hashcode1B(buffer[i].key);
                fingerprint[t] = fp;

                if(header.min_pos == -1){
                    header.min_pos = t;
                }else{
                    if(buffer[i].key < records[header.min_pos].key){
                        header.min_pos = t;
                    }
                }
            }
            clflush_len_no_fence(&records[0], 16*(size));
            clflush_then_sfence(&bitmap);

            ret.split_key = -1;

            return;

        } else {
            // * need to split
            int num = 0;
            for (int i = 0; i < SLOTS; i++) {
                entry_key_t tmp_key;
                if (bitmap[i] == 1) {

                    tmp_key = records[i].key;
                    num = 0;
                    for (int j = 0; j < SLOTS; j++) {
                        if (bitmap[j] == 1)
                            if (tmp_key > records[j].key) {
                                num++;
                            }
                    }
                    for (int j = 0; j <= size-1; j++) {
                        if (tmp_key > buffer[j].key)
                            num++;
                    }
                }

                if (num >= (size+SLOTS-rest_num)/2-3 && num <= (size+SLOTS-rest_num)/2+3) {
                    split_key = tmp_key;
                    goto split;

                }
                    
            }

            for (int i = 0; i <= size-1; i++) {
                entry_key_t tmp_key = buffer[i].key;
                num = 0;
                for (int j = 0; j < SLOTS; j++) {
                    if (bitmap[j] == 1)
                        if (tmp_key > records[j].key) {
                            num++;
                        }
                }
                for (int j = 0; j <= size-1; j++) {
                    if (tmp_key > buffer[j].key)
                        num++;
                }

                if (num >= (size+SLOTS-rest_num)/2-3 && num <= (size+SLOTS-rest_num)/2+3) {
                    split_key = tmp_key;
                    goto split;
                }

            }

        split:

            LeafNode *sibling = new_leaf_node();
            memcpy(sibling, this, sizeof(LeafNode));

            sibling->header.depth = header.depth;

            sibling->header.next[0] = NULL;
            sibling->header.next[1] = NULL;

            for (int i = 0; i < SLOTS; i++) {
                if (bitmap[i] == 1) {
                    if (records[i].key > split_key) {
                        bitmap[i] = 0;
                    } else if (records[i].key < split_key){
                        sibling->bitmap[i] = 0;
                    } else {
                        bitmap[i] = 0;
                        sibling->header.min_pos = i;
                    }
                }
            }

            int old_idx = 0;
            int new_idx = 0;
            for (auto i = 0; i <= size-1; i++) {
                if (buffer[i].key < split_key) {
                    for (; old_idx < SLOTS; old_idx++) {
                        if (bitmap[old_idx] == 0) {
                            records[old_idx].key = buffer[i].key;
                            records[old_idx].ptr = buffer[i].ptr;
                            bitmap[old_idx] = 1;
                            unsigned char fp = hashcode1B(buffer[i].key);
                            fingerprint[old_idx] = fp;

                            if(header.min_pos == -1){
                                header.min_pos = old_idx;
                            }else{
                                if(buffer[i].key < records[header.min_pos].key){
                                    header.min_pos = old_idx;
                                }
                            }
                            break;
                        }
                    }
                } else {
                    for (; new_idx < SLOTS; new_idx++) {
                        if (sibling->bitmap[new_idx] == 0) {
                            sibling->records[new_idx].key = buffer[i].key;
                            sibling->records[new_idx].ptr = buffer[i].ptr;
                            sibling->bitmap[new_idx] = 1;
                            unsigned char fp = hashcode1B(buffer[i].key);
                            sibling->fingerprint[new_idx] = fp;

                            if(split_key == buffer[i].key){
                                sibling->header.min_pos = new_idx;
                            }
                            break;
                        }
                    }
                }
            }

            sibling->header.next[sibling->header.usen] = header.next[header.usen];
            clflush_then_sfence(sibling);

            header.next[1-header.usen] = sibling;
            header.usen = 1-header.usen;
            clflush_then_sfence(&header);
            clflush_then_sfence(&bitmap);
            records[0].ptr = nullptr;
        
            internal_num++;
            ret.split_key = split_key;
            ret.value = (char *)sibling;

            return;

        }
    }

    void unsort_store(entry_key_t key, btree *bt, char *value, bool flush){
        
        std::unique_lock<std::shared_mutex> sul(header.sLock);
    
        
        if(header.next[header.usen]){
            LeafNode *next_n = header.next[header.usen];
            if(key > next_n->records[next_n->header.min_pos].key){
                sul.unlock();
                next_n->unsort_store(key, bt, value, flush);
                multi_thread_one_node++;
                return;
            }
        }
        
        for(int i = 0; i < SLOTS; i++){
            if(bitmap[i] == 0){

                unsigned char fp = hashcode1B(key);
                fingerprint[i] = fp;
                records[i].key = key;
                records[i].ptr = value;
                clflush_then_sfence(&records[i]);
                bitmap[i] = 1;
                clflush_then_sfence(&bitmap);

                if(header.min_pos == -1){
                    header.min_pos = i;
                }else{
                    if(key < records[header.min_pos].key){
                        header.min_pos = i;
                    }
                }

                return ;
            }else{
                if(records[i].key == key){
                    records[i].ptr = value;
                    clflush_then_sfence(&records[i]);

                    return ;
                }
            }
        }

        

        int split_key_pos = 0;

        for(int i = 0; i < SLOTS; i++){
            int num = 0;
            for(int j = 0; j < SLOTS; j++){
                if(records[j].key > records[i].key){
                    num++;
                }
            }
            if(num >= SLOTS/2-1 && num <=SLOTS/2+1){
                split_key_pos = i;
                break;
            }
        }

        entry_key_t split_key = records[split_key_pos].key;

        // std::cout<<"split\n";


        LeafNode *sibling = new_leaf_node();

        sibling->constructor();
        sibling->header.depth = header.depth;
        int sibling_num = 0;

        uint8_t this_num = SLOTS;

        for(int i = 0; i < SLOTS; i++){
            
            if(records[i].key >= split_key){
                sibling->records[sibling_num].key = records[i].key;
                sibling->records[sibling_num].ptr = records[i].ptr;
                sibling->bitmap[sibling_num] = 1;

                this_num--;
                sibling->fingerprint[sibling_num] = fingerprint[i];
                bitmap[i] = 0;
                fingerprint[i] = 0;

                if(i == split_key_pos){
                    sibling->header.min_pos = sibling_num;
                }

                sibling_num++;

                
            }
        }

        int use_n = header.usen;
        sibling->header.usen = 0;

        sibling->header.next[0] = header.next[use_n];
        clflush_then_sfence(sibling);

        header.next[1-use_n] = sibling;
        header.usen = 1-use_n;

        clflush_then_sfence(&header);
        clflush_then_sfence(&bitmap);

        if(key > split_key){
            sibling->records[sibling_num].key = key;
            sibling->records[sibling_num].ptr = value;
            sibling->bitmap[sibling_num] = 1;
            unsigned char fp = hashcode1B(key);
            fingerprint[sibling_num] = fp;

            clflush_then_sfence(&sibling->records[sibling_num]);
            clflush_then_sfence(&bitmap);
            sibling_num++;

        }else{
            records[split_key_pos].key = key;
            records[split_key_pos].ptr = value;
            bitmap[split_key_pos] = 1;
            unsigned char fp = hashcode1B(key);
            fingerprint[split_key_pos] = fp;
            clflush_then_sfence(&sibling->records[sibling_num]);
            clflush_then_sfence(&bitmap);
            this_num++;

        }

        if(bt->v_root == NULL){
            int depthe = header.depth + 1;
            bt->setNewRoot(this, split_key, sibling, depthe, this_num, sibling_num);
        }
    }


    void leaf_print_all(){
        
        std::ofstream ofs("../out_order_bulkload.txt", std::ios::app);
        for(int i = 0; i < SLOTS; i++){
            if(bitmap[i] == 1){
                numa++;
                ofs << records[i].key << std::endl;
            }
        }
        ofs<<std::endl;

    }

};

class InterNode{

    friend class btree;
    friend class LeafNode;

private:
    InterHeader header;
    InterEntry records[ISLOTS];
    
    

public:
    InterNode(){
        header.leftmost_ptr = NULL;
        header.next = NULL;
        header.has_uninsert = false;
        for(int i = 0; i < ISLOTS; i++){
            records[i].key = -1;
            records[i].ptr = NULL;
            header.bitmap[i] = 0;
            header.leaf_count[i] = 0;
        }
    }

    void constructor(){
        header.has_uninsert = false;
        header.leftmost_ptr = NULL;
        header.next = NULL;
        for(int i = 0; i < ISLOTS; i++){
            records[i].key = -1;
            records[i].ptr = NULL;
            header.bitmap[i] = 0;
            header.leaf_count[i] = 0;
        }
    }

    inline int count1() {
        uint8_t previous_switch_counter;
        int count = 0;

        do {
        previous_switch_counter = header.switch_counter;
        count = header.last_index + 1;


        while (count >= 0 && records[count].ptr!= NULL) {
            if ((previous_switch_counter % 2 == 0))
            ++count;
            else
            --count;
        }

        if (count < 0) {
            count = 0;
            while (records[count].ptr != NULL) {
            ++count;
            }
        }

        } while (previous_switch_counter != header.switch_counter);

        return count;
    }

    char *linear_search(entry_key_t key){
        // std::shared_lock<std::shared_mutex> sul(header.sLock);
        int i = 1;
        uint8_t previous_switch_counter;
        char *ret = NULL;
        char *t;
        entry_key_t k;
        do {
        previous_switch_counter = header.switch_counter;
        ret = NULL;

        if (IS_FORWARD(previous_switch_counter)) {
            if (key < (k = records[0].key)) {
                if ((t = (char *)header.leftmost_ptr) != records[0].ptr) {
                    ret = t;
                    continue;
                }
            }

            for (i = 1; records[i].ptr != NULL; ++i) {
                if (key < (k = records[i].key)) {
                    if ((t = records[i - 1].ptr) != records[i].ptr) {
                        ret = t;
                        break;
                    }
                }
            }

            if (!ret) {
                ret = records[i - 1].ptr;
                continue;
            }
        } else { // search from right to left
            for (i = count1() - 1; i >= 0; --i) {
                if (key >= (k = records[i].key)) {
                if (i == 0) {
                    if ((char *)header.leftmost_ptr != (t = records[i].ptr)) {
                        ret = t;
                        break;
                    }   
                } else {
                    if (records[i - 1].ptr != (t = records[i].ptr)) {
                        ret = t;
                        break;
                    }
                }
                }   
            }
        }
    } while (header.switch_counter != previous_switch_counter);

        if ((t = (char *)header.next) != NULL) {
            if (key >= header.next->records[0].key){
                return t;
            }
        }

        if (ret) {
            return ret;
        } else{
            return (char *)header.leftmost_ptr;
        }
    }

    int update_temp_key(entry_key_t key){
        if(header.buffer_start_loc == 0)
            return 0;
        for(int i = header.buffer_start_loc; i < ISLOTS; i++){
            if(header.bitmap[i] == 1)
            if(records[i].key == key){
                records[i].ptr = (char*)key;
                return 1;
            }
        }

        return 0;
    }

    entry_key_t internal_find_temp_key(entry_key_t key){
        if(header.buffer_start_loc == 0)
            return 0;
        for(int i = header.buffer_start_loc; i < ISLOTS; i++){
            if(header.bitmap[i] == 1)
            if(records[i].key == key){
                return key;
            }
        }

        return 0;
    }

    char *find_internal_leaf_key(entry_key_t key){
        if(records[0].key==-1){
            return (char *)this;
        }
        if(key < records[0].key){
            return header.leftmost_ptr;
        }
        for(int i = 1; i < ISLOTS; i++){
            if(records[i].key != -1){
                if(key < records[i].key){
                    return records[i-1].ptr;
                }
            }else{
                return records[i-1].ptr;
            }
        }

        return records[ISLOTS - 1].ptr;
    }

    char *find_internal_leaf_key_search_debug(entry_key_t key, LeafNode*& prev){
        if(records[0].key==-1){
            return (char *)this;
        }
        if(key < records[0].key){
            return header.leftmost_ptr;
        }
        for(int i = 1; i < header.buffer_start_loc-1; i++){
            if(key < records[i].key){
                if (i != 1)
                    prev = (LeafNode *)records[i-2].ptr;
                return records[i-1].ptr;
            }
        }
        prev = (LeafNode*)records[header.buffer_start_loc - 3].ptr;
        return records[header.buffer_start_loc - 2].ptr;
    }

    char *find_internal_leaf_key_search(entry_key_t key){
        if(records[0].key==-1){
            return (char *)this;
        }
        if(key < records[0].key){
            return header.leftmost_ptr;
        }
        for(int i = 1; i < header.buffer_start_loc-1; i++){
            if(key < records[i].key){
                return records[i-1].ptr;
            }
        }

        return records[header.buffer_start_loc - 2].ptr;
    }

    char *find_internal_leaf_key_buffer(entry_key_t key){
        if(records[0].key==-1){
            return (char *)this;
        }
        if(key < records[0].key){
            return header.leftmost_ptr;
        }
        for(int i = 1; i < header.buffer_start_loc; i++){
            if(records[i].key != -1){
                if(key < records[i].key){
                    return records[i-1].ptr;
                }
            }else{
                return records[i-1].ptr;
            }
        }

        return records[header.buffer_start_loc - 1].ptr;
    }

    void insert_key(entry_key_t key, int *end_pos, char *value){

        if (!IS_FORWARD(header.switch_counter))
            ++header.switch_counter;
        header.last_index = *end_pos;

        if(*end_pos == 0){
            records[0].key = key;
            records[0].ptr = value;

            return;
        }

        for(int i = *end_pos-1; i >= 0; i--){
            if(records[i].key > key){
                move_times++;
                records[i+1].key = records[i].key;
                records[i+1].ptr = records[i].ptr;

            }else{
                records[i+1].key = key;
                records[i+1].ptr = value;
                
                return;
            }
        }
        records[0].key = key;
        records[0].ptr = value;
    }

    void insert_key_count(entry_key_t key, int *end_pos, char *value, uint8_t num){

        if (!IS_FORWARD(header.switch_counter))
            ++header.switch_counter;

        header.last_index = *end_pos;

        if(*end_pos == 0){
            records[0].key = key;
            records[0].ptr = value;
            header.leaf_count[0] = num;
            return;
        }


        for(int i = *end_pos-1; i >= 0; i--){
            if(records[i].key > key){
                move_times++;
                records[i+1].key = records[i].key;
                records[i+1].ptr = records[i].ptr;
                header.leaf_count[i+1] = header.leaf_count[i];

            }else{
                records[i+1].key = key;
                records[i+1].ptr = value;
                header.leaf_count[i+1] = num;
                
                return;
            }
        }
        records[0].key = key;
        records[0].ptr = value;
        header.leaf_count[0] = num;
    }


    int lodge_internal_buffer_insert(entry_key_t key, btree* bt, char* value, bool is_pln) {

        std::unique_lock<std::shared_mutex> sul(header.sLock);

        int end_pos = 0;
        if (key < header.previous_node_split_key) {
            return -1;
        }

        if(header.next){
            if(key >= header.last_split_key){
                sul.unlock();
                header.next->lodge_internal_buffer_insert(key, bt, value, is_pln);

                return 0;
            }
        }

        int pos = header.buffer_start_loc-1;
        for(int i = 0; i < header.buffer_start_loc-1; i++){
            if(key < records[i].key){
                pos = i;
                break;
            }
        }

        for(int i = header.buffer_start_loc; i < ISLOTS; i++){
            if(header.bitmap[i] == 0){
                records[i].key = key;
                records[i].ptr = value;

                header.leaf_count[i] = pos;
                header.bitmap[i] = 1;

                sul.unlock();
                return 0;
            }else if(records[i].key == key){
                records[i].ptr = value;

                sul.unlock();
                return 0;
            }
        }

        end_pos = header.buffer_start_loc-2;
        int max_size = ISLOTS - header.buffer_start_loc+1;

        std::vector<std::vector<InterEntry>> buff(header.buffer_start_loc, std::vector<InterEntry>());

        std::vector<int> buff_point(header.buffer_start_loc, 0);
        
        for(int i = header.buffer_start_loc; i < ISLOTS; i++){
            int tmp_idx = header.leaf_count[i];

            if (buff[tmp_idx].size() == 0) {
                buff[tmp_idx].reserve(max_size);
            }

            buff[tmp_idx].emplace_back(records[i].key, records[i].ptr);
            records[i].key = -1;
            buff_point[tmp_idx]++;
            header.bitmap[i] = 0;
        }

        std::vector<insert_ret> split_entries;

        for(int i = 0; i <= header.buffer_start_loc-1; i++){

            if(i == pos){
                if (buff[i].size() == 0)
                    buff[i].reserve(max_size);

                buff[i].emplace_back(key, value);
                buff_point[i]++;
            }

            if(buff_point[i] == 0)
                continue;
            
            LeafNode* insert_leaf = bt->root;

            if(i == 0){
                insert_leaf = (LeafNode *)header.leftmost_ptr;
            }else{
                insert_leaf = (LeafNode *)records[i-1].ptr;
            }

            insert_ret ret;
            LEAF_PREF(&insert_leaf);
            insert_leaf->new_buffer_insert(buff[i], bt, buff[i].size(), worker_id, ret);

            if(ret.split_key != -1){
                split_entries.emplace_back(std::move(ret));
                header.leaf_count[i] = header.leaf_count[i] + buff_point[i]-1 ;
            }else{
                header.leaf_count[i] += buff_point[i]-1;
            }
            
        }

        if(split_entries.empty()){
            sul.unlock();
            return 0;
        }
        
        std::vector<insert_ret>::iterator it = split_entries.begin();
        
        for(; it != split_entries.end(); it++){
            int end_pos = header.buffer_start_loc-1;
            insert_key(it->split_key, &end_pos, it->value);
            header.buffer_start_loc++;

            if(header.buffer_start_loc > ISLOTS-4){
                goto internal_split;
            }
        }

        sul.unlock();
        return 0;


        internal_split:
        
        end_pos = header.buffer_start_loc-1;

        int split_key = (int)ceil(end_pos / 2);
        last_level_nodes++;
        entry_key_t split_key_value = records[split_key].key;

        
        InterNode *sibling = new InterNode();

        if (header.next)
            header.next->header.previous_node_split_key = header.last_split_key;
        sibling->header.last_split_key = header.last_split_key;
        sibling->header.previous_node_split_key = split_key_value;
        header.last_split_key = split_key_value;



        sibling->header.depth = header.depth;
        sibling->header.is_pln = header.is_pln;
        int sibling_num = 0;

        if(header.leftmost_ptr){

            //internal
            sibling->header.leftmost_ptr = records[split_key].ptr;

            for(int i = split_key+1; i < end_pos; i++){
                sibling->insert_key(records[i].key, &sibling_num, records[i].ptr);
                sibling_num++;
                records[i].key = -1;
                records[i].ptr = NULL;
            }
            records[split_key].key = -1;
            records[split_key].ptr = NULL;


        }
        if (IS_FORWARD(header.switch_counter))
            header.switch_counter += 2;
        else
            ++header.switch_counter; 

        header.last_index = split_key -1;

        header.buffer_start_loc = split_key+1;
        sibling->header.buffer_start_loc = sibling_num+1;

        sibling->header.next = header.next;
        header.next = sibling;


        for(; it != split_entries.end(); it++){

            if(it->split_key < split_key_value){
                int end_pos1 = header.buffer_start_loc-1;
                insert_key(it->split_key, &end_pos1, it->value);
                header.buffer_start_loc++;
            }else{
                int end_pos1 = sibling->header.buffer_start_loc-1;
                sibling->insert_key(it->split_key, &end_pos1, it->value);
                sibling->header.buffer_start_loc++;
            }
        }

        if(bt->v_root == this){
            InterNode *new_root = new InterNode();
            new_root->records[0].key = split_key_value;
            new_root->header.leftmost_ptr = (char *)this;
            new_root->records[0].ptr = (char *)sibling;
            new_root->header.depth = header.depth+1;
            new_root->header.is_pln = false;
            bt->v_root = new_root;

            sul.unlock();
            return 0;

        }else{
            uint32_t insert_depth = header.depth+1;
            sul.unlock();
            bt->bitree_insert_internal(split_key_value, (char* )sibling, insert_depth, false);

            return 0;
        }

    }

    int lodge_internal_buffer_insert_v2(entry_key_t key, btree* bt, char* value, bool is_pln) {

        std::unique_lock<std::shared_mutex> sul(header.sLock);

        int status;

        int end_pos = 0;
        if (key < header.previous_node_split_key) {
            status = -1;
            return status;
        }

        if(header.next){
            if(key >= header.last_split_key){
                sul.unlock();
                header.next->lodge_internal_buffer_insert_v2(key, bt, value, is_pln);
                status = 1;
                return status;
            }
        }

        int pos = header.buffer_start_loc-1;
        for(int i = 0; i < header.buffer_start_loc-1; i++){
            if(key < records[i].key){
                pos = i;
                break;
            }
        }

        for(int i = header.buffer_start_loc; i < ISLOTS; i++){
            if(header.bitmap[i] == 0){
                records[i].key = key;
                records[i].ptr = value;

                header.leaf_count[i] = pos;
                header.bitmap[i] = 1;

                sul.unlock();
                return status;
            }else if(records[i].key == key){
                records[i].ptr = value;
                // printf("ddwdwdw\n");
                sul.unlock();
                return status;
            }
        }

        status = 2;

        end_pos = header.buffer_start_loc-2;
        int max_size = ISLOTS - header.buffer_start_loc+1;

        std::vector<std::vector<InterEntry>> buff(header.buffer_start_loc, std::vector<InterEntry>());

        std::vector<int> buff_point(header.buffer_start_loc, 0);
        
        for(int i = header.buffer_start_loc; i < ISLOTS; i++){
            int tmp_idx = header.leaf_count[i];

            if (buff[tmp_idx].size() == 0) {
                buff[tmp_idx].reserve(max_size);
            }

            buff[tmp_idx].emplace_back(records[i].key, records[i].ptr);
            records[i].key = -1;
            buff_point[tmp_idx]++;
            header.bitmap[i] = 0;
        }

        std::vector<insert_ret> split_entries;

        for(int i = 0; i <= header.buffer_start_loc-1; i++){

            if(i == pos){
                if (buff[i].size() == 0)
                    buff[i].reserve(max_size);

                buff[i].emplace_back(key, value);
                buff_point[i]++;
            }

            if(buff_point[i] == 0)
                continue;
            
            LeafNode* insert_leaf = bt->root;

            if(i == 0){
                insert_leaf = (LeafNode *)header.leftmost_ptr;
            }else{
                insert_leaf = (LeafNode *)records[i-1].ptr;
            }

            insert_ret ret;
            LEAF_PREF(&insert_leaf);
            insert_leaf->new_buffer_insert(buff[i], bt, buff[i].size(), worker_id, ret);

            if(ret.split_key != -1){
                split_entries.emplace_back(std::move(ret));
                header.leaf_count[i] = header.leaf_count[i] + buff_point[i]-1 ;
            }else{
                header.leaf_count[i] += buff_point[i]-1;
            }
            
        }

        if(split_entries.empty()){
            sul.unlock();
            return status;
        }
        
        std::vector<insert_ret>::iterator it = split_entries.begin();
        
        for(; it != split_entries.end(); it++){
            int end_pos = header.buffer_start_loc-1;
            insert_key(it->split_key, &end_pos, it->value);
            header.buffer_start_loc++;

            if(header.buffer_start_loc > ISLOTS-4){
                goto internal_split;
            }
        }

        sul.unlock();
        return status;


        internal_split:

        status = 3;
        
        end_pos = header.buffer_start_loc-1;

        int split_key = (int)ceil(end_pos/3);
        int sec_split_key = (int)(ceil)(end_pos*2/3);
        last_level_nodes+=2;
        entry_key_t split_key_value = records[split_key].key;
        entry_key_t sec_split_key_value = records[sec_split_key].key;

        
        InterNode *sibling = new InterNode();
        InterNode *sec_sibling = new InterNode();

        if (header.next)
            header.next->header.previous_node_split_key = header.last_split_key;
        sec_sibling->header.previous_node_split_key = sec_split_key_value;
        sibling->header.previous_node_split_key = split_key_value;

        sec_sibling->header.last_split_key = header.last_split_key;
        sibling->header.last_split_key = sec_split_key_value;
        header.last_split_key = split_key_value;
        



        sibling->header.depth = header.depth;
        sibling->header.is_pln = header.is_pln;
        sec_sibling->header.depth = header.depth;
        sec_sibling->header.is_pln = header.is_pln;

        int sibling_num = 0;
        int sec_sibling_num = 0;

        if(header.leftmost_ptr){

            //internal
            sibling->header.leftmost_ptr = records[split_key].ptr;

            for(int i = split_key+1; i < sec_split_key; i++){
                sibling->insert_key(records[i].key, &sibling_num, records[i].ptr);
                sibling_num++;
                records[i].key = -1;
                records[i].ptr = NULL;
            }
            records[split_key].key = -1;
            records[split_key].ptr = NULL;

            sec_sibling->header.leftmost_ptr = records[sec_split_key].ptr;

            for(int i = sec_split_key+1; i < end_pos; i++){
                if(records[i].key != -1){
                    sec_sibling->insert_key(records[i].key, &sec_sibling_num, records[i].ptr);
                    sec_sibling_num++;
                    records[i].key = -1;
                    records[i].ptr = NULL;
                }
            }
            records[sec_split_key].key = -1;
            records[sec_split_key].ptr = NULL;

        }

        
        if (IS_FORWARD(header.switch_counter))
            header.switch_counter += 2;
        else
            ++header.switch_counter; 

        // header.last_index = split_key -1;

        header.buffer_start_loc = split_key + 1;
        sibling->header.buffer_start_loc = sibling_num + 1;
        sec_sibling->header.buffer_start_loc = sec_sibling_num + 1;

        sec_sibling->header.next = header.next;
        sibling->header.next = sec_sibling;
        header.next = sibling;


        for(; it != split_entries.end(); it++){

            if(it->split_key < split_key_value){
                int end_pos1 = header.buffer_start_loc-1;
                insert_key(it->split_key, &end_pos1, it->value);
                header.buffer_start_loc++;
            }else if(it->split_key < sec_split_key_value){
                int end_pos1 = sibling->header.buffer_start_loc-1;
                sibling->insert_key(it->split_key, &end_pos1, it->value);
                sibling->header.buffer_start_loc++;
            } else {
                int end_pos1 = sec_sibling->header.buffer_start_loc-1;
                sec_sibling->insert_key(it->split_key, &end_pos1, it->value);
                sec_sibling->header.buffer_start_loc++;
            }
        }

        if(bt->v_root == this){
            InterNode *new_root = new InterNode();
            new_root->records[0].key = split_key_value;
            new_root->header.leftmost_ptr = (char *)this;
            new_root->records[0].ptr = (char *)sibling;
            new_root->records[1].key = sec_split_key_value;
            new_root->records[1].ptr = (char *)sec_sibling;
            new_root->header.depth = header.depth+1;
            new_root->header.is_pln = false;
            bt->v_root = new_root;

            sul.unlock();
            return status;

        }else{
            sul.unlock();
            uint32_t insert_depth = header.depth+1;
            
            bt->bitree_insert_internal(split_key_value, (char* )sibling, insert_depth, false);
            bt->bitree_insert_internal(sec_split_key_value, (char* )sec_sibling, insert_depth, false);
            return status;
        }

    }



    void traditional_sort_store(entry_key_t key, btree *bt, char *value, bool is_pln){
        std::shared_lock<std::shared_mutex> slk(header.sLock);
        if(header.next){
        
            if(key > header.next->records[0].key){
                header.next->traditional_sort_store(key, bt, value, is_pln);
                return;
            }
        }

        int end_pos = count1();


        for(int i =0; i < ISLOTS; i++){
            if(records[i].key == key){
                records[i].ptr = value;
                return;
            }
        }
        header.buffer_start_loc++;
        
        if(end_pos < ISLOTS-4){
            insert_key(key, &end_pos, value);


            return;
        }else{

            int split_key = (int)ceil(end_pos / 2);
            entry_key_t split_key_value = records[split_key].key;

            InterNode *sibling = new InterNode();

            sibling->header.depth = header.depth;
            sibling->header.is_pln = header.is_pln;
            int sibling_num = 0;

            if(header.leftmost_ptr){

                //internal
                sibling->header.leftmost_ptr = records[split_key].ptr;

                for(int i = split_key+1; i < end_pos; i++){
                    if(records[i].key != -1){
                        sibling->insert_key(records[i].key, &sibling_num, records[i].ptr);
                        sibling_num++;
                        records[i].key = -1;
                        records[i].ptr = NULL;
                    }
                }
                records[split_key].key = -1;
                records[split_key].ptr = NULL;

            }else{
                internal_num++;

                //leaf
                for(int i = split_key; i < ISLOTS-1; i++){
                    sibling->insert_key(records[i].key, &sibling_num, records[i].ptr);
                    sibling_num++;
                    records[i].key = -1;
                    records[i].ptr = NULL;
                }
            }
            if (IS_FORWARD(header.switch_counter))
                header.switch_counter += 2;
            else
                ++header.switch_counter; 

            header.last_index = split_key -1;

            if(key >= split_key_value){
                
                sibling->insert_key(key, &sibling_num, value);
                sibling->header.buffer_start_loc = sibling_num+2;
                header.buffer_start_loc = split_key + 1;
            }else{
                int split_key_pos = split_key;
                insert_key(key, &split_key_pos, value);

                sibling->header.buffer_start_loc = sibling_num+1;
                header.buffer_start_loc = split_key + 2;
            }


            sibling->header.next = header.next;
            header.next = sibling;

            if(bt->v_root == this){
                InterNode *new_root = new InterNode();
                new_root->records[0].key = split_key_value;
                new_root->header.leftmost_ptr = (char *)this;
                new_root->records[0].ptr = (char *)sibling;
                new_root->header.depth = header.depth+1;
                new_root->header.is_pln = false;
                bt->v_root = new_root;
                // cout<<"new level:"<< header.depth+1<<endl;

            }else{
                uint32_t insert_depth = header.depth+1;
                bt->bitree_insert_internal(split_key_value, (char* )sibling, insert_depth, false);
            }

            return;

        }
    }

};




void btree::bitree_find_through_all(std::vector<entry_key_t> vec){
    
    for(std::vector<entry_key_t>::iterator it = vec.begin(); it != vec.end(); it++){
        LeafNode* insert_leaf = root;
        InterNode *temp_leaf = v_root;
        long long total_cap = 0;
        long long use_cap = 0;

        bool fail_search = false;

        while(!temp_leaf->header.is_pln){
            temp_leaf = (InterNode *)temp_leaf->header.leftmost_ptr;
        }
        insert_leaf = (LeafNode *)temp_leaf->header.leftmost_ptr;

        while(temp_leaf){
            for(int i = temp_leaf->header.buffer_start_loc; i < ISLOTS; i++){
                if(temp_leaf->records[i].key != -1){
                    use_cap++;
                    if(temp_leaf->records[i].key == *it){
                        std::cout<<"has fail search "<< *it << " in internal buffer\n";
                        fail_search = true;
                        goto next_it;
                    }
                }
                total_cap++;
            }
            temp_leaf = temp_leaf->header.next;
        }

        

        while(insert_leaf){
            for(int i = 0; i < SLOTS; i++){
                if(insert_leaf->bitmap[i] == 1){
                    if(insert_leaf->records[i].key == *it){
                        std::cout<<"has fail search "<< *it << " in leaf buffer\n";
                        fail_search = true;
                        goto next_it;
                    }
                    use_cap++;
                }
                total_cap++;
            }
            insert_leaf = insert_leaf->header.next[insert_leaf->header.usen];
        }

        next_it:
        if(!fail_search){
            std::cout<<"no search !!!\n";
        }
    }


}

void btree::bitree_print_all() {

    // original version
    // cout<<endl;
    LeafNode* insert_leaf = root;
    InterNode *temp_leaf = v_root;
    if(v_root == NULL){
        
        
    }else{
        while(!temp_leaf->header.is_pln){
            temp_leaf = (InterNode *)temp_leaf->header.leftmost_ptr;
        }
        
        insert_leaf = (LeafNode *)temp_leaf->header.leftmost_ptr;
    }
    std::ofstream ofs("../out_order_bulkload.txt", std::ios::app);
    //start count internal num
    while(temp_leaf){
        if(temp_leaf->header.buffer_start_loc != 0){
            for(int i = temp_leaf->header.buffer_start_loc; i < ISLOTS; i++){
                if(temp_leaf->header.bitmap[i] == 1){
                    
                    ofs<<temp_leaf->records[i].key<<std::endl;
                    numa++;
                }
            }
        }
        temp_leaf = temp_leaf->header.next;
        ofs<<std::endl;
    }

    ofs<<std::endl;

    while(insert_leaf != NULL){
        insert_leaf->leaf_print_all();
        insert_leaf = insert_leaf->header.next[insert_leaf->header.usen];
    }

}

void btree::constructor(){
    v_root = NULL;
    root = new_leaf_node();
    root->constructor();
    log = new_plog();
    // POBJ_NEW(pop, &log, plog, NULL, NULL);
    log->constructor();
}

btree::btree(){
    v_root = NULL;
    for(int i = 0; i < 50; i++){
        invalid[i] = 100000 * 32;
    }

    root = new_leaf_node();
    
    root->constructor();
    struct timespec t1, t2;
    clock_gettime(CLOCK_MONOTONIC, &t1);
    log = new_plog();
    clock_gettime(CLOCK_MONOTONIC, &t2);
    long long time = (t2.tv_sec - t1.tv_sec)*1000000000 + (t2.tv_nsec - t1.tv_nsec);

    std::cout<<"create log use time"<<time <<std::endl;
    log->constructor();
    printf("btree build completed\n");
}

void btree::lodge_buffer_insert(entry_key_t key, int tid) {

    int ver = log->add_log(key, tid);
    
    if(v_root!=NULL){
        reinsert:

        InterNode *temp_leaf = v_root;
        // std::array<InterNode*, 20> paths;
        int p = 0;
        while(!temp_leaf->header.is_pln){
            // paths[p] = temp_leaf;
            temp_leaf = (InterNode *)temp_leaf->linear_search(key);
        }
        int status = temp_leaf->lodge_internal_buffer_insert_v2(key, this, (char *)key, true);
        if (status == -1) {
            goto reinsert;
        }
        
    }else{

        LeafNode* temp_leaf = root;
        temp_leaf->unsort_store(key, this, (char *)key, true);
    }

}



void btree::bitree_insert(entry_key_t key, int tid) {
    
    if(v_root!=NULL){

        InterNode *temp_leaf = v_root;
        while(!temp_leaf->header.is_pln){
            temp_leaf = (InterNode *)temp_leaf->linear_search(key);
            
        }
        
        LeafNode* insert_leaf = root;

        insert_leaf = (LeafNode *)temp_leaf->find_internal_leaf_key_search(key);

        // LEAF_PREF(&insert_leaf);

        insert_leaf -> unsort_store(key, this, (char *)key, true);        

    }else{
        
        LeafNode* temp_leaf = root;
        
        temp_leaf->unsort_store(key, this, (char *)key, true);
        // D_RW(temp_leaf)->traditional_sort_store(key, this, (char *)key, true, tid);

    }

}


long long search_internal_time = 0;
long long search_leaf_time = 0;
long long search_leaf_times = 0;
long long search_search_times = 0;



entry_key_t btree::bitree_search(entry_key_t key) {

    // ofstream ofss("search_res.txt", ios::app);
    
    entry_key_t res = 0;
    int aa = 0;
    if(v_root!=NULL){

        InterNode *temp_leaf = v_root;

        
        while(!temp_leaf->header.is_pln){
            temp_leaf = (InterNode *)temp_leaf->linear_search(key);
            search_search_times++;
            aa++;
        }
        

        LeafNode* insert_leaf = root;

        // res = temp_leaf->internal_find_temp_key(key);
        
        insert_leaf = (LeafNode *)temp_leaf->find_internal_leaf_key(key);

        res = insert_leaf->find_internal_leaf_key_test(key);
        

    }else{
        LeafNode* temp_leaf = root;

        res = temp_leaf->find_internal_leaf_key_test(key);

    }

    return res;

}

void btree::bitree_buffer_update(entry_key_t key, int tid){
    entry_key_t res = 0;
    if(v_root!=NULL){
        
        InterNode *temp_leaf = v_root;

        while(!temp_leaf->header.is_pln){
            temp_leaf = (InterNode *)temp_leaf->linear_search(key);
        }

        
        std::unique_lock<std::shared_mutex> sul(temp_leaf->header.sLock);
        res = temp_leaf->update_temp_key(key);
        

        if(res == 0){

            LeafNode* insert_leaf = root;
            insert_leaf = (LeafNode *)temp_leaf->find_internal_leaf_key_search(key);
            res = insert_leaf->update_leaf_key_test(key);
            // res = insert_leaf->find_internal_leaf_key_fingerprints(key);
        }else{
            log->add_log(key, tid);
        }

    }else{
        LeafNode* temp_leaf = root;

        res = temp_leaf->find_internal_leaf_key_test(key);

    }
}

entry_key_t btree::bitree_buffer_search(entry_key_t key) {
    
    entry_key_t res = 0;
    if(v_root!=NULL){
        InterNode *temp_leaf = v_root;

        while(!temp_leaf->header.is_pln){
            temp_leaf = (InterNode *)temp_leaf->linear_search(key);
        }

        {
            // std::shared_lock<std::shared_mutex> sul(temp_leaf->header.sLock);
            NODE_PREF(&temp_leaf);
            res = temp_leaf->internal_find_temp_key(key);

            if(res == 0){
                LeafNode* insert_leaf = root;
                insert_leaf = (LeafNode *)temp_leaf->find_internal_leaf_key_search(key);
                LEAF_PREF(&insert_leaf);
                res = insert_leaf->find_internal_leaf_key_test(key);
                // res = insert_leaf->find_internal_leaf_key_fingerprints(key);
            }
            
        }

    }else{
        LeafNode* temp_leaf = root;
        res = temp_leaf->find_internal_leaf_key_test(key);
    }

    return res;

}

void btree::bitree_insert_internal(entry_key_t key, char *value, uint32_t depth, bool is_pln) {

    InterNode *temp_leaf = v_root;

    while(temp_leaf->header.depth != depth){
        temp_leaf = (InterNode *)temp_leaf->linear_search(key);
    }

    temp_leaf->traditional_sort_store(key, this, value, is_pln);
}

void btree::setNewRoot(LeafNode *left, entry_key_t key, LeafNode *right, uint32_t depth, uint8_t num0, uint8_t num1){
    InterNode *new_root = new InterNode();
    
    new_root->header.leftmost_ptr = (char *)left;
    
    new_root->records[0].key = key;
    new_root->records[0].ptr = (char *)right;

    new_root->header.leaf_count[0] = num0;
    new_root->header.leaf_count[1] = num1;

    new_root->header.depth = depth;
    new_root->header.is_pln = true;

    new_root->header.buffer_start_loc = 2;
    new_root->header.capacity = ISLOTS - 2;

    v_root = new_root;
}

void btree::search_fail_kv(entry_key_t key) {
    LeafNode* p = root;
    while (p->header.next[0] || p->header.next[1]) {
        entry_key_t res = p->find_internal_leaf_key_test(key);
        if (res == key) {
            res = p->find_internal_leaf_key_test(key);
            break;
        }

        p = p->header.next[p->header.usen];
    }
}

void btree::setNewRoot(LeafNode* left, entry_key_t key, LeafNode *right, uint32_t depth){
    
    InterNode *new_root = new InterNode();
    new_root->header.leftmost_ptr = (char *)left;
    
    new_root->records[0].key = key;
    new_root->records[0].ptr = (char *)right;
   
    new_root->header.depth = depth;
    new_root->header.is_pln = true;

    new_root->header.buffer_start_loc = 2;

    v_root = new_root;
   
    
}

void btree::prepare_nodes(){
    InterNode *temp_leaf = v_root;
    
    while(!temp_leaf->header.is_pln){
        temp_leaf = (InterNode *)temp_leaf->header.leftmost_ptr;
    }

    while(temp_leaf){
      
        temp_leaf = (InterNode *)temp_leaf->header.next;
    }
    

}


void btree::setNewRoot(LeafNode* left, entry_key_t key, LeafNode *right, uint32_t depth, entry_key_t l_key, LeafNode *middle){
    
    
    InterNode *new_root = new InterNode;
    
    new_root->header.leftmost_ptr = (char *)left;
    
    new_root->records[0].key = key;
    new_root->records[0].ptr = (char *)right;
    new_root->records[1].key = l_key;
    new_root->records[1].ptr = (char *)middle;
   
    new_root->header.depth = depth;
    new_root->header.is_pln = true;
    v_root = new_root;
   
    
}



static LeafNode *new_leaf_node()
{
    
    void *ret;
    nvm_dram_alloc(&ret, CACHE_LINE_SIZE, sizeof(LeafNode));
    return new (ret) LeafNode;
}


static plog *new_plog()
{

    void *ret;    
    std::cout<<"log size:"<<sizeof(plog)<<std::endl;
    nvm_dram_alloc(&ret, CACHE_LINE_SIZE, sizeof(plog));
    return new (ret) plog;
}


}

#endif //LODGE_H
