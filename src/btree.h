//
// Created by ss5hane1 on 2021/3/17.
//

#ifndef FAST_FAIR_MYSELF_BTREE_H
#define FAST_FAIR_MYSELF_BTREE_H

//
// Created by ss5hane11 on 2021/2/4.
//

#ifndef UNTITLED1_BINARY_TREE_H
#define UNTITLED1_BINARY_TREE_H
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

#include <x86intrin.h>

#include <thread>
#include "nvm-common.h"
#include "util.h"

long long hot_update_dram = 0;
long long hot_update_pm = 0;
long long hot_update_lodge = 0;
long long log_time = 0;
long long last_level_nodes = 0;
int node_t = 3;

const int SLOTS =27;
const int ISLOTS = 29;

class btree;
class InterNode;
class LeafNode;
class plog;

using namespace std;

using entry_key_t = int64_t;


typedef struct{
    entry_key_t split_key;
    char *value;
    uint8_t sibling_num;
}insert_ret;

long long move_times = 0;
long long find_time = 0;
long long binary_number = 0;
long long fast_number = 0;
long long insert_shift = 0;
long long internal_leftmost_shift_num = 0;


vector<entry_key_t> vec_fail;

static LeafNode *new_leaf_node();
static plog *new_plog();


class btree{

friend class InterNode;
friend class LeafNode;

private:
    LeafNode *root;
    // TOID(LeafNode) root;
    InterNode *v_root;
    int invalid[50];
    
    // LeafNode *root;

public:
    plog* log;
    btree();
    void constructor();
    friend class LeafNode;
    void bitree_insert(entry_key_t key, int tid);

    void bitree_buffer_insert(entry_key_t key, int tid);
    void bitree_buffer_update(entry_key_t key, int tid);
    entry_key_t bitree_buffer_search(entry_key_t key);
    entry_key_t bitree_search(entry_key_t key);
    entry_key_t bitree_search_range(entry_key_t key, int size);
    void bitree_insert_internal(entry_key_t key, char *value, int depth, bool is_pln);
    void bitree_insert_buffer_internal(entry_key_t key, char *value, int depth, bool is_pln);
    void prepare_nodes();
    void plog_add(entry_key_t key);
    void bitree_print_all();
    void bitree_find_through_all(vector<entry_key_t>);
    void setNewRoot(LeafNode *left, entry_key_t key, LeafNode *right, uint32_t depth);
    void setNewRoot(LeafNode *left, entry_key_t key, LeafNode *right, uint32_t depth, uint8_t num0, uint8_t num1);
    void setNewRoot(LeafNode* left, entry_key_t key, LeafNode *right, uint32_t depth, entry_key_t l_key, LeafNode *middle);
};

class InterHeader{
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
    pthread_rwlock_t *rwlock;
    bool is_pln;
    bool has_uninsert;
    uint8_t capacity;
    uint8_t switch_counter;
    int16_t last_index;
    uint16_t buffer_start_loc;
    uint64_t last_split_key;
    


public:
    InterHeader(){
        rwlock = new pthread_rwlock_t;
        if (pthread_rwlock_init(rwlock, NULL)) {
            perror("lock init fail");
            exit(1);
        }
        next = NULL;
        capacity = 0;
        depth = 0;
        leftmost_ptr = NULL;
        is_pln = false;
        switch_counter = 0;
        last_index = 0;
        buffer_start_loc = 15;
        last_split_key = -1;
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

    // int32_t last_index;
    pthread_rwlock_t *rwlock;


public:
    void constructor(){
        rwlock = new pthread_rwlock_t;
        if (pthread_rwlock_init(rwlock, NULL)) {
            perror("lock init fail");
            exit(1);
        }

        leftmost_ptr = NULL;
        next[0] = NULL;
        next[1] = NULL;
        usen = 0;
        depth = 0;
        min_pos = -1;
        
    }

    Header(){
        rwlock = new pthread_rwlock_t;
        if (pthread_rwlock_init(rwlock, NULL)) {
            perror("lock init fail");
            exit(1);
        }

        leftmost_ptr = NULL;
        next[0] = NULL;
        next[1] = NULL;
        usen = 0;
        depth = 0;
        min_pos = -1;
    }

    char *leftmost_ptr;
};

// const int space = PAGE_SIZE - sizeof(Header);


class InterEntry{

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
            ptr = NULL;
            key = -1;
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
        
        
        clflush_then_sfence(&log_entry[tid][version[tid]][pointer[tid]]);

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


class LeafNode{


private:
    Header header;
    uint8_t bitmap[SLOTS];

    uint8_t fingerprint[SLOTS];
    // BitMap *bits;
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
        // pthread_rwlock_rdlock(header.rwlock);
        for(int i = 0; i < SLOTS; i++){
            if(bitmap[i] == 1)
            if(records[i].key == key){  
                // pthread_rwlock_unlock(header.rwlock);
                records[i].ptr = (char*)key;
                clwb(&records[i]);
                return records[i].key;
            }
        }
        // ki = 10;
        // pthread_rwlock_unlock(header.rwlock);
        return 0;
        // return records[SLOTS-1].key;
        // return records[0].key;
    }

    entry_key_t find_internal_leaf_key_test(entry_key_t key){
        // pthread_rwlock_rdlock(header.rwlock);
        for(int i = 0; i < SLOTS; i++){
            if(bitmap[i] == 1)
            if(records[i].key == key){  
                // pthread_rwlock_unlock(header.rwlock);
                return records[i].key;
            }
        }
        // ki = 10;
        // pthread_rwlock_unlock(header.rwlock);
        return 0;
        // return records[SLOTS-1].key;
        // return records[0].key;
    }
// 
    entry_key_t find_internal_leaf_key_fingerprints(entry_key_t key){
        // pthread_rwlock_rdlock(header.rwlock);
        unsigned char fp = hashcode1B(key);
        for(int i = 0; i < SLOTS; i++){
            if(bitmap[i] == 1)
            if(fingerprint[i] == fp){  
                if(records[i].key == key)
                // pthread_rwlock_unlock(header.rwlock);
                    return records[i].key;
            }
        }
        // ki = 10;
        // pthread_rwlock_unlock(header.rwlock);
        return 0;
        // return records[SLOTS-1].key;
        // return records[0].key;
    }


    void insert_key(PMEMobjpool *pop,entry_key_t key, int end_pos, char *value, bool flush){

        if(end_pos == -1){
            records[0].key = key;
            records[0].ptr = value;
            if(flush){
                
                clflush_then_sfence(&records[0]);
                
            }
            header.min_pos++;

            return;
        }

        int i = end_pos, inserted = 0, to_flush_cnt = 0;
        header.min_pos++;

//        records[i+1].key = records[i].key;
        if(i != SLOTS-2)
            records[i+2].ptr = records[i+1].ptr;

        for(int i = end_pos; i >= 0; i--){
            if(records[i].key > key){
                move_times++;
                records[i+1].key = records[i].key;
                records[i+1].ptr = records[i].ptr;

                if (flush) {

                    uint64_t records_ptr = (uint64_t)(&records[i + 1]);

                    int remainder = records_ptr % CACHE_LINE_SIZE;
                    bool do_flush =
                            (remainder == 0) ||
                            ((((int)(remainder + sizeof(entry)) / CACHE_LINE_SIZE) == 1) &&
                             ((remainder + sizeof(entry)) % CACHE_LINE_SIZE) != 0);
                    if (do_flush) {
                        pmemobj_persist(pop, (void *)records_ptr, CACHE_LINE_SIZE);
                       
                        to_flush_cnt = 0;
                    } else
                        ++to_flush_cnt;
                }
            }else{
                records[i+1].key = key;
                records[i+1].ptr = value;
                if (flush) {
                    pmemobj_persist(pop, &records[i+1], CACHE_LINE_SIZE);
                   
                }

                

                return;
            }
        }

        records[0].key = key;
        records[0].ptr = value;
        if (flush) {
            pmemobj_persist(pop, &records[i+1], CACHE_LINE_SIZE);
         
        }else{
            internal_leftmost_shift_num++;
        }

        
        return;
    }

