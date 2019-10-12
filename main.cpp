#include <iostream>

using namespace std;

#include "gcobject.h"

#define WS_DEBUG_MACRO_TOOLS

class GeObject
{
public:
    GeObject()
        :hold(ws::smart_ptr<GeObject>(this, nullptr))
    {
        //ws::IOStudio::printLine("++sGeObjectNew[",this,"]");
    }
    ~GeObject(){
        //ws::IOStudio::printLine("--GeObjectDel[",this,"]");
    }

    void reference(ws::smart_ptr<GeObject> one){
        this->hold = one;
    }

    void print(std::string v){
        ws::IOStudio::printLine(v);
    }

private:
    ws::smart_ptr<GeObject> hold;
};

void test(/*ws::smart_ptr<GeObject> external*/){
    // 新建智能指针
    ws::smart_ptr<GeObject> c(&ws::global_object, nullptr);
    // 托管实例指针
    c = new GeObject();

    ws::smart_ptr<GeObject> f(&ws::global_object, nullptr);
    // 智能指针复制
    f = c;
    // 变更托管指针
    c = new GeObject();

    auto nptr = ws::gc_wrap(&ws::global_object, new GeObject);

    auto& c3 = *nptr;
    c3.reference(f);
    //c3.print("hello-nptr");

    // 循环引用
    f->reference(c);
    c->reference(f);



    // 环形悬空
    auto p1 = ws::gc_wrap(&ws::global_object, new GeObject);
    auto p2 = ws::gc_wrap(&ws::global_object, new GeObject);
    auto p3 = ws::gc_wrap(&ws::global_object, new GeObject);
    auto p4 = ws::gc_wrap(&ws::global_object, new GeObject);

    p4->reference(p1);
    p1->reference(p2);
    p2->reference(p3);
    p3->reference(p1);

}

int main()
{
    WS_TURNON_STACK_MESSAGE
    test();
    ws::IOStudio::printLine("TestDone.=====================");
    std::cin.get();
    return 0;
}

