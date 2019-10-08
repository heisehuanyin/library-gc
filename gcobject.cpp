#include "gcobject.h"

static ws::internal::GC_Collect worker;

ws::internal::Command::Command(ws::internal::Command::Type type, ws::internal::ge_ptr *ptr, GC_Object *host)
    :type(type),
      host_ptr(host),
      ptr(ptr){}

ws::internal::Command::Type ws::internal::Command::getType(){ return type; }

ws::GC_Object *ws::internal::Command::hostPointer(){ return host_ptr; }

ws::internal::ge_ptr *ws::internal::Command::ptrPointer(){ return ptr; }

ws::internal::PointerNew::PointerNew(GC_Object *host, ws::internal::ge_ptr *ptr)
    :Command (Type::POINTER_NEW, ptr, host){}

ws::internal::PointerDel::PointerDel(GC_Object *host, ws::internal::ge_ptr *ptr)
    :Command(Type::POINTER_DEL,ptr, host){}

ws::internal::PointerObjectRef::PointerObjectRef(GC_Object *host, ws::internal::ge_ptr *ptr, GC_Object *target)
    :Command(Type::POINTER_OBJECTREF, ptr, host),
      target(target){}

ws::GC_Object *ws::internal::PointerObjectRef::targetPointer(){
    return target;
}

ws::internal::PointerCancelRef::PointerCancelRef(GC_Object *host, ws::internal::ge_ptr *ptr, GC_Object *target)
    :Command (Type::POINTER_CANCELREF, ptr, host),
      target(target){}

ws::GC_Object *ws::internal::PointerCancelRef::targetPointer(){
    return target;
}

ws::BlockingQueue<ws::internal::Command*> ws::internal::GC_Collect::commands = ws::BlockingQueue<ws::internal::Command*>();
ws::BlockingQueue<ws::internal::Command*>* ws::internal::ge_ptr::queue = &ws::internal::GC_Collect::commands;

ws::internal::ge_ptr::ge_ptr(GC_Object *host)
    :host_ptr(host)
{
    auto post = new PointerNew(host, this);
    queue->pull(post);
}

ws::internal::ge_ptr::ge_ptr(ws::internal::ge_ptr &other)
    :ge_ptr(other.host_ptr)
{
    this->operator=(other);
}

ws::internal::ge_ptr::ge_ptr(ws::internal::ge_ptr &&other)
    :ge_ptr(other.host_ptr)
{
    this->operator=(other);
}

ws::internal::ge_ptr::~ge_ptr(){
    if(target_ptr){
        auto post = new PointerCancelRef(host_ptr, this, target_ptr);
        queue->pull(post);
    }

    auto post = new PointerDel(host_ptr, this);
    queue->pull(post);
}

ws::internal::ge_ptr &ws::internal::ge_ptr::operator=(ws::internal::ge_ptr &other){
    operator=(other.target_ptr);

    return *this;
}

ws::GC_Object *ws::internal::ge_ptr::operator->(){
    return target_ptr;
}

ws::internal::ge_ptr &ws::internal::ge_ptr::operator=(GC_Object *target){
    if(target_ptr){
        auto post = new PointerCancelRef(host_ptr, this, target_ptr);
        queue->pull(post);
    }

    this->target_ptr = target;
    auto post = new PointerObjectRef(host_ptr, this, target_ptr);
    queue->pull(post);

    return *this;
}

bool check_loop(ws::internal::PeerSymbo* item){
    for (auto it=item->members.cbegin(); it!=item->members.cend();++it) {

    }
}

void ws::internal::GC_Collect::fn_collect_calc(){
    while (true) {
        auto item = commands.take();
        auto peer_host = objs_map.find(item->hostPointer());

        switch (item->getType()) {
            case Command::POINTER_NEW:
                if(peer_host == objs_map.cend()){
                    auto peersym = new PeerSymbo(item->hostPointer());
                    objs_map.insert(std::make_pair(item->hostPointer(), peersym));

                    peersym->members.push_back(std::make_pair(item->ptrPointer(), nullptr));
                }
                else {
                    peer_host->second->members.push_back(std::make_pair(item->ptrPointer(), nullptr));
                }
                break;
            case Command::POINTER_DEL:
                if(peer_host != objs_map.cend()){
                    auto it = std::find(peer_host->second->members.cbegin(),
                                        peer_host->second->members.cend(),
                                        std::make_pair(item->ptrPointer(), nullptr));

                    if(it != peer_host->second->members.cend())
                        peer_host->second->members.erase(it);
                }
                break;
            case Command::POINTER_OBJECTREF:
                {
                    auto caseitem = static_cast<PointerObjectRef*>(item);
                    auto peer_target = objs_map.find(caseitem->targetPointer());
                    auto itt = std::find(peer_target->second->handlers_record.cbegin(),
                                         peer_target->second->handlers_record.cend(),
                                         caseitem->ptrPointer());

                    if(itt == peer_target->second->handlers_record.cend())
                        peer_target->second->handlers_record.push_back(caseitem->ptrPointer());
                }
                break;
            case Command::POINTER_CANCELREF:
                {
                    auto caseitem = static_cast<PointerObjectRef*>(item);
                    auto peer_target = objs_map.find(caseitem->targetPointer());

                    auto itt = std::find(peer_target->second->handlers_record.cbegin(),
                                         peer_target->second->handlers_record.cend(),
                                         caseitem->ptrPointer());

                    if(itt != peer_target->second->handlers_record.cend()){
                        peer_target->second->handlers_record.erase(itt);
                        if(!peer_target->second->handlers_record.size()){
                             delete peer_target->second->host_object;
                             delete peer_target->second;
                             objs_map.erase(peer_target);
                        }
                    }
                }
                break;
        }
    }
}
