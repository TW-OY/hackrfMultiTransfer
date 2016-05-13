#include "fmMod.cpp"
#include <iostream>
#include <libhackrf/hackrf.h>

using namespace std;

int hackrf_tx_callback(int8_t *buffer, uint32_t length) {

    m_mutex.lock();

    if (my::count == 0) {
        memset(buffer, 0, length);
    }
    else {
        memcpy(buffer, my::_buf[my::tail], length);
        my::tail = (my::tail + 1) % BUF_NUM;
        my::count--;
    }
    m_mutex.unlock();

    return 0;
}

int main() {
    uint32_t hackrf_sample = 2000000;
    uint32_t ret = hackrf_init();
    cout<<ret<<endl;
//    hackrf_device * _dev = NULL;
//    double freq = 433.00 * 1000000;
//    uint32_t gain = 90 / 100.0;
//    uint32_t tx_vga = 40;
//    uint32_t enableamp = 1;
//    int ret = hackrf_open(&_dev);
//    if (ret != HACKRF_SUCCESS) {
//        hackrf_close(_dev);
//        cout<<"hackrf open error"<<endl;
//    }
//    else {
//        hackrf_set_sample_rate(_dev, hackrf_sample);
//        hackrf_set_baseband_filter_bandwidth(_dev, 1750000);
//        hackrf_set_freq(_dev, freq);
//        hackrf_set_txvga_gain(_dev, tx_vga);
//        hackrf_set_amp_enable(_dev, enableamp);
//        ret = hackrf_start_tx(_dev, hackrf_tx_callback(my::_buf[0], BUF_LEN), (void *)this);
//        if (ret != HACKRF_SUCCESS) {
//            hackrf_close(_dev);
//            cout<<"hackrf transfer error"<<endl;
//        }
//    }
    return 0;
}
