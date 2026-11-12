/* amiga-ipv6.h: IPv6 definitions for AmigaOS 3 with ixemul.library,
   whose headers know IPv4 only.  Included from usocket.h.

   This only makes luasocket compile.  No Amiga TCP/IP stack used with
   ixemul speaks IPv6, so every IPv6 call fails at run time with the
   stack's own error; IPv4 is untouched.  LuaTeX keeps the socket
   library off unless asked (--socket).
   AmigaOS 4 and MorphOS have these definitions and do not need this.  */

#ifndef LUASOCKET_AMIGA_IPV6_H
#define LUASOCKET_AMIGA_IPV6_H

#ifndef AF_INET6
#define AF_INET6 30
#define PF_INET6 AF_INET6
#endif
#ifndef IPPROTO_IPV6
#define IPPROTO_IPV6 41
#endif
#ifndef INET6_ADDRSTRLEN
#define INET6_ADDRSTRLEN 46
#endif
#ifndef IPV6_V6ONLY
#define IPV6_UNICAST_HOPS   4
#define IPV6_MULTICAST_HOPS 10
#define IPV6_MULTICAST_LOOP 11
#define IPV6_ADD_MEMBERSHIP 12
#define IPV6_DROP_MEMBERSHIP 13
#define IPV6_V6ONLY         27
#endif

#ifndef IN6ADDR_ANY_INIT
struct in6_addr { unsigned char s6_addr[16]; };
#define IN6ADDR_ANY_INIT { { 0 } }
struct sockaddr_in6 {
  unsigned char  sin6_len;
  unsigned char  sin6_family;
  unsigned short sin6_port;
  unsigned long  sin6_flowinfo;
  struct in6_addr sin6_addr;
  unsigned long  sin6_scope_id;
};
struct ipv6_mreq {
  struct in6_addr ipv6mr_multiaddr;
  unsigned int    ipv6mr_interface;
};
#endif

#ifndef _SS_MAXSIZE
#define _SS_MAXSIZE 128
struct sockaddr_storage {
  unsigned char ss_len;
  unsigned char ss_family;
  char          __ss_pad[_SS_MAXSIZE - 2];
} __attribute__ ((aligned (4)));
#endif

#endif /* not LUASOCKET_AMIGA_IPV6_H */
