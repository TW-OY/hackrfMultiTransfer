#include <libs/libhackrf.h>

int8 hackrf_rx_buf[CAPLENGTH*2];
int hackrf_rx_count;

void hackrfInit() {

hackrf_init();

hackrf_open(&device);

hackrf_set_sample_rate(device, sampleRate, 1);

hackrf_set_baseband_filter_bandwidth(device, bandWidth);

hackrf_set_vga_gain(device, vga_gain);

hackrf_set_lna_gain(device, lna_gain);

hackrf_set_freq(device, freq);
}


void captureData() {
    hackrf_stop_rx(hackrf_dev); //中止接收，因为可能之前处于接收使能状态。

    hackrf_rx_count = 0;
    // 接收数据长度清零，这是一个全局变量，
    // 回调函数会用此变量来告诉前台程序现在接收到多少数据了，
    // 前台程序应该在每次接收前把此变量清零，这样回调函数下一次被驱动调用时，
    // 才会认为用户buffer是空的，从而把驱动收到的数据搬进用户buffer（其实是个全局数组）

    hackrf_start_rx(hackrf_dev, capbuf_hackrf_callback, NULL);
    // 启动接收，并且告诉驱动回调函数为capbuf_hackrf_callback

    while(hackrf_is_streaming(hackrf_dev) == HACKRF_TRUE) {
        //不断检测HackRF设备是否处于正常streaming状态
        if( hackrf_rx_count == (CAPLENGTH*2) )
            //如果接收数据长度达到了我们预订长度CAPLENGTH即退出，
            //*2是因为每个样点有I和Q两个数据
            break;
    }
    hackrf_is_streaming(hackrf_dev); //再次调用此函数

}

static int capbuf_hackrf_callback(hackrf_transfer* transfer) {
    size_t bytes_to_write; //本次调用向全局数组hackrf_rx_buf写入的长度
    size_t hackrf_rx_count_new = hackrf_rx_count + transfer->valid_length;
    //初步计算本次接收后数据总长度。

    int count_left = (CAPLENGTH*2) - hackrf_rx_count_new;
    //计算期望接收总长度与本次接收后总长度的差值。

    if ( count_left <= 0 ) { //如果本次接收后，总长度超出期望接收总长度
        bytes_to_write = transfer->valid_length + count_left;
        //则要减去超出的长度，否则待会儿hackrf_rx_buf装不下这么多数据会越界。
    } else { //如果不超
        bytes_to_write = transfer->valid_length;
        //则本次收到的数据可以全写入hackrf_rx_buf
    }

    if (bytes_to_write!=0)
    //如果需要把数据写入hackrf_rx_buf，即hackrf_rx_buf还没满
    {
        memcpy( hackrf_rx_buf+hackrf_rx_count, transfer->buffer, bytes_to_write );
        //把数据从驱动的buffer中搬到用户全局数组hackrf_rx_buf中（附加在之前的数据之后）

        hackrf_rx_count = hackrf_rx_count + bytes_to_write;
        //更新当前接收数据总长度。
        //这个回调函数具有“自锁”能力，
        //即接收数据总长度达到期望的CAPLENGTH*2后，
        //即使被驱动调用，也不会再往用户全局数组hackrf_rx_buf中写了，因为已经满了。
    }

    return(0);
}
