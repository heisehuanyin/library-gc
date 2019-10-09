# libgc
## 目的
1. 在C++领域探索一个通用的GC解决方案。
2. 希望在语言层面实现可用GC。

## 解决方案
1. 所有实例类型继承共同根类型；
2. 实例对等类型记录所有引用实例指针的智能指针记录；
3. 每次智能指针更改实例对象指向的时候检索所有引用，去除环形引用。

## 开发现状
1. 设计方案更改为专用线程后台垃圾收集
2. 完成设计，提供ws::smart_ptr能够自动将指针纳入管理，成功处理环形引用
3. 设计下一代gc管理系统
    * 设计非侵入式体系，任何类型不需要继承GC_Object就可以提供完整的GC功能；
    * 提供调用栈的追溯功能，gc出现问题的时候，提供记录手段；

## 优势
1. 专用垃圾收集线程，异步垃圾收集，完全不影响程序性能


## 使用方法
```
// define a managed object-type
class SomeType: public ws::GC_Object
{
public:
    SomeType() = default;
    virtual ~SomeType() = default;

    // else method
    ....
    void print(const string& content){
        cout << content << endl;
    }
}

// ws::smart_ptr 使用
ws::smart_ptr<SomeType> p= new SomeType();
p->print("成功调用");

```