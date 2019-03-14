#include <string>
#include <cstring>
#include <vector>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <getopt.h>
#include <signal.h>
#include <unistd.h>
#include <libhackrf/hackrf.h>

using namespace std;

const int SAMPLE_RATE = 2000000;
const int SYMBOL_RATE = 2000;

static hackrf_device *device = NULL;
static volatile bool do_exit = false;

uint8_t nibbles[9];
vector<int> data;
vector<uint8_t> out_cu8;
vector<int8_t> out_cs8;

void usage(const char *cmd)
{
    fprintf(stderr, "Usage: %s [-o file] [-i ID] [-c channel] [-t temp] [-h humidity] [-f freq] [-x gain]\n", cmd);
    exit(1);
}

void sigint_callback_handler(int signum)
{
    fprintf(stderr, "Caught signal %d\n", signum);
    do_exit = true;
}

static int tx_callback(hackrf_transfer *transfer) {
    static int ind = 0;
    memcpy(transfer->buffer, out_cs8.data() + ind, transfer->valid_length);
    ind = (ind + transfer->valid_length) % out_cs8.size();
    return 0;
}

void add_sync(vector<int> &v)
{
    v.push_back(1);
    v.push_back(0);
    v.push_back(0);
    v.push_back(0);
    v.push_back(0);
    v.push_back(0);
    v.push_back(0);
    v.push_back(0);
    v.push_back(0);
}

void add_zero(vector<int> &v)
{
    v.push_back(1);
    v.push_back(0);
    v.push_back(0);
}

void add_one(vector<int> &v)
{
    v.push_back(1);
    v.push_back(0);
    v.push_back(0);
    v.push_back(0);
    v.push_back(0);
}

void generate_data()
{
    for (int k = 0 ; k < 12 ; k++) {
        add_sync(data);
        for (int i = 0 ; i < 9 ; i++) {
            uint8_t mask = 0x08;
            for (int j = 0 ; j < 4 ; j++) {
                if (nibbles[i] & mask) {
                    add_one(data);
                } else {
                    add_zero(data);
                }
                mask >>= 1;
            }
        }
    }
}

void generate_samples()
{
    int spb = SAMPLE_RATE / SYMBOL_RATE; // samples per bit
    for (int i = 0 ; i < (int) data.size() ; i++) {
        for (int j = 0 ; j < spb ; j++) {
            out_cu8.push_back(data[i] ? 255 : 127);
            out_cu8.push_back(127);
            out_cs8.push_back(data[i] ? 127 : 0);
            out_cs8.push_back(0);
        }
    }
}

static void start_tx(int freq, int tx_gain)
{
    int result = hackrf_init();
    if (result != HACKRF_SUCCESS) {
        fprintf(stderr, "hackrf_init() failed: (%d)\n", result);
        exit(1);
    }
    result = hackrf_open(&device);
    if (result != HACKRF_SUCCESS) {
        fprintf(stderr, "hackrf_open() failed: (%d)\n", result);
        exit(1);
    }
    result = hackrf_set_sample_rate_manual(device, SAMPLE_RATE, 1);
    if (result != HACKRF_SUCCESS) {
        fprintf(stderr, "hackrf_sample_rate_set() failed: (%d)\n", result);
        exit(1);
    }
    uint32_t baseband_filter_bw_hz = hackrf_compute_baseband_filter_bw_round_down_lt(SAMPLE_RATE);
    result = hackrf_set_baseband_filter_bandwidth(device, baseband_filter_bw_hz);
    if (result != HACKRF_SUCCESS) {
        fprintf(stderr, "hackrf_baseband_filter_bandwidth_set() failed: (%d)\n", result);
        exit(1);
    }
    result = hackrf_set_freq(device, freq);
    if (result != HACKRF_SUCCESS) {
        fprintf(stderr, "hackrf_set_freq() failed: (%d)\n", result);
        exit(1);
    }
    // result = hackrf_set_amp_enable(device, 1);
    // if (result != HACKRF_SUCCESS) {
    //     fprintf(stderr, "hackrf_set_amp_enable() failed: (%d)\n", result);
    // }
    result = hackrf_set_txvga_gain(device, tx_gain);
    if (result != HACKRF_SUCCESS) {
        fprintf(stderr, "hackrf_set_txvga_gain() failed: (%d)\n", result);
        exit(1);
    }
    result = hackrf_start_tx(device, tx_callback, NULL);
    if (result != HACKRF_SUCCESS) {
        fprintf(stderr, "hackrf_start_tx() failed: (%d)\n", result);
        exit(1);
    }
}