    int count(){
       
        int end_pos;

        for(end_pos = 0; end_pos < SLOTS; end_pos++){
            if(records[end_pos].ptr != NULL){
                continue;
            }else{
                break;
            }
        }

        
        return end_pos-1;
    }

    insert_ret  buffer_insert(InterEntry buffer[], btree *bt, int size, int worker_id){

        // pthread_rwlock_wrlock(header.rwlock);

        int loc = size;

        insert_ret ret;

        // if(size >= 9){
        //     cout<<size<<endl;
        // }

    leaf_insert:
        int to_flush_num = 0;    
        int loc_s = -1;
        int loc_e = -1;
        int loc_t = -1;
        for(; loc >=0; loc--){
            // entry_key_t insert_key = buffer[loc].key;    

            loc_t = loc_unsort_insert_key(buffer[loc].key, buffer[loc].ptr);

            if(loc_t == -1){
                if(loc_s != -1){
                    if(loc_e != -1){
                         clflush_len_no_fence(&records[loc_s], 16*(loc_e-loc_s));
                         clflush_then_sfence(&bitmap);
                        //  clflush_then_sfence(bits);
                    }else{
                         clflush_len_no_fence(&records[loc_s], 16);
                         clflush_then_sfence(&bitmap);
                        // clflush_then_sfence(bits);
                    }
                }
                

                // loc = i;
                goto split;
            }else{
                if(loc_s == -1){
                    loc_s = loc_t;
                }else{
                    if(loc_t > loc_e){
                        loc_e = loc_t;
                    }
                }
            }
            
        }


        
        clflush_len_no_fence(&records[loc_s], 16*(loc_e-loc_s));
        clflush_then_sfence(&bitmap);
        // clflush_then_sfence(bits);


        ret.split_key = -1;

        // pthread_rwlock_unlock(header.rwlock);

        return ret;

    split:
        //do split the leaf
        split_time++;

        
        int split_key_pos = 0;

        bool flag = false;
        for(int i = 0; i < SLOTS; i++){
            // if(bits->HasExisted(i)){
            if(bitmap[i] == 1){
                for(int j = i+1; j < SLOTS; j++){
                    if(bitmap[j])
                        if(records[i].key == records[j].key){
                            bitmap[j] = 0;

                            if(header.min_pos == j){
                                header.min_pos = i;
                            }
                            // bits->ReSet(j);
                            hot_update_lodge++;
                            if(!flag){
                                flag = true;
                            }
                        }
                }
            }
        }

        if(flag){
            goto leaf_insert;

        }

        for(int i = 0; i < SLOTS; i++){
            int num = 0;
            for(int j = 0; j < SLOTS; j++){
                if(records[j].key > records[i].key){
                    num++;
                }
            }
            if(num >= SLOTS/2-1 && num <= SLOTS/2){
                split_key_pos = i;
                break;
            }
        }

        entry_key_t split_key = records[split_key_pos].key;
        
        // TOID(LeafNode) sibling_p;
        // POBJ_NEW(bt->pop, &sibling_p, LeafNode, NULL, NULL);
        // D_RW(sibling_p)->constructor(header.depth);
        LeafNode *sibling = new_leaf_node();
    
        
        
        sibling->constructor();
        sibling->header.depth = header.depth;

        sibling->header.next[0] = NULL;
        sibling->header.next[1] = NULL;
        int sibling_num = 0;

        for(int i = 0; i < SLOTS; i++){
            if(records[i].key >= split_key){
                sibling->records[sibling_num].key = records[i].key;
                sibling->records[sibling_num].ptr = records[i].ptr;
                sibling->fingerprint[sibling_num] = fingerprint[i];
                sibling->bitmap[sibling_num] = 1;
                // sibling->bits->Set(sibling_num);
                bitmap[i] = 0;
                // bits->ReSet(i);
                records[i].key = -1;
                records[i].ptr = NULL;

                if(i == split_key_pos){
                    sibling->header.min_pos = sibling_num;
                }

                sibling_num++;
                
            }
        }

        sibling->header.next[sibling->header.usen] = header.next[header.usen];
        clflush_then_sfence(sibling);

        header.next[1-header.usen] = sibling;
        header.usen = 1-header.usen;
        clflush_then_sfence(&header);
        clflush_then_sfence(&bitmap);


        int c_loc_t = -1, c_loc_s = -1, c_loc_e = -1;   //this leaf
        int s_loc_t = -1, s_loc_s = -1, s_loc_e = -1;   //sibling

        
        for(; loc >= 0; loc--){
            second_buffer_fail++;
            // cout<<buffer[loc].key<<endl;
            entry_key_t insert_key = buffer[loc].key;

            if(buffer[loc].key < split_key){

                c_loc_t = loc_unsort_insert_key(buffer[loc].key, buffer[loc].ptr);
                if(c_loc_t == -1){
                    second_buffer_fail++;
                }
                if(c_loc_s == -1){
                    c_loc_s = c_loc_t;
                }else{
                    if(c_loc_e < c_loc_t){
                        c_loc_e = c_loc_t;
                    }
                }

            }else{
                s_loc_t = sibling->loc_unsort_insert_key(buffer[loc].key, buffer[loc].ptr);
                sibling_num++;
                if(s_loc_t == -1){
                    second_buffer_fail++;
                }
                if(s_loc_s == -1){
                    s_loc_s = s_loc_t;
                }else{
                    if(s_loc_e < s_loc_t){
                        s_loc_e = s_loc_t;
                    }
                }
            }

        }
        if(c_loc_s != -1){
            if(c_loc_e != -1){
                clflush_len_no_fence(&records[c_loc_s], 16*(c_loc_e - c_loc_s));
                clflush_then_sfence(&bitmap);
                // clflush_then_sfence(&bits);
            }else{
                clflush_len_no_fence(&records[c_loc_s], 16*(c_loc_e - c_loc_s));
                clflush_then_sfence(&bitmap);
                // clflush_then_sfence(&bits);
            }
        }

        if(s_loc_s != -1){
            clflush_len_no_fence(&sibling->records[s_loc_s], 16*(s_loc_e - s_loc_s));
            clflush_then_sfence(&sibling->bitmap);
            // clflush_then_sfence(&bits);
        }

        int insert_depth = header.depth+1;
        
        internal_num++;
        ret.split_key = split_key;
        ret.value = (char *)sibling;
        ret.sibling_num = sibling_num;

        // pthread_rwlock_unlock(header.rwlock);

        return ret;
    }


    inline int unsort_insert_key(PMEMobjpool *pop, entry_key_t key, char *value){

        struct timespec t1, t2;

        for(int i = 0; i < SLOTS; i++){
            // if(!bits->HasExisted(i)){
            if(bitmap[i] == 0){
                records[i].key = key;
                records[i].ptr = value;
                // pmemobj_persist(pop, &records[i], CACHE_LINE_SIZE);
                bitmap[i] = 1;
                // bits->Set(i);
                // pmemobj_persist(pop, bitmap, CACHE_LINE_SIZE);

                if(header.min_pos == -1){
                    header.min_pos = i;
                }else{
                    if(key < records[header.min_pos].key){
                        header.min_pos = i;
                    }
                }

                return 1;
            }else{
                if(key == records[i].key){
                    hot_update_pm++;
                    return 1;
                }
            }
        }

        return 0;
    }

