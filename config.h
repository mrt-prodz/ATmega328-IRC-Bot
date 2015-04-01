// =============================================================================
//
// Arduino - IRC Bot
// -----------------
// by Themistokle "mrt-prodz" Benetatos
//
// Configuration file for the bot settings.
//
// References:
// - https://github.com/jcw/ethercard
//
// --------------------
// http://mrt-prodz.com
// http://github.com/mrt-prodz/ATmega328-IRC-Bot
//
// =============================================================================

#ifndef CONFIG_H
#define CONFIG_H

  // ---------------
  // ethernet module settings
  // ---------------
  // set a unique mac address
  #define USE_DHCP                          // comment this to use static IP instead of DHCP
  #define MODULE_IP    10, 0, 0, 100        // static IP for the module (ignore if USE_DHCP is uncommented)
  #define MODULE_GW    10, 0, 0, 1          // gateway                  (ignore if USE_DHCP is uncommented)
  #define MODULE_DNS   8, 8, 8, 8           // dns                      (ignore if USE_DHCP is uncommented)
  #define MODULE_MSK   255, 255, 255, 0     // mask                     (ignore if USE_DHCP is uncommented)
  #define MODULE_MAC   0xBA,0xDC,0x0F,0xFE,0xEE,0xEE
  
  // ---------------
  // server settings
  // ---------------
  #define USE_SERVER_IP                     // comment this to use URL instead of server IP
  #define SERVER_IP    10, 0, 0, 30         // IRC server IP
  #define SERVER_URL   "card.freenode.net"  // IRC server URL (ignore if USE_SERVER_IP is uncommented)
  #define SERVER_PORT  6667                 // IRC server port (no SSL support)
  
  // ---------------
  // client settings
  // ---------------
  #define CLIENT_USERNAME     "nanobot"     // [username]@irc_server
  #define CLIENT_MODE         "8"           // 0 : default - 8 : invisible
  #define CLIENT_REALNAME     "Nano Bot"    // real name
  #define CLIENT_NICK         "nanobot"     // nick
  #define CLIENT_CHAN         "#nanochan"   // channel (or list of channels separated by a commma)

  // ---------------
  // bot settings
  // ---------------
//#define USE_AUTH                          // uncomment this and only people with the password will be able to auth and run commands
  #define BOT_PASSWORD        "letmein"     // password for authorization to run commands (ignore if USE_AUTH is commented)
  #define BOT_AUTH_LEN        32            // maximum number of characters to store for authed user (including null byte)
  #define BOT_CMDPREFIX       '!'           // bot command prefix

#endif

