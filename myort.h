#ifndef MYORT_H
#define MYORT_H
#include"adresse.h"
#include"poi.h"
class MyOrt:public Adresse,public PoI
{
public:
    MyOrt(double l,double b, char t,string s,int h,int p,string g,string Na,string ka,string be);

    MyOrt& operator =(const MyOrt& other);
    int getId() const;

private:
    const int id=0;
};

#endif // MYORT_H
