// =============================================================================
//
// Arduino - IRC Bot
// -----------------
// by Themistokle "mrt-prodz" Benetatos
//
// This is an IRC bot for the ATMEGA328 and a PCB ENC28J60 ethernet module.
//
// Developed and tested with an Arduino Nano and a PCB ENC28J60 ethernet module.
//
// Notes: - I couldn't find a way to send multiple TCP requests with persistTcpConnection
//          so I ended making this function in tcpip.cpp:
//
//          void EtherCard::tcpSendPersist (uint16_t dlen) {
//              make_tcp_ack_from_any(get_tcp_data_len(),0); // send ack for tcp request
//              gPB[TCP_FLAGS_P] = TCP_FLAGS_ACK_V|TCP_FLAGS_PUSH_V;
//              make_tcp_ack_with_data_noflags(dlen);
//          }
//
//          and declared it in the EtherCard class (EtherCard.h):
//
//          /**   @brief  Send a TCP request with persistentTcpConnection enabled
//          *     @param  dlen Size of the TCP payload
//          */
//          static void tcpSendPersist (uint16_t dlen);

// Dependencies:
// - https://github.com/jcw/ethercard
//
// References:
// - https://github.com/jcw/ethercard
// - http://www.nongnu.org/avr-libc/user-manual/modules.html
// - https://tools.ietf.org/html/rfc2812
//
// --------------------
// http://mrt-prodz.com
// http://github.com/mrt-prodz/ATmega328-IRC-Bot
//
// =============================================================================
#define DEBUG                             // if you don't need debugging output comment this
                                          // to save storage space and memory usage
// ----------------------------------------------
// includes
// ----------------------------------------------
#include <EtherCard.h>
#include "config.h"                       // this header contains the bot configuration
#include "replycodes.h"                   // this header contains IRC server reply code

// ----------------------------------------------
// global variables
// ----------------------------------------------
uint8_t Ethernet::buffer[512];            // buffer size
static BufferFiller bfill;
static const uint32_t start_time = millis();

static bool auth = false;                 // flag to specify if someone authed
static char auth_user[BOT_AUTH_LEN];      // array to store authed user (limited to 32 chars including null byte)
static uint8_t nick_retry;                // used for autorename when server sends a ERR_NICKNAMEINUSE

// ----------------------------------------------
// macros
// ----------------------------------------------
// tcp send for persistent connection
#define send(...) bfill=ether.tcpOffset(); bfill.emit_p(__VA_ARGS__); ether.tcpSendPersist(bfill.position());

// ----------------------------------------------
// convert string to number (short)
// ----------------------------------------------
static uint16_t strtoshort(const char* str) {
    // convert to number
    uint8_t count = 3;
    uint16_t code = 0;
    while (count--) {
      code += (*str++ - 0x30);
      code *= 10;
    }
    code /= 10;
    return code;
}

// ----------------------------------------------
// display available memory
// http://playground.arduino.cc/code/AvailableMemory
// ----------------------------------------------
static uint16_t free_ram () {
  extern uint16_t __heap_start, *__brkval; 
  uint16_t v; 
  return (uint16_t) &v - (__brkval == 0 ? (uint16_t) &__heap_start : (uint16_t) __brkval); 
}

