////////////////////////////////////////////////////////////////////////////////
// The Loki Library
// Copyright (c) 2001 by Andrei Alexandrescu
// This code accompanies the book:
// Alexandrescu, Andrei. "Modern C++ Design: Generic Programming and Design 
//     Patterns Applied". Copyright (c) 2001. Addison-Wesley.
// Permission to use, copy, modify, distribute and sell this software for any 
//     purpose is hereby granted without fee, provided that the above copyright 
//     notice appear in all copies and that both that copyright notice and this 
//     permission notice appear in supporting documentation.
// The author or Addison-Wesley Longman make no representations about the 
//     suitability of this software for any purpose. It is provided "as is" 
//     without express or implied warranty.
////////////////////////////////////////////////////////////////////////////////

#ifndef LOKI_THREADS_H_
#define LOKI_THREADS_H_

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////

// $Header: /cvsroot/loki-lib/loki/include/loki/Threads.h,v 1.29 2006/06/04 20:18:39 vizowl Exp $

///  @defgroup  ThreadingGroup Threading
///  Policies to for the threading model:
///
///  - SingleThreaded
///  - ObjectLevelLockable
///  - ClassLevelLockable
///
///  All classes in Loki have configurable threading model.
///
///  The macro LOKI_DEFAULT_THREADING selects the default 
///  threading model for certain components of Loki 
///  (it affects only default template arguments)
///  
///  \par Usage:
/// 
///  To use a specific threading model define
///
///  - nothing, single-theading is default
///  - LOKI_OBJECT_LEVEL_THREADING for object-level-threading
///  - LOKI_CLASS_LEVEL_THREADING for class-level-threading
///
///  \par Supported platfroms:
///
///  - Windows (windows.h)
///  - POSIX (pthread.h)


#include <cassert>

#if defined(LOKI_CLASS_LEVEL_THREADING) || defined(LOKI_OBJECT_LEVEL_THREADING)

    #define LOKI_DEFAULT_THREADING_NO_OBJ_LEVEL ::Loki::ClassLevelLockable
    
    #if defined(LOKI_CLASS_LEVEL_THREADING) && !defined(LOKI_OBJECT_LEVEL_THREADING)
        #define LOKI_DEFAULT_THREADING ::Loki::ClassLevelLockable
    #else
        #define LOKI_DEFAULT_THREADING ::Loki::ObjectLevelLockable
    #endif
     
    #if defined(_WIN32)
        #include <windows.h> 
    #else
        #include <pthread.h>
    #endif
    
#else

    #define LOKI_DEFAULT_THREADING ::Loki::SingleThreaded
    #define LOKI_DEFAULT_THREADING_NO_OBJ_LEVEL ::Loki::SingleThreaded
    
#endif
    
#ifndef LOKI_DEFAULT_MUTEX
#define LOKI_DEFAULT_MUTEX ::Loki::Mutex
#endif

#if defined(_WINDOWS_) || defined(_WINDOWS_H) 

#define LOKI_THREADS_MUTEX(x)           CRITICAL_SECTION x
#define LOKI_THREADS_MUTEX_INIT(x)      ::InitializeCriticalSection x
#define LOKI_THREADS_MUTEX_DELETE(x)    ::DeleteCriticalSection x
#define LOKI_THREADS_MUTEX_LOCK(x)      ::EnterCriticalSection x
#define LOKI_THREADS_MUTEX_UNLOCK(x)    ::LeaveCriticalSection x
#define LOKI_THREADS_LONG               LONG

#define LOKI_THREADS_ATOMIC_FUNCTIONS                                   \
        static IntType AtomicIncrement(volatile IntType& lval)          \
        { return InterlockedIncrement(&const_cast<IntType&>(lval)); }   \
                                                                        \
        static IntType AtomicDecrement(volatile IntType& lval)          \
        { return InterlockedDecrement(&const_cast<IntType&>(lval)); }   \
                                                                        \
        static void AtomicAssign(volatile IntType& lval, IntType val)   \
        { InterlockedExchange(&const_cast<IntType&>(lval), val); }      \
                                                                        \
        static void AtomicAssign(IntType& lval, volatile IntType& val)  \
        { InterlockedExchange(&lval, val); }



