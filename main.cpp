#include <iostream>

using namespace std;

#include "gcobject.h"

class ABO : public ws::GCObject{
public:
    void print(){
        cout << "MM" << endl;
    }
};
class ACO:public ws::GCObject{
public:
    void print(){
        cout << "QP" << endl;
    }
};

int main()
{
    ws::__in_ptr one(ws::default_global_object);
    ws::auto_ptr<ABO> two(ws::default_global_object);
    one = new ABO();
    one->print();

    two = new ABO();
    two->print();
    return 0;
}

