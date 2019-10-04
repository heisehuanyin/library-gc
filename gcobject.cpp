#include "gcobject.h"

ws::GCObject::GCObject(){}

void ws::GCObject::ws_object_reference_decreasement(ws::__inner::middle_ptr &item){
    auto it = std::find(pointer_list.cbegin(), pointer_list.cend(), &item);
    if(it != pointer_list.cend()){
        pointer_list.remove(&item);
    }

    if(!pointer_list.size()){
        delete this;
    }
}

void ws::GCObject::ws_object_reference_increasement(ws::__inner::middle_ptr &item){
    auto it = std::find(pointer_list.cbegin(), pointer_list.cend(), &item);
    if(it == pointer_list.cend()){
        pointer_list.push_back(&item);
    }
}

void ws::GCObject::ws_object_register_member(ws::__inner::middle_ptr &item){
    auto it = std::find(member_list.cbegin(), member_list.cend(), &item);
    if(it == member_list.cend()){
        member_list.push_back(&item);
    }
}

void ws::GCObject::ws_object_unregister_member(ws::__inner::middle_ptr &item){
    auto it = std::find(member_list.cbegin(), member_list.cend(), &item);
    if(it != member_list.cend()){
        member_list.remove(&item);
    }
}

ws::__inner::middle_ptr::middle_ptr(ws::GCObject *host)
    :object_host(host),
      object_target(nullptr){
    host->ws_object_register_member(*this);
}

ws::__inner::middle_ptr::middle_ptr(ws::GCObject *host, ws::GCObject *target)
    :middle_ptr(host){
    this->operator=(target);
}

ws::__inner::middle_ptr::middle_ptr(const ws::__inner::middle_ptr &other)
    :object_target(other.object_target)
{
    this->operator=(other.object_target);
}

ws::__inner::middle_ptr::~middle_ptr(){
    if(object_target != nullptr)
        object_target->ws_object_reference_decreasement(*this);

    object_host->ws_object_unregister_member(*this);
}

ws::__inner::middle_ptr &ws::__inner::middle_ptr::operator=(ws::__inner::middle_ptr &other){
    return this->operator=(other.object_target);
}

ws::__inner::middle_ptr &ws::__inner::middle_ptr::operator=(ws::GCObject *target){
    if(object_target != nullptr)
        object_target->ws_object_reference_decreasement(*this);

    object_target = target;
    if(!circulate_for_loop(object_target))
        object_target->ws_object_reference_increasement(*this);


    return *this;
}

ws::GCObject *ws::__inner::middle_ptr::operator->(){
    if(!object_target){
        std::cout << "ERROR::对nullptr指针进行操作"<< std::endl;
        exit(0);
    }
    return object_target;
}

bool ws::__inner::middle_ptr::circulate_for_loop(ws::GCObject *ins){
    for (auto it=ins->member_list.cbegin(); it!=ins->member_list.cend(); it++) {
        if((*it) == this || ((*it)->object_target!=nullptr &&
                             circulate_for_loop((*it)->object_target)))
            return true;
    }

    return false;
}

