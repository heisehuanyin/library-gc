#ifndef GCOBJECT_H
#define GCOBJECT_H

#include <list>
#include <map>

#include "../ecosystem/sync.h"
#include "../ecosystem/excstream.h"

namespace ws {
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
        };

        // 可执行命令超类
        class Command{
        public:
            enum Type{
                POINTER_NEW,
                POINTER_DEL,
                POINTER_OBJECTREF,
                POINTER_CANCELREF
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
        // 智能指针的建立与删除
        class PointerOver: public Command
        {
        public:
            enum Type{
                NEW = Command::POINTER_NEW,
                DEL = Command::POINTER_DEL,
                CONNEC = Command::POINTER_OBJECTREF,
                CANCAL = Command::POINTER_CANCELREF
            };
            PointerOver(Type type, void* host, GC_Delegate* host_delegate, generic_ptr* ptr);
            virtual ~PointerOver() override = default;

            void* host_object();
            GC_Delegate* delegate_object();
            generic_ptr *smart_pointer();

            void exec(std::map<void*, PeerSymbo*>& map) override;

        private:
            generic_ptr*const ptr_mark;
            void*const host_ptr;
            GC_Delegate*const delegate_ins;
        };
        // 智能指针更改指向
        class PointerRef: public PointerOver
        {
        public:
            enum Type{
                BUILD = Command::POINTER_OBJECTREF,
                CANCEL = Command::POINTER_CANCELREF
            };
            PointerRef(Type type, void* host, generic_ptr* ptr, void* target, GC_Delegate* target_degelate);
            virtual ~PointerRef() override = default;

            void* target_pointer();

            void exec(std::map<void*, PeerSymbo*>& map) override;

        private:
            void*const target;
        };


        class PeerSymbo{
        public:
            explicit PeerSymbo()
                :delegates({}){}

            std::list<GC_Delegate*> delegates;
            std::list<std::pair<generic_ptr*, void*>> members;
            std::list<generic_ptr*> ref_records;
        };

        class generic_ptr{
        public:
            explicit generic_ptr(void *host, GC_Delegate *delegate_host, GC_Delegate* delegate_target);
            generic_ptr(const generic_ptr& other);
            generic_ptr(generic_ptr&& other);
            virtual ~generic_ptr();

            GC_Delegate *delegate_of_target() const ;

            generic_ptr &operator=(void* target);
            generic_ptr &operator=(const generic_ptr& other);
            void *operator->() const ;

        private:
            void *const host_ptr;
            GC_Delegate *const host_delegate;
            void * target_ptr;
            GC_Delegate *const target_delegate;
            static sync::BlockingQueue<Command*>* queue;
        };

        class GC_Worker : exec::Thread
        {
        public:
            static sync::BlockingQueue<Command*>&& commands;

            GC_Worker();
            ~GC_Worker() = default;

            void run();

            static bool check_loop(std::list<void *> &achor, PeerSymbo* item);

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

            void reset_target(T* p){
                obj = p;
            }

            virtual void manual_clear(){
                if(obj) delete obj;
            }

        private:
            T* obj;
        };
    }

    extern __internal__implement::GC_RawWrap<int> global_object;

    template <typename T>
    class smart_ptr : __internal__implement::generic_ptr
    {
    public:
        template<typename HostType>
        explicit smart_ptr(HostType* host)
            :generic_ptr(host,
                    new __internal__implement::GC_RawWrap<HostType>(host),
                    new __internal__implement::GC_RawWrap<T>(nullptr)) {}
        smart_ptr(const smart_ptr<T>& other)
            :generic_ptr(other){}
        smart_ptr(smart_ptr<T>&& rv)
            :generic_ptr(rv){}

        virtual ~smart_ptr() = default;


        smart_ptr<T>& operator=(T* target)
        {
            static_cast<__internal__implement::GC_RawWrap<T>*>
                    (delegate_of_target())->reset_target(target);

            __internal__implement::generic_ptr::operator=(target);

            return *this;
        }
        smart_ptr<T>& operator=(const smart_ptr<T>& other){
            static_cast<__internal__implement::GC_RawWrap<T>*>
                    (delegate_of_target())->reset_target(other.operator->());

            __internal__implement::generic_ptr::operator=(other);

            return *this;
        }

        T* operator->() const {
            return static_cast<T*>(__internal__implement::generic_ptr::operator->());
        }
    };


    template <typename HostType, typename T>
    smart_ptr<T> gc_wrap(HostType*host, T* target){
        auto one = smart_ptr<T>(host);
        return one = target;
    }
}

#endif // GCOBJECT_H