#elif defined(_PTHREAD_H) || defined(_POSIX_PTHREAD_H) //POSIX threads (pthread.h)


#define LOKI_THREADS_MUTEX(x)           pthread_mutex_t x
#define LOKI_THREADS_MUTEX_INIT(x)      ::pthread_mutex_init(x,0)
#define LOKI_THREADS_MUTEX_DELETE(x)    ::pthread_mutex_destroy x
#define LOKI_THREADS_MUTEX_LOCK(x)      ::pthread_mutex_lock x
#define LOKI_THREADS_MUTEX_UNLOCK(x)    ::pthread_mutex_unlock x
#define LOKI_THREADS_LONG               long

#define LOKI_THREADS_ATOMIC(x)                                           \
                pthread_mutex_lock(&atomic_mutex_);                      \
                x;                                                       \
                pthread_mutex_unlock(&atomic_mutex_)    
                
#define LOKI_THREADS_ATOMIC_FUNCTIONS                                    \
        private:                                                         \
            static pthread_mutex_t atomic_mutex_;                        \
        public:                                                          \
        static IntType AtomicIncrement(volatile IntType& lval)           \
        { LOKI_THREADS_ATOMIC( lval++ ); return lval; }                  \
                                                                         \
        static IntType AtomicDecrement(volatile IntType& lval)           \
        { LOKI_THREADS_ATOMIC(lval-- ); return lval; }                   \
                                                                         \
        static void AtomicAssign(volatile IntType& lval, IntType val)    \
        { LOKI_THREADS_ATOMIC( lval = val ); }                           \
                                                                         \
        static void AtomicAssign(IntType& lval, volatile IntType& val)   \
        { LOKI_THREADS_ATOMIC( lval = val ); }            

#else // single threaded

#define LOKI_THREADS_MUTEX(x)
#define LOKI_THREADS_MUTEX_INIT(x)      
#define LOKI_THREADS_MUTEX_DELETE(x)       
#define LOKI_THREADS_MUTEX_LOCK(x)         
#define LOKI_THREADS_MUTEX_UNLOCK(x)       
#define LOKI_THREADS_LONG               

#endif



namespace Loki
{

    ////////////////////////////////////////////////////////////////////////////////
    ///  \class Mutex
    //
    ///  \ingroup ThreadingGroup
    ///  A simple and portable Mutex.  A default policy class for locking objects.
    ////////////////////////////////////////////////////////////////////////////////

    class Mutex
    {
    public:
        Mutex()
        {
            LOKI_THREADS_MUTEX_INIT( (&mtx_) );
        }
        ~Mutex()
        {
            LOKI_THREADS_MUTEX_DELETE( (&mtx_) );
        }
        void Lock()
        {
            LOKI_THREADS_MUTEX_LOCK( (&mtx_) );
        }
        void Unlock()
        {
            LOKI_THREADS_MUTEX_UNLOCK( (&mtx_) );
        }
    private:
        /// Copy-constructor not implemented.
        Mutex( const Mutex & );
        /// Copy-assignement operator not implemented.
        Mutex & operator = ( const Mutex & );
        LOKI_THREADS_MUTEX( mtx_; )
    };


     ////////////////////////////////////////////////////////////////////////////////
    ///  \class SingleThreaded
    ///
    ///  \ingroup ThreadingGroup
    ///  Implementation of the ThreadingModel policy used by various classes
    ///  Implements a single-threaded model; no synchronization
    ////////////////////////////////////////////////////////////////////////////////
    template <class Host, class MutexPolicy = LOKI_DEFAULT_MUTEX>
    class SingleThreaded
    {
    public:
        /// \struct Lock
        /// Dummy Lock class
        struct Lock
        {
            Lock() {}
            explicit Lock(const SingleThreaded&) {}
            explicit Lock(const SingleThreaded*) {}
        };
        
