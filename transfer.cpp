#include "src/fmMod.h"
#include <iostream>
#include <unistd.h>

int main(int argc,char* argv[]) {


  //  Read_Wave(argv[1]);

    char * path="/Users/MakeitBetter/input.wav";

    Read_Wave(path);

   // on_chunk();
    makeCache();
    hackrfWork();

    while(true){
        sleep(1);

    }
    std::cout<<"finish"<<std::endl;
    return 0;
}
