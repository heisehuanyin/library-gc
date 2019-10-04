#ifndef GCOBJECT_H
#define GCOBJECT_H

#include <list>
#include <iostream>
#include <algorithm>

namespace ws {
    namespace __inner {
        class middle_ptr;
    }

    class GCObject
    {
        friend __inner::middle_ptr;
    public:
        explicit GCObject(){}
        virtual ~GCObject() = default;

    private:
        /**
         * @brief 记录对指针对象的解引用操作
         * @param item 指针对象
         */
        void ws_object_reference_decreasement(__inner::middle_ptr& item){
            auto it = std::find(pointer_list.cbegin(), pointer_list.cend(), &item);
            if(it != pointer_list.cend()){
                pointer_list.remove(&item);
            }

            if(!pointer_list.size()){
                delete this;
            }
        }
        /**
         * @brief 记录对指针对象的引用操作
         * @param item 指针对象
         */
        void ws_object_reference_increasement(__inner::middle_ptr& item){
            auto it = std::find(pointer_list.cbegin(), pointer_list.cend(), &item);
            if(it == pointer_list.cend()){
                pointer_list.push_back(&item);
            }
        }
        /**
         * @brief 记录对象实例范围中成员指针注册
         * @param item 成员指针
         */
        void ws_object_register_member(__inner::middle_ptr& item){
            auto it = std::find(member_list.cbegin(), member_list.cend(), &item);
            if(it == member_list.cend()){
                member_list.push_back(&item);
            }
        }
        /**
         * @brief 记录对象实例范围内成员指针反注册
         * @param item 成员指针
         */
        void ws_object_unregister_member(__inner::middle_ptr& item){
            auto it = std::find(member_list.cbegin(), member_list.cend(), &item);
            if(it != member_list.cend()){
                member_list.remove(&item);
            }
        }

        std::list<__inner::middle_ptr*> pointer_list;
        std::list<__inner::middle_ptr*> member_list;
    };

    namespace __inner {
        class middle_ptr {
        protected:
            explicit middle_ptr(GCObject* host)
                :object_host(host),
                  object_target(nullptr){
                host->ws_object_register_member(*this);
            }
            explicit middle_ptr(GCObject* host, GCObject* target)
                :middle_ptr(host){
                this->operator=(target);
            }
            virtual ~middle_ptr(){
                if(object_target != nullptr)
                    object_target->ws_object_reference_decreasement(*this);

                object_host->ws_object_unregister_member(*this);
            }

            middle_ptr(middle_ptr& other){
                this->operator=(other);
            }
            middle_ptr(middle_ptr&& other){
                this->operator=(other);
            }

            virtual middle_ptr& operator=(GCObject* target){
                if(object_target != nullptr)
                    object_target->ws_object_reference_decreasement(*this);

                object_target = target;
                object_target->ws_object_reference_increasement(*this);

                return *this;
            }
            virtual middle_ptr& operator=(middle_ptr& other){
                this->object_host = other.object_host;
                this->object_target = other.object_target;

                return *this;
            }
            virtual GCObject* operator->(){
                if(!object_target){
                    std::cout << "对nullptr指针进行操作"<< std::endl;
                    exit(0);
                }
                return object_target;
            }

        private:
            GCObject* object_host;
            GCObject* object_target;
        };
    }

    template<typename T>
    class auto_ptr final : public __inner::middle_ptr{
    public:
        explicit auto_ptr(GCObject* host)
            :__inner::middle_ptr (host){}

        explicit auto_ptr(GCObject*host, GCObject* target)
            :auto_ptr(host){
            this->operator=(target);
        }
        virtual ~auto_ptr() = default;

        auto_ptr(auto_ptr<T>& other){
            this->operator=(other);
        }
        auto_ptr(auto_ptr<T> && other){
            this->operator=(other);
        }



        virtual auto_ptr<T>& operator=(GCObject* target){
            __inner::middle_ptr::operator=(target);
            return *this;
        }

        virtual auto_ptr<T>& operator=(auto_ptr& other){
            __inner::middle_ptr::operator=(other);
            return *this;
        }

        virtual T* operator->(){
            return static_cast<T*>(__inner::middle_ptr::operator->());
        }

    };




}

#endif // GCOBJECT_H
