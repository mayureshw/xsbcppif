using namespace std;

#include "xsb2cpp.h"

using tuptyp = tuple<string, int, string, float>;
using ntuptyp = tuple<string, tuple<string,string> >;


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
    db.load("tdat",{{"t",3}});
    auto tl = db.terms2tuples<tuptyp>({"t",3});
    cout << "Read tuples " << tl.size() << endl;
    for(auto t:tl) cout << get<0>(t) << "(" << get<1>(t) << "," << get<2>(t) << "," << ")" << endl;

    db.load("tdat",{{"nt",1}});
    auto ntl = db.terms2tuples<ntuptyp>({"nt",1});
    cout << "Read tuples " << ntl.size() << endl;
    for(auto t:ntl) cout << get<0>(t) << "(" << get<0>(get<1>(t)) << "(" << get<1>(get<1>(t)) << ")" << endl;

    // Dump the data loaded. Please browse this function in xsb2cpp.h to see
    // how to access the loaded predicates in your application.
    db.dump();
}
