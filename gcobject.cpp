#include "gcobject.h"
#include <iostream>

using namespace ws::__internal__implement;

static GC_Worker worker;
static GC_RawWrap<int> invilid_node(nullptr);
GC_RawWrap<int> ws::global_object(nullptr);

sync::BlockingQueue<Command*>&& GC_Worker::commands = sync::BlockingQueue<Command*>();
std::map<void*, PeerSymbo*> GC_Worker::objs_map = std::map<void*, PeerSymbo*>();
sync::BlockingQueue<Command*>* generic_ptr::queue = &GC_Worker::commands;

Command::Command(Command::Type type):type(type){}

Command::Type Command::command_type(){ return type; }

generic_ptr::generic_ptr(void *host, void *target, GC_Delegate *delegate_target)
    :host_ptr(host),
      target_ptr(target),
      target_delegate(delegate_target)
{
    auto post = new PointerRefer(PointerRefer::BUILD, host, this, target, target_delegate);
    queue->push(post);
}

generic_ptr::generic_ptr(const generic_ptr &other)
    :generic_ptr(other.host_ptr, other.target_ptr, other.target_delegate->clone())
{
    this->operator=(other);
}

generic_ptr::generic_ptr(generic_ptr &&other)
    :generic_ptr(other.host_ptr, other.target_ptr, other.target_delegate->clone())
{
    this->operator=(other);
}

generic_ptr::~generic_ptr(){
    if(target_ptr){
        auto post = new PointerRefer(PointerRefer::CANCEL, host_ptr, this, target_ptr, target_delegate);
        queue->push(post);
    }
}

GC_Delegate *generic_ptr::delegate_of_target() const{
    return target_delegate;
}

generic_ptr &generic_ptr::operator=(const generic_ptr &other)
{
    operator=(other.target_ptr);

    return *this;
}

void *generic_ptr::operator->() const
{
    return target_ptr;
}

void generic_ptr::print_stack(PrintStack::Type type)
{
    auto post = new PrintStack(type);
    queue->push(post);
}

generic_ptr &generic_ptr::operator=(void *target)
{
    if(target == target_ptr)
        return *this;

    if(target_ptr){
        auto post = new PointerRefer(PointerRefer::CANCEL, host_ptr, this, target_ptr, target_delegate);
        queue->push(post);
    }

    this->target_ptr = target;
    auto post = new PointerRefer(PointerRefer::BUILD, host_ptr, this, target_ptr, target_delegate);
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

        item->exec(objs_map);
        delete item;
    }
}

bool GC_Worker::check_root(std::list<void *> &records, std::list<void*> &remains)
{
    auto _it_ = --remains.cend();
    auto this_node = *_it_;

    auto _peer_ = objs_map.find(this_node);
    if(_peer_ == objs_map.cend()){
        if(this_node != &invilid_node && this_node != &global_object){
            IOStudio::printLine("ERROR:Out of registed except objects<",this_node,">.");
        }

        return true;
    }
    auto this_peer = _peer_->second;

    auto find_result = std::find(records.cbegin(), records.cend(), this_node);
    if(!this_peer->referfrom.size() && find_result == records.cend())
        return true;
    records.push_back(this_node);
    remains.erase(_it_);

    // ================next check====================
    for (auto it=this_peer->referfrom.cbegin(); it != this_peer->referfrom.cend(); ++it) {
        auto find_result = std::find(records.cbegin(), records.cend(), it->second);
        auto find_r2 = std::find(remains.cbegin(), remains.cend(), it->second);

        if(find_result == records.cend() && find_r2 == remains.cend()){
            remains.push_back(it->second);
        }
    }

    while (remains.size()) {
        if(check_root(records, remains))
            return true;
    }

    return false;
}

PointerRefer::PointerRefer(PointerRefer::Type type, void *host, generic_ptr* ptr, void *target, GC_Delegate *target_degelate)
    :Command (static_cast<Command::Type>(type)), host_ptr(host), pointer(ptr),target(target),delegate_ins(target_degelate){}

void *PointerRefer::target_object()
{
    return target;
}

