
#include "src/lodge.h"
#include <pthread.h>
#include <sstream>
#include <vector>
// using namespace std;

static inline int file_existss(char const *file) { return access(file, F_OK); }



int main(int argc, char **argv) {

    struct timespec time1, time2;
    std::vector<int64_t> keyss;
    std::vector<int64_t> opers;
    std::vector<int64_t> types;

    long long numdata = 64000000;
    long long operdata = 64000000;
    long long ops_s = 0;

    keyss.resize(numdata, 0);
    types.resize(operdata, 0);
    opers.resize(operdata, 0);

    // char *input_path = "./out_bulkload.txt";
    // char *operation_path = "/home/astl/wsz/new_fast/txn_ycsbkey_workloada_norepeat";
    // char *operation_path = "/home/astl/wsz/new_fast/txn_ycsbkey_workloada_latest";
    // char *operation_path = "/home/wsz/microbench/index-microbench-master/workloads/txnsa_zipf_int_100M.dat";
    // char *operation_path = "/home/astl/wsz/new_fast/workloads/txn_ycsbkey_workloadb_zipf_0.8hot_0.5read_1kw";
    // char *operation_path = "/home/astl/wsz/new_fast/workloadb_0.95r/load_ycsbkey_workloadb";
    // char *input_path = "/home/wsz/microbench/index-microbench-master/workloads/loada_zipf_int_100M.dat";
    // char *input_path = "/home/astl/wsz/new_fast/load_ycsbkey_workloada_latest";
    // char *input_path = "/home/astl/wsz/new_fast/workloadb_0.95r/load_ycsbkey_workloadb";
    // char *operation_path = "../lbtree_workload.txt";
    char *operation_path = "/home/wsz/microbench/index-microbench-master/workloads/msst/insert_all_no_repeat_txn";
    char *input_path = "/home/wsz/microbench/index-microbench-master/workloads/msst/insert_all_no_repeat_load";
    int n_threads = 16;

    int workload_insert_num = 0;
    long long sizee = 320000;

    int c;
    while ((c = getopt(argc, argv, "n:w:t:i:p:o:q:s:e:")) != -1) {
        switch (c) {
            case 'n':
                numdata = atoi(optarg);
                break;
            case 'o':
                operdata = atoi(optarg);
                break;
            case 't':
                n_threads = atoi(optarg);
                break;
            case 'i':
                input_path = optarg;
                break;
            case 'q':
                operation_path = optarg;
                break;
            case 's':
                sizee = atoi(optarg);
                break;
            case 'e':
                node_t = atoi(optarg);
                break;
            default:
                break;
        }
    }

    clear_cache();

    std::cout<<"[INFO] excute threads: "<<n_threads<<std::endl;
    std::cout<<"[INFO] warm up data num: "<<numdata<<std::endl;
    std::cout<<"[INFO] operation data num: "<<operdata<<std::endl;
    std::cout<<"[INFO] warmup data path: "<<input_path<<std::endl;
    std::cout<<"[INFO] operation data path: "<<operation_path<<std::endl;


    std::cout << "[INFO] LeafNode Size:\t" << sizeof(lodge::LeafNode) <<std::endl;
    std::cout << "[INFO] LeafHeader Size:\t" << sizeof(lodge::Header) <<std::endl;
    std::cout << "[INFO] LeafNode SLOTS:\t" << SLOTS << std::endl;
    std::cout << "[INFO] Internal Size:\t" << sizeof(lodge::InterNode) <<std::endl;
    std::cout << "[INFO] Internal Header:\t" << sizeof(lodge::InterHeader) <<std::endl;
    std::cout << "[INFO] Internal SLOTS:\t" << ISLOTS << std::endl;

    std::ifstream ifs, operation_ifs;
    ifs.open(input_path);
    operation_ifs.open(operation_path);
    if (!ifs) {
        std::cout << "[ERROR] input loading error!" << std::endl;
        exit(-1);
    }

    for (int i = 0; i < numdata; ++i) {
        std::string temp;
        ifs >> temp;
        ifs >> keyss[i];
    }   

    if (!operation_ifs) {
        std::cout << "[ERROR] input loading error!" << std::endl;
        exit(-1);
    }
    
    for (int i = 0; i < operdata; ++i) {
        std::string temp;
        operation_ifs >> temp;
        if(!temp.compare("INSERT")){
            types[i] = 0;
            workload_insert_num++;
        }else if(!temp.compare("UPDATE")){
            types[i] = 1;
            workload_insert_num++;
        }else if(!temp.compare("READ")){
            types[i] = 2;
        }
        operation_ifs >> opers[i];

    }


    ifs.close();
    operation_ifs.close();

    std::cout<<"[INFO] load workload completed\n";

    the_thread_nvmpools.init(n_threads, "/mnt/pmem0/cltree.pool", sizee*MB);

    std::cout<<"[INFO] nvm init completed\n";

    int fail_num = 0;


    worker_id = 0;
    lodge::btree *nbt = new lodge::btree;
    std::thread workers[n_threads];
    long data_per_thread = (numdata) / (n_threads);

    auto operate_func = [&opers, &nbt, &types](int start, int end, int id){
        worker_id = id;
        // struct timespec t1, t2;

        for(int i = start; i < end; i++){
            // clock_gettime(CLOCK_MONOTONIC, &t1);
            if(types[i] == 0){
                nbt->lodge_buffer_insert(opers[i], id);
            }else if(types[i] == 1){
                // nbt->bitree_buffer_update(opers[i], id);
                nbt->lodge_buffer_insert(opers[i], id);
            }else{
                nbt->bitree_buffer_search(opers[i]);
            }
        }

    };

    auto insert_func_load = [&keyss, &nbt, &types](int start, int end, int id){
        worker_id = id;
        for(int i = start; i < end; i++){
            if((i+1)%10000000 == 0){
                printf("[INFO] %d operations are completed\n", (i+1));
            }
            nbt->lodge_buffer_insert(keyss[i], id);
        }

    };

    auto insert_func = [&opers, &nbt, &types](int start, int end, int id){
        worker_id = id;
        for(int i = start; i < end; i++){

            if((i+1)%10000000 == 0){
                printf("[INFO] %d operations are completed\n", (i+1));
            }
            nbt->lodge_buffer_insert(opers[i], id);
        }

    };

    auto search_func = [&opers, &nbt, &n_threads, &fail_num](int start, int end,int id){
        worker_id = id;
        for(int i = start; i < end; i++){
            // cout<<i<<endl;
            if((i+1)%10000000 == 0){
                printf("[INFO] %d operations are completed\n", (i+1));
            }

            if(opers[i] != nbt->bitree_buffer_search(opers[i])){
                // std::cout<<opers[i]<<std::endl;
                fail_num++;
                // nbt->search_fail_kv(opers[i]);
                // nbt->bitree_buffer_search(opers[i]);
            }
        }

    };    

    //start
    std::cout<<"[INFO] start warm up!\n";

    struct timespec t1, t2;

    int warm_thread = 1;

    clock_gettime(CLOCK_MONOTONIC, &t1); 

    for(int i = 0; i < warm_thread; i++){
        int from  = data_per_thread * i;
        int to = (i == warm_thread - 1) ? numdata : from + data_per_thread;
        workers[i] = std::thread(insert_func_load, from, to, i);
    }
    
    
    for (int i=0; i<warm_thread; i++) workers[i].join();
    
    clock_gettime(CLOCK_MONOTONIC, &t2);

    // the_thread_nvmpools.print_usage();


    long long t =  (t2.tv_sec - t1.tv_sec) * 1000000000 + (t2.tv_nsec - t1.tv_nsec);

    std::cout<<"[RESULT] warm up using time: "<<t*1.0/1000000000<<std::endl;
    std::cout<<"[RESULT] warm up throughput: "<<numdata/(t*1.0/1000000000)<<std::endl;    

    

    std::cout<<"[INFO] start test\n";
    
    data_per_thread = (operdata) / (n_threads);

    
    

    std::stringstream ss;
    ss<<node_t;
    std::string path = "res"+ss.str()+".txt";
    // std::cout<<"go path:"<<path<<std::endl;
    std::ofstream tail_insert("lodge_tail_insert", std::ios::app);
    std::ofstream tail_search("lodge_tail_search.txt", std::ios::app);

    // std::cout<<"node_t"<<node_t<<std::endl;

    

    std::ofstream oof("./result", std::ios::app);

    long long time;

    auto record_thread_fun = [&nbt, &ops_s](int second){
        std::ofstream ofs("exec_th.txt", std::ios::app);
            
        for(int i = 0; i < second; i++){
            ofs<<ops_s<<std::endl;
            ops_s = 0;
            sleep(1);
        }
    };
    clock_gettime(CLOCK_MONOTONIC, &time1);
    // auto record = std::thread(record_thread_fun, 20);

    // for(int i = 0; i < operdata; i++){
    //     nbt->bitree_buffer_insert(opers[i], i%8);
    //     ops_s++;
    // }
    last_level_nodes=0;
    for(int i = 0; i < n_threads; i++){
        int from  = data_per_thread * i;
        int to = (i == n_threads - 1) ? operdata : from + data_per_thread;
        workers[i] = std::thread(operate_func, from, to, i);
    }
    
    
    for (int i=0; i<n_threads; i++) workers[i].join();
    clock_gettime(CLOCK_MONOTONIC, &time2);
    // record.join();


    int total_num = operdata;

    time =  (time2.tv_sec - time1.tv_sec) * 1000000000 + (time2.tv_nsec - time1.tv_nsec);
    std::cout << "[RESULT] use time:" << (time * 1.0)/1000000000 << std::endl;
    double uset = time * 1.0 / 1000000000;
    // tail_of<<"throughput:" << total_num*1.0/uset << "ops/s"<<endl;
    std::cout << "[RESULT] throughput:" << total_num*1.0/uset << "ops/s"<<std::endl;
    std::cout << "[RESULT] last level nodes:" << last_level_nodes <<std::endl;

//-------------------------------search------------------------------
    std::cout << "[INFO] start searching\n";

    clock_gettime(CLOCK_MONOTONIC, &time1);
    int search_threads = n_threads;
    data_per_thread = operdata / search_threads;
    for(int i = 0; i < search_threads; i++){
        int from  = data_per_thread * i;
        int to = (i == n_threads - 1) ? operdata : from + data_per_thread;
        workers[i] = std::thread(search_func, from, to, i);
    }
    
    for (int i=0; i<search_threads; i++) workers[i].join();


    clock_gettime(CLOCK_MONOTONIC, &time2);

    time =  (time2.tv_sec - time1.tv_sec) * 1000000000 + (time2.tv_nsec - time1.tv_nsec);

    std::cout << "[RESULT] use time:" << (time * 1.0)/1000000000 << std::endl;

    uset = time*1.0 / 1000000000;

    std::cout << "[RESULT] throughput:" << total_num*1.0/uset << "ops/s"<<std::endl;

    std::cout<<"[RESULT] search fail num: "<<fail_num<<std::endl;

    clear_cache();
    
    
    return 0;
}
