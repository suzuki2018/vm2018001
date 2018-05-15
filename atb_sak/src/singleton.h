#ifndef _SINGLETON_H_
#define _SINGLETON_H_

#include <functional>


template <class T>
class singleton
{
protected:
    singleton(){};
private:
    singleton(const singleton&){};
    singleton& operator=(const singleton&){};
    static T* m_instance;
public:
    template <typename... Args>
    static T* GetInstance(Args&&... args)
    {
        if(m_instance == nullptr)
            m_instance = new T(std::forward<Args>(args)...);
        return m_instance;
    }


    static void DestroyInstance()
    {
        if(m_instance )
            delete m_instance;
        m_instance = nullptr;
    }
};


template <class T>
T* singleton<T>::m_instance = nullptr;

#endif
