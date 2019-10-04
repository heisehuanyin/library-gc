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
        explicit GCObject();
        virtual ~GCObject() = default;

    private:
        /**
         * @brief 记录对引用记录-1操作
         * @param item 指针对象
         */
        void ws_object_reference_decreasement(__inner::middle_ptr& item);
        /**
         * @brief 记录对引用记录+1操作
         * @param item 指针对象
         */
        void ws_object_reference_increasement(__inner::middle_ptr& item);

        /**
         * @brief 记录对象实例范围中成员指针注册
         * @param item 成员指针
         */
        void ws_object_register_member(__inner::middle_ptr& item);
        /**
         * @brief 记录对象实例范围内成员指针反注册
         * @param item 成员指针
         */
        void ws_object_unregister_member(__inner::middle_ptr& item);

        std::list<__inner::middle_ptr*> pointer_list;
        std::list<__inner::middle_ptr*> member_list;
    };

    namespace __inner {
        class middle_ptr
        {
            friend GCObject;
        protected:
            explicit middle_ptr(GCObject* host);
            explicit middle_ptr(GCObject* host, GCObject* target);
            explicit middle_ptr(const middle_ptr& other);
            virtual ~middle_ptr();


            virtual middle_ptr &operator=(middle_ptr& other);
            virtual middle_ptr &operator=(GCObject* target);
            virtual GCObject *operator->();

        private:
            GCObject* object_host;
            GCObject* object_target;

            /**
             * @brief 查找实例引用回环
             * @param ins 实例
             * @return 引用环指示
             */
            bool circulate_for_loop(GCObject *ins);
        };
    }

    /**
     * @brief 唯一全局对象，适用于非类成员的纯函数中使用智能指针，
     * 将host参数关联到全局对象指针，仅用于拼凑参数。
     * 在类内部范围使用的时候直接传递类实例指针（this）作为host。
     * 这一步很关键，是能否正常进行内存实例管理的关键步骤。
     */
    static GCObject default_global_object;

    template<typename T>
    class auto_ptr final : public __inner::middle_ptr{
    public:
        explicit auto_ptr(GCObject* host=&default_global_object)
            :__inner::middle_ptr (host){}

        explicit auto_ptr(GCObject*host, GCObject* target)
            :auto_ptr(host){
            this->operator=(target);
        }
        virtual ~auto_ptr() = default;

        explicit auto_ptr(const auto_ptr<T>& other){
            this->operator=(other);
        }

        explicit auto_ptr(const auto_ptr<T> && other){
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
