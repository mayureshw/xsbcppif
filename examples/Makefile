# Please set the compiler that supports at least c++14 standard
CPP=/usr/pkg/gcc12/bin/c++

%.out:	%.cpp
	$(CPP) $(CPPFLAGS) $(LDFLAGS) -o $@ $<

all:	xsb2cpp.out cpp2xsb.out

include $(XSBCPPIFDIR)/Makefile.xsbcppif
