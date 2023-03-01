//
// Created by ss5hane1 on 2021/11/25.
//

#include "betree_single.h"


int flush_cnt = 0;
int betree_node_num = 0;
int trace_key_insert_times = 0;
int64_t trace_key = 7456608551052981402;
long long sort_time = 0;
int buffer_fail_num[32];
long long compare_num = 0;
long long internal_be_num = 0;

#define ALIGN(addr, alignment) ((char *)((unsigned long)(addr) & ~((alignment) - 1)))
#define CACHE_LINE_SIZE 64
#define CACHELINE_ALIGN(addr) ALIGN(addr, 64)


void clwb_len(volatile void *data, int len)
{  

    volatile char *ptr = CACHELINE_ALIGN(data);
    for (; ptr < (char *)data + len; ptr += CACHE_LINE_SIZE)
    {
      
        asm volatile("clwb %0"
        : "+m"(*(volatile char *)ptr));
        // sfence();
    }
    // flush_cnt++;
    sfence();

    


}

static inline unsigned char hashcode1B(int64_t x) {
    x ^= x>>32;
    x ^= x>>16;
    x ^= x>>8;
    return (unsigned char)(x&0x0ffULL);
}


int nvm_dram_alloc_betree(void **ptr, size_t align, size_t size)
{
    // assert(size < 1073741824UL);
    // int ret = posix_memalign(ptr, align, size);
    *ptr = nvmpool_alloc(size);
    // *ptr = nvmpool_alloc(size);
    // ptr = alloc(size);
    return 0;
}


betree::entry::entry() {
    key = -1;
    ptr = NULL;
}

betree::entry::entry(uint64_t key_, char* value_) {
    key = key_;
    ptr = value_;
}


betree::header::header() {
    epsilon = Internal;
    next = NULL;
    last_index = 0;
    leftmost = NULL;
    last_index_r = 0;
    depth = 0;
    for(int i = 0; i < LEN; i++){
        bitmap[i] = 0;
    }
}


void betree::header::constructor(){
     for(int i = 0; i < LEN; i++){
        bitmap[i] = 0;
    }
    epsilon = Internal;
    next = NULL;
    last_index = 0;
    leftmost = NULL;
    last_index_r = 0;
    depth = 0;
   
}

betree::header::header(epsilon_t et) {
    epsilon = et;
    next = NULL;
    last_index = 0;
    leftmost = NULL;
    last_index_r = 0;
    depth = 0;
    for(int i = 0; i < LEN; i++){
        bitmap[i] = 0;
    }

}





betree::Node::Node() {
    for(auto & record : records){
        record.key = -1;
        record.ptr = NULL;
    }

    for(int i = 0; i < LEN; i++){
        leaf_count[i] = 0;
    }
}


void betree::Node::constructor() {
    hdr.constructor();
    for(auto & record : records){
        record.key = -1;
        record.ptr = NULL;
    }
    hdr.rwlock = new pthread_rwlock_t;
    if (pthread_rwlock_init(hdr.rwlock, NULL)) {
        perror("lock init fail");
        exit(1);
    }
    for(int i = 0; i < LEN; i++){
        leaf_count[i] = 0;
    }
}


