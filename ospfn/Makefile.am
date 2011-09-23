## Automake.am for OSPF API client

INCLUDES = @INCLUDES@ -I.. -I$(top_srcdir) -I$(top_srcdir)/lib

sbin_PROGRAMS = ospfn

ospfn_SOURCES = \
	ccnx_opaque_lsa.c tables.c ndn_route.c utility.c ccn_fib.c ospfn.c

ospfn_LDADD = ../ospfclient/libospfapiclient.la \
	../ospfd/libospf.la ../lib/libzebra.la @LIBCAP@ -lcrypto -lccn

ospfn_CFLAGS = $(AM_CFLAGS) $(PICFLAGS)
ospfn_LDFLAGS = $(AM_LDFLAGS) $(PILDFLAGS)

