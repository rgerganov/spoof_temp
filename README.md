Command line program for spoofing the outdoor sensor for this temperature station (sold by [LIDL](https://www.google.com/search?q=lidl+temperature+station)):

![spoof](/temp_station.png)

The original sensor transmits on 433MHz and uses OOK to encode data.
This program can encode data in the same format and transmit it with HackRF.
You can see it in action here:

[![alt](https://img.youtube.com/vi/uqBe81vcZOM/0.jpg)](https://www.youtube.com/watch?v=uqBe81vcZOM)

Building and running
---

On Ubuntu 18.04:
```
sudo apt install libhackrf-dev
make
./spoof_temp -t 12.3 -h 40
```

On OSX:
```
brew install hackrf
make
./spoof_temp -t 12.3 -h 40
```

Testing with rtl_433
---
```bash
$ spoof_temp -t 12.3 -h 40 -o test
ID: 244, channel: 1, temperature: 12.30, humidity: 40
Saving to test.cu8
Saving to test.cs8

$ rtl_433 -s 2000000 -r test.cu8
rtl_433 version 18.12-134-g64139f3 branch master at 201903110821 inputs file rtl_tcp
[...]
Test mode active. Reading samples from file: test.cu8
_ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _
time      : @0.069540s
model     : Nexus Temperature/Humidity             House Code: 244
Channel   : 1            Battery   : OK            Temperature: 12.30 C      Humidity  : 40 %
```
