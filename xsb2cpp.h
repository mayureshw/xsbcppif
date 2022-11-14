#ifndef _XSB2CPP_H
#define _XSB2CPP_H

#include <vector>
#include <list>
#include <map>
#include <string>
#include <cstdlib>
#include <iostream>
#include <xsb_config.h>

#define byte BYTE
#include <cinterf.h>
#undef byte

#define AS(TYP,T) ((PAtom<TYP>*) T)->get()

class PTerm
{
    const string _f;
    vector<PTerm*> _args;
public:
    virtual bool isList() { return false; }
    virtual bool isAtom() { return false; }
    virtual bool isInt() { return false; }
    virtual bool isFloat() { return false; }
    virtual bool isString() { return false; }
    string functor() { return _f; }
    vector<PTerm*>& args() { return _args; }
    virtual string tostr()
    {
        string retstr = _f + "(";
        for(unsigned i = 0; i < _args.size(); i++)
        {
            retstr += _args[i]->tostr();
            if ( i < _args.size() - 1 ) retstr += ",";
        }
        retstr += ")";
        return retstr;
    }
    PTerm(string f) : _f(f), _args(0) {}
    PTerm(string f, vector<PTerm*>& argv) : _f(f), _args(argv) {}
    virtual ~PTerm() { for(auto a:_args) delete a; }
};

class PList : public PTerm
{
    vector<PTerm*> _v;
public:
    virtual bool isList() { return true; }
    string tostr()
    {
        string retstr = "[";
        for(unsigned i = 0; i < _v.size(); i++)
        {
            retstr += _v[i]->tostr();
            if ( i < _v.size() - 1 ) retstr += ",";
        }
        retstr += "]";
        return retstr;
    }
    // We slightly deviate, instead of ./2 we use arity 0
    // Avoid interpreting "." functor's arity
    PList(vector<PTerm*> v) : PTerm("."), _v(v) {}
    ~PList() { for(auto e:_v) delete e; }
};

template <typename T> class PAtom : public PTerm
{
    const T _a;
    string _tostr(T a)
    {
        if constexpr ( is_same<T, string> :: value ) return a;
        else return to_string(a);
    }
public:
    bool isInt() { if constexpr ( is_same<T, int> :: value ) return true; else return false; }
    bool isFloat() { if constexpr ( is_same<T, float> :: value ) return true; else return false; }
    bool isString() { if constexpr ( is_same<T, string> :: value ) return true; else return false; }
    virtual bool isAtom() { return true; }
    T get() { return _a; }
    string tostr() { return _tostr(_a); }
    PAtom(T a) : PTerm(_tostr(a)), _a(a) {}
};

