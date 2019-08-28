#ifndef SHARED_PTR_HPP
#define SHARED_PTR_HPP

#include <pthread.h>
#include <assert.h>
#include <stdlib.h>

namespace cs540
{

//

class HelperClass{
public:
    //avoid optimization since it needs to be thread-safe
    volatile int refcount;
    pthread_mutex_t* lockMutex;
    //pthread_mutex_t* lockMutex;

    HelperClass(): refcount(0) {
        lockMutex = new pthread_mutex_t();
        assert(pthread_mutex_init(lockMutex, nullptr) == 0);
    }
    virtual ~HelperClass() {
        //assert(pthread_mutex_destroy(lockMutex) == 0);
        //delete lockMutex;
    }

    pthread_mutex_t* getLock() const { return lockMutex; }

};

template <typename U>
class Helper2Class: public HelperClass{
public:
    U* helperPtr;
    //using HelperClass::refcount;

    Helper2Class(): helperPtr(nullptr) {
        //HelperClass();
        //refcount = 0;
    }
    Helper2Class(U* tempPtr): helperPtr(tempPtr){
        refcount = 1;
    }
    virtual ~Helper2Class() {
        delete helperPtr;
        assert(pthread_mutex_destroy(HelperClass::lockMutex) == 0);
        delete HelperClass::lockMutex;
    }

    // void inc(){
    //     assert(pthread_mutex_lock(lockMutex) == 0);
    //     refcount++;
    //     assert(pthread_mutex_unlock(lockMutex) == 0);
    //
    // }
    // void dec(){
    //     assert(pthread_mutex_lock(lockMutex) == 0);
    //     refcount--;
    //     if(refcount == 0){
    //         assert(pthread_mutex_unlock(lockMutex) == 0);
    //         assert(pthread_mutex_destroy(lockMutex) == 0);
    //         delete lockMutex;
    //         delete helperPtr;
    //
    //         helperPtr = nullptr;
    //         objPtr = nullptr;
    //     } else{
    //         assert(pthread_mutex_unlock(lockMutex) == 0);
    //     }
    // }
};

template <typename T>
class SharedPtr{
private:
    T* objPtr;
    HelperClass* helperPtr;

public:
    //allow functions from implicityly convertible types
    template <typename U> friend class SharedPtr;
    //increment decrement functions
    void inc(){
        assert(pthread_mutex_lock(helperPtr->lockMutex) == 0);
        helperPtr->refcount++;
        assert(pthread_mutex_unlock(helperPtr->lockMutex) == 0);

    }
    void dec(){
        assert(pthread_mutex_lock(helperPtr->lockMutex) == 0);
        helperPtr->refcount--;
        if(helperPtr->refcount == 0){
            assert(pthread_mutex_unlock(helperPtr->lockMutex) == 0);
            delete helperPtr;

            helperPtr = nullptr;
            objPtr = nullptr;

        } else{
            assert(pthread_mutex_unlock(helperPtr->lockMutex) == 0);
        }
    }


    //Constructors
    //default constructor pointing to null
    SharedPtr(): objPtr(nullptr), helperPtr(nullptr){
        // lockMutex = new pthread_mutex_t();
        // assert(pthread_mutex_init(lockMutex, nullptr) == 0);
    }

    //constructor pointing to given object with refcount 1
    template <typename U>
    explicit SharedPtr(U *tmpObj): objPtr(tmpObj), helperPtr(new Helper2Class<U>(tmpObj)){}

    //constructor that increments refcount if p not null
    //and constructor for implicitly convertible types
    SharedPtr(const SharedPtr &p){
        if(p != nullptr){
            objPtr = p.objPtr;
            helperPtr = p.helperPtr;
            //lockMutex = p.getLock();
            inc();
        } else {
            objPtr = nullptr;
            helperPtr = nullptr;
        }
    }
    template <typename U>
    SharedPtr(const SharedPtr<U> &p){
        if(p != nullptr){
            objPtr = static_cast<T*>(p.objPtr);
            helperPtr = p.helperPtr;
            //lockMutex = p.getLock();
            inc();
        }
    }

