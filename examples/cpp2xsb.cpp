using namespace std;

#include "cpp2xsb.h"
#include <iostream>


int main()
{
    // Create a store for a relation of rel1(int,string) form
    Rel<int,string> rel1("rel1");

    // Another example: rel2(long, string, long)
    Rel<long,string,long> rel2("rel2");

    // populate these relations with some data
    rel1.add({1,"one"});
    rel1.add({2,"two"});
    
    rel2.add({100,"hundred",-100});
    rel2.add({1000,"thousand",-1000});

    // Dump both relations to a file named "store"
    ofstream of("store");
    rel1.dump(of);
    rel2.dump(of);
    cout << "Generated file: store" << endl;
}
