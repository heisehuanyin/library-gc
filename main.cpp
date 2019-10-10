#include <iostream>

using namespace std;

#include "gcobject.h"

class GeObject
{
public:
    GeObject()
        :hold(ws::smart_ptr<GeObject>(this))
    {
        cout << "GeObjectNew" << endl;
    }
    ~GeObject(){
        cout << "GeObjectDel" << endl;
    }

    void reference(ws::smart_ptr<GeObject> one){
        this->hold = one;
    }

    void print(const string &msg){
        cout << msg << endl;
    }

private:
    ws::smart_ptr<GeObject> hold;
};



void test(){
    // 新建智能指针
    ws::smart_ptr<GeObject> c(&ws::global_object);
    // 托管实例指针
    c = new GeObject();

    ws::smart_ptr<GeObject> f(&ws::global_object);
    // 智能指针复制
    f = c;
    // 变更托管指针
    c = new GeObject();

    auto nptr = ws::gc_wrap(&ws::global_object, new GeObject);
    nptr->print("simple");

    // 循环引用
    f->reference(c);
    c->reference(f);

    f->print("f-msg");
    c->print("c-msg");
}

int main()
{
    test();
    cout << "test-done" << endl;
    std::cin.get();
    return 0;
}