    //Move (clearing p afterwards) without changing refcount
    //Move for implicitly convertible types
    SharedPtr(SharedPtr &&p){
        objPtr = p.objPtr;
        helperPtr = p.helperPtr;
        //lockMutex = p.getLock();
        p.objPtr = nullptr;
        p.helperPtr = nullptr;
        //delete p; no need to delete just clear it
    }
    template <typename U>
    SharedPtr(SharedPtr<U> &&p){
        objPtr = p.objPtr;
        helperPtr = p.helperPtr;
        //lockMutex = p.getLock();
        p.objPtr = nullptr;
        p.helperPtr = nullptr;
        //delete p; no need to delete
    }

    //decrement refcount or delete if 0
    ~SharedPtr(){
        if(helperPtr != nullptr)
            dec();
        helperPtr = nullptr;
        objPtr = nullptr;
        //lockMutex = nullptr;
    }

    //Operators
    //copy assignment decrementing refcount of current obj and
    //incrementing refcount of given obj
    SharedPtr &operator=(const SharedPtr &sptr){
        //Handle self assignment and if pointing to the same object
        //dont decrement just return
        if(sptr == *this || (sptr != nullptr && objPtr == sptr.objPtr)){
            return *this;
        } else{
            //decrement current obj
            if(helperPtr != nullptr){
                dec();
                helperPtr = nullptr;
                objPtr = nullptr;
                //lockMutex = nullptr;
            }
            //copy given obj
            if(sptr != nullptr){
                objPtr = sptr.objPtr;
                helperPtr = sptr.helperPtr;
                //lockMutex = sptr.getLock();
                //increment given obj
                inc();
            } else{
                objPtr = nullptr;
                helperPtr = nullptr;
                //lockMutex = nullptr;
            }
        }
        return *this;
    }
    template <typename U>
    SharedPtr<T> &operator=(const SharedPtr<U> &sptr){
        //Handle self assignment and if pointing to the same object
        //dont decrement just return
        if(sptr == *this || (sptr != nullptr && objPtr == sptr.objPtr)){
            return *this;
        } else{
            //decrement current obj
            if(helperPtr != nullptr){
                dec();
                helperPtr = nullptr;
                objPtr = nullptr;
                //lockMutex = nullptr;
            }
            //copy given obj
            if(sptr != nullptr){
                objPtr = sptr.objPtr;
                helperPtr = sptr.helperPtr;
                //lockMutex = sptr.getLock();
                //increment given obj
                inc();
            } else{
                objPtr = nullptr;
                helperPtr = nullptr;
                //lockMutex = nullptr;
            }
        }
        return *this;
    }

    //Move (clearing p afterwards), keeping refcount the same
    SharedPtr &operator=(SharedPtr &&p){
        //Handle self assignment and if pointing to the same object
        //dont decrement just return
        if(p == *this || (p != nullptr && objPtr == p.objPtr)){
            return *this;
        } else{
            //move given obj
            if(p != nullptr){
                objPtr = p.objPtr;
                helperPtr = p.helperPtr;
                //lockMutex = p.getLock();
                p.objPtr = nullptr;
                p.helperPtr = nullptr;

            } else{
                objPtr = nullptr;
                helperPtr = nullptr;
                //lockMutex = nullptr;
            }
        }
        return *this;
    }
    template <typename U>
    SharedPtr &operator=(SharedPtr<U> &&p){
        //Handle self assignment and if pointing to the same object
        //dont decrement just return
        if(p == *this || (p != nullptr && objPtr == p.objPtr)){
            return *this;
        } else{
            //move given obj
            if(p != nullptr){
                objPtr = p.objPtr;
                helperPtr = p.helperPtr;
                //lockMutex = p.getLock();
                p.objPtr = nullptr;
                p.helperPtr = nullptr;

            } else{
                objPtr = nullptr;
                helperPtr = nullptr;
            }
        }
        return *this;
    }

