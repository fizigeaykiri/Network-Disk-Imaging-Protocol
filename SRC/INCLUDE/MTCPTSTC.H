/**
 * mTcpTstC.H
 * By @marssaturn91
 */

// Global options, can be overridden locally.  Use for setting tracing
// and torture testing/consistency testing.

#ifndef CONFIG_H
#define CONFIG_H

#include "Global.Cfg"
 
#define COMPILE_UDP
#define COMPILE_TCP
#define COMPILE_DNS
#define COMPILE_ICMP

// Tracing support
//
// If we didn't set it globally then make sure it is turned off locally.

#if !defined(NOTRACE)

#undef NOTRACE
// #define NOTRACE

#endif


#define PKT_DUMP_BYTES (256)

// Use only for torture testing

#undef CONSISTENCY_CHK
//#define CONSISTENCY_CHK


// Packet Layer defines
//
#define PACKET_BUFFERS       (5)   // Number of incoming buffers: max is 42!
#define PACKET_BUFFER_LEN (1514)   // Size of each incoming buffer



// ARP configuration defines
//
#define ARP_MAX_ENTRIES   (5)   // Size of ARP cache
#define ARP_MAX_PENDING   (1)   // Max number of pending requests
#define ARP_RETRIES       (3)   // Total number of attempts to make

#define ARP_TIMEOUT   (500ul) // Time between retries



// IP Defines
#define IP_FRAGMENTS_ON
#define IP_MAX_FRAG_PACKETS              (4)
#define IP_MAX_FRAGS_PER_PACKET         (10)
#define IP_BIGPACKET_SIZE             (1500)
#define IP_FRAG_REASSEMBLY_TIMEOUT    (4000ul)
//#define IP_SEND_UDP_FRAGS


// ICMP Defines
#ifdef COMPILE_ICMP
#define ICMP_ECHO_OPT_DATA (256)
#endif


// TCP configuration defines
//
#ifdef COMPILE_TCP

#define TCP_MAX_SOCKETS          (2)   // 8 bits only
#define TCP_MAX_XMIT_BUFS        (5)   // 8 bits only, Go no higher than 40
#define TCP_SOCKET_RING_SIZE     (4)   // Must be power of 2
//#define TCP_RETRANS_COUNT        (5)

#define TCP_CLOSE_TIMEOUT    (10000ul)
#define TCP_PA_TIMEOUT       (10000ul)   // Pending accept timeout
#define TCP_RETRANS_TIMEOUT   (4000ul)
//#define TCP_PROBE_INTERVAL    (2000ul)


// Optional code
#define TCP_LISTEN_CODE


#endif


// UDP configuration defines
//
#ifdef COMPILE_UDP
#define UDP_MAX_CALLBACKS (1)
#endif


// DNS
//
#ifdef COMPILE_DNS

#define DNS_MAX_NAME_LEN  (128)
#define DNS_MAX_ENTRIES     (1)        // 7 bits only (max 127)
#define DNS_HANDLER_PORT   (57)

#define DNS_RECURSION_DESIRED  (1)

#define DNS_INITIAL_SEND_TIMEOUT   (500ul)   //  0.5 seconds
#define DNS_RETRY_THRESHOLD       (2000ul)   //  2 seconds
#define DNS_TIMEOUT              (10000ul)   // 10 seconds

#endif

#endif