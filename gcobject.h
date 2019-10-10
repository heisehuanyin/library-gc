#ifndef GCOBJECT_H
#define GCOBJECT_H

#include <iostream>
#include <thread>
#include <list>
#include <map>
#include <algorithm>

#include "../ecosystem/sync.h"
#include "../ecosystem/excstream.h"

namespace ws {
    // Pre-DECL
    namespace __internal {
        class Command;
        class PointerNew;
        class PointerDel;
        class PointerObjectRef;
        class PointerCancelRef;
        class ge_ptr;
        class PeerSymbo;
    }

    namespace __internal {
        class GC_Object{
        public:
            GC_Object() = default;
            virtual ~GC_Object() = default;

            virtual void manual_clear() = 0;
        };

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

        private:
            Type type;
        };

        class PointerOver: public Command
        {
        public:
            enum Type{
                NEW = Command::POINTER_NEW,
                DEL = Command::POINTER_DEL,
                CONNEC = Command::POINTER_OBJECTREF,
                CANCAL = Command::POINTER_CANCELREF
            };
            PointerOver(Type type, void* host, GC_Object* mgro, ge_ptr* ptr);
            virtual ~PointerOver() = default;

            void* host_object();
            GC_Object* delegate_object();
            ge_ptr *smart_pointer();

        private:
            ge_ptr*const ptr_mark;
            void*const host_ptr;
            GC_Object*const mgro;
        };

        class PointerRef: public PointerOver
        {
        public:
            enum Type{
                BUILD = Command::POINTER_OBJECTREF,
                CANCEL = Command::POINTER_CANCELREF
            };
            PointerRef(Type type, void* host, GC_Object* mgro, ge_ptr* ptr, void* target);
            virtual ~PointerRef() = default;

            void* target_pointer();

        private:
            void*const target;
        };


        class PeerSymbo{
        public:
            explicit PeerSymbo()
                :delegates({}){}

            std::list<GC_Object*> delegates;
            std::list<std::pair<ge_ptr*, void*>> members;
            std::list<ge_ptr*> ref_records;
        };

        class ge_ptr{
        public:
            explicit ge_ptr(void *host, GC_Object *delegate);
            ge_ptr(const ge_ptr& other);
            ge_ptr(ge_ptr&& other);
            virtual ~ge_ptr();


            ge_ptr &operator=(void* target);
            ge_ptr &operator=(const ge_ptr& other);
            void *operator->();

        private:
            void *const host_ptr;
            GC_Object *const host_delegate;
            void * target_ptr;
            static sync::BlockingQueue<Command*>* queue;
        };

        class GC_Worker : exec::Thread
        {
        public:
            static sync::BlockingQueue<Command*>&& commands;

            GC_Worker();
            ~GC_Worker() = default;

            void run();

        private:
            //       original-ptr  all-managed
            static std::map<void*, PeerSymbo*> objs_map;
            bool check_loop(std::list<void *> &achor, PeerSymbo* item);
        };

        template <typename T>
        class GC_Delegate : public GC_Object
        {
        public:
            GC_Delegate(T* host):obj(host){}
            virtual ~GC_Delegate(){}

            virtual void manual_clear(){
                if(obj) delete obj;
            }

        private:
            T* obj;
        };
    }

    extern __internal::GC_Delegate<int> global_object;

    template <typename T>
    class smart_ptr : __internal::ge_ptr
    {
    public:
        template<typename HostType>
        explicit smart_ptr(HostType* host)
            :ge_ptr(host, new __internal::GC_Delegate<HostType>(host)) {}
        smart_ptr(const smart_ptr<T>& other)
            :ge_ptr(other){}
        smart_ptr(smart_ptr<T>&& rv)
            :ge_ptr(rv){}

        virtual ~smart_ptr() = default;


        smart_ptr<T>& operator=(T* target)
        {
            __internal::ge_ptr::operator=(target);

            return *this;
        }
        smart_ptr<T>& operator=(const smart_ptr<T>& other){
            __internal::ge_ptr::operator=(other);

            return *this;
        }

        T* operator->(){
            return static_cast<T*>(__internal::ge_ptr::operator->());
        }
    };


    template <typename HostType, typename T>
    smart_ptr<T> gc_wrap(HostType*host, T* target){
        auto one = smart_ptr<T>(host);
        one = target;
        return one;
    }
}

#endif // GCOBJECT_H
