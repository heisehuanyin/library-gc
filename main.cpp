#include <iostream>

using namespace std;

#include "gcobject.h"

class ABO : public ws::GCObject{
public:
    ABO():stack(this){
        cout << "ABO=================" << endl;
    }
    virtual ~ABO(){
        cout << "ABO<<<<<<<<<<<<<<<<<<" << endl;
    }

    virtual void print(){
        cout << "MM" << endl;
    }

    void set(ws::auto_ptr<ABO>& ot){
        stack = ot;
    }
private:
    ws::auto_ptr<ABO> stack;
};
class ACO:public ABO{
public:
    ACO(){
        cout<< "ACO NEW" << endl;
    }
    ~ACO(){
        cout<< "ACO DELETE" << endl;
    }

    void print(){
        cout << "QP" << endl;
    }
};

void t(){
    ws::auto_ptr<ABO> ptrone;
    ptrone=new ACO();
    ptrone->print();

    ws::auto_ptr<ABO> p3;
    p3 = new ABO();
    ptrone->set(p3);
    p3->set(ptrone);


    ws::auto_ptr<ABO> ptrtwo(&ws::default_global_object);
    ptrtwo = ptrone;
    ptrtwo->print();
}

int main()
{
    t();
    return 0;
}