    inline int loc_unsort_insert_key(entry_key_t key, char *value){

        for(int i = 0; i < SLOTS; i++){
            if(bitmap[i] == 0){
            // if(!bits->HasExisted(i)){
                records[i].key = key;
                records[i].ptr = value;
                // pmemobj_persist(pop, &records[i], CACHE_LINE_SIZE);
                bitmap[i] = 1;
                unsigned char fp = hashcode1B(key);
                fingerprint[i] = fp;
                // bits->Set(i);
                // pmemobj_persist(pop, bitmap, CACHE_LINE_SIZE);

                if(header.min_pos == -1){
                    header.min_pos = i;
                }else{
                    if(key < records[header.min_pos].key){
                        header.min_pos = i;
                    }
                }

                return i;
            }else{
                if(key == records[i].key){
                    hot_update_pm++;
                    return i;
                }
            }
        }

        return -1;
    }


    
    void unsort_store(entry_key_t key, btree *bt, char *value, bool flush){

        pthread_rwlock_wrlock(header.rwlock);
        struct timespec t1, t2;
    
        
        if(header.next[header.usen]){
            LeafNode *next_n = header.next[header.usen];
            if(key > next_n->records[next_n->header.min_pos].key){
                pthread_rwlock_unlock(header.rwlock);
                next_n->unsort_store(key, bt, value, flush);
                multi_thread_one_node++;
                return;
            }
        }

    
        // if(worker_id == 0){
        //     clock_gettime(CLOCK_MONOTONIC, &t1);
        // }

        
        for(int i = 0; i < SLOTS; i++){
            if(bitmap[i] == 0){

                // clock_gettime(CLOCK_MONOTONIC, &t2);

                // if(worker_id == 0){
                //     long long time = (t2.tv_sec - t1.tv_sec)*1000000000 + (t2.tv_nsec - t1.tv_nsec);

                //     leaf_search_time += time;
                // }

                unsigned char fp = hashcode1B(key);
                fingerprint[i] = fp;
                records[i].key = key;
                records[i].ptr = value;
                clflush_then_sfence(&records[i]);
                bitmap[i] = 1;
                clflush_then_sfence(&bitmap);
                // bits->Set(i);
                // clflush_then_sfence(bits);

                if(header.min_pos == -1){
                    header.min_pos = i;
                }else{
                    if(key < records[header.min_pos].key){
                        header.min_pos = i;
                    }
                }

                pthread_rwlock_unlock(header.rwlock);
                // spin_unlock(&header.lock);

                return ;
            }else{
                if(records[i].key == key){
                    records[i].ptr = value;
                    clflush_then_sfence(&records[i]);
                    pthread_rwlock_unlock(header.rwlock);
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
                // sibling->bits->Set(sibling_num);
                // bits->ReSet(i);

                if(i == split_key_pos){
                    sibling->header.min_pos = sibling_num;
                }

                sibling_num++;

                
            }
        }

        int use_n = header.usen;
        sibling->header.usen = 0;

        sibling->header.next[0] = header.next[use_n];

        // sibling->header.min_pos = sibling_num;
        // pmemobj_persist(bt->pop, sibling, sizeof(LeafNode));
        clflush_then_sfence(sibling);

        header.next[1-use_n] = sibling;
        header.usen = 1-use_n;
        // pmemobj_persist(bt->pop, &header, CACHE_LINE_SIZE);
        // pmemobj_persist(bt->pop, &bitmap, CACHE_LINE_SIZE);
        clflush_then_sfence(&header);
        clflush_then_sfence(&bitmap);
        // clflush_then_sfence(bits);

        


        if(key > split_key){
            sibling->records[sibling_num].key = key;
            sibling->records[sibling_num].ptr = value;
            sibling->bitmap[sibling_num] = 1;
            unsigned char fp = hashcode1B(key);
            fingerprint[sibling_num] = fp;
            // sibling->bits->Set(sibling_num);
            // pmemobj_persist(bt->pop, &sibling->records[sibling_num], sizeof(entry));
            // pmemobj_persist(bt->pop, sibling->bitmap, CACHE_LINE_SIZE);
            clflush_then_sfence(&sibling->records[sibling_num]);
            clflush_then_sfence(&bitmap);
            sibling_num++;
            // clflush_then_sfence(sibling->bits);
        }else{
            records[split_key_pos].key = key;
            records[split_key_pos].ptr = value;
            bitmap[split_key_pos] = 1;
            unsigned char fp = hashcode1B(key);
            fingerprint[split_key_pos] = fp;
            // bits->Set(split_key_pos);
            clflush_then_sfence(&sibling->records[sibling_num]);
            clflush_then_sfence(&bitmap);
            this_num++;
            // clflush_then_sfence(bits);
        }

        
        

        

        if(bt->v_root == NULL){
            int depthe = header.depth + 1;
            bt->setNewRoot(this, split_key, sibling, depthe, this_num, sibling_num);

            // spin_unlock(&header.lock);
            pthread_rwlock_unlock(header.rwlock);

        }else{

            int insert_depth = header.depth+1;
            // spin_unlock(&header.lock);
            pthread_rwlock_unlock(header.rwlock);
            // bt->bitree_insert_internal(split_key, (char *)sibling_p.oid.off, insert_depth, true);
            bt->bitree_insert_buffer_internal(split_key, (char *)sibling, insert_depth, true);
            internal_num++;
        }

    }


    void leaf_print_all(){
        
        ofstream ofs("../out_order_bulkload.txt", ios::app);
        for(int i = 0; i < SLOTS; i++){
            if(bitmap[i] == 1){
            // if(bits->HasExisted(i)){
            // if(records[i].key != -1){
                numa++;
                // cout<<records[i].key<<endl;
                ofs << records[i].key << endl;
            }
        }
        // cout<<endl;
        ofs<<endl;

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
        // pthread_rwlock_rdlock(header.rwlock);
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
            // pthread_rwlock_unlock(header.rwlock);
            return t;
        }
      }

      if (ret) {
        //   pthread_rwlock_unlock(header.rwlock);
        return ret;
      } else{
        //   pthread_rwlock_unlock(header.rwlock);
        return (char *)header.leftmost_ptr;
      }
    }

    int update_temp_key(entry_key_t key){
        // pthread_rwlock_rdlock(header.rwlock);
        if(header.buffer_start_loc == 0)
            return 0;
        for(int i = header.buffer_start_loc; i < ISLOTS; i++){
            if(header.bitmap[i] == 1)
            if(records[i].key == key){
                records[i].ptr = (char*)key;
                // pthread_rwlock_unlock(header.rwlock);
                return 1;
            }
        }

        // pthread_rwlock_unlock(header.rwlock);
        return 0;
    }

    entry_key_t internal_find_temp_key(entry_key_t key){
        // pthread_rwlock_rdlock(header.rwlock);
        if(header.buffer_start_loc == 0)
            return 0;
        for(int i = header.buffer_start_loc; i < ISLOTS; i++){
            if(header.bitmap[i] == 1)
            if(records[i].key == key){
                // pthread_rwlock_unlock(header.rwlock);
                return key;
            }
        }

        // pthread_rwlock_unlock(header.rwlock);
        return 0;
    }

    char *find_internal_leaf_key(entry_key_t key){
        // pthread_rwlock_rdlock(header.rwlock);
        if(records[0].key==-1){
            // pthread_rwlock_unlock(header.rwlock);
            return (char *)this;
        }
        if(key < records[0].key){
            // pthread_rwlock_unlock(header.rwlock);
            return header.leftmost_ptr;
        }
        for(int i = 1; i < ISLOTS; i++){
            if(records[i].key != -1){
                if(key < records[i].key){
                    // pthread_rwlock_unlock(header.rwlock);
                    return records[i-1].ptr;
                }
            }else{
                // pthread_rwlock_unlock(header.rwlock);
                return records[i-1].ptr;
            }
        }

        // pthread_rwlock_unlock(header.rwlock);
        return records[ISLOTS - 1].ptr;
    }

    char *find_internal_leaf_key_search(entry_key_t key){
        // pthread_rwlock_rdlock(header.rwlock);
        if(records[0].key==-1){
            // pthread_rwlock_unlock(header.rwlock);
            return (char *)this;
        }
        if(key < records[0].key){
            // pthread_rwlock_unlock(header.rwlock);
            return header.leftmost_ptr;
        }
        for(int i = 1; i < header.buffer_start_loc-1; i++){
            if(key < records[i].key){
                    // pthread_rwlock_unlock(header.rwlock);
                return records[i-1].ptr;
            }
        }

        // pthread_rwlock_unlock(header.rwlock);
        return records[header.buffer_start_loc - 2].ptr;
    }

