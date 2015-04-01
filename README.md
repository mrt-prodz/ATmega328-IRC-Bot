#ATmega328 IRC Bot (Arduino Nano)
 
####IRC Bot made for the ATmega328 and PCB ENC28J60 ethernet module

This is an IRC bot for the ATmega328 and a PCB ENC28J60 ethernet module using [EtherCard](https://github.com/jcw/ethercard) library.

Thanks to [EtherCard](https://github.com/jcw/ethercard) the sketch uses only around 37% of storage and around 50% of dynamic memory on the ATmega328 when disabling debugging output. Storage usage is around ~11,500 bytes which means it leaves enough room to add and program for more components and let your control them remotely over IRC. Memory usage is around ~1,000 bytes.

In order to make this work properly you will have to add a couple lines of code in tcpip.cpp and EtherCard.h, the reason being I couldn't find another way to properly send multiple TCP requests while having persistTcpConnection set to true.

tcpip.cpp:
```c
void EtherCard::tcpSendPersist (uint16_t dlen) {
    make_tcp_ack_from_any(get_tcp_data_len(),0); // send ack for tcp request
    gPB[TCP_FLAGS_P] = TCP_FLAGS_ACK_V|TCP_FLAGS_PUSH_V;
    make_tcp_ack_with_data_noflags(dlen);
}
```

EtherCard.h:
```c
/**   @brief  Send a TCP request with persistentTcpConnection enabled
*     @param  dlen Size of the TCP payload
*/
static void tcpSendPersist (uint16_t dlen);
```

All you have to do is change the configuration of the bot in config.h and add your own functions if you want to interact with your Arduino remotely.

SSL is not supported.

Developed and tested with an Arduino Nano and a PCB ENC28J60 ethernet module.

##Features:
* connect over IRC
* control your ATmega328 remotely by sending commands
* DHCP or static IP method for your module
* auto-join channels after connecting to server
* auto-rename bot if nick is already in use
* simple authorization (only authorized user will be able to send commands if enabled)
* display uptime and memory usage
* run raw IRC commands remotely
* lightweight, leaves enough resources to code and interact with more components

This is a project for the ATmega328 and ENC28J60.

##TODO:
* Reconnect to server if connection is lost

##Screenshot:
![IRC screenshot](https://raw.githubusercontent.com/mrt-prodz/ATmega328-IRC-Bot/master/screenshot.jpg)

##References:
[EtherCard](https://github.com/jcw/ethercard)

[AVR-libc](http://www.nongnu.org/avr-libc/user-manual/modules.html)

[RFC2812](https://tools.ietf.org/html/rfc2812)
