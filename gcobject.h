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
    class GC_Object;

    namespace __internal {
        class Command{
        public:
            enum Type{
                POINTER_NEW,
                POINTER_DEL,
                POINTER_OBJECTREF,
                POINTER_CANCELREF,
                OBJECT_NEW,
                OBJECT_DEL
            };
            explicit Command(Type type, GC_Object* host);
            virtual ~Command() = default;

            Type commandType();
            GC_Object* hostObject();

        private:
            Type type;
            GC_Object*const host_ptr;
        };

        class PointerOver: public Command
        {
        public:
            enum Type{
                NEW = Command::POINTER_NEW,
                DEL = Command::POINTER_DEL,
                REFER = Command::POINTER_OBJECTREF,
                CANCAL = Command::POINTER_CANCELREF
            };
            PointerOver(Type type, GC_Object* host, ge_ptr* ptr);
            virtual ~PointerOver() = default;

            ge_ptr *pointer();

        private:
            ge_ptr*const ptr_mark;
        };

        class PointerRef: public PointerOver
        {
        public:
            enum Type{
                BUILD = Command::POINTER_OBJECTREF,
                CANCEL = Command::POINTER_CANCELREF
            };
            PointerRef(Type type, GC_Object* host, ge_ptr* ptr, GC_Object* target);
            virtual ~PointerRef() = default;

            GC_Object* targetPointer();

        private:
            GC_Object*const target;
        };

        class ObjectOver : public Command
        {
        public:
            enum Type{
                New = Command::OBJECT_NEW,
                Del = Command::OBJECT_DEL
            };
            ObjectOver(Type type, GC_Object* obj);
            virtual ~ObjectOver() = default;
        };

        class PeerSymbo{
        public:
            explicit PeerSymbo();

            std::list<std::pair<ge_ptr*, GC_Object*>> members;
            std::list<ge_ptr*> ref_records;
        };

        class ge_ptr{
        public:
            explicit ge_ptr(GC_Object* host);
            ge_ptr(const ge_ptr& other);
            ge_ptr(ge_ptr&& other);
            virtual ~ge_ptr();


            ge_ptr &operator=(GC_Object* target);
            ge_ptr &operator=(const ge_ptr& other);
            GC_Object *operator->();

        private:
            GC_Object*const host_ptr;
            GC_Object* target_ptr;
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
            static std::map<GC_Object*, PeerSymbo*> objs_map;
            bool check_loop(GC_Object* achor, PeerSymbo* item);
        };
    }

    class GC_Object{
    public:
        GC_Object() = default;
        virtual ~GC_Object() = default;
    };

    template <typename T>
    class smart_ptr : public __internal::ge_ptr
    {
    public:
        explicit smart_ptr(GC_Object* host)
            :ge_ptr(host) {}
        smart_ptr(const smart_ptr<T>& other)
            :ge_ptr (other){}
        smart_ptr(smart_ptr<T>&& rv)
            :ge_ptr(rv){}

        virtual ~smart_ptr() = default;


        smart_ptr<T>& operator=(GC_Object* target)
        {
            ge_ptr::operator=(target);

            return *this;
        }
        smart_ptr<T>& operator=(const smart_ptr<T>& other){
            ge_ptr::operator=(other);

            return *this;
        }

        T* operator->(){
            return static_cast<T*>(ge_ptr::operator->());
        }
    };
}

#endif // GCOBJECT_H
