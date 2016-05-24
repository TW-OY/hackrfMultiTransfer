#include "rxSrc/hackrfTool.cpp"


int main(int argc,char* argv[]) {

    hackrfInit();
    captureData();

    doSomeThing(hackrf_rx_buf);


    while(true){
        sleep(1);

    }
}

