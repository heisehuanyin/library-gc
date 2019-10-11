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

generic_ptr::generic_ptr(void *host, GC_Delegate *delegate_target)
    :host_ptr(host),
      target_ptr(nullptr),
      target_delegate(delegate_target->clone())
{
    auto post = new PointerOver(PointerOver::NEW, host, this, target_delegate);
    queue->push(post);
}

generic_ptr::generic_ptr(const generic_ptr &other)
    :generic_ptr(other.host_ptr, other.target_delegate)
{
    this->operator=(other);
}

generic_ptr::generic_ptr(generic_ptr &&other)
    :generic_ptr(other.host_ptr, other.target_delegate)
{
    this->operator=(other);
}

generic_ptr::~generic_ptr(){
    if(target_ptr){
        auto post = new PointerRef(PointerRef::CANCEL, host_ptr, this, target_ptr, target_delegate);
        queue->push(post);
    }

    auto post = new PointerOver(PointerOver::DEL, host_ptr, this, target_delegate);
    queue->push(post);
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
    if(target_ptr){
        auto post = new PointerRef(PointerRef::CANCEL, host_ptr, this, target_ptr, target_delegate);
        queue->push(post);
    }

    this->target_ptr = target;
    auto post = new PointerRef(PointerRef::BUILD, host_ptr, this, target_ptr, target_delegate);
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

bool GC_Worker::check_root(std::list<void *> &records_node, std::list<void*> &remain_forks)
{
    auto _it_ = --remain_forks.cend();
    auto this_node = *_it_;
    remain_forks.erase(_it_);

    auto _peer_ = objs_map.find(this_node);
    if(_peer_ == objs_map.cend()){
        if(this_node != &invilid_node && this_node != &global_object){
            PrintStudio::printLine("ERROR:Out of registed except objects<",this_node,">.");
        }

        return true;
    }
    auto this_peer = _peer_->second;

    auto find_result = std::find(records_node.cbegin(), records_node.cend(), this_node);
    if(!this_peer->ref_records.size() && find_result == records_node.cend())
        return true;
    records_node.push_back(this_node);

    // ================next check====================
    for (auto it=this_peer->ref_records.cbegin(); it != this_peer->ref_records.cend(); ++it) {
        auto find_result = std::find(records_node.cbegin(), records_node.cend(), it->second);
        auto find_r2 = std::find(remain_forks.cbegin(), remain_forks.cend(), it->second);

        if(find_result == records_node.cend() && find_r2 == remain_forks.cend()){
            remain_forks.push_back(it->second);
        }
    }

    while (remain_forks.size()) {
        if(check_root(records_node, remain_forks))
            return true;
    }

    return false;
}

PointerRef::PointerRef(PointerRef::Type type, void *host, generic_ptr *ptr, void *target, GC_Delegate *target_degelate)
    :PointerOver (static_cast<PointerOver::Type>(type),host, ptr, target_degelate),target(target){}

void *PointerRef::target_object()
{
    return target;
}

void PointerRef::exec(std::map<void *, PeerSymbo *> &map)
{
    auto host_it = map.find(this->host_object());
    auto target_it = map.find(this->target_object());


    switch (this->command_type()) {
        case POINTER_OBJECTREF: {
                if(host_it == map.cend()){
                    return;
                }

                auto ref_assciate_it = host_it->second->members.find(this->smart_pointer());
                if(ref_assciate_it != host_it->second->members.cend())
                    host_it->second->members[this->smart_pointer()] = this->target_object();

                if(target_it == map.cend()){
                    auto ato = new PeerSymbo;
                    map[this->target_object()] = ato;

                    target_it = map.find(this->target_object());
                }

                // 引用计数
                auto itt = target_it->second->ref_records.find(this->smart_pointer());
                if(itt == target_it->second->ref_records.cend())
                    target_it->second->ref_records[this->smart_pointer()] = this->host_object();
            }
            break;
        case POINTER_CANCELREF: {
                if(target_it == map.cend())
                    break;

                auto ptr_it = target_it->second->ref_records.find(this->smart_pointer());
                if(ptr_it != target_it->second->ref_records.cend()){
                    target_it->second->ref_records.erase(ptr_it);

                    std::list<void*> records = {this->host_object()};
                    std::list<void*> remains = {this->target_object()};

                    if(target_it->second->ref_records.size()){
                        if(GC_Worker::check_root(records, remains)){
                            PrintStudio::printLine("gc-cancel<<<<<<<<<<<<");
                            auto xx = new PrintStack(PrintStack::CODE_MDDOC);
                            xx->exec(map);
                            delete xx;
                            PrintStudio::printLine("---------------------");
                            break;
                        }
                    }
                    // 删除实际内存对象
                    PrintStudio::printLine("delete-object::", this->target_object());
                    this->delegate_object()->manual_clear();
                    // 删除对等对象
                    PrintStudio::printLine("delete-peer");
                    delete target_it->second;
                    // 解除占位
                    map.erase(target_it);
                }
            }
            break;
        default:
            break;
    }

}


PointerOver::PointerOver(PointerOver::Type type, void *host, generic_ptr *ptr, GC_Delegate *delfree)
    :Command (static_cast<Command::Type>(type)), ptr_mark(ptr),host_ptr(host),delegate_ins(delfree){}

void *PointerOver::host_object(){
    return host_ptr;
}

GC_Delegate *PointerOver::delegate_object(){
    return delegate_ins;
}

generic_ptr *PointerOver::smart_pointer(){
    return ptr_mark;
}

void PointerOver::exec(std::map<void *, PeerSymbo *> &map)
{
    auto host_it = map.find(this->host_object());

    switch (this->command_type()) {
        case POINTER_NEW:
            {
                if(host_it == map.cend()){
                    auto peersym = new PeerSymbo;
                    peersym->members[this->smart_pointer()] = &invilid_node;

                    // 注册对等对象
                    map[this->host_object()] = peersym;
                }
                else {
                    host_it->second->members[this->smart_pointer()] = &invilid_node;
                }
            }
            break;
        case POINTER_DEL:
            {
                if(host_it != map.cend()){
                    auto result_it = host_it->second->members.find(this->smart_pointer());
                    if(result_it != host_it->second->members.cend())
                        host_it->second->members.erase(result_it);

                    PrintStudio::printLine("delete-delegate");
                    delete this->delegate_object();
                }
            }
            break;
        default:
            break;
    }
}

PrintStack::PrintStack(PrintStack::Type type):Command(Command::Type::PRINT_STACK),type(type){}

void PrintStack::exec(std::map<void *, PeerSymbo *> &map){
    std::map<void*, std::string> node_translate;
    std::map<std::string, void*> arrow_records;
    for (auto node : map) {
        if(node_translate.find(node.first) == node_translate.cend())
            node_translate[node.first] = "node_"+ std::to_string(node_translate.size());

        for (auto arrows : node.second->members) {
            if(node_translate.find(arrows.second) == node_translate.cend())
                node_translate[arrows.second] = "node_"+std::to_string(node_translate.size());

            auto item = node_translate[node.first] + " -> " + node_translate[arrows.second]
                    +"[label=\"ptr=";
            arrow_records[item] = arrows.first;
        }
    }

    if(type == Type::CODE_MDDOC)
        PrintStudio::printLine("```graphviz");
    PrintStudio::printLine("digraph stack_tracker{ ");
    for (auto node : node_translate) {
        PrintStudio::printLine(node.second, "[label=\"obj=", node.first, "\"]");
    }
    for (auto item : arrow_records) {
        PrintStudio::printLine(item.first, item.second, "\"];");
    }
    PrintStudio::printLine("}");

    if(type == Type::CODE_MDDOC)
        PrintStudio::printLine("```");
}

std::mutex ws::PrintStudio::global_lock;

void ws::print_gc_stack(StackPrintStyle style){
    __internal__implement::generic_ptr::print_stack(
                static_cast<__internal__implement::PrintStack::Type>(style));
}

void ws::PrintStudio::element_print(){}

void ws::PrintStudio::element_append(){}
