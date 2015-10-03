/*
 * dht22.c:
 *	Simple test program to test the wiringPi functions
 *	Based on the existing dht11.c
 *	Amended by technion@lolware.net
 */

#include <wiringPi.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>

#include "locking.h"

#define MAXTIMINGS 82
static int DHTPIN = 5;
static int dht22_dat[5] = { 0, 0, 0, 0, 0 };

static uint8_t sizecvt( const int read ) {
 /* digitalRead() and friends from wiringpi are defined as returning a value
 < 256. However, they are returned as int() types. This is a safety function */

	if ( read > 255 || read < 0 ) {
		fprintf( stderr, "Invalid data from wiringPi library\n" );
		exit( EXIT_FAILURE );
	}
	return ( uint8_t ) read;
}

static int read_dht22_dat() {
	uint8_t laststate = LOW;
	uint8_t counter = 0;
	uint8_t j = 0, i, ref, max = 255;
	float t, h;

	dht22_dat[0] = dht22_dat[1] = dht22_dat[2] = dht22_dat[3] = dht22_dat[4] = 0;
	
	// Prepare to send start signal
	pinMode( DHTPIN, OUTPUT );
	digitalWrite( DHTPIN, HIGH );
	delay( 100 );				// Simulate pull-up resistor
	
	// Send start signal
	digitalWrite( DHTPIN, LOW );
	delay( 10 );				// 1-10 ms
	digitalWrite( DHTPIN, HIGH );
	delayMicroseconds( 20 );	// 20-40 us
	
	// Prepare to read
	pinMode( DHTPIN, INPUT );

	// Detect change and read data
	for ( i = 0; i < MAXTIMINGS; i++ ) {
		counter = 0;
		while ( sizecvt( digitalRead( DHTPIN ) ) == laststate ) {
			delayMicroseconds( 4 );
			if ( ++counter > max ) {
				break;
			}
		}
		laststate = sizecvt( digitalRead( DHTPIN ) );
		if ( counter == 255 ) break;
		
		// Signal HIGH 
		if ( i%2 == 1 ) {
			
			// Data bit
			if ( i > 2 ) {
				dht22_dat[j/8] <<= 1;
				if ( counter >= ref )
					dht22_dat[j/8] |= 1;
				j++;
				
			// Reference signal 80us
			} else if ( i == 2 ) {
				max = counter;
			}
			
		// Signal LOW - 50us reference
		} else {
			ref = counter;
		}
	}
	
	// Calculate checksum, humidity and temperature
	i = ( ( dht22_dat[0] + dht22_dat[1] + dht22_dat[2] + dht22_dat[3] ) & 0xFF );
	h = ( ( int ) dht22_dat[0] * 256 + ( int ) dht22_dat[1] ) / 10.0;
	t = ( ( int )( dht22_dat[2] & 0x7F ) * 256 + ( int ) dht22_dat[3] ) / 10.0;
	if ( (dht22_dat[2] & 0x80 ) != 0 ) t *= -1;
	
	// Data OK! 40 bits + checksum
	if ( (j >= 40 ) && ( dht22_dat[4] == i ) ) {
		printf( "Temperature: %.1f*C \tHumidity: %.1f%%\n", t, h );
		return 1;
		
	// Error
	} else {
		fprintf( stderr, "Invalid data: %d bits [%02x %02x %02x %02x %02x] Chk=%02x T:%.1f H:%.1f\n", 
			j, dht22_dat[0], dht22_dat[1], dht22_dat[2], dht22_dat[3], dht22_dat[4], i, t, h );
		return 0;
	}
}

int main ( int argc, char *argv[] ) {
	int lockfd;
	uint8_t rErr = 0;

	if ( argc != 2 )
		printf ( "%s <pin>\n    pin : wiringPi pin No (default %d)\n",argv[0], DHTPIN );
	else
		DHTPIN = atoi( argv[1] );
	
	lockfd = open_lockfile( LOCKFILE );

	if ( wiringPiSetup () == -1 )
		exit( EXIT_FAILURE ) ;
		
	if ( setuid( getuid() ) < 0 ) {
		perror( "Dropping privileges failed\n" );
		exit( EXIT_FAILURE );
	}

	while ( read_dht22_dat() == 0 && rErr++ < 10 ) {
		delay( 1000 ); // wait 1sec to refresh
	}

	delay( 1500 );	// No retry within 1 sec
	close_lockfile( lockfd );

	return 0 ;
}
