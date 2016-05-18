#include "src/fmMod.h"
#include <iostream>
#include <unistd.h>

using namespace std;

int main(int argc,char* argv[]) {


  //  Read_Wave(argv[1]);

    char * path="/Users/MakeitBetter/input.wav";

    Read_Wave(path);
    makeCache();
    cout<<"cache finish"<<endl;

   // on_chunk();
    hackrfWork();

    while(true){
        sleep(1);

    }
    std::cout<<"finish"<<std::endl;
    return 0;
}
