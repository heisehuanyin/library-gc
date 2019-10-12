#ifndef GCOBJECT_H
#define GCOBJECT_H

#include <list>
#include <map>
#include <iostream>

#include "../ecosystem/sync.h"
#include "../ecosystem/excstream.h"

#ifndef WS_DEBUG_MACRO_TOOLS
#define WS_DEBUG_MACRO_TOOLS
#define WS_TURNON_STACK_MESSAGE() \
    ws::__wether_print_debug_state = true;
#endif


namespace ws {
    extern bool __wether_print_debug_state;

    // Pre-DECL
    namespace __internal__implement {
        class generic_ptr;
        class PeerSymbo;
    }

    namespace __internal__implement {
        class GC_Delegate{
        public:
            GC_Delegate() = default;
            virtual ~GC_Delegate() = default;

            virtual void manual_clear() = 0;
            virtual GC_Delegate* newins(void* ptr = nullptr) = 0;
        };

        // 可执行命令超类
        class Command{
        public:
            enum Type{
                POINTER_NEW,
                POINTER_DEL,
                POINTER_OBJECTREF,
                POINTER_CANCELREF,
                PRINT_STACK
            };
            explicit Command(Type type);
            virtual ~Command() = default;

            Type command_type();

            /**
             * @brief 命令执行反转
             * @param item 抽象命令
             * @param map 对象引用图
             */
            virtual void exec(std::map<void*, PeerSymbo*>& map) = 0;

        private:
            Type type;
        };
        class PrintStack : public Command{
        public:
            enum Type{
                CODE_MDDOC,
                CODE_GRAPHVIZ
            };
            explicit PrintStack(Type type);
            virtual ~PrintStack() = default;

            void exec(std::map<void*, PeerSymbo*> &map);

        private:
            Type type;
        };

        // 智能指针更改指向
        class PointerRefer: public Command
        {
        public:
            enum Type{
                BUILD = Command::POINTER_OBJECTREF,
                CANCEL = Command::POINTER_CANCELREF
            };
            PointerRefer(Type type, void* host, generic_ptr* ptr, void* target, GC_Delegate* target_degelate);
            virtual ~PointerRefer() override = default;

            void* host_object();
            generic_ptr* smart_pointer();
            void* target_object();
            GC_Delegate* delegate_object();

            void exec(std::map<void*, PeerSymbo*>& map) override;

        private:
            void *const host_ptr;
            generic_ptr *const pointer;
            void *const target;
            GC_Delegate*const delegate_ins;
        };


        class PeerSymbo{
        public:
            explicit PeerSymbo();

            GC_Delegate* delegate;
            //  smart_ptr_ref      target_object:pointto
            std::map<generic_ptr*, void*> pointerto;
            //  samrt_ptr_ref      host_object:pointfrom
            std::map<generic_ptr*, void*> referfrom;
        };

        class generic_ptr{
        public:
            explicit generic_ptr(void *host, void* target, GC_Delegate* delegate_target);
            generic_ptr(const generic_ptr& other);
            generic_ptr(generic_ptr&& other);
            virtual ~generic_ptr();

            generic_ptr &operator=(void* target);
            generic_ptr &operator=(const generic_ptr& other);
            void *operator->() const ;

            static void print_stack(PrintStack::Type type);

        private:
            void *const host_ptr;
            void * target_ptr;
            GC_Delegate * delegate_gen;
            static sync::BlockingQueue<Command*>* queue;
        };

        class GC_Worker : exec::Thread
        {
        public:
            static sync::BlockingQueue<Command*>&& commands;
            static bool check_root(std::list<void *> &records, std::list<void *> &remains);

            GC_Worker();
            ~GC_Worker() = default;

            void run();
        private:
            //       original-ptr  all-managed
            static std::map<void*, PeerSymbo*> objs_map;
        };

        template <typename T>
        class GC_RawWrap : public GC_Delegate
        {
        public:
            GC_RawWrap(T* host):obj(host){}
            virtual ~GC_RawWrap(){}

            virtual void manual_clear(){
                if(obj) delete obj;
            }

            virtual GC_Delegate* newins(void* ptr){
                if(ptr)
                    return new GC_RawWrap(static_cast<T*>(ptr));
                else
                    return new GC_RawWrap(obj);
            }

        private:
            T*const obj;
        };
    }

    extern __internal__implement::GC_RawWrap<int> global_object;

    template <typename T>
    class smart_ptr : __internal__implement::generic_ptr
    {
    public:
        template<typename HostType>
        explicit smart_ptr(HostType* host, T* target)
            :generic_ptr(host, target, new __internal__implement::GC_RawWrap<T>(target)) {}
        template<typename HostType>
        explicit smart_ptr(HostType* host)
            :generic_ptr(host, nullptr, new __internal__implement::GC_RawWrap<T>(nullptr)){}
        smart_ptr(const smart_ptr<T>& other)
            :generic_ptr(other){}
        smart_ptr(smart_ptr<T>&& rv)
            :generic_ptr(rv){}

        virtual ~smart_ptr() = default;


        smart_ptr<T>& operator=(T* target){
            __internal__implement::generic_ptr::operator=(target);

            return *this;
        }
        smart_ptr<T>& operator=(const smart_ptr<T>& other){
            __internal__implement::generic_ptr::operator=(other);

            return *this;
        }

        T& operator*() const {
            return *this->operator->();
        }

        T* operator->() const {
            return static_cast<T*>(__internal__implement::generic_ptr::operator->());
        }
    };

    enum StackPrintStyle{
        MD_SNIPPETS = __internal__implement::PrintStack::CODE_MDDOC,
        GRAPHVIZ = __internal__implement::PrintStack::CODE_GRAPHVIZ
    };

    template <typename HostType, typename T>
    smart_ptr<T> gc_wrap(HostType*host, T* target){
        return smart_ptr<T>(host, target);
    }

    class IOStudio
    {
    public:
        IOStudio() = default;
        ~IOStudio() = default;

        template <typename... Args>
        void appendLineToStack(Args... args){
            std::lock_guard<std::mutex> locker(ins_lock);
            lines.push_back("");

            element_append(args...);
        }
        void clear(){
            lines.clear();
        }

        template <typename... Args>
        static void printLine(Args... args){
            std::lock_guard<std::mutex> locker(global_lock);

            element_print(args...);
            std::cout << std::endl;
        }

        static void print(IOStudio& ins){
            std::lock_guard<std::mutex> locker(global_lock);

            for (auto line : ins.lines) {
                std::cout << line << std::endl;
            }
        }

    private:
        static std::mutex global_lock;
        static void element_print();
        template<typename T, typename... Args>
        static void element_print(T first, Args... args){
            std::cout << first;
            element_print(args...);
        }

        std::list<std::string> lines;
        std::mutex ins_lock;
        void element_append();
        template<typename T, typename... Args>
        void element_append(T first, Args... args){
            auto item_it = --lines.end();
            auto item = *item_it;
            item += std::to_string(first);
            lines.insert(item_it, item);
            lines.erase(++item_it);

            element_append(args...);
        }
    };


    void print_gc_stack(StackPrintStyle style);
}

#endif // GCOBJECT_H