void betree::Node::group_insert_key_left(betree_single *bt, vector<entry> &entries, entry& split_entry) {
    //insert group key-value pairs to leaf node
    struct timespec t1, t2;
    vector<entry>::iterator it;

    split_entry.key = -1;

    int j = 0;
    it = entries.begin();

    // clock_gettime(CLOCK_MONOTONIC, &t1);

    reinsert:
    int c_s = 0;
    
    for(; it != entries.end(); it++){

        for(; j < LEN; j++){
            if(hdr.bitmap[j] == 0){
                records[j].key = it->key;
                records[j].ptr = it->ptr;
                unsigned char fp = hashcode1B(it->key);
                hdr.bitmap[j] = 1;
                fps[j] = fp;
                if(it == entries.begin()){
                    c_s = j;
                }

                break;
            }
        }

        if(j == LEN && it != entries.end()){
            clwb_len(&records[c_s], (j-c_s+1)*16);
            flush_cnt += (j-c_s+1);
            clwb_len(&hdr, 64);
            // clock_gettime(CLOCK_MONOTONIC, &t2);
            // sort_time += (t2.tv_sec - t1.tv_sec)*1000000000 + (t2.tv_nsec - t1.tv_nsec);
            goto leaf_split;
        }

    }
    
    clwb_len(&records[c_s], (j-c_s+1)*16);
    flush_cnt += (j-c_s+1);
    clwb_len(&hdr, 64);

    // clock_gettime(CLOCK_MONOTONIC, &t2);
    // sort_time += (t2.tv_sec - t1.tv_sec)*1000000000 + (t2.tv_nsec - t1.tv_nsec);

    return;

    leaf_split:

    Node *node = construct_new_node(true);
    node->constructor();

    bool flag_re = false;
    // erase the repeat keys
    for(int i = 0; i < LEN; i++){
        if(hdr.bitmap[i] == 1)
            for(int kk = 0; kk < LEN; kk++){
                if(i==kk)continue;
                if(hdr.bitmap[kk] == 1){
                    if(records[i].key == records[kk].key){
                        
                        hdr.bitmap[kk] = 0;
                        flag_re = true;
                    }
                }
            }
    }

    if(flag_re){
        j = 0;
        goto reinsert;
    }

    //find the split key
    
    int split_pos = LEN * internal_epsilon / 2;
    entry_key_t split_key = records[split_pos].key;
    for(int i = 0; i < LEN; i++){
        int num = 0;
        for(int k = 0; k < LEN; k++){
            if(records[k].key >= records[i].key){
                num++;
            }

        }

        if(num >= LEN/2 - 3 && num <= LEN/2-3){
            split_pos = i;
            split_key = records[i].key;
            break;
        }

    }

    //transfer half key-value pairs to next node
    int sibling_cnt = 0;

    for(int i = 0; i < LEN; i++){
        if(records[i].key >= split_key){
            node->records[sibling_cnt].key = records[i].key;
            node->records[sibling_cnt].ptr = records[i].ptr;
            node->hdr.bitmap[sibling_cnt] = 1;
            node->fps[sibling_cnt] = fps[i];
            hdr.bitmap[i] = 0;

            sibling_cnt++;
            
        }
    }


    node->hdr.next = hdr.next;
    hdr.next = node;
    node->hdr.epsilon = Leaf;
    node->hdr.leftmost = NULL;
    node->hdr.depth = hdr.depth;

    clwb_len(node, sizeof(Node));
    clwb_len(&hdr, 64);


    //insert the last key-value pairs
    for(; it != entries.end(); it++){
        int k = 0;
        if(it->key < split_key){
            for(; k < LEN; k++){
                if(hdr.bitmap[k] == 0){
                    records[k].key = it->key;
                    records[k].ptr = it->ptr;
                    fps[k] = hashcode1B(it->key);
                    hdr.bitmap[k] = 1;
                    break;
                }
            }

        }else{
            node->records[sibling_cnt].key = it->key;
            node->records[sibling_cnt].ptr = it->ptr;
            node->fps[sibling_cnt] = hashcode1B(it->key);
            node->hdr.bitmap[sibling_cnt] = 1;
            sibling_cnt++;
            
        }

    }

    
    


    if(bt->root == this){
        //this node is the root node
        Node *internal = construct_new_node(false);
        internal->constructor();
        internal->hdr.epsilon = Internal;
        internal->hdr.depth = hdr.depth + 1;
        internal->hdr.leftmost = (char *)this;
        internal->records[0].key = split_key;
        internal->records[0].ptr = (char *)node;
        cout<<"new level:"<<hdr.depth+1<<endl;
        internal->hdr.last_index = 0;
        bt->root = internal;

        // clwb_len(bt->root, sizeof(Node));

        return;

    }else{
//        int insert_depth = hdr.depth+1;
//        bt->betree_insert_internal(bt, split_key, (char*)node, insert_depth);
        split_entry.key = split_key;
        split_entry.ptr = (char *)node;

        // clock_gettime(CLOCK_MONOTONIC, &t2);
        // sort_time += (t2.tv_sec - t1.tv_sec)*1000000000 + (t2.tv_nsec - t1.tv_nsec);

        return;
    }

}

void betree::Node::insert_key_left(betree_single *bt, entry_key_t key, char *value, entry& split_entry, bool shift) {


    pthread_rwlock_wrlock(hdr.rwlock);

    if(split_entry.key != -1 && shift == false){
        if(hdr.next)
            if(key >= split_entry.key){
                pthread_rwlock_unlock(hdr.rwlock);
                hdr.next->insert_key_left(bt, key, value, split_entry, true);
                return;
            }
    }

    if(hdr.next){
        if(key >= hdr.next->records[0].key){
            pthread_rwlock_unlock(hdr.rwlock);
            hdr.next->insert_key_left(bt, key, value, split_entry, true);
            return;
        }
    }

    

    

    int border = 0;
    if(hdr.epsilon == Internal){
        border = internal_epsilon * LEN;
    }else{
        border = leaf_epsilon * LEN;
    }

    int end_pos = hdr.last_index + 1;

    if(end_pos < border) {
        //space is not full, insert key to the index part

        int to_flush_cnt = 0;

        hdr.last_index = end_pos;
        records[end_pos].key = records[end_pos-1].key;
        records[end_pos].ptr = records[end_pos-1].ptr;

        for (int i = end_pos - 1; i >= 0; i--) {
            if (records[i].key <= key) {
                records[i+1].key = key;
                records[i+1].ptr = value;
                // clwb_len(&records[i+1], 64);
                //call the clflush
                pthread_rwlock_unlock(hdr.rwlock);
                return;
            }else{
                records[i+1].key = records[i].key;
                records[i+1].ptr = records[i].ptr;
            }
        }

        records[0].key = key;
        records[0].ptr = value;
        flush_cnt+=1;
        // clwb_len(&records[0], 64);

        pthread_rwlock_unlock(hdr.rwlock);

        return;

    }


        //do split
        int split_pos = border/2;
        entry_key_t split_key = records[split_pos].key;
        Node *node = construct_new_node(false);
        node->constructor();
        node->hdr.epsilon = hdr.epsilon;


        int sibling_cnt = 0;
        int buffer_sibling_cnt = border;
        hdr.last_index = split_pos-1;


        if(hdr.leftmost){
            //internal node
            for(int i = split_pos + 1; i < border; i++){
                node->records[sibling_cnt].key = records[i].key;
                node->records[sibling_cnt].ptr = records[i].ptr;
                // records[i].key = -1;
                sibling_cnt++;
            }
            node->hdr.last_index = sibling_cnt-1;

            records[split_pos].key = -1;
            node->hdr.leftmost = records[split_pos].ptr;

            for(int i = border; i < LEN; i++){
                if(hdr.bitmap[i] == 1)
                    if(records[i].key >= split_key){
                        node->records[buffer_sibling_cnt].key = records[i].key;
                        node->records[buffer_sibling_cnt].ptr = records[i].ptr;
                        node->hdr.bitmap[buffer_sibling_cnt] = 1;
                        node->fps[buffer_sibling_cnt] = fps[i];
                        hdr.bitmap[i] = 0;

                        buffer_sibling_cnt++;
                    }
            }
        }


        // TODO: not insert the key
        entry ss;
        if(key >= split_key){
            node->insert_key_left_unlock(bt, key, value, ss, true);

        }else{
            insert_key_left_unlock(bt, key, value, ss, true);
        }

        node->hdr.next = hdr.next;
        hdr.next = node;
        node->hdr.depth = hdr.depth;

        //TODO: insert key-ptr to internal nodes

        if(bt->root == this){
            //this node is the root node
            Node *internal = construct_new_node(false);
            internal->constructor();
            internal->hdr.epsilon = Internal;
            internal->hdr.depth = hdr.depth + 1;
            internal->hdr.leftmost = (char *)this;
            internal->records[0].key = split_key;
            internal->records[0].ptr = (char *)node;
            internal->hdr.last_index = 0;
            cout<<"new level:"<<hdr.depth+1<<endl;
            bt->root = internal;

            pthread_rwlock_unlock(hdr.rwlock);
            return;

        }else{
            split_entry.key = split_key;
            split_entry.ptr = (char*)node;

            pthread_rwlock_unlock(hdr.rwlock);
            return ;
        }

}

