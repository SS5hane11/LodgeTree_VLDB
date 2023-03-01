
#include "src/btree.h"
#include <pthread.h>
#include <sstream>
#include <vector>


static inline int file_existss(char const *file) { return access(file, F_OK); }



int main(int argc, char **argv) {


    struct timespec time1, time2;
    std::vector<uint64_t> keyss;
    std::vector<uint64_t> opers;
    std::vector<uint64_t> types;
    // int64_t *keyss = new int64_t[1000000005];
    // int64_t *opers = new int64_t[200000005];
    // int8_t *types = new int8_t[200000005];

    long long numdata = 64;
    long long operdata = 64;
    long long ops_s = 0;

    keyss.resize(numdata, 0);
    types.resize(operdata, 0);
    opers.resize(operdata, 0);

    // char *input_path = "./out_bulkload.txt";
    // char *operation_path = "/home/astl/wsz/new_fast/txn_ycsbkey_workloada_norepeat";
    // char *operation_path = "/home/astl/wsz/new_fast/txn_ycsbkey_workloada_latest";
    char *operation_path = "/home/wsz/microbench/index-microbench-master/workloads/txnsa_zipf_int_100M.dat";
    // char *operation_path = "/home/astl/wsz/new_fast/workloads/txn_ycsbkey_workloadb_zipf_0.8hot_0.5read_1kw";
    // char *operation_path = "/home/astl/wsz/new_fast/workloadb_0.95r/load_ycsbkey_workloadb";
    char *input_path = "/home/wsz/microbench/index-microbench-master/workloads/loada_zipf_int_100M.dat";
    // char *input_path = "/home/astl/wsz/new_fast/load_ycsbkey_workloada_latest";
    // char *input_path = "/home/astl/wsz/new_fast/workloadb_0.95r/load_ycsbkey_workloadb";
    // char *operation_path = "../lbtree_workload.txt";
    int n_threads = 32;
    char *persistent_path = "/mnt/pmem0/cltree.pool";

    int workload_insert_num = 0;
    long long sizee = 160000;

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
            case 'p':
                persistent_path = optarg;
                break;
            case 's':
                sizee = atoi(optarg);
            case 'e':
                node_t = atoi(optarg);
            default:
                break;
        }
    }

    clear_cache();

    cout<<"excute threads: "<<n_threads<<endl;
    cout<<"warm up data num: "<<numdata<<endl;
    cout<<"operation data num: "<<operdata<<endl;
    cout<<"warmup data path: "<<input_path<<endl;
    cout<<"operation data path: "<<operation_path<<endl;

    the_thread_nvmpools.init(n_threads, "/mnt/pmem0/cltree.pool", sizee*MB);

    std::cout<<"nvm init completed\n";

    ifstream ifs, operation_ifs;
    ifs.open(input_path);
    operation_ifs.open(operation_path);
    if (!ifs) {
        cout << "input loading error!" << endl;
        
        exit(-1);
    }

    for (int i = 0; i < numdata; ++i) {
        string temp;
        ifs >> temp;
        ifs >> keyss[i];
    }   

    if (!operation_ifs) {
        cout << "input loading error!" << endl;
        exit(-1);
    }

    cout << "[info]LeafNode Size:\t" << sizeof(LeafNode) <<endl;
    cout << "[info]LeafHeader Size:\t" << sizeof(Header) <<endl;
    cout << "[info]LeafNode SLOTS:\t" << SLOTS << endl;
    cout << "[info]Internal Size:\t" << sizeof(InterNode) <<endl;
    cout << "[info]Internal Header:\t" << sizeof(InterHeader) <<endl;
    cout << "[info]Internal SLOTS:\t" << ISLOTS << endl;

    // cout << "[info]Internal Size:\t" << sizeof(betree::Node) <<endl;
    
    for (int i = 0; i < operdata; ++i) {
        string temp;
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

    move_times = 0;

    cout<<"load workload completed\n";

    int fail_num = 0;
    worker_id = 0;
    btree *nbt = new btree;
    thread workers[n_threads];
    long data_per_thread = (numdata) / (n_threads);

    auto operate_func = [&opers, &nbt, &types](int start, int end, int id){
        worker_id = id;
        // struct timespec t1, t2;

        for(int i = start; i < end; i++){
            // clock_gettime(CLOCK_MONOTONIC, &t1);
            if(types[i] == 0){
                nbt->bitree_buffer_insert(opers[i], id);
            }else if(types[i] == 1){
                nbt->bitree_buffer_insert(opers[i], id);
            }else{
                nbt->bitree_buffer_search(opers[i]);
            }
            // clock_gettime(CLOCK_MONOTONIC, &t2);
            // long long time = (t2.tv_sec - t1.tv_sec)*1000000000 + (t2.tv_nsec - t1.tv_nsec);
            // tail_insert<<time<<endl;
        }

    };

    auto insert_func_load = [&keyss, &nbt, &types](int start, int end, int id){
        worker_id = id;
        for(int i = start; i < end; i++){
            if((i+1)%10000000 == 0){
                printf("%d kvs completed\n", (i+1));
            }
            nbt->bitree_buffer_insert(keyss[i], id);
        }

    };

    auto insert_func = [&opers, &nbt, &types](int start, int end, int id){
        worker_id = id;
        for(int i = start; i < end; i++){

            if((i+1)%10000000 == 0){
                printf("%d kvs completed\n", (i+1));
            }

            nbt->bitree_buffer_insert(opers[i], id);
                // nbt->bitree_insert(opers[i], id);
        }

    };



    auto search_func = [&opers, &nbt, &n_threads, &fail_num](int start, int end,int id){
        worker_id = id;
        for(int i = start; i < end; i++){
            // cout<<i<<endl;
            if((i+1)%10000000 == 0){
                printf("%d kvs completed\n", (i+1));
            }

            if(opers[i] != nbt->bitree_buffer_search(opers[i])){
                // cout<<opers[i]<<endl;
                fail_num++;
            }
        }

    };    

    //start
    cout<<"start warm up!\n";

    int halfnum = numdata/2;

    struct timespec t1, t2;

    clock_gettime(CLOCK_MONOTONIC, &t1); 

    for(int i = 0; i < n_threads; i++){
        int from  = data_per_thread * i;
        int to = (i == n_threads - 1) ? numdata : from + data_per_thread;
        // workers[i] = thread(insert_func, from, to, i);
        workers[i] = thread(insert_func_load, from, to, i);
    }
    
    
    for (int i=0; i<n_threads; i++) workers[i].join();
    
    
    // for(int i = 0; i < numdata; i++){
    //     if((i+1) % 10000000 == 0){
    //         cout<<i+1<<" kvs are completed\n";
    //     }

    //     nbt->bitree_buffer_insert(keyss[i], 0);
    // }
    clock_gettime(CLOCK_MONOTONIC, &t2);

    the_thread_nvmpools.print_usage();


    // int pre_fail_num = 0;
    // for(int i = 0; i < numdata; i++){

    //     if(nbt->bitree_buffer_search(keyss[i]) != keyss[i]){
    //         // cout<<keyss[i]<<endl;

    //         pre_fail_num++;
    //     }
    // }

    // cout<<"pre fail num:"<<pre_fail_num<<endl;

    long long t =  (t2.tv_sec - t1.tv_sec) * 1000000000 + (t2.tv_nsec - t1.tv_nsec);

    cout<<"warm up using time: "<<t*1.0/1000000000<<endl;
    cout<<"warm up throughput: "<<numdata/(t*1.0/1000000000)<<endl;    

    

    cout<<"start test\n";
    
    data_per_thread = (operdata) / (n_threads);

    
    

    stringstream ss;
    ss<<node_t;

    string path = "res"+ss.str()+".txt";
    cout<<"go path:"<<path<<endl;
    ofstream tail_insert("lodge_tail_insert", ios::app);
    ofstream tail_search("lodge_tail_search.txt", ios::app);

    cout<<"node_t"<<node_t<<endl;

    

    ofstream oof("./result", ios::app);

    long long time;

    auto record_thread_fun = [&nbt, &ops_s](int second){
        ofstream ofs("exec_th.txt", ios::app);
            
        for(int i = 0; i < second; i++){
            ofs<<ops_s<<endl;
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
        // workers[i] = thread(insert_func, from, to, i);
        workers[i] = thread(operate_func, from, to, i);
    }
    
    
    for (int i=0; i<n_threads; i++) workers[i].join();
    clock_gettime(CLOCK_MONOTONIC, &time2);
    // record.join();


    int total_num = operdata;

    time =  (time2.tv_sec - time1.tv_sec) * 1000000000 + (time2.tv_nsec - time1.tv_nsec);
    cout << "use time:" << (time * 1.0)/1000000000 << endl;
    double uset = time * 1.0 / 1000000000;
    // tail_of<<"throughput:" << total_num*1.0/uset << "ops/s"<<endl;
    cout << "throughput:" << total_num*1.0/uset << "ops/s"<<endl;
    cout << "last level nodes:" << last_level_nodes <<endl;

//-------------------------------search------------------------------

    clock_gettime(CLOCK_MONOTONIC, &time1);

    for(int i = 0; i < n_threads; i++){
        int from  = data_per_thread * i;
        int to = (i == n_threads - 1) ? operdata : from + data_per_thread;
        workers[i] = thread(search_func, from, to, i);
    }
    // workers[0] = thread(search_func, n_threads-1);
    
    for (int i=0; i<n_threads; i++) workers[i].join();


    clock_gettime(CLOCK_MONOTONIC, &time2);

    time =  (time2.tv_sec - time1.tv_sec) * 1000000000 + (time2.tv_nsec - time1.tv_nsec);

    cout << "use time:" << (time * 1.0)/1000000000 << endl;

    uset = time*1.0 / 1000000000;

    cout << "throughput:" << total_num*1.0/uset << "ops/s"<<endl;

    // cout<<"search start\n";
    cout<<"fail num: "<<fail_num<<endl;

    clear_cache();
    
    
    return 0;
}
