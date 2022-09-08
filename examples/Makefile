%.out:	%.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $<

BINS	=	xsb2cpp.out cpp2xsb.out

all:	$(BINS)

clean:
	rm -f $(BINS)

include $(XSBCPPIFDIR)/Makefile.xsbcppif
