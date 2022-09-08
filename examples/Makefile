%.out:	%.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $<

all:	xsb2cpp.out cpp2xsb.out

include $(XSBCPPIFDIR)/Makefile.xsbcppif
