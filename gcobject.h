#ifndef GCOBJECT_H
#define GCOBJECT_H

#include <iostream>
#include <thread>
#include <list>
#include <map>
#include <algorithm>

namespace ws {
    // Pre-DECL
    namespace internal {
        class Command;
        class PointerNew;
        class PointerDel;
        class PointerObjectRef;
        class PointerCancelRef;
        class ge_ptr;
        class PeerSymbo;
    }
    class GC_Object;


    template<typename T>
    class BlockingQueue
    {
    public:
        explicit BlockingQueue(){}
        virtual ~BlockingQueue(){}

        void pull(T one){}
        T take(){}

    private:
        std::list<T> con;
    };

    namespace internal {
        class Command{
        public:
            enum Type{
                POINTER_NEW,
                POINTER_DEL,
                POINTER_OBJECTREF,
                POINTER_CANCELREF
            };
            explicit Command(Type type, ge_ptr* ptr, GC_Object* host);
            virtual ~Command() = default;

            Type getType();
            GC_Object* hostPointer();
            ge_ptr* ptrPointer();

        private:
            Type type;
            GC_Object*const host_ptr;
            ge_ptr* ptr;
        };

        class PointerNew: public Command{
        public:
            explicit PointerNew(GC_Object* host, ge_ptr*ptr);
            virtual ~PointerNew() = default;
        };

        class PointerDel: public Command{
        public:
            explicit PointerDel(GC_Object* host, ge_ptr*ptr);
            virtual ~PointerDel() = default;
        };

        class PointerObjectRef: public Command{
        public:
            explicit PointerObjectRef(GC_Object* host, ge_ptr*ptr, GC_Object* target);
            virtual ~PointerObjectRef() = default;

            GC_Object* targetPointer();
        private:
            GC_Object*const target;
        };

        class PointerCancelRef: public Command{
        public:
            explicit PointerCancelRef(GC_Object* host, ge_ptr*ptr, GC_Object* target);
            virtual ~PointerCancelRef() = default;

            GC_Object* targetPointer();
        private:
            GC_Object *const target;
        };

        class PeerSymbo{
        public:
            explicit PeerSymbo(GC_Object* host)
                :host_object(host){}

            GC_Object*const host_object;
            std::list<std::pair<internal::ge_ptr*, PeerSymbo*>> members;
            std::list<internal::ge_ptr*> handlers_record;
        };

        class ge_ptr{
        public:
            explicit ge_ptr(GC_Object* host);
            ge_ptr(ge_ptr& other);
            ge_ptr(ge_ptr&& other);
            virtual ~ge_ptr();


            ge_ptr &operator=(GC_Object* target);
            ge_ptr &operator=(ge_ptr& other);
            GC_Object *operator->();

        private:
            GC_Object*const host_ptr;
            GC_Object* target_ptr;
            static BlockingQueue<Command*>* queue;
        };

        class GC_Collect : public std::thread
        {
        public:
            static BlockingQueue<internal::Command*> commands;

            GC_Collect()
                :thread(fn_collect_calc)
            {
                this->detach();
            }
            ~GC_Collect() = default;


        private:
            static std::map<GC_Object*, internal::PeerSymbo*> objs_map;
            static void fn_collect_calc();
        };
    }

    class GC_Object{
    public:
        GC_Object() = default;
        virtual ~GC_Object() = default;
    };

    template <typename T>
    class smart_ptr : public internal::ge_ptr
    {
    public:
        explicit smart_ptr(GC_Object* host)
            :ge_ptr(host) {}
        smart_ptr(smart_ptr<T>& other)
            :ge_ptr (other){}
        smart_ptr(smart_ptr<T>&& rv)
            :ge_ptr(rv){}

        virtual ~smart_ptr() = default;


        smart_ptr<T>& operator=(T* target){
            ge_ptr::operator=(target);

            return *this;
        }
        smart_ptr<T>& operator=(smart_ptr<T>& other){
            ge_ptr::operator=(other);

            return *this;
        }

        T* operator->(){
            return static_cast<T*>(ge_ptr::operator->());
        }
    };
}

#endif // GCOBJECT_H
