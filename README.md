# libgc
## 目的
1. 在C++领域探索一个通用的GC解决方案。
2. 希望在语言层面实现可用GC。

## 解决方案
1. 智能指针托管所有使用中的堆上裸指针对象;
2. 每次智能指针更改指向，检索所有引用去除环形引用。

## 开发现状
1. 设计方案为专用线程后台垃圾收集;
2. 提供ws::smart_ptr能够自动将指针纳入管理，成功处理环形引用;
3. 完成非侵入式体系，对任何类型都可以提供完整的GC功能；
3. 设计下一代gc管理系统
    * 提供调用栈的追溯功能，gc出现问题的时候，提供记录手段；

## 优势
1. 专用垃圾收集线程，异步垃圾收集，完全不影响程序性能
2. 非侵入式设计，对任意堆上指针对象进行管理


## 使用方法
```
// define a managed object-type
class SomeType
{
public:
    // smart_ptr init
    SomeType()
        :pointer(new ws::smart_ptr<SomeType>(this)){}
    virtual ~SomeType() = default;

    // else method
    void print(const string& content){
        cout << content << endl;
    }
    ....

    // define smart_ptr member
    ws::smart_ptr<SomeType> pointer;
}

// ws::smart_ptr 基本使用
// 初始话过程中需要指明该指针的所有者，不允许所有者为nullptr
// libgc提供默认全局对象ws::global_object供非成员变量使用
ws::smart_ptr<SomeType> p(&ws::global_object);
p= new SomeType;
p->print("成功调用");

// auto 集成
auto nptr = ws::gc_wrap(&ws::global_object, new SomeType);
nptr->print("成功输出");

auto temp = p->pointer.operator->();
p->pointer = nptr->pointer;
nptr->pointer = temp;

```