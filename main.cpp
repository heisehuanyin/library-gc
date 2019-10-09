#include <iostream>

using namespace std;

#include "gcobject.h"

class GeObject : public ws::GC_Object
{
public:
    GeObject(){
        cout << "GeObjectNew" << endl;
    }
    ~GeObject(){
        cout << "GeObjectDel" << endl;
    }
};

void test(){
    ws::smart_ptr<GeObject> c(nullptr);
    c = new GeObject();

    ws::smart_ptr<GeObject> f(nullptr);
    f = c;
    c = new GeObject();
}

int main()
{
    test();
    std::cin.get();
    return 0;
}

