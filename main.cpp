// freq = 433968400 Hz
#include <string>
#include <vector>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <getopt.h>

using namespace std;

void usage(const char *cmd)
{
    fprintf(stderr, "Usage: %s [-o file] [-i ID] [-c channel] [-t temp] [-h humidity]\n", cmd);
    exit(1);
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

template<typename T>
void save_to_file(const string &fname, vector<T> &out)
{
    printf("Saving to %s\n", fname.c_str());
    FILE *f = fopen(fname.c_str(), "wb");
    fwrite(out.data(), 1, out.size(), f);
    fclose(f);
}

const int sample_rate = 2000000;
const int symbol_rate = 2000;

int main(int argc, char *argv[])
{
    int opt;
    float temp_f = 26.3;
    uint8_t id = 244;
    int8_t humidity = 20, channel = 1;
    string fname = "data";

    while ((opt = getopt(argc, argv, "i:c:t:h:o:")) != -1) {
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
            default:
                usage(argv[0]);
                break;
        }
    }

    printf("ID: %d, channel: %d, temperature: %.2f, humidity: %d\n", id, channel, temp_f, humidity);

    int16_t temp = (int16_t)(temp_f * 10);
    vector<int> data;
    vector<uint8_t> out_cu8;
    vector<int8_t> out_cs8;
    uint8_t nibbles[9];

    nibbles[0] = (id >> 4) & 0x0f;
    nibbles[1] = id & 0x0f;
    nibbles[2] = 7 + channel;
    nibbles[3] = (temp >> 8) & 0x0f;
    nibbles[4] = (temp >> 4) & 0x0f;
    nibbles[5] = temp & 0x0f;
    nibbles[6] = 0x0f;
    nibbles[7] = (humidity >> 4) & 0x0f;
    nibbles[8] = humidity & 0x0f;

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

    int spb = sample_rate / symbol_rate; // samples per bit
    for (int i = 0 ; i < data.size() ; i++) {
        for (int j = 0 ; j < spb ; j++) {
            out_cu8.push_back(data[i] ? 255 : 127);
            out_cu8.push_back(127);
            out_cs8.push_back(data[i] ? 127 : 0);
            out_cs8.push_back(0);
        }
    }

    save_to_file(fname + ".cu8", out_cu8);
    save_to_file(fname + ".cs8", out_cs8);
    return 0;
}