void betree::Node::insert_key_left_unlock(betree_single *bt, entry_key_t key, char *value, entry& split_entry, bool shift) {


    // pthread_rwlock_wrlock(hdr.rwlock);

    if(split_entry.key != -1 && shift == false){
        if(hdr.next)
            if(key >= split_entry.key){
                // pthread_rwlock_unlock(hdr.rwlock);
                hdr.next->insert_key_left(bt, key, value, split_entry, true);
                return;
            }
    }

    if(hdr.next){
        if(key >= hdr.next->records[0].key){
            // pthread_rwlock_unlock(hdr.rwlock);
            hdr.next->insert_key_left(bt, key, value, split_entry, true);
            return;
        }
    }

    

    

    int border = 0;
    if(hdr.epsilon == Internal){
        border = internal_epsilon * LEN;
    }else{
        border = leaf_epsilon * LEN;
    }

    int end_pos = hdr.last_index + 1;

    if(end_pos < border) {
        //space is not full, insert key to the index part

        int to_flush_cnt = 0;

        hdr.last_index = end_pos;
        records[end_pos].key = records[end_pos-1].key;
        records[end_pos].ptr = records[end_pos-1].ptr;

        for (int i = end_pos - 1; i >= 0; i--) {
            if (records[i].key <= key) {
                records[i+1].key = key;
                records[i+1].ptr = value;
                // clwb_len(&records[i+1], 64);
                //call the clflush
                // pthread_rwlock_unlock(hdr.rwlock);
                return;
            }else{
                records[i+1].key = records[i].key;
                records[i+1].ptr = records[i].ptr;
            }
        }

        records[0].key = key;
        records[0].ptr = value;
        flush_cnt+=1;
        // clwb_len(&records[0], 64);

        // pthread_rwlock_unlock(hdr.rwlock);

        return;

    }


        //do split
        int split_pos = border/2;
        entry_key_t split_key = records[split_pos].key;
        Node *node = construct_new_node(false);
        node->constructor();
        node->hdr.epsilon = hdr.epsilon;


        int sibling_cnt = 0;
        int buffer_sibling_cnt = border;
        hdr.last_index = split_pos-1;


        if(hdr.leftmost){
            //internal node
            for(int i = split_pos + 1; i < border; i++){
                node->records[sibling_cnt].key = records[i].key;
                node->records[sibling_cnt].ptr = records[i].ptr;
                // records[i].key = -1;
                sibling_cnt++;
            }
            node->hdr.last_index = sibling_cnt-1;

            records[split_pos].key = -1;
            node->hdr.leftmost = records[split_pos].ptr;

            for(int i = border; i < LEN; i++){
                if(hdr.bitmap[i] == 1)
                    if(records[i].key >= split_key){
                        node->records[buffer_sibling_cnt].key = records[i].key;
                        node->records[buffer_sibling_cnt].ptr = records[i].ptr;
                        node->hdr.bitmap[buffer_sibling_cnt] = 1;
                        node->fps[buffer_sibling_cnt] = fps[i];
                        hdr.bitmap[i] = 0;

                        buffer_sibling_cnt++;
                    }
            }
        }


        // TODO: not insert the key
        entry ss;
        if(key >= split_key){
            node->insert_key_left_unlock(bt, key, value, ss, true);

        }else{
            insert_key_left_unlock(bt, key, value, ss, true);
        }

        node->hdr.next = hdr.next;
        hdr.next = node;
        node->hdr.depth = hdr.depth;

        //TODO: insert key-ptr to internal nodes

        if(bt->root == this){
            //this node is the root node
            Node *internal = construct_new_node(false);
            internal->constructor();
            internal->hdr.epsilon = Internal;
            internal->hdr.depth = hdr.depth + 1;
            internal->hdr.leftmost = (char *)this;
            internal->records[0].key = split_key;
            internal->records[0].ptr = (char *)node;
            internal->hdr.last_index = 0;
            cout<<"new level:"<<hdr.depth+1<<endl;
            bt->root = internal;

            // pthread_rwlock_unlock(hdr.rwlock);
            return;

        }else{
            split_entry.key = split_key;
            split_entry.ptr = (char*)node;

            // pthread_rwlock_unlock(hdr.rwlock);
            return ;
        }

}


