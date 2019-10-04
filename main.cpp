#include <iostream>

using namespace std;

#include "gcobject.h"

class ABO : public ws::GCObject{
public:
    ABO():stack(this){
        cout << "ABO Construct" << endl;
    }
    virtual ~ABO(){
        cout << "ABO Distroy" << endl;
    }

    virtual void print(){
        cout << "MM" << endl;
    }

    void Set(ABO* ot){
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
    ws::auto_ptr<ABO> ptrone(&ws::default_global_object);
    ptrone=new ACO();
    ptrone->print();

    ws::auto_ptr<ABO> ppone(&ws::default_global_object);
    ppone = new ACO();

    ptrone->Set(ppone.operator->());
    ppone->Set(ptrone.operator->());

    cout<< "~~~~~~~~~~~~~~~~~~~~~~~~~~"<< endl;

}

int main()
{
    t();
    return 0;
}

