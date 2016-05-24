#include "txSrc/fmMod.h"
#include <iostream>
#include <unistd.h>

using namespace std;

int main(int argc,char* argv[]) {


  //  getPcmData(argv[1]);

    char * path="/Users/MakeitBetter/input.wav";
//    char * path="input.wav";

    getPcmData(path);
    makeCache();
    cout<<"cache finish"<<endl;

    hackrfWork();

    while(true){
        sleep(1);

    }
}
