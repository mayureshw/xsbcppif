#ifndef _CPP2XSB_H
#define _CPP2XSB_H

#include <list>
#include <tuple>
#include <string>
#include <fstream>

template<typename... Ts> class Rel
{
    const string _functor;
    list<tuple<Ts...>> _rels;
    string pstr(long l) { return to_string(l); }
    string pstr(string& a)
    {
        return "'" + a + "'";
    }

    template <size_t I = 0>
    typename enable_if<I == sizeof...(Ts), void>::type
    printTuple(tuple<Ts...> tup, ofstream& ofs)
    {
        ofs << ")." << endl;
    }
     
    template <size_t I = 0>
    typename enable_if<(I < sizeof...(Ts)), void>::type
    printTuple(tuple<Ts...> tup, ofstream& ofs)
    {
        if ( I == 0 ) ofs << _functor << "(";
        ofs << pstr( get<I>(tup) );
        if ( I < sizeof...(Ts) - 1 ) ofs << ",";
        printTuple<I + 1>(tup, ofs);
    }

public:
    void dump(ofstream& ofs)
    {
        for(auto r:_rels) printTuple(r,ofs);
    }
    void add(tuple<Ts...> t) { _rels.push_back(t); }
    Rel(string functor) : _functor(functor) {}
};

#endif
