PREFIX=/usr/

all: 
	gcc -o loldht \
		-lwiringPi \
		dht22.c locking.c

clean:
	- rm -f loldht core

.PHONY: clean