    //return ref to obj
    T &operator*() const{
        return *objPtr;
    }
    //return ptr to obj
    T *operator->() const{
        return objPtr;
    }
    //return true if not null
    explicit operator bool() const{
        return (objPtr != nullptr);
    }

    //Modifiers
    //set smartptr to point to nullptr and decrement refcount of obj
    void reset(){
        if(helperPtr != nullptr){
            dec();
        }
        objPtr = nullptr;
        helperPtr = nullptr;
        //lockMutex = nullptr;

    }
    //replace with another ptr, if current has no other ref then delete
    template <typename U>
    void reset(U *p){
        if(helperPtr != nullptr){
            dec();
        }
        objPtr = nullptr;
        helperPtr = nullptr;
        //lockMutex = nullptr;

        helperPtr = new Helper2Class<U>(p);
        objPtr = p;
        //lockMutex = new pthread_mutex_t();
        //assert(pthread_mutex_init(lockMutex, nullptr) == 0);

    }

    //template <typename U>
    //void setHelper(SharedPtr<U> &sp) const{
    //    helperPtr = sp.helperPtr;
    //}

    //Observers (minus operators)
    //returns pointer to obj
    T *get() const{
        return objPtr;
    }

    void set(T* ptr){
        objPtr = ptr;
    }

};

//Free functions
//remember objPtr is private so replace all of it with get()
//true if both ptrs point to same obj or if both point to null
template <typename T1, typename T2>
bool operator==(const SharedPtr<T1> &sptr1, const SharedPtr<T2> &sptr2){
    //return(sptr1.objPtr == sptr2.objPtr);
    //objPtr is private
    return(sptr1.get() == sptr2.get());
}

//Compare against nullptr
template <typename T>
bool operator==(const SharedPtr<T> &sptr, std::nullptr_t nptr){
    return(sptr.get() == nptr);
}
template <typename T>
bool operator==(std::nullptr_t nptr, const SharedPtr<T> &sptr){
    return(sptr.get() == nptr);
}

//true if both ptrs point to diff objs or if only one points to null
template <typename T1, typename T2>
bool operator!=(const SharedPtr<T1> &sptr1, const SharedPtr<T2> &sptr2){
    if((sptr1.get() == nullptr && sptr2.get() != nullptr) || (sptr2.get() == nullptr && sptr1.get() != nullptr)){
        return true;
    }
    return(sptr1.get() != sptr2.get());
}

//compare against nullptr
template <typename T>
bool operator!=(const SharedPtr<T> &sptr, const std::nullptr_t& nptr){
    return(sptr.get() != nptr);
}
template <typename T>
bool operator!=(const std::nullptr_t& nptr, const SharedPtr<T> &sptr){
    return(sptr.get() != nptr);
}

//convert sp using static_cast to cast the contained ptr
template <typename T, typename U>
SharedPtr<T> static_pointer_cast(const SharedPtr<U> &sp){
    //SharedPtr<T> tmpPtr(sp);
    //couldn't use sp.objPtr because it is private
    SharedPtr<T> tempPtr(sp);
    tempPtr.set(static_cast<T*>(sp.get()));
    return tempPtr;

}

//convert sp using dynamic_cast to the contained ptr
template <typename T, typename U>
SharedPtr<T> dynamic_pointer_cast(const SharedPtr<U> &sp){
    //SharedPtr<T> tmpPtr(sp);
    //couldn't use sp.objPtr because it is private
    //T* tempPtr = dynamic_cast<T*>(sp.get());

    SharedPtr<T> tempPtr(sp);
    tempPtr.set(dynamic_cast<T*>(sp.get()));
    return tempPtr;
}


}

#endif
