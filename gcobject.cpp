#include "gcobject.h"

using namespace ws::__internal;

static GC_Worker worker;
static GC_Delegate<int> invilid_node(nullptr);
GC_Delegate<int> ws::global_object(nullptr);


Command::Command(Command::Type type)
    :type(type){}

Command::Type Command::command_type(){ return type; }



sync::BlockingQueue<Command*>&& GC_Worker::commands = sync::BlockingQueue<Command*>();
std::map<void*, PeerSymbo*> GC_Worker::objs_map = std::map<void*, PeerSymbo*>();

sync::BlockingQueue<Command*>* ge_ptr::queue = &GC_Worker::commands;

ge_ptr::ge_ptr(void *host, GC_Object* delegate)
    :host_ptr(host),
      host_delegate(delegate),
      target_ptr(nullptr)
{
    auto post = new PointerOver(PointerOver::NEW, host, host_delegate, this);
    queue->push(post);
}

ge_ptr::ge_ptr(const ge_ptr &other)
    :ge_ptr(other.host_ptr, other.host_delegate)
{
    this->operator=(other);
}

ge_ptr::ge_ptr(ge_ptr &&other)
    :ge_ptr(other.host_ptr, other.host_delegate)
{
    this->operator=(other);
}

ge_ptr::~ge_ptr(){
    if(target_ptr){
        auto post = new PointerRef(PointerRef::CANCEL, host_ptr, host_delegate, this, target_ptr);
        queue->push(post);
    }

    auto post = new PointerOver(PointerOver::DEL, host_ptr, host_delegate, this);
    queue->push(post);
}

ge_ptr &ge_ptr::operator=(const ge_ptr &other)
{
    operator=(other.target_ptr);

    return *this;
}

void *ge_ptr::operator->()
{
    return target_ptr;
}

ge_ptr &ge_ptr::operator=(void *target)
{
    if(target_ptr){
        auto post = new PointerRef(PointerRef::CANCEL, host_ptr, host_delegate, this, target_ptr);
        queue->push(post);
    }

    this->target_ptr = target;
    auto post = new PointerRef(PointerRef::BUILD, host_ptr, host_delegate, this, target_ptr);
    queue->push(post);

    return *this;
}

GC_Worker::GC_Worker()
{
    this->start();
}

