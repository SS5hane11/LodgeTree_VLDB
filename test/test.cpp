
#include "src/btree.h"
#include "src/betree_single.h"
#include <pthread.h>
#include <sstream>


static inline int file_existss(char const *file) { return access(file, F_OK); }



int main(int argc, char **argv) {

    struct timespec time1, time2;
    int64_t *keyss = new int64_t[1000000005];
    int64_t *opers = new int64_t[200000005];
    int8_t *types = new int8_t[200000005];

    long long numdata = 16000000;
    long long operdata = 16000000;
    long long ops_s = 0;

    // char *input_path = "./out_bulkload.txt";
    // char *operation_path = "/home/astl/wsz/new_fast/txn_ycsbkey_workloada_norepeat";
    // char *operation_path = "/home/astl/wsz/new_fast/txn_ycsbkey_workloada_latest";
    // char *operation_path = "/home/astl/wsz/new_fast/0111/txn_ycsbkey_workloada";
    // // char *operation_path = "/home/astl/wsz/new_fast/workloads/txn_ycsbkey_workloadb_zipf_0.8hot_0.5read_1kw";
    // // char *operation_path = "/home/astl/wsz/new_fast/workloadb_0.95r/load_ycsbkey_workloadb";
    // char *input_path = "/home/astl/wsz/new_fast/0111/load_ycsbkey_workloada";
    // char *input_path = "/home/astl/wsz/new_fast/load_ycsbkey_workloada_latest";
    // char *input_path = "/home/astl/wsz/new_fast/workloadb_0.95r/load_ycsbkey_workloadb";
    // char *operation_path = "../lbtree_workload.txt";
    char *operation_path = "/home/wsz/microbench/index-microbench-master/workloads/msst/insert_all_no_repeat_txn";
    char *input_path = "/home/wsz/microbench/index-microbench-master/workloads/msst/insert_all_no_repeat_load";
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
        delete[] keyss;
        exit(-1);
    }


    for (int i = 0; i < numdata; ++i) {
        string temp;
        ifs >> temp;
        ifs >> keyss[i];
    }   

    if (!operation_ifs) {
        cout << "input loading error!" << endl;
        delete[] opers;
        exit(-1);
    }

    cout << "[info]LeafNode Size:\t" << sizeof(LeafNode) <<endl;
    cout << "[info]LeafHeader Size:\t" << sizeof(Header) <<endl;
    cout << "[info]LeafNode SLOTS:\t" << SLOTS << endl;
    cout << "[info]Internal Size:\t" << sizeof(InterNode) <<endl;
    cout << "[info]Internal Header:\t" << sizeof(InterHeader) <<endl;
    cout << "[info]Internal SLOTS:\t" << ISLOTS << endl;

    cout << "[info]Internal Size:\t" << sizeof(betree::Node) <<endl;
    
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

    /*
    -------------------------------------------------------------------------

        new nvm mgr

    -------------------------------------------------------------------------
    */


    
    /**
     *  for betree
     * 
     * */
    
    cout<<"first test betree\n";

    worker_id = 0;

    auto *bet = new betree::betree_single();

    struct timespec time111, time222;

    // trace_key = opers[25000000];

    // cout<<"trace this key: "<<opers[45000000]<<endl;

    auto insert_func_load = [&bet, &keyss, &types](int from, int to, int id){
      thread_local int worker_id = id;
      for(int i = from; i < to; i++){
        bet->btree_insert_key(keyss[i], (char *)keyss[i]);
      }
    };
    thread workers[n_threads];
    clock_gettime(CLOCK_MONOTONIC, &time111);

    long data_per_thread = (numdata) / (n_threads);
    for(int i = 0; i < n_threads; i++){
        int from  = data_per_thread * i;
        int to = (i == n_threads - 1) ? numdata : from + data_per_thread;
        
        workers[i] = thread(insert_func_load, from, to, i);
    }
    
    for (int i=0; i<n_threads; i++) workers[i].join();
    // for(int i = 0; i < numdata; i++){

    //     // cout<<i<<endl;        

    //     // point insert
    //     // bet->btree_insert_single_key(keyss[i], (char *)keyss[i]);

    //     // last level buffer insert
    //     bet->betree_insert_key(keyss[i], (char *)keyss[i]);

    //     // betree insert
    //     // bet->betree_insert_key(keyss[i], (char *)keyss[i]);
    // }
    // clock_gettime(CLOCK_MONOTONIC, &time222);
    long long ttdw1 = (time222.tv_sec - time111.tv_sec)*1000000000 + (time222.tv_nsec - time111.tv_nsec);

    cout<<"betree warmup time: "<<ttdw1*1.0/1000000000<<endl;

    flush_cnt = 0;

    ofstream ofs("./tail_betree_i.txt", ios::app);
    ofstream ofs_search("./tail_betree_s.txt", ios::app);

    
    auto bet_insert_func = [&opers, &bet, &types, &ofs, &ops_s](int start, int end, int id){
        worker_id = id;
        struct timespec time111, time222;
        for(int i = start; i < end; i++){
            
            ops_s++;
            if((i+1)%10000000 == 0){
                printf("%d kvs completed\n", (i+1));
            }
            if(types[i] == 2)
                bet->betree_search_key(opers[i]);
            else 
                bet->betree_insert_key(opers[i], (char*)opers[i]);
                // nbt->bitree_insert(opers[i], id)
        }

    };

    auto bet_search_func = [&opers, &bet, &types, &ofs_search](int start, int end, int id){
        worker_id = id;
        struct timespec time111, time222;

        
        for(int i = start; i < end; i++){
            if((i+1)%10000000 == 0){
                printf("%d kvs completed\n", (i+1));
            }
            
            bet->betree_search_key(opers[i]);
           
                // nbt->bitree_insert(opers[i], id);
        }

    };


    // auto record_thread_fun = [&ops_s](int second){
    //     ofstream ofs("ops_s.txt", ios::app);
    //     for(int i = 0; i < second; i++){
    //         ofs<<ops_s<<endl;
    //         ops_s = 0;
    //         sleep(1);
    //     }

    // };
    // auto record = std::thread(record_thread_fun, 70);

    // ofstream ofs_tail("tail_last.txt", ios::app);
    sort_time = 0;
    compare_num = 0;

    //multi threads
    data_per_thread = operdata/n_threads;

    auto record_thread_fun = [&ops_s](int second){
        ofstream ofs("exec_th.txt", ios::app);
            
        for(int i = 0; i < second; i++){
            ofs<<ops_s<<endl;
            ops_s = 0;
            sleep(1);
        }


    };
    
    // auto record = std::thread(record_thread_fun, 40);
    clock_gettime(CLOCK_MONOTONIC, &time111);

    for(int i = 0; i < n_threads; i++){
        int from  = data_per_thread * i;
        int to = (i == n_threads - 1) ? operdata : from + data_per_thread;
        workers[i] = thread(bet_insert_func, from, to, i);
    }
    
    for (int i=0; i<n_threads; i++) workers[i].join();
    // record.join();

    clock_gettime(CLOCK_MONOTONIC, &time222);


    // single thread

    // clock_gettime(CLOCK_MONOTONIC, &time111);
    // for(int i = 0; i < operdata; i++){
        
    //     if(i == 50376)
    //         cout<<"debug\n";
    //     // cout<<i<<endl;
    //     if(i%10000000 == 0)
    //         cout<<i<<"kvs are completed\n";
    //     // point insert
    //     // bet->btree_insert_single_key(keyss[i], (char *)keyss[i]);

    //     // last level insert
    //     // bet->btree_insert_key(opers[i], (char *)opers[i]);

    //     // betree insert
    //     bet->betree_insert_key(opers[i], (char *)opers[i]);

    //     ops_s++;
        
    //     // ofs_tail<<((time222.tv_sec - time111.tv_sec)*1000000000 + (time222.tv_nsec - time111.tv_nsec))<<endl;
    // }

    // clock_gettime(CLOCK_MONOTONIC, &time222);
    // // record.join();

    long long ttdw = (time222.tv_sec - time111.tv_sec)*1000000000 + (time222.tv_nsec - time111.tv_nsec);

    cout<<"betree insert elapsed time: "<<ttdw*1.0/1000000000<<endl;

    // for(int i = 1; i < 30; i++)
    //     cout<<"buffer fail num "<<i<<": "<<buffer_fail_num[i]<<endl;

    cout<<"test sort time:" <<sort_time*1.0/1000000000<<endl;
    cout<<"test compare num:" <<compare_num<<endl;

    cout<<"betree_node_num:"<<betree_node_num<<"\n"<<endl;
    cout<<"internal_node_num:"<<internal_be_num<<"\n"<<endl;


    clock_gettime(CLOCK_MONOTONIC, &time111);

    // for(int i = 0; i < n_threads; i++){
    //     int from  = data_per_thread * i;
    //     int to = (i == n_threads - 1) ? operdata : from + data_per_thread;
    //     workers[i] = thread(bet_search_func, from, to, i);
    // }
    
    // for (int i=0; i<n_threads; i++) workers[i].join();
    // int fail_num_b = 0;
    ofstream ootail("tail_bet_new.txt", ios::app);

    for(int i = 0; i < operdata; i++){
        clock_gettime(CLOCK_MONOTONIC, &time111);
        if(i == 240){
            cout<<"debug\n";
        }
        betree::entry_key_t value = bet->betree_search_key(opers[i]);
        clock_gettime(CLOCK_MONOTONIC, &time222);
        ttdw = (time222.tv_sec - time111.tv_sec)*1000000000 + (time222.tv_nsec - time111.tv_nsec);
        ootail<<ttdw<<endl;
    }

    clock_gettime(CLOCK_MONOTONIC, &time222);

    ttdw = (time222.tv_sec - time111.tv_sec)*1000000000 + (time222.tv_nsec - time111.tv_nsec);

    cout<<"betree elapsed time: "<<ttdw*1.0/1000000000<<endl;
    double uset = ttdw*1.0/1000000000;

    cout<<"throughput: "<<operdata*1.0/uset<<endl;

    // cout<<"search fail num: "<<fail_num_b<<endl;

    cout<<"flush count: "<<flush_cnt<<endl;
    // cout<<"node count: "<<internal_be_num<<endl;

    bet->find_all_keys();

    // range query
    // clock_gettime(CLOCK_MONOTONIC, &time111);
    // int total = 0;
    // for(int i = 0; i < 2; i++){
    //     cout<<i<<endl;
        
    //     if(opers[i] < opers[i+1])
    //         total+=bet->betree_range(opers[i], opers[i+1]);
    //     else
    //         total+=bet->betree_range(opers[i+1], opers[i]);
    // }   
    // clock_gettime(CLOCK_MONOTONIC, &time222);
    // ttdw = (time222.tv_sec - time111.tv_sec)*1000000000 + (time222.tv_nsec - time111.tv_nsec);
    // cout<<"range elapsed time: "<<ttdw*1.0/1000000000<<endl;
    // cout<<"total num: "<<total<<endl;
    
    
    return 0;
}
