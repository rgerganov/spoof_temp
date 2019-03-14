CFLAGS = -Wall
LDFLAGS = $(shell pkg-config --libs libhackrf)

all: spoof_temp

spoof_temp: main.cpp
	$(CXX) main.cpp -o spoof_temp -std=c++11 $(CFLAGS) $(LDFLAGS)