        typedef Host VolatileType;

        typedef int IntType; 

        static IntType AtomicAdd(volatile IntType& lval, IntType val)
        { return lval += val; }
        
        static IntType AtomicSubtract(volatile IntType& lval, IntType val)
        { return lval -= val; }

        static IntType AtomicMultiply(volatile IntType& lval, IntType val)
        { return lval *= val; }
        
        static IntType AtomicDivide(volatile IntType& lval, IntType val)
        { return lval /= val; }
        
        static IntType AtomicIncrement(volatile IntType& lval)
        { return ++lval; }
        
        static IntType AtomicDecrement(volatile IntType& lval)
        { return --lval; }
        
        static void AtomicAssign(volatile IntType & lval, IntType val)
        { lval = val; }
        
        static void AtomicAssign(IntType & lval, volatile IntType & val)
        { lval = val; }
    };
    

#if defined(_WINDOWS_) || defined(_WINDOWS_H) || defined(_PTHREAD_H) || defined(_POSIX_PTHREAD_H) 

    ////////////////////////////////////////////////////////////////////////////////
    ///  \class ObjectLevelLockable
    ///
    ///  \ingroup ThreadingGroup
    ///  Implementation of the ThreadingModel policy used by various classes
    ///  Implements a object-level locking scheme
    ////////////////////////////////////////////////////////////////////////////////
    template < class Host, class MutexPolicy = LOKI_DEFAULT_MUTEX >
    class ObjectLevelLockable
    {
        mutable MutexPolicy mtx_;

    public:
        ObjectLevelLockable() : mtx_() {}

        ObjectLevelLockable(const ObjectLevelLockable&) : mtx_() {}

        ~ObjectLevelLockable() {}

        class Lock;
        friend class Lock;
        
        ///  \struct Lock
        ///  Lock class to lock on object level
        class Lock
        { 
        public:
            
            /// Lock object
            explicit Lock(const ObjectLevelLockable& host) : host_(host)
            {
                host_.mtx_.Lock();
            }

            /// Lock object
            explicit Lock(const ObjectLevelLockable* host) : host_(*host)
            {
                host_.mtx_.Lock();
            }

            /// Unlock object
            ~Lock()
            {
                host_.mtx_.Unlock();
            }

        private:
            /// private by design of the object level threading
            Lock();
            Lock(const Lock&);
            Lock& operator=(const Lock&);
            const ObjectLevelLockable& host_;
        };

        typedef volatile Host VolatileType;

        typedef LOKI_THREADS_LONG IntType; 
        
        LOKI_THREADS_ATOMIC_FUNCTIONS   
        
    };

#if defined(_PTHREAD_H) || defined(_POSIX_PTHREAD_H) 
    template <class Host, class MutexPolicy>
    pthread_mutex_t ObjectLevelLockable<Host, MutexPolicy>::atomic_mutex_ = PTHREAD_MUTEX_INITIALIZER;
#endif
    
    ////////////////////////////////////////////////////////////////////////////////
    ///  \class ClassLevelLockable
    ///
    ///  \ingroup ThreadingGroup
    ///  Implementation of the ThreadingModel policy used by various classes
    ///  Implements a class-level locking scheme
    ////////////////////////////////////////////////////////////////////////////////
    template <class Host, class MutexPolicy = LOKI_DEFAULT_MUTEX >
    class ClassLevelLockable
    {
        struct Initializer
        {   
            bool init_;
            MutexPolicy mtx_;

            Initializer() : init_(false), mtx_()
            {
                init_ = true;
            }
            ~Initializer()
            {
                assert(init_);
            }
        };

        static Initializer initializer_;

    public:

        class Lock;
        friend class Lock;

        ///  \struct Lock
        ///  Lock class to lock on class level
        class Lock
        {    
        public:

            /// Lock class
            Lock()
            {
                assert(initializer_.init_);
                initializer_.mtx_.Lock();
            }

            /// Lock class
            explicit Lock(const ClassLevelLockable&)
            {
                assert(initializer_.init_);
                initializer_.mtx_.Lock();
            }