    char *find_internal_leaf_key_buffer(entry_key_t key){
        // pthread_rwlock_rdlock(header.rwlock);
        if(records[0].key==-1){
            // pthread_rwlock_unlock(header.rwlock);
            return (char *)this;
        }
        if(key < records[0].key){
            // pthread_rwlock_unlock(header.rwlock);
            return header.leftmost_ptr;
        }
        for(int i = 1; i < header.buffer_start_loc; i++){
            if(records[i].key != -1){
                if(key < records[i].key){
                    // pthread_rwlock_unlock(header.rwlock);
                    return records[i-1].ptr;
                }
            }else{
                // pthread_rwlock_unlock(header.rwlock);
                return records[i-1].ptr;
            }
        }

        // pthread_rwlock_unlock(header.rwlock);
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

        int i;

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

        int i;

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

    int internal_buffer_insert_descend_lazy_v2(entry_key_t key, btree *bt, char *value, bool is_pln, bool background=false){

        //  struct timespec t1, t2;
        // clock_gettime(CLOCK_MONOTONIC, &t1);
        int end_pos = 0;
        pthread_rwlock_wrlock(header.rwlock);

        if(background){
            goto start_insert;
        }

        if(header.has_uninsert){
            background = true;
            // cout<<"start uninsert insert\n";
            // the second stage of predictive triple split
            header.has_uninsert = false;
            InterNode *sibling = header.next;
            entry_key_t split_key_value = header.last_split_key;

            int insert_depth = header.depth+1;
            bt->bitree_insert_internal(split_key_value, (char* )sibling, insert_depth, false); 

            
            // clock_gettime(CLOCK_MONOTONIC, &t2); 

            // ofstream oofs("flush_internal_split_latency.txt", ios::app);

            // long long time = (t2.tv_sec - t1.tv_sec)*1000000000 + (t2.tv_nsec - t1.tv_nsec);
            // oofs<<time<<endl;
            flush_5_nodes++;
            
            // bt->bitree_insert_internal(sec_split_key_value, (char* )sec_sibling, insert_depth, false);
            
        }
        

        start_insert:
        if(header.next){
            if(key >= header.last_split_key){
                pthread_rwlock_unlock(header.rwlock);

                header.next->internal_buffer_insert_descend_lazy_v2(key, bt, value, is_pln, background);

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

                pthread_rwlock_unlock(header.rwlock);
                return 0;
            }else if(records[i].key == key){
                records[i].ptr = value;

                pthread_rwlock_unlock(header.rwlock);
                return 0;
            }
        }

        end_pos = header.buffer_start_loc-2;

        InterEntry buff[ISLOTS][ISLOTS];

        int buff_point[ISLOTS];

        for(int i = 0; i < ISLOTS; i++)
            buff_point[i] = 0;


        for(int i = header.buffer_start_loc; i < ISLOTS; i++){
            buff[header.leaf_count[i]][buff_point[header.leaf_count[i]]].key = records[i].key;
            buff[header.leaf_count[i]][buff_point[header.leaf_count[i]]].ptr = records[i].ptr;
            buff_point[header.leaf_count[i]]++;
            header.bitmap[i] = 0;
        }

        vector<insert_ret> split_entries;

        for(int i = 0; i <= header.buffer_start_loc-1; i++){

            if(i == pos){
                buff[i][buff_point[i]].key = key;
                buff[i][buff_point[i]].ptr = value;
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



            insert_ret ret = insert_leaf->buffer_insert(buff[i], bt, buff_point[i]-1, worker_id);

            if(ret.split_key != -1){
                split_entries.push_back(ret);
            }
            
        }

        // insert the split keys

        if(split_entries.empty()){
            pthread_rwlock_unlock(header.rwlock);
            return 0;
        }
        
        vector<insert_ret>::iterator it = split_entries.begin();

        if(split_entries.size()>7){
            cout<<split_entries.size()<<endl;
        }

        for(; it != split_entries.end(); it++){
            int end_pos = header.buffer_start_loc-1;
            insert_key(it->split_key, &end_pos, it->value);
            header.buffer_start_loc++;

            if(header.buffer_start_loc > ISLOTS-4){
                goto internal_split;
            }
        }

        pthread_rwlock_unlock(header.rwlock);
        return 0;


        internal_split:
        
        end_pos = header.buffer_start_loc-1;

        int split_key = (int)ceil(end_pos / 3);
        int sec_split_key = (int)ceil(end_pos / 3) * 2;
        entry_key_t split_key_value = records[split_key].key;
        entry_key_t sec_split_key_value = records[sec_split_key].key;

        
        InterNode *sibling = new InterNode();
        InterNode *sec_sibling = new InterNode();

        sibling->header.last_split_key = sec_split_key_value;
        sec_sibling->header.last_split_key = header.last_split_key;
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

        }else{
            
        }
        if (IS_FORWARD(header.switch_counter))
            header.switch_counter += 2;
        else
            ++header.switch_counter; 

        header.last_index = split_key -1;

        header.buffer_start_loc = split_key+1;
        sibling->header.buffer_start_loc = sibling_num+1;
        sec_sibling->header.buffer_start_loc = sec_sibling_num+1;

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
            }else{
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
            // cout<<"new level:"<< header.depth+1<<endl;
            pthread_rwlock_unlock(header.rwlock);
            
            return 0;
        }

        // }else{
        //     int insert_depth = header.depth+1;
        //     pthread_rwlock_unlock(header.rwlock);
            
        //     bt->bitree_insert_internal(split_key_value, (char* )sibling, insert_depth, false);
        //     bt->bitree_insert_internal(sec_split_key_value, (char* )sec_sibling, insert_depth, false);


        //     return 0;
        // }
        pthread_rwlock_unlock(header.rwlock);
        header.has_uninsert = true;
        sibling->header.has_uninsert = true;

        
        // clock_gettime(CLOCK_MONOTONIC, &t2); 

        // ofstream oofs("flush_internal_split_latency.txt", ios::app);

        // long long time = (t2.tv_sec - t1.tv_sec)*1000000000 + (t2.tv_nsec - t1.tv_nsec);
        // oofs<<time<<endl;
        // flush_5_nodes++;
        
        // cout<<"split key uninsert\n";
        return 0;
    }

    int internal_buffer_insert_descend_lazy(entry_key_t key, btree *bt, char *value, bool is_pln, bool background=false){

        pthread_rwlock_wrlock(header.rwlock);
        int end_pos = 0;

        if(header.next){
            if(key >= header.last_split_key){
                pthread_rwlock_unlock(header.rwlock);

                header.next->internal_buffer_insert_descend_lazy(key, bt, value, is_pln);

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

                pthread_rwlock_unlock(header.rwlock);
                return 0;
            }else if(records[i].key == key){
                records[i].ptr = value;

                pthread_rwlock_unlock(header.rwlock);
                return 0;
            }
        }

        end_pos = header.buffer_start_loc-2;

        InterEntry buff[ISLOTS][ISLOTS];

        int buff_point[ISLOTS];

        for(int i = 0; i < ISLOTS; i++)
            buff_point[i] = 0;


        for(int i = header.buffer_start_loc; i < ISLOTS; i++){
            buff[header.leaf_count[i]][buff_point[header.leaf_count[i]]].key = records[i].key;
            buff[header.leaf_count[i]][buff_point[header.leaf_count[i]]].ptr = records[i].ptr;
            buff_point[header.leaf_count[i]]++;
            header.bitmap[i] = 0;
        }

        vector<insert_ret> split_entries;

        for(int i = 0; i <= header.buffer_start_loc-1; i++){

            if(i == pos){
                buff[i][buff_point[i]].key = key;
                buff[i][buff_point[i]].ptr = value;
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



            insert_ret ret = insert_leaf->buffer_insert(buff[i], bt, buff_point[i]-1, worker_id);

            if(ret.split_key != -1){
                split_entries.push_back(ret);
            }
            
        }

        // insert the split keys

        if(split_entries.empty()){
            pthread_rwlock_unlock(header.rwlock);
            return 0;
        }
        
        vector<insert_ret>::iterator it = split_entries.begin();

        for(; it != split_entries.end(); it++){
            int end_pos = header.buffer_start_loc-1;
            insert_key(it->split_key, &end_pos, it->value);
            header.buffer_start_loc++;

            if(header.buffer_start_loc > ISLOTS-4){
                goto internal_split;
            }
        }

        pthread_rwlock_unlock(header.rwlock);
        return 0;


        internal_split:
        
        end_pos = header.buffer_start_loc-1;

        int split_key = (int)ceil(end_pos / 2);

        entry_key_t split_key_value = records[split_key].key;


        
        InterNode *sibling = new InterNode();


        sibling->header.last_split_key = header.last_split_key;
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
            // cout<<"new level:"<< header.depth+1<<endl;
            pthread_rwlock_unlock(header.rwlock);
            
            return 0;
        }else{
            int insert_depth = header.depth+1;
            pthread_rwlock_unlock(header.rwlock);
            bt->bitree_insert_internal(split_key_value, (char* )sibling, insert_depth, false);
            return 0;
        }
        // pthread_rwlock_unlock(header.rwlock);
        // header.has_uninsert = true;
        // sibling->header.has_uninsert = true;
        // // cout<<"split key uninsert\n";
        // return 0;

    }