void betree::Node::insert_key_left_triple(betree_single *bt, entry_key_t key, char *value, entry& split_entry, bool shift) {


    if(split_entry.key != -1 && shift == false){
        if(hdr.next)
            if(key >= split_entry.key){
                hdr.next->insert_key_left(bt, key, value, split_entry, true);
                return;
            }
    }

    if(hdr.next){
        if(key >= hdr.next->records[0].key){
            hdr.next->insert_key_left(bt, key, value, split_entry, true);
            return;
        }
    }

    int border = 0;
    if(hdr.epsilon == Internal){
        border = internal_epsilon * LEN;
    }else{
        border = leaf_epsilon * LEN;
    }

    int end_pos = hdr.last_index + 1;

    if(end_pos < border) {
        //space is not full, insert key to the index part

        int to_flush_cnt = 0;

        hdr.last_index = end_pos;
        records[end_pos].key = records[end_pos-1].key;
        records[end_pos].ptr = records[end_pos-1].ptr;

        for (int i = end_pos - 1; i >= 0; i--) {
            if (records[i].key <= key) {
                records[i+1].key = key;
                records[i+1].ptr = value;
                // clwb_len(&records[i+1], 64);
                //call the clflush

                return;
            }else{
                records[i+1].key = records[i].key;
                records[i+1].ptr = records[i].ptr;
            }
        }

        records[0].key = key;
        records[0].ptr = value;
        flush_cnt+=1;
        // clwb_len(&records[0], 64);


        return;

    }


        //do split
        int split_pos = border/2;

        entry_key_t split_key = records[split_pos].key;
        Node *node = construct_new_node(false);
        node->constructor();
        node->hdr.epsilon = hdr.epsilon;


        int sibling_cnt = 0;
        int buffer_sibling_cnt = border;
        hdr.last_index = split_pos-1;


        if(hdr.leftmost){
            //internal node
            for(int i = split_pos + 1; i < border; i++){
                node->records[sibling_cnt].key = records[i].key;
                node->records[sibling_cnt].ptr = records[i].ptr;
                // records[i].key = -1;
                sibling_cnt++;
            }
            node->hdr.last_index = sibling_cnt-1;

            records[split_pos].key = -1;
            node->hdr.leftmost = records[split_pos].ptr;

            for(int i = border; i < LEN; i++){
                if(hdr.bitmap[i] == 1)
                    if(records[i].key >= split_key){
                        node->records[buffer_sibling_cnt].key = records[i].key;
                        node->records[buffer_sibling_cnt].ptr = records[i].ptr;
                        node->hdr.bitmap[buffer_sibling_cnt] = 1;
                        node->fps[buffer_sibling_cnt] = fps[i];
                        hdr.bitmap[i] = 0;

                        buffer_sibling_cnt++;
                    }
            }
        }


        // TODO: not insert the key
        entry ss;
        if(key >= split_key){
            node->insert_key_left(bt, key, value, ss, true);

        }else{
            insert_key_left(bt, key, value, ss, true);
        }

        node->hdr.next = hdr.next;
        hdr.next = node;
        node->hdr.depth = hdr.depth;

        //TODO: insert key-ptr to internal nodes

        if(bt->root == this){
            //this node is the root node
            Node *internal = construct_new_node(false);
            internal->constructor();
            internal->hdr.epsilon = Internal;
            internal->hdr.depth = hdr.depth + 1;
            internal->hdr.leftmost = (char *)this;
            internal->records[0].key = split_key;
            internal->records[0].ptr = (char *)node;
            internal->hdr.last_index = 0;
            cout<<"new level:"<<hdr.depth+1<<endl;
            bt->root = internal;

            return;

        }else{
            split_entry.key = split_key;
            split_entry.ptr = (char*)node;
            return ;
        }

}

betree::Node* betree::construct_new_node(bool is_leaf) {
    void *ret;

    if(is_leaf){
        betree_node_num++;
        nvm_dram_alloc_betree(&ret, CACHE_LINE_SIZE, sizeof(Node));
    }else{
        internal_be_num++;
        posix_memalign(&ret, 64, sizeof(Node));        
    }

    return new (ret) Node;
}

char* betree::Node::find_key_binary(entry_key_t key){
    // cout<<"ddwd\n";
    // struct timespec t1, t2;
    // clock_gettime(CLOCK_MONOTONIC, &t1);

    if(key < records[0].key){
        compare_num++;
        // clock_gettime(CLOCK_MONOTONIC, &t2);
        // sort_time += ((t2.tv_sec - t1.tv_sec)*1000000000 + (t2.tv_nsec - t1.tv_nsec));
        return hdr.leftmost;
    }else{
        int end_pos = hdr.last_index;
        // if(key > records[end_pos].key){
        //     clock_gettime(CLOCK_MONOTONIC, &t2);
        //     sort_time += ((t2.tv_sec - t1.tv_sec)*1000000000 + (t2.tv_nsec - t1.tv_nsec));
        //     return records[end_pos].ptr;
        // }

        
        int low = 0, high = end_pos;
        int pos = (low+high)/2;
        
        if(records[pos].key > key){
            high = pos;
        }else{
            low = pos;
        }

        for(int i = low; i <= high; i++){
            if(key < records[i].key){
                
                // clock_gettime(CLOCK_MONOTONIC, &t2);
                // sort_time += ((t2.tv_sec - t1.tv_sec)*1000000000 + (t2.tv_nsec - t1.tv_nsec));
                return records[i-1].ptr;
            }else{
                compare_num++;  
            }
        }

        compare_num++;
        return records[high].ptr;

        // while(low < high){
        //     pos = (low + high)/2;
        //     if(records[pos].key<key && records[pos+1].key>key)
        //         return records[pos].ptr;

        //     if(records[pos].key > key){
        //         high = pos;
        //         continue;
        //     }

        //     if(records[pos].key < key && records[pos+1].key < key){
        //         low = pos+1;
        //         continue;
        //     }
        // }
    } 
    
    
}