static void stop_tx()
{
    int result = hackrf_stop_tx(device);
    if (result != HACKRF_SUCCESS) {
        fprintf(stderr, "hackrf_stop_tx() failed: (%d)\n", result);
    }
    result = hackrf_close(device);
    if (result != HACKRF_SUCCESS) {
        fprintf(stderr, "hackrf_close() failed: (%d)\n", result);
    }
}

template<typename T>
void save_to_file(const string &fname, vector<T> &out)
{
    printf("Saving to %s\n", fname.c_str());
    FILE *f = fopen(fname.c_str(), "wb");
    fwrite(out.data(), 1, out.size(), f);
    fclose(f);
}

int main(int argc, char *argv[])
{
    int opt;
    float temp_f = 26.3;
    uint8_t id = 244;
    int8_t humidity = 20, channel = 1;
    int freq = 433968400;
    int tx_gain = 0;
    string fname;

    while ((opt = getopt(argc, argv, "i:c:t:h:o:f:x:")) != -1) {
        switch (opt) {
            case 'i':
                id = atoi(optarg);
                break;
            case 'c':
                channel = atoi(optarg);
                if (channel < 1 || channel > 3) {
                    fprintf(stderr, "Invalid argument: channel must be in [1, 3]\n");
                    exit(1);
                }
                break;
            case 't':
                temp_f = atof(optarg);
                if (temp_f < -204.7 || temp_f > 204.7) {
                    fprintf(stderr, "Invalid argument: temperature must be in [-204.7, 204.7]\n");
                    exit(1);
                }
                break;
            case 'h':
                humidity = atoi(optarg);
                if (humidity < 0 || humidity > 100) {
                    fprintf(stderr, "Invalid argument: humidity must be in [0, 100]\n");
                    exit(1);
                }
                break;
            case 'o':
                fname = optarg;
                break;
            case 'f':
                freq = atoi(optarg);
                break;
            case 'x':
                tx_gain = atoi(optarg);
                break;
            default:
                usage(argv[0]);
                break;
        }
    }

    printf("ID: %d, channel: %d, temperature: %.2f, humidity: %d\n", id, channel, temp_f, humidity);

    int16_t temp = (int16_t)(temp_f * 10);

    nibbles[0] = (id >> 4) & 0x0f;
    nibbles[1] = id & 0x0f;
    nibbles[2] = 7 + channel;
    nibbles[3] = (temp >> 8) & 0x0f;
    nibbles[4] = (temp >> 4) & 0x0f;
    nibbles[5] = temp & 0x0f;
    nibbles[6] = 0x0f;
    nibbles[7] = (humidity >> 4) & 0x0f;
    nibbles[8] = humidity & 0x0f;

    generate_data();
    generate_samples();

    if (!fname.empty()) {
        save_to_file(fname + ".cu8", out_cu8);
        save_to_file(fname + ".cs8", out_cs8);
        return 0;
    }

    signal(SIGINT, &sigint_callback_handler);
    signal(SIGILL, &sigint_callback_handler);
    signal(SIGFPE, &sigint_callback_handler);
    signal(SIGSEGV, &sigint_callback_handler);
    signal(SIGTERM, &sigint_callback_handler);
    signal(SIGABRT, &sigint_callback_handler);
    start_tx(freq, tx_gain);
    printf("Transmitting: freq=%dHz, tx_gain=%d\n", freq, tx_gain);
    printf("Press Ctrl+C to stop\n");
    while ((hackrf_is_streaming(device) == HACKRF_TRUE) && !do_exit) {
        sleep(1);
    }
    stop_tx();
    return 0;
}
