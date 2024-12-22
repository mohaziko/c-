#include "myort.h"

MyOrt::MyOrt(double l, double b, char t, string s, int h, int p, string g, string Na,string ka,string be) : Ort(l,b,t,0,Na), Adresse(l,b,t,0,s,h,p,g,Na),PoI(l,b,t,0,ka,be,Na){}



MyOrt &MyOrt::operator =(const MyOrt &other)
{
    Adresse::operator =(other);
    PoI::operator =(other);
    return *this;
}

int MyOrt::getId() const
{
    return id;
}