    vector<int> internal_buffer_insert_descend_v1(entry_key_t key, btree *bt, char *value, bool is_pln, int ver, bool background=false){
        vector<int> invalids;
        pthread_rwlock_wrlock(header.rwlock);
        int end_pos = 0;

        if(header.next){
            if(key >= header.last_split_key){
                pthread_rwlock_unlock(header.rwlock);

                return header.next->internal_buffer_insert_descend_v1(key, bt, value, is_pln, ver);
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
                header.versions[i] = ver;
                header.leaf_count[i] = pos;
                header.bitmap[i] = 1;

                pthread_rwlock_unlock(header.rwlock);
                return invalids;
            }else if(records[i].key == key){
                records[i].ptr = value;
                invalids.push_back(header.versions[i]);
                pthread_rwlock_unlock(header.rwlock);
                return invalids;
            }
        }

        end_pos = header.buffer_start_loc-2;

        InterEntry buff[ISLOTS][ISLOTS];

        int buff_point[ISLOTS];

        for(int i = 0; i < ISLOTS; i++){
            
            buff_point[i] = 0;
        }


        for(int i = header.buffer_start_loc; i < ISLOTS; i++){
            buff[header.leaf_count[i]][buff_point[header.leaf_count[i]]].key = records[i].key;
            buff[header.leaf_count[i]][buff_point[header.leaf_count[i]]].ptr = records[i].ptr;
            buff_point[header.leaf_count[i]]++;
            header.bitmap[i] = 0;
            invalids.push_back(header.versions[i]);
        }

        vector<insert_ret> split_entries;

        for(int i = 0; i <= header.buffer_start_loc-1; i++){

            if(i == pos){
                buff[i][buff_point[i]].key = key;
                buff[i][buff_point[i]].ptr = value;
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



            insert_ret ret = insert_leaf->buffer_insert(buff[i], bt, buff_point[i]-1, worker_id);

            if(ret.split_key != -1){
                split_entries.push_back(ret);
                header.leaf_count[i] = header.leaf_count[i] + buff_point[i]-1 ;
            }else{
                header.leaf_count[i] += buff_point[i]-1;
            }
            
        }

        // insert the split keys

        if(split_entries.empty()){
            pthread_rwlock_unlock(header.rwlock);
            return invalids;
        }
        
        vector<insert_ret>::iterator it = split_entries.begin();

        if(split_entries.size()>7){
            cout<<split_entries.size()<<endl;
        }

        for(; it != split_entries.end(); it++){
            int end_pos = header.buffer_start_loc-1;
            insert_key(it->split_key, &end_pos, it->value);
            header.buffer_start_loc++;

            if(header.buffer_start_loc > ISLOTS-4){
                goto internal_split;
            }
        }

        pthread_rwlock_unlock(header.rwlock);
        return invalids;


        internal_split:
        
        end_pos = header.buffer_start_loc-1;

        int split_key = (int)ceil(end_pos / 3);
        int sec_split_key = (int)ceil(end_pos / 3) * 2;
        entry_key_t split_key_value = records[split_key].key;
        entry_key_t sec_split_key_value = records[sec_split_key].key;

        
        InterNode *sibling = new InterNode();
        InterNode *sec_sibling = new InterNode();

        sibling->header.last_split_key = sec_split_key_value;
        sec_sibling->header.last_split_key = header.last_split_key;
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

        }else{
            
        }
        if (IS_FORWARD(header.switch_counter))
            header.switch_counter += 2;
        else
            ++header.switch_counter; 

        header.last_index = split_key -1;

        header.buffer_start_loc = split_key+1;
        sibling->header.buffer_start_loc = sibling_num+1;
        sec_sibling->header.buffer_start_loc = sec_sibling_num+1;

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
            }else{
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
            // cout<<"new level:"<< header.depth+1<<endl;
            pthread_rwlock_unlock(header.rwlock);
            
            return invalids;

        }else{
            int insert_depth = header.depth+1;
            pthread_rwlock_unlock(header.rwlock);
            // 
            bt->bitree_insert_internal(split_key_value, (char* )sibling, insert_depth, false);
            bt->bitree_insert_internal(sec_split_key_value, (char* )sec_sibling, insert_depth, false);


            return invalids;
        }

    }

    int internal_buffer_insert_descend_v2(entry_key_t key, btree *bt, char *value, bool is_pln, bool background=false){
        
        struct timespec t1, t2;

        clock_gettime(CLOCK_MONOTONIC, &t1);
        pthread_rwlock_wrlock(header.rwlock);
        int end_pos = 0;

        if(header.next){
            if(key >= header.last_split_key){
                pthread_rwlock_unlock(header.rwlock);

                header.next->internal_buffer_insert_descend_v2(key, bt, value, is_pln);

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

                pthread_rwlock_unlock(header.rwlock);
                return 0;
            }else if(records[i].key == key){
                records[i].ptr = value;

                pthread_rwlock_unlock(header.rwlock);
                return 0;
            }
        }

        end_pos = header.buffer_start_loc-2;

        InterEntry buff[ISLOTS][ISLOTS];

        int buff_point[ISLOTS];

        for(int i = 0; i < ISLOTS; i++){
            
            buff_point[i] = 0;
        }


        for(int i = header.buffer_start_loc; i < ISLOTS; i++){
            buff[header.leaf_count[i]][buff_point[header.leaf_count[i]]].key = records[i].key;
            buff[header.leaf_count[i]][buff_point[header.leaf_count[i]]].ptr = records[i].ptr;
            buff_point[header.leaf_count[i]]++;
            header.bitmap[i] = 0;
        }

        vector<insert_ret> split_entries;

        for(int i = 0; i <= header.buffer_start_loc-1; i++){

            if(i == pos){
                buff[i][buff_point[i]].key = key;
                buff[i][buff_point[i]].ptr = value;
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



            insert_ret ret = insert_leaf->buffer_insert(buff[i], bt, buff_point[i]-1, worker_id);

            if(ret.split_key != -1){
                split_entries.push_back(ret);
                header.leaf_count[i] = header.leaf_count[i] + buff_point[i]-1 ;
            }else{
                header.leaf_count[i] += buff_point[i]-1;
            }
            
        }

        // insert the split keys

        if(split_entries.empty()){
            pthread_rwlock_unlock(header.rwlock);
            return 0;
        }
        
        vector<insert_ret>::iterator it = split_entries.begin();

        if(split_entries.size()>7){
            cout<<split_entries.size()<<endl;
        }

        for(; it != split_entries.end(); it++){
            int end_pos = header.buffer_start_loc-1;
            insert_key(it->split_key, &end_pos, it->value);
            header.buffer_start_loc++;

            if(header.buffer_start_loc > ISLOTS-4){
                goto internal_split;
            }
        }

        pthread_rwlock_unlock(header.rwlock);
        return 0;


        internal_split:
        
        end_pos = header.buffer_start_loc-1;

        int split_key = (int)ceil(end_pos / 2);
        last_level_nodes++;
        entry_key_t split_key_value = records[split_key].key;

        
        InterNode *sibling = new InterNode();

        sibling->header.last_split_key = header.last_split_key;
        header.last_split_key = split_key_value;

        sibling->header.depth = header.depth;
        sibling->header.is_pln = header.is_pln;
        int sibling_num = 0;
        int sec_sibling_num = 0;

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
            // cout<<"new level:"<< header.depth+1<<endl;
            pthread_rwlock_unlock(header.rwlock);

            
            return 0;

        }else{
            int insert_depth = header.depth+1;
            pthread_rwlock_unlock(header.rwlock);
            
            bt->bitree_insert_internal(split_key_value, (char* )sibling, insert_depth, false);

            clock_gettime(CLOCK_MONOTONIC, &t2);

            long long time = (t2.tv_sec - t1.tv_sec)*1000000000 + (t2.tv_nsec - t1.tv_nsec);

            ofstream tail_2("tail_split2.txt", ios::app);

            tail_2<<time<<endl;

            return 0;
        }

    }


    vector<int> internal_buffer_insert_descend_leaf_count_based(entry_key_t key, btree *bt, char *value, bool is_pln, int version, bool background=false){
        
        vector<int> invalids;
        // struct timespec t1, t2;
        // clock_gettime(CLOCK_MONOTONIC, &t1);

        pthread_rwlock_wrlock(header.rwlock);
        int end_pos = 0;

        if(header.next){
            if(key >= header.last_split_key){
                pthread_rwlock_unlock(header.rwlock);
                return header.next->internal_buffer_insert_descend_leaf_count_based(key, bt, value, is_pln, version);
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
                header.versions[i] = version;

                header.capacity--;

                if(header.capacity == 0){
                    goto internal_flush;
                }

                pthread_rwlock_unlock(header.rwlock);
                return invalids;
            }else if(records[i].key == key){
                records[i].ptr = value;
                invalids.push_back(header.versions[i]);
                pthread_rwlock_unlock(header.rwlock);
                return invalids;
            }
        }

        end_pos = header.buffer_start_loc-2;

        internal_flush:
        
        // buffer flush all
        InterEntry buff[ISLOTS][ISLOTS];

        int buff_point[ISLOTS];

        set<int> order_set;
        vector<insert_ret> split_entries;
        bool compare_all = false;


        for(int i = 0; i < ISLOTS; i++)
            buff_point[i] = 0;

        for(int i = header.buffer_start_loc; i < ISLOTS; i++){
            if(compare_all){
                
                if(order_set.find(header.leaf_count[i]) != order_set.end()){
                    buff[header.leaf_count[i]][buff_point[header.leaf_count[i]]].key = records[i].key;
                    buff[header.leaf_count[i]][buff_point[header.leaf_count[i]]].ptr = records[i].ptr;
                    buff_point[header.leaf_count[i]]++;
                    header.bitmap[i] = 0;
                    invalids.push_back(header.versions[i]);
                    
                    header.capacity++;
                }
            }else{
                
                buff[header.leaf_count[i]][buff_point[header.leaf_count[i]]].key = records[i].key;
                buff[header.leaf_count[i]][buff_point[header.leaf_count[i]]].ptr = records[i].ptr;
                buff_point[header.leaf_count[i]]++;

                order_set.insert(header.leaf_count[i]);
                header.bitmap[i] = 0;

                header.capacity++;
                invalids.push_back(header.versions[i]);
                if(order_set.size() >= node_t || header.leaf_count[i] == SLOTS-2)
                    compare_all = true;
            }
            

        }

        for(set<int>::iterator ita = order_set.begin(); ita != order_set.end(); ita++){
            int i = *ita;

            LeafNode* insert_leaf = bt->root;

            if(i == 0){
                insert_leaf = (LeafNode *)header.leftmost_ptr;
            }else{
                insert_leaf = (LeafNode *)records[i-1].ptr;
            }


            
            insert_ret ret = insert_leaf->buffer_insert(buff[i], bt, buff_point[i]-1, worker_id);

            if(ret.split_key != -1){
                split_entries.push_back(ret);
                header.leaf_count[i] = header.leaf_count[i] + buff_point[i]-1 - ret.sibling_num;
            }else{
                header.leaf_count[i] += buff_point[i]-1;
            }
        }

        if(split_entries.empty()){
            pthread_rwlock_unlock(header.rwlock);
            return invalids;
        }
        
        

        vector<insert_ret>::iterator it = split_entries.begin();

        for(; it != split_entries.end(); it++){
           
            int end_pos = header.buffer_start_loc-1;
            insert_key_count(it->split_key, &end_pos, it->value, it->sibling_num);
            header.buffer_start_loc++;
            header.capacity--;

            if(header.buffer_start_loc > ISLOTS-4){
                goto internal_split;
            }
        }

        

         for(int i = header.buffer_start_loc; i < ISLOTS; i++){
            if(header.bitmap[i] == 1)
                for(int ii = 0; ii < header.buffer_start_loc-1; ii++){
                    if(records[i].key < records[ii].key){
                        header.leaf_count[i] = ii;
                        break;
                    }

                    if(ii == header.buffer_start_loc - 2){
                        header.leaf_count[i] = header.buffer_start_loc-1;
                    }

                }
                
         }

         if(header.capacity == 0){
            
            goto internal_flush;
        }

        pthread_rwlock_unlock(header.rwlock);

        return invalids;
        

        internal_split:

        // cout<<"internal split\n";
        
        end_pos = header.buffer_start_loc-1;

        int split_key = (int)ceil(end_pos / 3);
        int sec_split_key = (int)ceil(end_pos / 3) * 2;
        last_level_nodes+=2;
        entry_key_t split_key_value = records[split_key].key;
        entry_key_t sec_split_key_value = records[sec_split_key].key;

        
        InterNode *sibling = new InterNode();
        InterNode *sec_sibling = new InterNode();

        sibling->header.last_split_key = sec_split_key_value;
        sec_sibling->header.last_split_key = header.last_split_key;
        header.last_split_key = split_key_value;

        sibling->header.depth = header.depth;
        sibling->header.is_pln = header.is_pln;
        sec_sibling->header.depth = header.depth;
        sec_sibling->header.is_pln = header.is_pln;
        int sibling_num = 0;
        int sec_sibling_num = 0;



        sibling->header.leftmost_ptr = records[split_key].ptr;

        for(int i = split_key+1; i < sec_split_key; i++){
            header.capacity++;
            sibling->insert_key_count(records[i].key, &sibling_num, records[i].ptr, header.leaf_count[i]);
            sibling_num++;
            records[i].key = -1;
            records[i].ptr = NULL;
            header.leaf_count[i] = 0;
        }
        records[split_key].key = -1;
        records[split_key].ptr = NULL;
        header.leaf_count[split_key] = 0;
        header.capacity++;

        


        sec_sibling->header.leftmost_ptr = records[sec_split_key].ptr;

        for(int i = sec_split_key+1; i < end_pos; i++){
            if(records[i].key != -1){
                header.capacity++;
                sec_sibling->insert_key_count(records[i].key, &sec_sibling_num, records[i].ptr, header.leaf_count[i]);
                sec_sibling_num++;
                records[i].key = -1;
                records[i].ptr = NULL;
                header.leaf_count[i] = 0;
            }
        }
        records[sec_split_key].key = -1;
        records[sec_split_key].ptr = NULL;
        header.leaf_count[sec_split_key] = 0;
        header.capacity++;



        
        if (IS_FORWARD(header.switch_counter))
            header.switch_counter += 2;
        else
            ++header.switch_counter; 

        header.last_index = split_key -1;

        
        sibling->header.buffer_start_loc = sibling_num+1;
        sec_sibling->header.buffer_start_loc = sec_sibling_num+1;

        sec_sibling->header.next = header.next;
        sibling->header.next = sec_sibling;
        header.next = sibling;

        

        header.buffer_start_loc = split_key+1;

        it++;
        for(; it != split_entries.end(); it++){

            if(it->split_key < split_key_value){
                int end_pos1 = header.buffer_start_loc-1;
                insert_key_count(it->split_key, &end_pos1, it->value, it->sibling_num);
                header.buffer_start_loc++;
                header.capacity--;
            }else if(it->split_key < sec_split_key_value){
                int end_pos1 = sibling->header.buffer_start_loc-1;
                sibling->insert_key_count(it->split_key, &end_pos1, it->value, it->sibling_num);
                sibling->header.buffer_start_loc++;
            }else{
                int end_pos1 = sec_sibling->header.buffer_start_loc-1;
                sec_sibling->insert_key_count(it->split_key, &end_pos1, it->value, it->sibling_num);
                sec_sibling->header.buffer_start_loc++;
                
            }
        }


        sibling->header.capacity = ISLOTS - sibling->header.buffer_start_loc;
        sec_sibling->header.capacity = ISLOTS - sec_sibling->header.buffer_start_loc;

        //move the buffer part to the sibling and second_sibling
        int sibling_b_num = 0, sec_sibling_b_num = 0;
        for(int i = header.buffer_start_loc; i < ISLOTS; i++){
            if(header.bitmap[i] == 1)
                if(records[i].key < split_key_value){
                    header.leaf_count[i] = header.buffer_start_loc-1;
                    for(int ii = 0; ii < header.buffer_start_loc-1; ii++){
                        if(records[i].key < records[ii].key){
                            header.leaf_count[i] = ii;
                            break;
                        }

                        if(ii == header.buffer_start_loc-2)
                            header.leaf_count[i] = ii + 1;

                    }
                
                }else if(records[i].key >= split_key_value && records[i].key < sec_split_key_value){
                    sibling->records[sibling->header.buffer_start_loc+sibling_b_num].key = records[i].key;
                    sibling->records[sibling->header.buffer_start_loc+sibling_b_num].ptr = records[i].ptr;
                    sibling->header.versions[sibling->header.buffer_start_loc+sibling_b_num] = header.versions[i];
                    sibling->header.capacity--;
                    header.capacity++;
                    for(int ii = 0; ii < sibling->header.buffer_start_loc-1; ii++){
                        if(records[i].key < sibling->records[ii].key){
                            sibling->header.leaf_count[sibling->header.buffer_start_loc+sibling_b_num] = ii;
                            break;
                        }

                        if(ii == sibling->header.buffer_start_loc-2){
                            sibling->header.leaf_count[sibling->header.buffer_start_loc+sibling_b_num] = ii+1;
                        }

                    }
                    sibling->header.bitmap[sibling->header.buffer_start_loc+sibling_b_num] = 1;
                    header.bitmap[i] = 0;
                    sibling_b_num++;
                }else if(records[i].key >= sec_split_key_value){
                    sec_sibling->header.capacity--;
                    header.capacity++;
                    sec_sibling->records[sec_sibling->header.buffer_start_loc+sec_sibling_b_num].key = records[i].key;
                    sec_sibling->records[sec_sibling->header.buffer_start_loc+sec_sibling_b_num].ptr = records[i].ptr;
                    sec_sibling->header.versions[sec_sibling->header.buffer_start_loc+sec_sibling_b_num] = header.versions[i];
                    // sec_sibling->header.leaf_count[i] = sec_sibling->header.buffer_start_loc-1;
                    for(int ii = 0; ii < sec_sibling->header.buffer_start_loc-1; ii++){
                        if(records[i].key < sec_sibling->records[ii].key){
                            sec_sibling->header.leaf_count[sec_sibling->header.buffer_start_loc+sec_sibling_b_num] = ii;
                            break;
                        }

                        if(ii == sec_sibling->header.buffer_start_loc-2){
                            sec_sibling->header.leaf_count[sec_sibling->header.buffer_start_loc+sec_sibling_b_num] = ii+1;
                        }
                    }
                    sec_sibling->header.bitmap[sec_sibling->header.buffer_start_loc+sec_sibling_b_num] = 1;
                    header.bitmap[i] = 0;
                    sec_sibling_b_num++;
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
            // cout<<"new level:"<< header.depth+1<<endl;
            pthread_rwlock_unlock(header.rwlock);
            

        }else{
            int insert_depth = header.depth+1;
            pthread_rwlock_unlock(header.rwlock);
            
            bt->bitree_insert_internal(split_key_value, (char* )sibling, insert_depth, false);
            bt->bitree_insert_internal(sec_split_key_value, (char* )sec_sibling, insert_depth, false);
        }

        return invalids;

    }



    void traditional_sort_store(entry_key_t key, btree *bt, char *value, bool is_pln){


        pthread_rwlock_wrlock(header.rwlock);
        
       


        if(header.next){
            if(key > header.next->records[0].key){
                pthread_rwlock_unlock(header.rwlock); 
                header.next->traditional_sort_store(key, bt, value, is_pln);
                return;
            }
        }

        int end_pos = count1();


        for(int i =0; i < ISLOTS; i++){
            if(records[i].key == key){
                records[i].ptr = value;
                pthread_rwlock_unlock(header.rwlock);
                return;
            }
        }
        header.buffer_start_loc++;
        
        if(end_pos < ISLOTS-4){
            insert_key(key, &end_pos, value);


            pthread_rwlock_unlock(header.rwlock);
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
                pthread_rwlock_unlock(header.rwlock);

            }else{
                int insert_depth = header.depth+1;
                pthread_rwlock_unlock(header.rwlock);
                bt->bitree_insert_internal(split_key_value, (char* )sibling, insert_depth, false);
            }

            return;

        }
    }
    
    

    void last_inner_sort_store(entry_key_t key, btree *bt, char *value, bool is_pln){
        pthread_rwlock_wrlock(header.rwlock);
        // pthread_rwlock_lock(header.rwlock);
        if(header.next){
            if(key > header.next->records[0].key){
                pthread_rwlock_unlock(header.rwlock);
                header.next->last_inner_sort_store(key, bt, value, is_pln);
                return;
            }
        }

        int end_pos = 0;

        for(int i = 0; i <= ISLOTS; i++){
            if(records[i].key == -1){
                end_pos = i;
                break;
            }
        }
        

        if(records[header.buffer_start_loc-2].key != -1)
            header.buffer_start_loc++;
        

        

        if(end_pos < ISLOTS - 4){
            insert_key(key, &end_pos, value);

            pthread_rwlock_unlock(header.rwlock);
            return;
        }else{

            int split_key = (int)ceil(end_pos / 3);
            int sec_split_key = (int)ceil(end_pos / 3) * 2;
            entry_key_t split_key_value = records[split_key].key;
            entry_key_t sec_split_key_value = records[sec_split_key].key;

            InterNode *sibling = new InterNode();
            InterNode *sec_sibling = new InterNode();

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
                    sec_sibling->insert_key(records[i].key, &sec_sibling_num, records[i].ptr);
                    sec_sibling_num++;
                    records[i].key = -1;
                    records[i].ptr = NULL;
                }
                records[sec_split_key].key = -1;
                records[sec_split_key].ptr = NULL;

            }else{
                
            }
            if (IS_FORWARD(header.switch_counter))
                header.switch_counter += 2;
            else
                ++header.switch_counter; 

            header.last_index = split_key -1;

            if(key >= sec_split_key_value){
                sec_sibling->insert_key(key, &sec_sibling_num, value);

                header.buffer_start_loc = split_key+1;
                sec_sibling->header.buffer_start_loc = sec_sibling_num+2;
                sibling->header.buffer_start_loc = sibling_num+1;
            }else if(key >= split_key_value){
                sibling->insert_key(key, &sibling_num, value);

                header.buffer_start_loc = split_key+1;
                sibling->header.buffer_start_loc = sibling_num+2;
                sec_sibling->header.buffer_start_loc = sec_sibling_num+1;
            }else{
                int split_key_pos = split_key;
                insert_key(key, &split_key_pos, value);

                header.buffer_start_loc = split_key+2;
                sibling->header.buffer_start_loc = sibling_num+1;
                sec_sibling->header.buffer_start_loc = sec_sibling_num+1;
            }

            sec_sibling->header.next = header.next;
            sibling->header.next = sec_sibling;
            header.next = sibling;

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
                // cout<<"new level:"<< header.depth+1<<endl;
                pthread_rwlock_unlock(header.rwlock);

            }else{
                int insert_depth = header.depth+1;
                pthread_rwlock_unlock(header.rwlock);
                bt->bitree_insert_internal(split_key_value, (char* )sibling, insert_depth, false);
                bt->bitree_insert_internal(sec_split_key_value, (char* )sec_sibling, insert_depth, false);
                internal_num+=2;
                
            }

            return;

        }
    }

    


};