typedef pair<string,unsigned> t_predspec;
class PDb
{
    map<t_predspec,list<PTerm*>> _termmap;
    void initxsb()
    {
        char *xsbargv[] = { getenv("XSBDIR") };
        if ( ! xsbargv[0] )
        {
            cout << "XSBDIR environment variable is not set" << endl;
            exit(1);
        }
        xsb_init(1,xsbargv);
    }
    void closexsb()
    {
        xsb_close();
    }
    void consult(string modulename)
    {
        cout << "xsb2cpp: Consulting module " << modulename << endl;
        string cmd = "[" + modulename + "].";
        int retval = xsb_command_string((char*)cmd.c_str());
        if ( retval ) cout << "xsb2cpp: Module consult failed : " << modulename << endl;
    }
    void loadfile(string flnm)
    {
        cout << "xsb2cpp: Loading file " << flnm << endl;
        string cmd = "load_dync(" + flnm + ").";
        int retval = xsb_command_string((char*)cmd.c_str());
        if ( retval ) cout << "xsb2cpp: File not found: " << flnm << endl;
    }
    // Note: empty list [] is an atom, it won't come here
    PTerm* pt2cpp_list(prolog_term pt)
    {
        vector<PTerm*> v;
        do
        {
            auto head = p2p_car(pt);
            pt = p2p_cdr(pt);
            v.push_back( pt2cpp(head) );
        } while ( is_list(pt) );
        return new PList(v);
    }
    PTerm* pt2cpp_atom(prolog_term pt)
    {
        PTerm *retterm =
            is_string(pt) ? new PAtom<string>(p2c_string(pt)) :
            is_int(pt) ? new PAtom<int>(p2c_int(pt)) :
            is_float(pt) ? new PAtom<float>(p2c_float(pt)) :
            (PTerm*) NULL;

        if ( ! retterm )
        {
            cout << "Unhandled prolog atom type" << endl;
            exit(1);
        }
        return retterm;
    }
    PTerm* pt2cpp_term(prolog_term pt)
    {
        auto functor = p2c_functor(pt);
        unsigned arity = p2c_arity(pt);
        vector<PTerm*> argv;
        for(unsigned i=1; i<=arity; i++)
            argv.push_back(pt2cpp(p2p_arg(pt,i)));
        return new PTerm(functor, argv);
    }
    // BEWARE: XSB uses $BOX$ to represent floats
    // TODO: What if pt is list in the top call?
    PTerm* pt2cpp(prolog_term pt)
    {
        return is_functor(pt) ? pt2cpp_term(pt) :
            is_list(pt) ? pt2cpp_list(pt) : pt2cpp_atom(pt);
    }
    void loadpred(t_predspec ps)
    {
        char* functor = (char*) ps.first.c_str();
        auto arity = ps.second;
        cout << "Querying " << functor << "/" << arity <<  "... ";
        c2p_functor(functor, arity, reg_term(1));
        int retval = xsb_query();
        while ( ! retval )
        {
            // reg_term(1) returns functor(...)
            // reg_term(2) returns ret(...)
            // We use (1). We could have thrown away the functor as it
            // repeats, but need something to bind together the args anyway
            auto pt = pt2cpp( reg_term(1) );
            _termmap[ps].push_back(pt);
            retval = xsb_next();
        }
        cout << "Got " << _termmap[ps].size() << " entries" << endl;
    }
public:
    template<typename T> T term2val(PTerm* t)
    {
        if ( not t->isAtom() )
        {
            cout << "term2val called on a non atomic term " << t->tostr() << endl;
            exit(1);
        }
        if constexpr ( is_integral<T>::value )
        {
            if ( t->isInt() ) return AS(T,t);
            else
            {
                cout << "Sought as int " << t->tostr() << endl;
                exit(1);
            }
        }
        if constexpr ( is_floating_point<T>::value )
        {
            if ( t->isFloat() ) return AS(T,t);
            else
            {
                cout << "Sought as float " << t->tostr() << endl;
                exit(1);
            }
        }
        if constexpr ( is_same<string,T>::value )
        {
            if ( t->isString() ) return AS(T,t);
            else
            {
                cout << "Sought as string " << t->tostr() << endl;
                exit(1);
            }
        }
    }
    template<typename T, typename... Ts> tuple<T,Ts...> term2tuple(PTerm *t,  int pos=0)
    {
        auto args = t->args();
        if ( pos >= args.size() )
        {
            cout << "Arg " << pos << " sought from term of arity " << args.size() << " : " << t->tostr() << endl;
            exit(1);
        }
        tuple<T> t0 = { term2val<T>( args[pos] ) };

        if constexpr ( sizeof...(Ts) == 0 ) return t0;
        else
        {
            tuple<T,Ts...> ts = tuple_cat( t0, term2tuple<Ts...>(t,pos+1) );
            return ts;
        }
    }
    // terms2tuples API for a given t_predspec
    // Currently supports only non-nested terms with integer, string atoms
    template<typename... Ts> void terms2tuples(t_predspec ps, list<tuple<Ts...>>& l)
    {
        for(auto t:get(ps)) l.push_back( term2tuple<Ts...>(t) );
    }
    list<PTerm*>& get(t_predspec ps) { return _termmap[ps]; }
    void dump()
    {
        for(auto ftermspairs:_termmap)
        {
            auto functor = ftermspairs.first.first;
            auto arity = ftermspairs.first.second;
            cout << functor << "/" << arity << endl;
            for(auto term:ftermspairs.second)
                cout << "\t" << term->tostr() << endl;
        }
    }
    void load(string flnm, vector<t_predspec> pspecs)
    {
        initxsb();
        loadfile(flnm);
        for(auto ps:pspecs) loadpred(ps);
        closexsb();
    }
    void call(string modulename, vector<t_predspec> pspecs)
    {
        initxsb();
        consult(modulename);
        for(auto ps:pspecs) loadpred(ps);
        closexsb();
    }
    ~PDb()
    {
        for(auto ftermspairs:_termmap)
            for(auto term:ftermspairs.second)
                delete term;
    }
};

#endif
