using namespace std;

#include "xsb2cpp.h"


int main()
{
    PDb db;

    // Read a file named "dat" (included with this package)
    // and load predicates p/2 and p/3 from the same
    db.load("dat",{{"p",2}, {"p",3}});

    // Consult module "module.P" and invoke pred/2 from it
    // and return the results
    db.call("module",{{"pred",2}});

    // Convert read data to C++ list of tuples
    db.load("tdat",{{"t",2}});
    list<tuple<int,string>> tl;
    db.terms2tuples({"t",2}, tl);
    cout << "Read tuples" << endl;
    for(auto t:tl)
        cout << get<0>(t) << " " << get<1>(t) << endl;

    // Dump the data loaded. Please browse this function in xsb2cpp.h to see
    // how to access the loaded predicates in your application.
    db.dump();
}
