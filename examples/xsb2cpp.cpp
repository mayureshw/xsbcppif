using namespace std;

#include "xsb2cpp.h"


int main()
{
    PDb db;

    // Read a file named "dat" (included with this package)
    // and load predicates p/2 and p/3 from the same
    db.load("dat",{{"p",2}, {"p",3}});

    // Dump the data loaded. Please browse this function in xsb2cpp.h to see
    // how to access the loaded predicates in your application.
    db.dump();
}
