#include<iostream>
#include<fstream>
#include<string>

int main() {

    std::string input_path = "/home/wsz/microbench/index-microbench-master/workloads/loada_zipf_int_100M.dat"; 
    std::string output_path = "/home/wsz/microbench/index-microbench-master/workloads/lbtree_loada_64M.dat";

    std::ifstream ifs;
    std::ofstream ofs;
    ifs.open(input_path.data());
    if (!ifs) {
        std::cout<<"wrong path\n";
    }

    ofs.open(output_path.data());

    int key_num = 64000000;
    std::string ops;
    uint64_t p;
    for (int i = 0; i < key_num; i++) {
        ifs>>ops;
        ifs>>p;
        ofs<<p<<std::endl;
    }

    ifs.close();
    ofs.close();
}