#include "gcobject.h"

using namespace ws::__internal;

static GC_Worker worker;


Command::Command(Command::Type type, ws::GC_Object *host)
    :type(type),
      host_ptr(host){}

Command::Type Command::commandType(){ return type; }

ws::GC_Object *Command::hostObject(){ return host_ptr; }


sync::BlockingQueue<Command*>&& GC_Worker::commands = sync::BlockingQueue<Command*>();
std::map<ws::GC_Object*, PeerSymbo*> GC_Worker::objs_map = std::map<ws::GC_Object*, PeerSymbo*>();

sync::BlockingQueue<Command*>* ge_ptr::queue = &GC_Worker::commands;

ge_ptr::ge_ptr(ws::GC_Object *host)
    :host_ptr(host)
{
    auto post = new PointerOver(PointerOver::NEW, host, this);
    queue->push(post);
}

ge_ptr::ge_ptr(const ge_ptr &other)
    :ge_ptr(other.host_ptr)
{
    this->operator=(other);
}

ge_ptr::ge_ptr(ge_ptr &&other)
    :ge_ptr(other.host_ptr)
{
    this->operator=(other);
}

ge_ptr::~ge_ptr(){
    if(target_ptr){
        auto post = new PointerRef(PointerRef::CANCEL, host_ptr, this, target_ptr);
        queue->push(post);
    }

    auto post = new PointerOver(PointerOver::DEL, host_ptr, this);
    queue->push(post);
}

ge_ptr &ge_ptr::operator=(const ge_ptr &other){
    operator=(other.target_ptr);

    return *this;
}

ws::GC_Object *ge_ptr::operator->(){
    return target_ptr;
}

ge_ptr &ge_ptr::operator=(GC_Object *target){
    if(target_ptr){
        auto post = new PointerRef(PointerRef::CANCEL, host_ptr, this, target_ptr);
        queue->push(post);
    }

    this->target_ptr = target;
    auto post = new PointerRef(PointerRef::BUILD, host_ptr, this, target_ptr);
    queue->push(post);

    return *this;
}

GC_Worker::GC_Worker()
{
    this->start();
}

void GC_Worker::run(){
    while (true) {
        auto item = commands.take();
        auto host_it = objs_map.find(item->hostObject());

        switch (item->commandType()) {
            case Command::POINTER_NEW:
                {
                    auto caseitem = static_cast<PointerOver*>(item);

                    if(host_it == objs_map.cend()){
                        auto peersym = new PeerSymbo();

                        objs_map[item->hostObject()] = peersym;
                        peersym->members.push_back(std::make_pair(caseitem->pointer(), nullptr));
                    }
                    else {
                        host_it->second->members.push_back(std::make_pair(caseitem->pointer(), nullptr));
                    }
                }
                break;
            case Command::POINTER_DEL:
                {
                    auto caseitem = static_cast<PointerOver*>(item);

                    if(host_it != objs_map.cend()){
                        for (auto pointer_it = host_it->second->members.cbegin();
                             pointer_it != host_it->second->members.cend();
                             ++pointer_it) {

                            if(pointer_it->first == caseitem->pointer())
                                host_it->second->members.erase(pointer_it);
                        }
                    }
                }
                break;
            case Command::POINTER_OBJECTREF:
                {
                    auto caseitem = static_cast<PointerRef*>(item);
                    auto target_it = objs_map.find(caseitem->targetPointer());

                    if(target_it == objs_map.cend()){
                        objs_map.insert(std::make_pair(caseitem->targetPointer(), new PeerSymbo));
                        target_it = objs_map.find(caseitem->targetPointer());
                    }

                    auto itt = std::find(target_it->second->ref_records.cbegin(),
                                         target_it->second->ref_records.cend(),
                                         caseitem->pointer());

                    if(itt == target_it->second->ref_records.cend()){
                        target_it->second->ref_records.push_back(caseitem->pointer());

                        for (auto ref_assciate_it=host_it->second->members.cbegin();
                             ref_assciate_it != host_it->second->members.cend();
                             ++ref_assciate_it)
                        {
                            if(ref_assciate_it->first == caseitem->pointer() &&
                               !check_loop(host_it->first, host_it->second)){
                                host_it->second->members.insert(ref_assciate_it,
                                                                std::make_pair(caseitem->pointer(),
                                                                               caseitem->targetPointer()));
                                host_it->second->members.erase(ref_assciate_it);
                                break;
                            }
                        }
                    }
                }
                break;
            case Command::POINTER_CANCELREF:
                {
                    auto caseitem = static_cast<PointerRef*>(item);
                    auto target_it = objs_map.find(caseitem->targetPointer());

                    if(target_it == objs_map.cend())
                        break;

                    auto itt = std::find(target_it->second->ref_records.cbegin(),
                                         target_it->second->ref_records.cend(),
                                         caseitem->pointer());

                    if(itt != target_it->second->ref_records.cend()){
                        target_it->second->ref_records.erase(itt);
                        if(!target_it->second->ref_records.size()){
                            delete target_it->first;
                            delete target_it->second;
                            objs_map.erase(target_it);
                        }
                    }
                }
                break;
            case Command::OBJECT_NEW:
                {
                    auto caseitem = static_cast<ObjectOver*>(item);
                    auto object_it = objs_map.find(caseitem->hostObject());

                    if(object_it != objs_map.cend())
                        break;

                    objs_map[caseitem->hostObject()] = new PeerSymbo();
                }
                break;
            case Command::OBJECT_DEL:
                {
                    auto caseitem = static_cast<ObjectOver*>(item);
                    auto object_it = objs_map.find(caseitem->hostObject());

                    if(object_it == objs_map.cend())
                        break;

                    delete object_it->second;
                    objs_map.erase(object_it);
                }
                break;
        }
        delete item;
    }
}

bool GC_Worker::check_loop(ws::GC_Object *achor, PeerSymbo *item)
{
    for (auto it=item->members.cbegin(); it!=item->members.cend();++it) {
        auto object = it->second;
        if(object == achor){
            return true;
        }
        else {
            auto it2 = objs_map.find(object);
            if(it2 != objs_map.cend()){
                if(check_loop(achor, it2->second))
                    return true;
            }
        }
    }
    return false;
}

PointerOver::PointerOver(PointerOver::Type type, ws::GC_Object *host, ge_ptr *ptr)
    :Command(static_cast<Command::Type>(type), host),
      ptr_mark(ptr){}

ge_ptr *PointerOver::pointer()
{
    return ptr_mark;
}

PointerRef::PointerRef(PointerRef::Type type, ws::GC_Object *host, ge_ptr *ptr, ws::GC_Object *target)
    :PointerOver(static_cast<PointerOver::Type>(type), host, ptr),
      target(target){}

ws::GC_Object *PointerRef::targetPointer(){
    return target;
}

ObjectOver::ObjectOver(ObjectOver::Type type, ws::GC_Object *obj)
    :Command (static_cast<Command::Type>(type), obj){}

PeerSymbo::PeerSymbo()
{

}