void btree::bitree_find_through_all(vector<entry_key_t> vec){
    

    

    for(vector<entry_key_t>::iterator it = vec.begin(); it != vec.end(); it++){
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
                        cout<<"has fail search "<< *it << " in internal buffer\n";
                        fail_search = true;
                        goto next_it;
                    }
                }
                total_cap++;
            }
            temp_leaf = temp_leaf->header.next;
            // insert_leaf = insert_leaf->header.next[insert_leaf->header.usen];
        }

        

        while(insert_leaf){
            for(int i = 0; i < SLOTS; i++){
                if(insert_leaf->bitmap[i] == 1){
                    if(insert_leaf->records[i].key == *it){
                        cout<<"has fail search "<< *it << " in leaf buffer\n";
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
            cout<<"no search !!!\n";
        }
    }

    // cout<<"already use slots: "<<use_cap<<endl;
    // cout<<"total slots: "<<total_cap<<endl;
    // cout<<"slots utility rate: "<<use_cap*1.0/total_cap<<endl;


}

void btree::bitree_print_all() {

    

    // output read amplification
    // TOID(LeafNode) out_leaf = root;
    // InterNode *temp_leaf = v_root;

    // ofstream ofs("read_amplification.txt", ios::app);

    // while(temp_leaf->header.is_pln != true){
    //     int depth = temp_leaf->header.depth;
    //     long long numa = 0;
    //     long long numn = 0;
    //     cout<<depth<<endl;
    //     InterNode *out_temp = temp_leaf;
    //     do{
    //         ofs<<out_temp->header.depth<<" "<<out_temp->header.modify_times<<endl;
    //         records[depth] += out_temp->header.modify_times;
    //         numa +=  out_temp->header.modify_times;
    //         numn++;
    //         if(out_temp->header.next)
    //             out_temp = (InterNode *)out_temp->header.next;
    //     }while(out_temp->header.next);

    //     cout << depth << " " << numa/numn << endl;

    //     temp_leaf = (InterNode *)temp_leaf->header.leftmost_ptr;

    // }

    // cout<<"start cout\n";

    // for(int i = 4; i > 1; i--){
    //     cout<<i<<" "<<records[i]<<endl;
    // }


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
    ofstream ofs("../out_order_bulkload.txt", ios::app);
    //start count internal num
    while(temp_leaf){
        if(temp_leaf->header.buffer_start_loc != 0){
            for(int i = temp_leaf->header.buffer_start_loc; i < ISLOTS; i++){
                if(temp_leaf->header.bitmap[i] == 1){
                    
                    ofs<<temp_leaf->records[i].key<<endl;
                    numa++;
                }
            }
        }
        temp_leaf = temp_leaf->header.next;
        ofs<<endl;
    }

    ofs<<endl;
    
    

    // LeafNode *temp_leaf = root;
    // while(D_RO(temp_leaf)->header.leftmost_ptr != NULL){
    //     temp_leaf.oid.off = (uint64_t)D_RW(temp_leaf)->header.leftmost_ptr;
    // }


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

    cout<<"create log use time"<<time <<endl;
    log->constructor();
    printf("btree build completed\n");
}


void btree::bitree_buffer_insert(entry_key_t key, int tid) {
    int ver = log->add_log(key, tid);

    if(v_root!=NULL){

        InterNode *temp_leaf = v_root;

        while(!temp_leaf->header.is_pln){
            temp_leaf = (InterNode *)temp_leaf->linear_search(key);
        }
        // temp_leaf->internal_buffer_insert_descend_v2(key, this, (char *)key, true);
        // vector<int> invalids = temp_leaf->internal_buffer_insert_descend_v1(key, this, (char *)key, true, ver);
        vector<int> invalids = temp_leaf->internal_buffer_insert_descend_leaf_count_based(key, this, (char *)key, true, ver);


        //invalidate the logs
        if(!invalids.empty()){
            for(int i = 0; i < invalids.size(); i++){
                invalid[invalids[i]]--;
            }
        }
        
    }else{
        LeafNode* temp_leaf = root;
        // flush_num++;
        temp_leaf->unsort_store(key, this, (char *)key, true);
        
    }

}



void btree::bitree_insert(entry_key_t key, int tid) {
    struct timespec t1, t2;
    if(worker_id == 2){
            clock_gettime(CLOCK_MONOTONIC, &t1);
        }

    
    if(v_root!=NULL){

        InterNode *temp_leaf = v_root;
        while(!temp_leaf->header.is_pln){
            temp_leaf = (InterNode *)temp_leaf->linear_search(key);
            
        }
        
        LeafNode* insert_leaf = root;

        insert_leaf = (LeafNode *)temp_leaf->find_internal_leaf_key_search(key);

        // LEAF_PREF(&insert_leaf);

        insert_leaf -> unsort_store(key, this, (char *)key, true);
        
        // D_RW(insert_leaf) -> traditional_sort_store(key, this, (char *)key, true, tid);
        

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
    struct timespec time1, time2;

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

        pthread_rwlock_rdlock(temp_leaf->header.rwlock);
        res = temp_leaf->update_temp_key(key);

        if(res == 0){

            LeafNode* insert_leaf = root;
            insert_leaf = (LeafNode *)temp_leaf->find_internal_leaf_key_search(key);
            res = insert_leaf->update_leaf_key_test(key);
            // res = insert_leaf->find_internal_leaf_key_fingerprints(key);
            pthread_rwlock_unlock(temp_leaf->header.rwlock);
        }else{
            log->add_log(key, tid);
            pthread_rwlock_unlock(temp_leaf->header.rwlock);
            

        }

    }else{
        LeafNode* temp_leaf = root;

        res = temp_leaf->find_internal_leaf_key_test(key);

    }
}

entry_key_t btree::bitree_buffer_search(entry_key_t key) {
    struct timespec time1, time2;

    // ofstream ofss("search_res.txt", ios::app);
    
    entry_key_t res = 0;
    if(v_root!=NULL){
        
        InterNode *temp_leaf = v_root;

        while(!temp_leaf->header.is_pln){
            temp_leaf = (InterNode *)temp_leaf->linear_search(key);
        }

        pthread_rwlock_rdlock(temp_leaf->header.rwlock);
        res = temp_leaf->internal_find_temp_key(key);

        if(res == 0){

            LeafNode* insert_leaf = root;
            insert_leaf = (LeafNode *)temp_leaf->find_internal_leaf_key_search(key);
            // res = insert_leaf->find_internal_leaf_key_test(key);
            res = insert_leaf->find_internal_leaf_key_fingerprints(key);
            pthread_rwlock_unlock(temp_leaf->header.rwlock);
        }else{
            pthread_rwlock_unlock(temp_leaf->header.rwlock);
            return res;

        }

    }else{
        LeafNode* temp_leaf = root;

        res = temp_leaf->find_internal_leaf_key_test(key);

    }

    return res;

}

void btree::bitree_insert_buffer_internal(entry_key_t key, char *value, int depth, bool is_pln) {

    InterNode *temp_leaf = v_root;
    while(temp_leaf->header.depth != depth){
        temp_leaf = (InterNode *)temp_leaf->linear_search(key);
    }
    temp_leaf->last_inner_sort_store(key, this, value, is_pln);
}



void btree::bitree_insert_internal(entry_key_t key, char *value, int depth, bool is_pln) {

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
    cout<<"log size:"<<sizeof(plog)<<endl;
    nvm_dram_alloc(&ret, CACHE_LINE_SIZE, sizeof(plog));
    return new (ret) plog;
}




#endif //UNTITLED1_BINARY_TREE_H

#endif //FAST_FAIR_MYSELF_BTREE_H