void PointerRefer::exec(std::map<void *, PeerSymbo *> &map)
{
    auto host_it = map.find(this->host_object());
    auto target_it = map.find(this->target_object());

    switch (this->command_type()) {
        case POINTER_OBJECTREF: {
                if(host_it == map.cend()){
                    map[this->host_object()] = new PeerSymbo;
                    host_it = map.find(this->host_object());
                }
                host_it->second->members[this->smart_pointer()] = this->target_object();

                if(target_it == map.cend()){
                    map[this->target_object()] = new PeerSymbo;
                    target_it = map.find(this->target_object());
                }

                // 引用计数
                target_it->second->referfrom[this->smart_pointer()] = this->host_object();

                IOStudio::printLine("## gc-ReferBuild-target[",this->smart_pointer(),"]\n");
                PrintStack xx(PrintStack::CODE_MDDOC);
                xx.exec(map);
            }
            break;
        case POINTER_CANCELREF: {
                if(target_it == map.cend())
                    break;

                auto ptr1 = host_it->second->members.find(this->smart_pointer());
                if(ptr1 != host_it->second->members.cend())
                    host_it->second->members.erase(ptr1);

                auto ptr2 = target_it->second->referfrom.find(this->smart_pointer());
                if(ptr2 != target_it->second->referfrom.cend()){
                    target_it->second->referfrom.erase(ptr2);

                    /*

                    std::list<void*> records = {this->host_object()};
                    std::list<void*> remains = {this->target_object()};
                    if(!target_it->second->referfrom.size() ||
                       !GC_Worker::check_root(records, remains)){
                        // 删除实际内存对象
                        this->delegate_object()->manual_clear();
                        // 删除对等对象
                        delete target_it->second;
                        // 解除占位
                        map.erase(target_it);
                    }*/
                }

                IOStudio::printLine("## gc-ReferCancel-target[",this->smart_pointer(),"]\n");
                PrintStack xx(PrintStack::CODE_MDDOC);
                xx.exec(map);
            }
            break;
        default:
            break;
    }

}

void *PointerRefer::host_object(){
    return host_ptr;
}

GC_Delegate *PointerRefer::delegate_object(){
    return delegate_ins;
}

generic_ptr *PointerRefer::smart_pointer(){
    return pointer;
}

PrintStack::PrintStack(PrintStack::Type type):Command(Command::Type::PRINT_STACK),type(type){}

void PrintStack::exec(std::map<void *, PeerSymbo *> &map){
    std::map<void*, std::string> node_translate;
    std::list<std::pair<std::string, generic_ptr*>> arrow_records;
    for (auto node : map) {
        if(node_translate.find(node.first) == node_translate.cend())
                node_translate[node.first] = "node_"+ std::to_string(node_translate.size());

        for (auto arrow : node.second->members) {
            if(arrow.second == &invilid_node)
                continue;

            if(node_translate.find(arrow.second) == node_translate.cend())
                node_translate[arrow.second] = "node_"+std::to_string(node_translate.size());

            auto item = node_translate[node.first]+"->"+node_translate[arrow.second]+" [label=\"ptr:";
            arrow_records.push_back(std::make_pair(item, arrow.first));
        }
    }


    if(type == Type::CODE_MDDOC)
        IOStudio::printLine("```graphviz");
    IOStudio::printLine("digraph stack_tracker{ ");
    for (auto node : node_translate) {
        if(node.first == &global_object)
            IOStudio::printLine(node.second, "[label=\"obj:", "global-root", "\"]");
        else if(node.first == &invilid_node)
            IOStudio::printLine(node.second, "[label=\"obj:invilid-leaf\"]");
        else if(node.first == nullptr)
            IOStudio::printLine(node.second, "[label=\"obj:nullptr\"]");
        else
            IOStudio::printLine(node.second, "[label=\"obj:", node.first, "\"]");
    }
    for (auto item : arrow_records) {
        IOStudio::printLine(item.first, item.second, "\"];");
    }
    IOStudio::printLine("}");

    if(type == Type::CODE_MDDOC)
        IOStudio::printLine("```");
}

std::mutex ws::IOStudio::global_lock;

void ws::print_gc_stack(StackPrintStyle style){
    __internal__implement::generic_ptr::print_stack(
                static_cast<__internal__implement::PrintStack::Type>(style));
}

void ws::IOStudio::element_print(){}

void ws::IOStudio::element_append(){}