// ----------------------------------------------
// run command request from user
// ----------------------------------------------
static bool run_command(char* whois, char* chan, char* cmd, char* param = 0) {
  // if USE_AUTH is defined only allow authed user to run commands
  #ifdef USE_AUTH
    // auth user to let him run commands
    if (!memcmp("auth", cmd, 4) && param) {
      // check for pass (really simple auth method)
      if (!strncmp(param, BOT_PASSWORD, strlen(BOT_PASSWORD))) {
        // set auth flag
        auth = true;
        // set auth_user with whois info (user!realname@ip)
        memset(auth_user, 0, BOT_AUTH_LEN);
        strncpy(auth_user, whois, BOT_AUTH_LEN);
        // make sure to terminate with a null byte
        auth_user[BOT_AUTH_LEN-1] = '\0';
        send(PSTR("PRIVMSG $S :granted access to $S\r\n"), chan, whois);
      }
      return true;
    }
    // if the user sending a command is not authed return
    if (!auth || strncmp(auth_user, whois, sizeof(auth_user))) return false;
  #endif

  // commands without parameter
  // show available commands
  if (!memcmp("help", cmd, 4)) {
    send(PSTR("PRIVMSG $S :custom commands: auth, free, raw, uptime\r\n"), chan);
    return true;
  }
  // show available memory
  if (!memcmp("free", cmd, 4)) {
    send(PSTR("PRIVMSG $S :free ram: $D bytes\r\n"), chan, free_ram());
    return true;
  }
  // show uptime since module started
  if (!memcmp("uptime", cmd, 6)) {
    send(PSTR("PRIVMSG $S :uptime: $L seconds\r\n"), chan, (millis()-start_time)/1000);
    return true;
  }
  // commands with parameter(s)
  // return if parameter is not set
  if (!param) return false;
  // this will execute any command/parameter sent by a user (not recommended without using auth)
  // worst case scenario would be to implement a white list of commands and only execute these
  // usage example: !raw JOIN #chan1,#chan2,#chan3
  // this will make the bot joins #chan1 #chan2 #chan3, you can run any raw IRC command like such
  if (!memcmp("raw", cmd, 3)) {
    send(PSTR("$S\r\n"), param);
    return true;
  }
  // example of how to implement a command with a parameter
  if (!memcmp("slap", cmd, 4)) {
    send(PSTR("PRIVMSG $S :$S\r\n"), chan, param);
    return true;
  }

  return false;
}

// ----------------------------------------------
// parse code from server reply
// ----------------------------------------------
static bool parse_server_code(const uint16_t code) {
  switch(code) {
    case ERR_NICKNAMEINUSE:
      nick_retry++;
      send(PSTR("NICK "CLIENT_NICK"_$D\r\n"), nick_retry);
      return true;

    case ERR_NOMOTD:
    case RPL_ENDOFMOTD:
      send(PSTR("JOIN "CLIENT_CHAN"\r\n"));
      return true;
      
    default:
      break;
  }
  return false;
}

// ----------------------------------------------
// parse reply from server
// ----------------------------------------------
static bool parse_reply(char* reply) {
  switch(reply[0]) {
    // if reply starts with P it's probably a PING request
    case 'P':
      // check if reply is indeed a PING request from server
      if (!memcmp("PING", reply, 4)) {
        send(PSTR("PONG :$S\r\n"), reply+6);
        return true;
      }
      break;

    case ':':
      // split by space
      // [0] user!realname@ip [1] code|command [2] channel|user [3] :!bot_command [4] bot_command_parameter
      char* token = strtok(reply+1, " "),
          * whois,
          * separator,
          * chan,
          * cmd,
          * param_ptr;
      char user[18] = {0},
           param[64] = {0};
      uint8_t part = 0,
              param_len = 0;
      while (token != NULL) {
        switch(part) {
          // [0] user!realname@ip
          case 0:
            // store user info
            if (strchr(token, '!'))
              whois = token;
            break;
          // [1] code|command
          case 1:
            // if it's a server code parse it
            if (token[0] >= '0' && token[0] <= '9') {
                return parse_server_code(strtoshort(token));
            }
            break;

          // [2] channel|user
          case 2:
            // if token starts with # reply in a channel
            if (token[0] == '#') {
              chan = token;
              break;
            }
            // if token is the bot name it's a private message
            else if (!memcmp(token, CLIENT_NICK, strlen(CLIENT_NICK)) && user) {
              // get user nick to reply to
              if ((separator = strchr(reply+1, '!'))) {
                memset(user, 0, sizeof(user));
                strncpy(user, reply+1, (separator-(reply+1)));
                if (user) chan = user;
              }
            }
            break;

          // [3] :!bot_command
          case 3:
            // if it's not parsed properly abort
            if (token[0] != ':' || token[1] != BOT_CMDPREFIX) return false;
            else token += 2;
            // save command for later
            cmd = token;
            break;

          // [4] bot_command_parameter
          case 4:
            // check that cmd and chan are set before replying
            if (cmd == NULL || chan == NULL) return false;
            // save parameter
            memset(param, 0, sizeof(param));
            param_ptr = param;
            while(true) {
              param_len = strlen(token);
              if (param_ptr - param > sizeof(param)) return false;
              strncpy(param_ptr, token, param_len);
              param_ptr += param_len;
              *param_ptr = ' ';
              param_ptr++;
              token = strtok(NULL, " ");
              if (token == NULL) break;
            }
            break;

        }
        token = strtok(NULL, " ");
        part++;
      }
      // run command if cmd and chan are set properly
      if (chan && cmd) {
        // let commands.h deal with this
        return run_command(whois, chan, cmd, param);
      }
      break;
  }
  return true;
}

