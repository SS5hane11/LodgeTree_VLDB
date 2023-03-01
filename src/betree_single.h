//
// Created by ss5hane1 on 2021/11/25.
//

#ifndef BETREE_BETREE_SINGLE_H
#define BETREE_BETREE_SINGLE_H


#include<iostream>
#include<cstdio>
#include<cstdlib>
#include<vector>
#include<algorithm>
#include<cctype>
#include<unistd.h>
#include<climits>
#include<pthread.h>
#include<mutex>
#include<cmath>
#include <shared_mutex>

#include "nvm-common.h"

#define LEN 60

using namespace std;

extern int flush_cnt;
extern int betree_node_num;
extern int64_t trace_key;
extern int trace_key_insert_times;
extern long long internal_be_num;
extern int buffer_fail_num[32];
extern long long sort_time;
extern long long compare_num;

namespace betree {

    const double internal_epsilon = 0.5;
    const int leaf_epsilon = 1;

    enum epsilon_t{
        Internal, Leaf
    };

    using entry_key_t = int64_t;

    class betree_single;
    class Node;

    class entry{
        friend class betree_single;
        friend class Node;

    private:
        entry_key_t key;
        char* ptr;

    public:
        entry(uint64_t key_, char* value_);
        entry();
    };

    class betree_single {


    private:
        Node *root;
        long long flush_num;
    public:
        friend class Node;
        betree_single();
        void constructor();
        void betree_insert_key(entry_key_t key, char* value);
        void betree_insert_internal(betree_single *bt, entry_key_t key, char* value, int depth);
        entry_key_t betree_search_key(entry_key_t key);
        void find_all_keys();

        //traditional b+tree insert method
        void btree_insert_key(entry_key_t key, char* value);
        void btree_insert_single_key(entry_key_t key, char* value);
        entry_key_t btree_search_key(entry_key_t key);
    
        int betree_range(entry_key_t key_s, entry_key_t key_e);


    };

    class header{
        friend class betree_single;
        friend class Node;

    private:
        epsilon_t epsilon;
        Node *next;
        uint32_t last_index;
        char* leftmost;
        uint32_t last_index_r;
        uint32_t depth;
        pthread_rwlock_t *rwlock;
        uint8_t bitmap[LEN];

    public:
        header();
        header(epsilon_t);
        void constructor();

    };

    class Node{
        friend class betree_single;
        friend class entry;
    private:
        header hdr;
        unsigned char fps[LEN];
        uint8_t leaf_count[LEN];
        entry records[LEN];

    public:
        Node();

        void constructor();

        //insert key-ptr to internal node
        void insert_key_left(betree_single *bt,entry_key_t key, char* value, entry& split_entry, bool shift);

        void insert_key_left_unlock(betree_single *bt,entry_key_t key, char* value, entry& split_entry, bool shift);

        void insert_key_left_triple(betree_single *bt,entry_key_t key, char* value, entry& split_entry, bool shift);

        //insert key-value pairs to leaf node
        void group_insert_key_left(betree_single *bt, vector<entry>& entries, entry& split_entry);

        //buffer flush to next node
        void buffered_insert_key_right(betree_single *bt, vector<entry>&, entry& split_entry);

        char* find_key(entry_key_t key);

        char* find_key_binary(entry_key_t key);

        entry_key_t search_key(entry_key_t key);

        // void new_buffer_insert(betree_single *bt, vector<entry>&)

        void last_level_buffer_insert(betree_single *bt, entry_key_t key, char* value, entry& split_entry);

        void single_insert_key_left(betree_single *bt, entry_key_t key, char* value, entry& split_entry);
    };


    static Node* construct_new_node(bool is_leaf);

}
#endif //BETREE_BETREE_SINGLE_H
