#include <iostream>

using namespace std;

#include "gcobject.h"

int main()
{
    int *cc = nullptr;
    ws::smart_ptr<void> c(nullptr);
    c = nullptr;
    ws::smart_ptr<int> cm(nullptr);
    cm = cc;
    std::cin.get();
    return 0;
}