char* betree::Node::find_key(entry_key_t key) {
    // cout<<"wiwiwi\n";
    // struct timespec t1, t2;
    // clock_gettime(CLOCK_MONOTONIC, &t1);
    // pthread_rwlock_rdlock(hdr.rwlock);
    if(key < records[0].key){
        compare_num++;
        // clock_gettime(CLOCK_MONOTONIC, &t2);
        // sort_time += ((t2.tv_sec - t1.tv_sec)*1000000000 + (t2.tv_nsec - t1.tv_nsec));
        // pthread_rwlock_unlock(hdr.rwlock);
        return hdr.leftmost;
    }

    int end_pos = hdr.last_index;

    // for(int i = border-1; i >= 0; i--){
    //     if(records[i].key != -1){
    //         end_pos = i;
    //         break;
    //     }
    // }

    for(int i = 1; i <= end_pos; i++){
        if(key < records[i].key){
            
            // clock_gettime(CLOCK_MONOTONIC, &t2);
            // sort_time += ((t2.tv_sec - t1.tv_sec)*1000000000 + (t2.tv_nsec - t1.tv_nsec));
            // pthread_rwlock_unlock(hdr.rwlock);
            return records[i-1].ptr;
        }else{
            compare_num++;
        }
    }

    // clock_gettime(CLOCK_MONOTONIC, &t2);
    // sort_time += ((t2.tv_sec - t1.tv_sec)*1000000000 + (t2.tv_nsec - t1.tv_nsec));
    compare_num++;
    // pthread_rwlock_unlock(hdr.rwlock);
    return records[end_pos].ptr;
}

//void betree::Node::insert_key_right(entry_key_t key, char *value) {
//
//    if(hdr.epsilon == Leaf){
//
//    }else{
//
//    }
//
//    int border = internal_epsilon * LEN;
//
//
//    // if has empty space, write
//    for(int i = border; i < LEN; i++){
//        if(hdr.bitmap[i] == 0){
//            records[i].key = key;
//            records[i].ptr = value;
//            hdr.bitmap[i] = 1;
//            clwb_len(&records[i], 16);
//            clwb_len(&hdr, sizeof(header));
//
//            return ;
//        }else{
//            if(records[i].key == key){
//                records[i].ptr = value;
//                clwb_len(&records[i], 16);
//                return;
//            }
//        }
//    }
//
//
//    //no empty space, do flush
//    vector<entry> split_entry;
//    vector<entry> insert_queue;
//
//    for(int i = 0; i < border; i++){
//
//        for(int j = border; j < LEN; j++){
//            if(hdr.bitmap[j] == 1){
//                if(records[j].key <= records[i].key){
//                    insert_queue.push_back(records[j]);
//                    hdr.bitmap[j] = 0;
//                }
//            }
//        }
//        Node *leaf;
//        entry temp;
//        temp.key = -1;
//        if(i == 0) {
//            leaf = (Node *) hdr.leftmost;
//        }else{
//            leaf = (Node *) records[i-1].ptr;
//        }
//
//        leaf->buffered_insert_key_right(insert_queue, temp);
//
//        // if leaf has split
//        if(temp.key == -1){
//            split_entry.push_back(temp);
//        }
//    }
//
//    clwb_len(&hdr, sizeof(header));
//
//    vector<entry>::iterator it;
//    for(it = split_entry.begin(); it != split_entry.end(); it++){
//        insert_key_left(it->key, it->ptr, false);
//    }
//
//
//}


