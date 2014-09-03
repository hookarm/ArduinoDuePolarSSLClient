#include <Ethernet.h>
#include <SPI.h>

#include "polarssl/ssl.h"
#include "polarssl/ctr_drbg.h"

const char *opt_psk = "c033f52671c61c8128f7f8a40be88038bcf2b07a6eb3095c36e3759f0cf40837";
const char *opt_psk_identity = "Client_identity";

unsigned char psk[256];
size_t psk_len = 0;

ssl_context ssl;

EthernetClient client;
byte mac[] = { 0x90, 0xA2, 0xDA, 0x0F, 0x24, 0xA8 };
IPAddress server(10, 166, 11, 65);

// from https://github.com/mpflaga/Arduino-MemoryFree/blob/master/MemoryFree.cpp
extern "C" char* sbrk(int incr);
int freeRam ()
{
	char top;
	return &top - reinterpret_cast<char*>(sbrk(0));
}

// from polarssl/programs/ssl/ssl_client2.c
void psk_unhexify() {
    unsigned char c;
    size_t j;

    if( strlen( opt_psk ) % 2 != 0 )
    {
        Serial.println("pre-shared key not valid hex\n");
        return;
    }

    psk_len = strlen( opt_psk ) / 2;

    for( j = 0; j < strlen( opt_psk ); j += 2 )
    {
        c = opt_psk[j];
        if( c >= '0' && c <= '9' )
            c -= '0';
        else if( c >= 'a' && c <= 'f' )
            c -= 'a' - 10;
        else if( c >= 'A' && c <= 'F' )
            c -= 'A' - 10;
        else
        {
            Serial.println("pre-shared key not valid hex\n");
            return;
        }
        psk[ j / 2 ] = c << 4;

        c = opt_psk[j + 1];
        if( c >= '0' && c <= '9' )
            c -= '0';
        else if( c >= 'a' && c <= 'f' )
            c -= 'a' - 10;
        else if( c >= 'A' && c <= 'F' )
            c -= 'A' - 10;
        else
        {
            Serial.println("pre-shared key not valid hex\n");
            return;
        }
        psk[ j / 2 ] |= c;
    }
}

/*char *hex_convert(int n, char *s)
{
	const char *hh = "0123456789ABCDEF";
	s[0] = hh[(n&0xF0)>>4];
	s[1] = hh[n&0x0F];
	s[2] = 0;
}
*/

static void my_debug(void *ctx, int level, const char *str)
{
	Serial.println(str);
}

int ar_recv(void *ctx, unsigned char *buf, size_t len)
{
	Serial.print("recv ");
	Serial.println(len);
	
	int recv = 0;
	while (true) {
		if (!client.available()) {
			Serial.println("nothing available");
			delay(100);
			continue;
		}
		char c = client.read();
		//Serial.println((int)c);
		buf[recv++] = c;
		if (recv >= len) break;
	}

	Serial.print("received ");
	Serial.println(recv);

	return recv;
}

int ar_send(void *ctx, const unsigned char *buf, size_t len)
{
	Serial.print("send ");
	Serial.println(len);
	
	/*for (int i=0; i<len; i++) {
		Serial.print(buf[i]);
		Serial.print(" ");
	}
	Serial.println();*/

	int sent = client.write(buf, len);
	client.flush();

	Serial.print("sent ");
	Serial.println(sent);

	return sent;
}

int ar_random(void *p_rng, unsigned char *output, size_t output_len)
{
	Serial.println("random");
	// This is probably really really bad
	return 0;
}

void setup()
{
	Serial.begin(9600);
	Serial.println("setup");

	Serial.print("free ram = ");
	Serial.println(freeRam());

	// Ethernet

	Serial.println("ethernet");

	Ethernet.begin(mac);

	delay(1000);

	Serial.println("connection");

	if (!client.connect(server, 4433)) {
		Serial.println("connection failed");
		return;
	}

	// PolarSSL initialization

	Serial.println("unhexify");

	psk_unhexify();

	Serial.println("ssl_init");

	ssl_init(&ssl);

	Serial.println("ssl_set_rng");

	ssl_set_rng(&ssl, ar_random, 0);

	Serial.println("ssl_set_dbg");

	ssl_set_dbg(&ssl, my_debug, 0);

	Serial.println("ssl_set_bio");

	ssl_set_bio(&ssl, ar_recv, 0, ar_send, 0);

	Serial.println("ssl_set_psk");

	ssl_set_psk(&ssl, psk, psk_len, (const unsigned char *)opt_psk_identity, strlen(opt_psk_identity));

	Serial.println("ssl_handshake");

	int hs_res = ssl_handshake(&ssl);

	Serial.print("ssl_handshake = ");
	Serial.println(hs_res);

	// Send something

	char buf[512];
	strcpy(buf, "hello world");

	ssl_write(&ssl, (const unsigned char*)buf, strlen(buf));

	// Read a response

	int read_bytes = ssl_read(&ssl, (unsigned char*)buf, 512);
	buf[read_bytes] = '\0';
	Serial.println(buf);

	// Let's see how much RAM we have left

	Serial.print("free ram = ");
	Serial.println(freeRam());
}

void loop()
{
	Serial.println("loop");
	delay(1000);
}