void GC_Worker::run(){
    while (true) {
        auto item = static_cast<PointerOver*>(commands.take());
        auto host_it = objs_map.find(item->host_object());

        switch (item->command_type()) {
            case Command::POINTER_NEW:
                {
                    if(host_it == objs_map.cend()){
                        auto peersym = new PeerSymbo;
                        peersym->members.push_back(std::make_pair(item->smart_pointer(), &invilid_node));

                        // 注册对等对象
                        objs_map[item->host_object()] = peersym;
                    }
                    else {
                        host_it->second->members.push_back(std::make_pair(item->smart_pointer(), &invilid_node));
                    }

                    // 注册实际内存管理委托对象
                    host_it = objs_map.find(item->host_object());
                    auto iii = std::find(host_it->second->delegates.cbegin(),
                                         host_it->second->delegates.cend(),
                                         item->delegate_object());

                    if(iii == host_it->second->delegates.cend())
                        host_it->second->delegates.push_back(item->delegate_object());
                }
                break;
            case Command::POINTER_DEL:
                {
                    if(host_it != objs_map.cend()){
                        for (auto pointer_it = host_it->second->members.cbegin();
                             pointer_it != host_it->second->members.cend();
                             ++pointer_it) {

                            if(pointer_it->first == item->smart_pointer())
                                host_it->second->members.erase(pointer_it);
                        }

                        // 删除指定指针对应的委托对象，留下一个用于实际管理指向对象
                        if(host_it->second->delegates.size() > 1){
                            auto itdel = std::find(host_it->second->delegates.cbegin(),
                                                   host_it->second->delegates.cend(),
                                                   item->delegate_object());

                            if(itdel != host_it->second->delegates.cend()){
                                delete *itdel;
                                host_it->second->delegates.erase(itdel);
                            }
                        }
                    }
                }
                break;
            case Command::POINTER_OBJECTREF:
                {
                    auto caseitem = static_cast<PointerRef*>(item);

                    for (auto ref_assciate_it=host_it->second->members.cbegin();
                         ref_assciate_it != host_it->second->members.cend();
                         ++ref_assciate_it)
                    {
                        // 修改指针指向
                        if(ref_assciate_it->first == caseitem->smart_pointer()){
                            host_it->second->members.insert(ref_assciate_it,
                                                            std::make_pair(caseitem->smart_pointer(),
                                                                           caseitem->target_pointer()));

                            host_it->second->members.erase(ref_assciate_it);
                            break;
                        }
                    }


                    auto target_it = objs_map.find(caseitem->target_pointer());
                    if(target_it == objs_map.cend()){
                        auto ato = new PeerSymbo;
                        objs_map[caseitem->target_pointer()] = ato;
                        ato->delegates.push_back(item->delegate_object());

                        target_it = objs_map.find(caseitem->target_pointer());
                    }

                    std::list<void*> temp = { host_it->first };
                    auto itt = std::find(target_it->second->ref_records.cbegin(),
                                         target_it->second->ref_records.cend(),
                                         caseitem->smart_pointer());

                    if(itt == target_it->second->ref_records.cend()){
                        if(check_loop(temp, host_it->second))
                            break;
                        target_it->second->ref_records.push_back(caseitem->smart_pointer());
                    }
                }
                break;
            case Command::POINTER_CANCELREF:
                {
                    auto caseitem = static_cast<PointerRef*>(item);
                    auto target_it = objs_map.find(caseitem->target_pointer());

                    if(target_it == objs_map.cend()){
                        break;
                    }

                    auto ptr_it = std::find(target_it->second->ref_records.cbegin(),
                                         target_it->second->ref_records.cend(),
                                         caseitem->smart_pointer());

                    if(ptr_it != target_it->second->ref_records.cend()){
                        target_it->second->ref_records.erase(ptr_it);

                        // 如果引用记录为0，通过委托对象删除指向内存对象
                        if(!target_it->second->ref_records.size()){
                            // 删除实际内存对象
                            (*target_it->second->delegates.cbegin())->manual_clear();
                            // 删除委托对象
                            delete *target_it->second->delegates.cbegin();
                            // 删除对等对象
                            delete target_it->second;
                            // 解除占位
                            objs_map.erase(target_it);
                        }
                    }
                }
                break;
        }
        delete item;
    }
}

bool GC_Worker::check_loop(std::list<void *> &achor, PeerSymbo *item)
{
    for (auto it=item->members.cbegin(); it!=item->members.cend();++it) {
        auto object = it->second;

        auto itorrrr = std::find(achor.cbegin(), achor.cend(), object);
        if(itorrrr != achor.cend()){
            return true;
        }
        else {
            achor.push_back(object);

            auto it2 = objs_map.find(object);
            if(it2 != objs_map.cend()){
                if(check_loop(achor, it2->second)){
                    return true;
                }
            }

            achor.remove(object);
        }
    }
    return false;
}


PointerRef::PointerRef(PointerRef::Type type, void *host, GC_Object *mgro, ge_ptr *ptr, void *target)
    :PointerOver (static_cast<PointerOver::Type>(type), host, mgro, ptr),target(target){}

void *PointerRef::target_pointer()
{
    return target;
}


PointerOver::PointerOver(PointerOver::Type type, void *host, GC_Object *mgro, ge_ptr *ptr)
    :Command (static_cast<Command::Type>(type)), ptr_mark(ptr),host_ptr(host),  mgro(mgro){}

void *PointerOver::host_object(){
    return host_ptr;
}

GC_Object *PointerOver::delegate_object(){
    return mgro;
}

ge_ptr *PointerOver::smart_pointer(){
    return ptr_mark;
}