void betree::Node::last_level_buffer_insert(betree_single *bt, entry_key_t key, char* value, entry& split_entry){
    int border = hdr.epsilon == Internal ? internal_epsilon * LEN : 0;
    split_entry.key = -1;
    vector<entry>::iterator it, ita;
    //first find if the buffer part has enough space
    int end_pos = 0;

    pthread_rwlock_wrlock(hdr.rwlock);
    

    if(hdr.next){
        if(key > hdr.next->records[0].key){
            pthread_rwlock_unlock(hdr.rwlock);
            hdr.next->last_level_buffer_insert(bt, key, value, split_entry);

            return;
        }
    }
    
    // int c_s = border;
    int pos = border;
    for(int i = 0; i <= hdr.last_index; i++)
        if(key < records[i].key){
            pos = i;
            break;
        }

    vector<entry> split_entries;

    

    for(int j = border; j < LEN; j++){
        if(hdr.bitmap[j] == 0){
            records[j].key = key;
            records[j].ptr = value;
            hdr.bitmap[j] = 1;
            leaf_count[j] = pos;
            if(pos != border)
                leaf_count[pos]++;

            pthread_rwlock_unlock(hdr.rwlock);
            return;
        }else{
            //TODO: update algorithm 
            if(records[j].key == key){
                records[j].ptr = value;

                pthread_rwlock_unlock(hdr.rwlock);
                return ;
            }
        }
        
            
    }


    end_pos = hdr.last_index;

    //for internal node, flush to the next level
    
    //flush all buffered key-value pairs to next level
    
    struct timespec t1, t2;

    // first check if there are multiple flush

    vector<vector<entry>> insert_queues;
    // vector<vector<int>> erase_queues;
    for(int i = 0; i <= end_pos+1; i++){
        vector<entry> queue;
        vector<int> bit_queue;
        insert_queues.push_back(queue);
        // erase_queues.push_back(bit_queue);
    }

    for(int i = border; i < LEN; i++){
        if(leaf_count[i] != border){
            insert_queues[leaf_count[i]].push_back(records[i]);
            // erase_queues[leaf_count[i]].push_back(i);
        }else{
            insert_queues[end_pos+1].push_back(records[i]);
            // erase_queues[end_pos+1].push_back(i);
        }
        // hdr.bitmap[i] = 0;
        
    }

   

    for(int i = 0; i <= end_pos; i++){

        if(pos == i){
            entry aa;
            aa.key = key;
            aa.ptr = value;
            insert_queues[i].push_back(aa);
        }

        if(insert_queues[i].empty()){
            continue;
        }

        Node *leaf;
        if (i == 0) {
            leaf = (Node *) hdr.leftmost;
        }else{
            leaf = (Node *) records[i-1].ptr;
        }

        leaf_count[i] = 0;
        entry split_entry_t;
        split_entry_t.key = -1;
        
        // clock_gettime(CLOCK_MONOTONIC, &t1);
        // LEAF_PREF(leaf);
        // clock_gettime(CLOCK_MONOTONIC, &t1);
        leaf->group_insert_key_left(bt, insert_queues[i], split_entry_t);
        
        // clock_gettime(CLOCK_MONOTONIC, &t2);
        

        // sort_time += ((t2.tv_sec - t1.tv_sec) * 1000000000 + (t2.tv_nsec - t1.tv_nsec));


        if (split_entry_t.key != -1) {
            split_entries.push_back(split_entry_t);
        }

    }

    //insert key > end_pos
    if(pos == border){
        entry aa;
        aa.key = key;
        aa.ptr = value;
        insert_queues[end_pos+1].push_back(aa);
    }

    Node *leaf = (Node *)records[end_pos].ptr;

    entry split_entry_end;
    split_entry_end.key = -1;
    if(leaf->hdr.epsilon == Internal)
        leaf->buffered_insert_key_right(bt, insert_queues[end_pos+1], split_entry_end);
    else
        leaf->group_insert_key_left(bt, insert_queues[end_pos+1], split_entry_end);


    if (split_entry_end.key != -1) {
        split_entries.push_back(split_entry_end);
    }

    for(int i = border; i < LEN; i++)
        hdr.bitmap[i] = 0;


    for (it = split_entries.begin(); it != split_entries.end(); it++) {
        insert_key_left_unlock(bt, it->key, it->ptr, split_entry, false);
    }


    pthread_rwlock_unlock(hdr.rwlock);

    return;

}




void betree::Node::buffered_insert_key_right(betree_single *bt, vector<entry>& entries, entry &split_entry) {

    // pthread_rwlock_wrlock(hdr.rwlock);
    int border = hdr.epsilon == Internal ? internal_epsilon * LEN : 0;
    split_entry.key = -1;
    //first find if the buffer part has enough space
    vector<entry>::iterator it, ita;
    int end_pos = 0;


    ita = entries.begin();
    
    int c_s = border;

    for(; ita != entries.end(); ita++){
        int j = border;

        for(j = border; j < LEN; j++){
            if(hdr.bitmap[j] == 0){
                records[j].key = ita->key;
                records[j].ptr = ita->ptr;
                // fps[j] = hashcode1B(ita->key);
                hdr.bitmap[j] = 1;
                break;
            }else{
                //TODO: update algorithm 
                if(records[j].key == ita->key){
                    records[j].ptr = ita->ptr;
                    break;
                }
            }
            
        }

        if(ita != entries.end() && j == LEN){
            goto flush;
        }

    }

    // pthread_rwlock_unlock(hdr.rwlock);

    return ;


    flush:

    end_pos = hdr.last_index;
    // for(int i = border-1; i >= 0; i--)
    //     if(records[i].key != -1) {
    //         end_pos = i;
    //         break;
    //     }

    //for internal node, flush to the next level
    vector<entry> split_entries;
    //flush all buffered key-value pairs to next level

    for (int i = 0; i <= end_pos; i++) {

        vector<entry> insert_queue;

        for (int k = border; k < LEN; k++) {
            if (hdr.bitmap[k] == 1) {
                if (records[k].key < records[i].key) {

                    insert_queue.push_back(records[k]);

                    hdr.bitmap[k] = 0;
                }
            }
        }

        if(insert_queue.empty()){
            continue;
        }

        Node *leaf;
        if (i == 0) {
            leaf = (Node *) hdr.leftmost;
        }else{
            leaf = (Node *) records[i-1].ptr;
        }

        entry split_entry_t;
        split_entry_t.key = -1;

        if(leaf->hdr.epsilon == Internal)
            leaf->buffered_insert_key_right(bt, insert_queue, split_entry_t);
        else
            leaf->group_insert_key_left(bt, insert_queue, split_entry_t);

        if (split_entry_t.key != -1) {
            split_entries.push_back(split_entry_t);
        }

    }

    //insert key > end_pos
    vector<entry> insert_queue;
    for(int i = border; i < LEN; i++){
        if(hdr.bitmap[i] == 1){
            insert_queue.push_back(records[i]);
            hdr.bitmap[i] = 0;
        }
    }

    Node *leaf = (Node *)records[end_pos].ptr;

    entry split_entry_end;
    split_entry_end.key = -1;
    if(leaf->hdr.epsilon == Internal)
        leaf->buffered_insert_key_right(bt, insert_queue, split_entry_end);
    else
        leaf->group_insert_key_left(bt, insert_queue, split_entry_end);


    if (split_entry_end.key != -1) {
        split_entries.push_back(split_entry_end);
    }


    int new_num = border;
    for (; ita != entries.end(); ita++) {
        records[new_num].key = ita->key;
        records[new_num].ptr = ita->ptr;
        fps[new_num] = hashcode1B(ita->key);
        hdr.bitmap[new_num] = 1;
        new_num++;
    }


    for (it = split_entries.begin(); it != split_entries.end(); it++) {

        insert_key_left(bt, it->key, it->ptr, split_entry, false);
    }

    // pthread_rwlock_unlock(hdr.rwlock);

}

