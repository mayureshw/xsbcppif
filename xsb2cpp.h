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
protected:
    const string _f;
    vector<PTerm*> _args;
    string args2str()
    {
        string retstr;
        for(unsigned i = 0; i < _args.size(); i++)
        {
            retstr += _args[i]->tostr();
            if ( i < _args.size() - 1 ) retstr += ",";
        }
        return retstr;
    }
public:
    virtual bool isList() { return false; }
    virtual bool isTuple() { return false; }
    virtual bool isAtom() { return false; }
    virtual bool isInt() { return false; }
    virtual bool isFloat() { return false; }
    virtual bool isString() { return false; }
    bool isGeneralTerm() { return not( isList() or isTuple() or isAtom() ); }
    virtual int asInt()
    {
        cout << "xsb2cpp asInt sought on non-atom" << endl;
        exit(1);
    }
    virtual float asFloat()
    {
        cout << "xsb2cpp asFloat sought on non-atom" << endl;
        exit(1);
    }
    virtual string asString()
    {
        cout << "xsb2cpp asString sought on non-atom" << endl;
        exit(1);
    }
    string functor() { return _f; }
    int arity() { return _args.size(); }
    vector<PTerm*>& args() { return _args; }
    virtual string tostr()
    {
        string retstr = _f + "(";
        for(unsigned i = 0; i < arity(); i++)
        {
            retstr += _args[i]->tostr();
            if ( i < arity() - 1 ) retstr += ",";
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
public:
    bool isList() { return true; }
    string tostr()
    {
        string retstr = "[" + args2str() + "]";
        return retstr;
    }
    // We slightly deviate, instead of ./2 we use arity 0
    // Avoid interpreting "." functor's arity
    PList(vector<PTerm*> v) : PTerm(".",v) {}
};

class PTuple : public PTerm
{
public:
    bool isTuple() { return true; }
    string tostr()
    {
        string retstr = "(" + args2str() + ")";
        return retstr;
    }
    PTuple(vector<PTerm*> v) : PTerm(",",v) {}
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
    bool isAtom() { return true; }
    int asInt()
    {
        if constexpr ( is_arithmetic<T> :: value )  return (int) _a;
        cout << "xsb2cpp: string sought as int " << _a << endl;
        exit(1);
    }
    float asFloat()
    {
        if constexpr ( is_arithmetic<T> :: value )  return (float) _a;
        cout << "xsb2cpp: string sought as float " << _a << endl;
        exit(1);
    }
    string asString()
    {
        if constexpr ( is_same<T,string> :: value ) return _a;
        cout << "xsb2cpp: non string sought as string " << _a << endl;
        exit(1);
    }
    T get() { return _a; }
    string tostr() { return _tostr(_a); }
    PAtom(T a) : PTerm(_tostr(a)), _a(a) {}
};

typedef pair<string,unsigned> t_predspec;
class PDb
{
    map<t_predspec,list<PTerm*>> _pstermmap;
    map<string,list<PTerm*>> _strtermmap;
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
        string cmd = "load_dync('" + flnm + "').";
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
    PTerm* pt2cpp_tuple(prolog_term pt)
    {
        vector<PTerm*> v;
        auto curpt = pt;
        while( true )
        {
            auto arg0 = p2p_arg(curpt,1);
            auto arg1 = p2p_arg(curpt,2);
            v.push_back(pt2cpp(arg0));
            if ( is_functor(arg1) and string(p2c_functor(arg1)) == string(",") ) curpt = arg1;
            else
            {
                v.push_back(pt2cpp(arg1));
                break;
            }
        }
        return new PTuple(v);
    }
    PTerm* pt2cpp_atom(prolog_term pt)
    {
        // Note: XSB models empty list as an atom, we convert it to PList here
        PTerm *retterm =
            is_string(pt) ?
                ( string(p2c_string(pt)) == string("[]") ? (PTerm*) new PList({}) :
                  (PTerm*) new PAtom<string>(p2c_string(pt))
                ) :
            is_int(pt) ? new PAtom<int>(p2c_int(pt)) :
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
        // Note: If the functor is "," we create a flat PTuple, instead of ,/2 nested terms
        auto functor = p2c_functor(pt);
        if ( string(functor) == string(",") ) return pt2cpp_tuple(pt);
        unsigned arity = p2c_arity(pt);
        vector<PTerm*> argv;
        for(unsigned i=1; i<=arity; i++)
            argv.push_back(pt2cpp(p2p_arg(pt,i)));
        return new PTerm(functor, argv);
    }
    // TODO: What if pt is list in the top call?
    PTerm* pt2cpp(prolog_term pt)
    {
        // float needs to be dealt with specially as it is modeled as a term in XSB
        return is_float(pt) ?  new PAtom<float>(p2c_float(pt)) :
            is_functor(pt) ? pt2cpp_term(pt) : is_list(pt) ? pt2cpp_list(pt) : pt2cpp_atom(pt);
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
            _pstermmap[ps].push_back(pt);
            retval = xsb_next();
        }
        cout << "Got " << _pstermmap[ps].size() << " entries" << endl;
    }
    void loadpred(string str)
    {
        cout << "Querying " << str << "..." << endl;
        int retval = xsb_query_string((char*)str.c_str());
        while ( ! retval )
        {
            auto pt = pt2cpp( reg_term(1) );
            _strtermmap[str].push_back(pt);
            retval = xsb_next();
        }
    }
    template <typename T> void _call(string modulename, vector<T> pspecs)
    {
        initxsb();
        consult(modulename);
        for(auto ps:pspecs) loadpred(ps);
        closexsb();
    }
public:
    template<typename T, typename... Ts> void args2tuple(PTerm *term, tuple<T,Ts...>& ret, int pos=0)
    {
        auto argtoprocess = term->args()[pos];
        T t;
        term2tuple(argtoprocess, t);
        tuple<T> tt = make_tuple(t);
        if constexpr ( sizeof...(Ts) == 0 ) ret = tt;
        else
        {
            tuple<Ts...> trest;
            args2tuple( term, trest, pos+1 );
            ret = tuple_cat( tt, trest );
        }
    }
    // term2tuple is an overloaded function with disambiguation by parameter
    // instead of a dummy parameter we use the same parameter to return value
    // Variant 1: Prolog terms and Prolog tuples to C++ tuples
    template<typename T, typename... Ts> void term2tuple(PTerm *term, tuple<T,Ts...>& ret)
    {
        if ( not( term->isGeneralTerm() or term->isTuple() ) )
        {
            cout << "xsb2cpp: Non tuple, non term sought as tuple: " << term->tostr() << endl;
            exit(1);
        }
        if ( term->isGeneralTerm() )
        {
            static_assert( is_same<T,string>::value ); // functor name always a string
            tuple<Ts...> argst;
            args2tuple(term, argst);
            ret = tuple_cat( make_tuple( term->functor() ), argst );
        }
        else args2tuple(term, ret);
    }
    // Variant 2: Prolog lists to C++ lists
    template<typename T> void term2tuple(PTerm *term, list<T>& ret)
    {
        if ( not term->isList() )
        {
            cout << "xsb2cpp: Non list sought as list: " << term->tostr() << endl;
            exit(1);
        }
        for(auto e:term->args())
        {
            T et;
            term2tuple(e,et);
            ret.push_back(et);
        }
    }
    // Variant 3: Prolog atoms to C++ types
    template<typename T> void term2tuple(PTerm *term, T& ret)
    {
        if constexpr ( is_integral<T>::value ) ret = (T) term->asInt();
        else if constexpr ( is_floating_point<T>::value ) ret = (T) term->asFloat();
        else if constexpr ( is_same<string,T>::value ) ret = term->asString();
        else
        {
            cout << "xsb2cpp: Non atom sought as atom: " << term->tostr() << endl;
            exit(1);
        }
    }
    template<typename... Ts> void _terms2tuples( t_predspec ps, list<tuple<Ts...>>& l )
    {
        for(auto term:get(ps))
        {
            tuple<Ts...> t;
            term2tuple(term, t);
            l.push_back( t );
        }
    }
    template<typename T> list<T> terms2tuples(t_predspec ps)
    {
        list<T> l;
        _terms2tuples(ps, l);
        return l;
    }
    list<PTerm*>& get(t_predspec ps) { return _pstermmap[ps]; }
    list<PTerm*>& get(string str) { return _strtermmap[str]; }
    void dump()
    {
        for(auto ftermspairs:_pstermmap)
        {
            auto functor = ftermspairs.first.first;
            auto arity = ftermspairs.first.second;
            cout << functor << "/" << arity << endl;
            for(auto term:ftermspairs.second)
                cout << "\t" << term->tostr() << endl;
        }
        for(auto strtermpairs:_strtermmap)
        {
            auto str = strtermpairs.first;
            cout << str << endl;
            for(auto term:strtermpairs.second)
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
    void callSpec(string modulename, vector<t_predspec> pspecs)
    {
        _call<t_predspec>(modulename, pspecs);
    }
    void callStr(string modulename, vector<string> pstrs)
    {
        _call<string>(modulename, pstrs);
    }
    ~PDb()
    {
        for(auto ftermspairs:_pstermmap)
            for(auto term:ftermspairs.second)
                delete term;
        for(auto strtermpairs:_strtermmap)
            for(auto term:strtermpairs.second)
                delete term;
    }
};

#endif
