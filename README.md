# xsbcppif
A C++ interface with XSB Prolog

XSB Prolog ( http://xsb.sourceforge.net ) is a Prolog / deductive database
system. It already provides a foreign language interface to C and Python. See
its user manual for details.

This component provides a C++ wrapper around the C interface provided by XSB.

Currently a limited goal of this component is to:

1. Be able to create objects that can be dumped into a file in Prolog format.
   This is done by cpp2xsb.h

2. Be able to load a Prolog database of facts in canonical form (i.e. one that
   can be loaded using load_dync interface in XSB prolog.) and access each fact
   as a C++ object.

If you wish to do something more, consider using XSB's native C language
interface.

Please see Makefile.xsbcppif for system requirements and usage.

Please see the examples directory for sample code for using xsb2cpp and cpp2xsb
interfaces.