betree::entry_key_t betree::Node::search_key(entry_key_t key) {

    pthread_rwlock_rdlock(hdr.rwlock);
    
    int border = hdr.epsilon == Internal ? internal_epsilon * LEN : 0;
    for(int i = border; i < LEN; i++){
            if(hdr.bitmap[i] == 1){
                if(records[i].key == key){
                    pthread_rwlock_unlock(hdr.rwlock);
                    return records[i].key;
                }
            }
        }
    // if(hdr.epsilon == Internal){
    //     for(int i = border; i < LEN; i++){
    //         if(hdr.bitmap[i] == 1){
    //             if(records[i].key == key){
    //                 pthread_rwlock_unlock(hdr.rwlock);
    //                 return records[i].key;
    //             }
    //         }
    //     }
    // }else{
    //     unsigned char hash = hashcode1B(key);
    //     for(int i = border; i < LEN; i++){
    //         if(hdr.bitmap[i] == 1)
    //             if(fps[i] == hash){
    //                 if(records[i].key == key){
    //                     pthread_rwlock_unlock(hdr.rwlock);
    //                     return records[i].key;
    //                 }
    //             }
    //     }
        
    // }

    pthread_rwlock_unlock(hdr.rwlock);
    return -1;

}

betree::entry_key_t betree::betree_single::betree_search_key(entry_key_t key) {
    Node *node = root;
    entry_key_t value;

    while(node->hdr.leftmost){
        value = node->search_key(key);

        if(value != -1){
            return value;
        }else{
            node = (Node*)node->find_key(key);
        }
    }

    value = node->search_key(key);

    return value;

}

// 7443558
// 7460317
// 7457782419750375417

betree::betree_single::betree_single() {
    root = construct_new_node(true);
    root->constructor();
    root->hdr.depth = 0;
    root->hdr.leftmost = NULL;
    root->hdr.epsilon = Leaf;
    flush_num = 0;

}


void betree::betree_single::constructor(){

    root->constructor();
    root->hdr.depth = 0;
    root->hdr.leftmost = NULL;
    root->hdr.epsilon = Leaf;
    flush_num = 0;
}


void betree::betree_single::find_all_keys(){
    betree::Node * r = root;
    int num = 0;
    betree::Node *t = r;
    while(t->hdr.leftmost){
        //for internal nodes

        betree::Node *p = t;
        while(p){
            for(int i = internal_epsilon * LEN; i < LEN; i++){
                if(p->hdr.bitmap[i] == 1){
                    // cout<<p->records[i].key<<endl;
                    num++;
                }
            }

            p = p->hdr.next;
        }

        t = (Node *)t->hdr.leftmost;
        // cout<<endl;
    }

    while(t){
        for(int i = 0; i < LEN; i++){
            if(t->hdr.bitmap[i] == 1){
                // cout<<t->records[i].key<<endl;
                num++;
            }
        }

        t = (Node *)t->hdr.next;
    }

    cout<<"total insert num:" << num <<endl;
}

void betree::betree_single::betree_insert_key(entry_key_t key, char *value) {

    Node *l = root;
    vector<entry> queue;
    entry a;
    a.key = key;
    a.ptr = value;
    queue.push_back(a);
    entry split_entry;
    split_entry.key = -1;
    if(l->hdr.epsilon == Leaf){
        l->group_insert_key_left(this, queue, split_entry);
    }else{
        l->buffered_insert_key_right(this, queue, split_entry);
    }

}


void betree::betree_single::betree_insert_internal(betree_single *bt, entry_key_t key, char *value, int depth) {
    Node *node = bt->root;

    while(node->hdr.depth != depth){
        node = (Node *)node->find_key(key);
    }

    entry split_entry;

    node->insert_key_left(this, key, value, split_entry, false);

}


int betree::betree_single::betree_range(entry_key_t key_s, entry_key_t key_e){
    Node *node_s = root, *node_e = root, *prev_s = root, *prev_e = root;
    vector<entry> res;
    
    while(node_s->hdr.depth != 0){
        node_s = (Node *)prev_s->find_key(key_s);
        node_e = (Node *)prev_e->find_key(key_e);

        if(!node_s){
            break;
        }

        prev_s = node_s;
        prev_e = node_e;

        while(node_s->hdr.next != node_e){
            for(int i = LEN*0.5; i < LEN; i++){
                res.push_back(node_s->records[i]);
            }
            node_s = node_s->hdr.next;
        }
    }

    return res.size();
}