// ----------------------------------------------
// callback for server replies
// ----------------------------------------------
static uint8_t result_cb(uint8_t fd, uint8_t statuscode, uint16_t datapos, uint16_t len) {
  // make a clean copy in case the buffer changes during callback
  char reply[len+1];
  memset(reply, 0, len+1);
  strncpy(reply, (const char*)Ethernet::buffer+datapos, len);
  Ethernet::buffer[0] = '\0';
  // if not empty parse it into tokens and reply accordingly
  if (reply) {
    #ifdef DEBUG
      Serial.println(reply);
    #endif
    // split server reply into tokens
    char* curr_pos = reply,
        * next_pos;
    uint16_t token_len;
    while ((next_pos = strchr(curr_pos, '\n')) != NULL) {
      // parse token and reply accordingly
      token_len = (next_pos - curr_pos);
      char token[token_len+1];
      memset(token, 0, token_len+1);
      strncpy(token, curr_pos, token_len);
      parse_reply(token);
      // keep looking for tokens
      curr_pos = next_pos+1;
    }
  }
  
  return 0;
}

// ----------------------------------------------
// callback for server request data
// ----------------------------------------------
static uint16_t datafill_cb(uint8_t fd) {
  // called once by clientTcpReq
  bfill = ether.tcpOffset();
  bfill.emit_p(PSTR("USER "CLIENT_USERNAME" "CLIENT_MODE" * :"CLIENT_REALNAME"\r\nNICK "CLIENT_NICK"\r\n"));
  return bfill.position();
}

// ----------------------------------------------
// connect to server (initiate client tcp request)
// ----------------------------------------------
static void connect() {
  // flag for debugging purpose during setup
  bool check = true;
  // unique mac address for the ethernet module
  uint8_t mac[] = { MODULE_MAC };
  // initialise the network interface on port 10 instead of 8 (default)
  check = ether.begin(sizeof Ethernet::buffer, mac, 10);
  #ifdef DEBUG
    if (!check) Serial.println(F("Failed to access Ethernet controller"));
  #endif
  // small delay to wait for configuration (otherwise static IP doesn't work in some cases)
  delay(1000);
  // set persistent tcp connection
  ether.persistTcpConnection(true);
    
  // set static IP or use DHCP
  #ifdef USE_DHCP
    // configure network interface with DHCP
    check = ether.dhcpSetup();
    #ifdef DEBUG
      if (!check) Serial.println(F("DHCP failed"));
    #endif
  #else
    // configure network interface with static IP
    uint8_t ip[]  = { MODULE_IP };
    uint8_t gw[]  = { MODULE_GW };
    uint8_t dns[] = { MODULE_DNS };
    uint8_t msk[] = { MODULE_MSK };
    ether.staticSetup(ip, gw, dns, msk);
  #endif

  // set IRC server IP or URL
  #ifdef USE_SERVER_IP
    uint8_t serverip[] = { SERVER_IP };
    ether.copyIp(ether.hisip, serverip);
    while (ether.clientWaitingGw())
      ether.packetLoop(ether.packetReceive());
  #else
    if (!ether.dnsLookup(PSTR(SERVER_URL)))
      Serial.println(F("DNS failed"));
  #endif
  // specify port (no ssl available)
  ether.hisport = SERVER_PORT;

  #ifdef DEBUG
    // debug connection
    ether.printIp("IP:   ", ether.myip);
    ether.printIp("MASK: ", ether.netmask);
    ether.printIp("GW:   ", ether.gwip);
    ether.printIp("DNS:  ", ether.dnsip);
    ether.printIp("SERV: ", ether.hisip);
  #endif
  
  // reset global variables
  if (!auth) memset(auth_user, 0, BOT_AUTH_LEN);
  nick_retry = 0;
  // datafill_cb will send the first packet
  // result_cb will deal with parsing replies from the server
  ether.clientTcpReq(&result_cb, &datafill_cb, ether.hisport);
}

// ----------------------------------------------
// setup connection
// ----------------------------------------------
void setup () {
  // initialize serial output if DEBUG is defined
  #ifdef DEBUG
    Serial.begin(57600);
  #endif

  // connect to server
  connect();  
}

// ----------------------------------------------
// main loop
// ----------------------------------------------
void loop () {
  // receive and parse packets
  ether.packetLoop(ether.packetReceive());
}