            /// Lock class
            explicit Lock(const ClassLevelLockable*)
            {
                assert(initializer_.init_);
                initializer_.mtx_.Lock();
            }

            /// Unlock class
            ~Lock()
            {
                assert(initializer_.init_);
                initializer_.mtx_.Unlock();
            }

        private:
            Lock(const Lock&);
            Lock& operator=(const Lock&);
        };

        typedef volatile Host VolatileType;

        typedef LOKI_THREADS_LONG IntType; 

        LOKI_THREADS_ATOMIC_FUNCTIONS
        
    };

#if defined(_PTHREAD_H) || defined(_POSIX_PTHREAD_H) 
    template <class Host, class MutexPolicy>
    pthread_mutex_t ClassLevelLockable<Host, MutexPolicy>::atomic_mutex_ = PTHREAD_MUTEX_INITIALIZER;
#endif

    template < class Host, class MutexPolicy >
    typename ClassLevelLockable< Host, MutexPolicy >::Initializer 
    ClassLevelLockable< Host, MutexPolicy >::initializer_;

#endif // defined(_WINDOWS_) || defined(_WINDOWS_H) || defined(_PTHREAD_H) || defined(_POSIX_PTHREAD_H) 
  
} // namespace Loki



////////////////////////////////////////////////////////////////////////////////
// Change log:
// June 20, 2001: ported by Nick Thurn to gcc 2.95.3. Kudos, Nick!!!
// July 26, 2005: some asserts by Peter Kümmel
////////////////////////////////////////////////////////////////////////////////


#endif

// $Log: Threads.h,v $
// Revision 1.29  2006/06/04 20:18:39  vizowl
// added a check for _POSIX_PTHREAD_H to detect pthread.h on OS X
//
// Revision 1.28  2006/04/15 00:39:50  rich_sposato
// Stylistic change so I can put breakpoint on lines within Mutex functions.
//
// Revision 1.27  2006/01/22 15:33:41  syntheticpp
// a -pedantic fix
//
// Revision 1.26  2006/01/22 13:37:33  syntheticpp
// use macro LOKI_DEFAULT_MUTEX for Mutex default value, defined in Threads.h
//
// Revision 1.25  2006/01/22 00:32:29  syntheticpp
// add dummy Mutex for single threading and additional template parameter
//
// Revision 1.24  2006/01/21 14:11:09  syntheticpp
// complete usage of Loki::Mutex, gcc can't compile without these corrections
//
// Revision 1.23  2006/01/21 13:09:45  syntheticpp
// don't compile Mutex when multi threading is disabled
//
// Revision 1.22  2006/01/21 01:05:44  rich_sposato
// Removed volatile keyword from 2 data members.
//
// Revision 1.21  2006/01/21 01:02:12  rich_sposato
// Added Mutex class to Loki.  Made it the default policy class for locking.
//
// Revision 1.20  2006/01/16 19:05:09  rich_sposato
// Added cvs keywords.
//
// Revision 1.19  2005/11/15 11:08:57  syntheticpp
// also compile with the original gnu pthread.h
//
// Revision 1.18  2005/11/03 12:43:35  syntheticpp
// more doxygen documentation, modules added
//
// Revision 1.17  2005/11/02 20:01:10  syntheticpp
// more doxygen documentation, modules added
//
// Revision 1.16  2005/10/30 14:03:23  syntheticpp
// replace tabs space
//
// Revision 1.15  2005/10/24 20:35:12  syntheticpp
// small changes for Threads; add compile test for Threads.h
//
// Revision 1.14  2005/10/24 15:05:24  syntheticpp
// adding support for POSIX threads (pthreads.h), Thanks to Ilya Volvovski
//
// Revision 1.13  2005/09/26 07:33:04  syntheticpp
// move macros into LOKI_ namespace
//
// Revision 1.12  2005/07/31 14:00:48  syntheticpp
// make object level threading possible
//
// Revision 1.11  2005/07/28 14:26:09  syntheticpp
// add cvs Header/Log
//