// for last level buffer insert
void betree::betree_single::btree_insert_key(entry_key_t key, char* value){
    Node *node = root;
    vector<char *> vec;
    vector<entry> insert_entries;
    entry cc, split_entry;
    split_entry.key = -1;
    struct timespec t1, t2;
    

    if(node->hdr.depth != 0){

        
        vec.push_back((char *)node);
        while(node->hdr.depth != 1){
            node = (Node *)node->find_key(key);
            // node = (Node *)node->find_key_binary(key);
            NODE_PREF(node);
            vec.push_back((char *)node);
        }
        
        // clock_gettime(CLOCK_MONOTONIC, &t1);
        node->last_level_buffer_insert(this, key, value, split_entry);

        // clock_gettime(CLOCK_MONOTONIC, &t2);

        // sort_time += ((t2.tv_sec - t1.tv_sec) * 1000000000 + (t2.tv_nsec - t1.tv_nsec));
        // node->buffered_insert_key_right(this, insert_entries, split_entry);
        vec.pop_back();

        
        while(split_entry.key != -1){
            
            entry new_split;
            new_split.key = -1;
            Node *temp = (Node *)vec.at(vec.size()-1);
            temp->insert_key_left(this, split_entry.key, split_entry.ptr, new_split, false);
            split_entry.key = new_split.key;
            split_entry.ptr = new_split.ptr;
            vec.pop_back();
        }
        
       

    }else{
        cc.key = key;
        cc.ptr = value;

        insert_entries.push_back(cc);
        node->group_insert_key_left(this, insert_entries, split_entry);
        
    }

    return ; 
    
}





betree::entry_key_t betree::betree_single::btree_search_key(entry_key_t key){
    Node *node = root;
    entry_key_t value;

    while(node->hdr.depth != 1){
        
        node = (Node*)node->find_key(key);
        
    }

    value = node->search_key(key);

    if(value != -1){
        return value;
    }else{
        node = (Node *)node->find_key(key);
    }

    value = node->search_key(key);

    return value;
}



//for single insert
void betree::betree_single::btree_insert_single_key(entry_key_t key, char* value){
    Node *node = root;
    vector<char *> vec;
    entry cc, split_entry;
    split_entry.key = -1;

    while(node->hdr.leftmost){
        vec.push_back((char*)node);
        node = (Node *)node->find_key(key);
        // node = (Node *)node->find_key_binary(key);
    }

    node->single_insert_key_left(this, key, value, split_entry);

    while(split_entry.key != -1){
        entry new_split;
        new_split.key = -1;
        Node *temp = (Node *)vec.at(vec.size()-1);
        temp->insert_key_left(this, split_entry.key, split_entry.ptr, new_split, false);
        split_entry.key = new_split.key;
        split_entry.ptr = new_split.ptr;
        vec.pop_back();
    }

    return ; 
    
}


void betree::Node::single_insert_key_left(betree_single *bt, entry_key_t key, char* value, entry& split_entry){
    vector<entry>::iterator it;

    split_entry.key = -1;

    int j = 0;

    for(; j < LEN; j++){
        if(hdr.bitmap[j] == 0){
            records[j].key = key;
            records[j].ptr = value;
            unsigned char fp = hashcode1B(key);
            hdr.bitmap[j] = 1;
            fps[j] = fp;

            clwb_len(&records[j], CACHE_LINE_SIZE);
            clwb_len(&hdr, 64);

            return;
        }
    }

    Node *node = construct_new_node(true);
    node->constructor();

    //erase the repeat keys
    for(int i = 0; i < LEN; i++){
        if(hdr.bitmap[i] == 1)
            for(int kk = 0; kk < LEN; kk++){
                if(i==kk)continue;
                if(hdr.bitmap[kk] == 1){
                    if(records[i].key == records[kk].key){
                        records[kk].key = key;
                        records[kk].ptr = value;
                        unsigned char fp = hashcode1B(key);
                        fps[kk] = fp;

                        clwb_len(&records[kk], CACHE_LINE_SIZE);
                        clwb_len(&hdr, 64);

                        return;
                    }
                }
            }
    }



    //find the split key
    int split_pos = -1;
    entry_key_t split_key;
    for(int i = 0; i < LEN; i++){
        int num = 0;
        for(int k = 0; k < LEN; k++){
            if(records[k].key >= records[i].key){
                num++;
            }

        }

        if(num >= LEN/2 - 1 && num <= LEN/2 + 1){
            split_pos = i;
            split_key = records[i].key;
            break;
        }

    }

    //transfer half key-value pairs to next node
    int sibling_cnt = 0;

    for(int i = 0; i < LEN; i++){
        if(records[i].key >= split_key){
            node->records[sibling_cnt].key = records[i].key;
            node->records[sibling_cnt].ptr = records[i].ptr;
            node->hdr.bitmap[sibling_cnt] = 1;
            node->fps[sibling_cnt] = fps[i];
            hdr.bitmap[i] = 0;

            sibling_cnt++;
            
        }
    }


    node->hdr.next = hdr.next;
    hdr.next = node;
    node->hdr.epsilon = Leaf;
    node->hdr.leftmost = NULL;
    node->hdr.depth = hdr.depth;

    clwb_len(node, sizeof(Node));
    clwb_len(&hdr, 64);


    if(key > split_key){
        node->records[sibling_cnt].key = key;
        node->records[sibling_cnt].ptr = value;
        fps[sibling_cnt] = hashcode1B(key);
        hdr.bitmap[sibling_cnt] = 1;
            
    }else{
        records[split_pos].key = key;
        records[split_pos].ptr = value;
        fps[split_pos] = hashcode1B(key);
        hdr.bitmap[split_pos] = 1;
    }


    if(bt->root == this){
        //this node is the root node
        Node *internal = construct_new_node(false);
        internal->constructor();
        internal->hdr.epsilon = Internal;
        internal->hdr.depth = hdr.depth + 1;
        internal->hdr.leftmost = (char *)this;
        internal->records[0].key = split_key;
        internal->records[0].ptr = (char *)node;
        cout<<"new level:"<<hdr.depth+1<<endl;
        internal->hdr.last_index = 0;
        bt->root = internal;

        // clwb_len(bt->root, sizeof(Node));

        return;

    }else{
        split_entry.key = split_key;
        split_entry.ptr = (char *)node;
        return;
    }


}