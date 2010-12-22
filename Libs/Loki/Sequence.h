////////////////////////////////////////////////////////////////////////////////
// The Loki Library
// Copyright (c) 2005 by Peter Kümmel
// Permission to use, copy, modify, distribute and sell this software for any 
//     purpose is hereby granted without fee, provided that the above copyright 
//     notice appear in all copies and that both that copyright notice and this 
//     permission notice appear in supporting documentation.
// The author makes no representations about the 
//     suitability of this software for any purpose. It is provided "as is" 
//     without express or implied warranty.
////////////////////////////////////////////////////////////////////////////////


#ifndef LOKI_SEQUENCE_INC_
#define LOKI_SEQUENCE_INC_

// $Header: /cvsroot/loki-lib/loki/include/loki/Sequence.h,v 1.3 2006/01/16 19:05:09 rich_sposato Exp $

#include "Typelist.h"

namespace Loki
{

    template
    <
        class T01=NullType,class T02=NullType,class T03=NullType,class T04=NullType,class T05=NullType,
        class T06=NullType,class T07=NullType,class T08=NullType,class T09=NullType,class T10=NullType,
        class T11=NullType,class T12=NullType,class T13=NullType,class T14=NullType,class T15=NullType,
        class T16=NullType,class T17=NullType,class T18=NullType,class T19=NullType,class T20=NullType
    >
    struct Seq;

    template<class T01>
    struct Seq<T01>
    {
        typedef 
        Typelist<T01,
        NullType
        > 
        Type;
    };
    template<class T01,class T02>
    struct Seq<T01,T02>
    {
        typedef 
        Typelist<T01,
        Typelist<T02,
        NullType
        > > 
        Type;
    };
    template<class T01,class T02,class T03>
    struct Seq<T01,T02,T03>
    {
        typedef 
        Typelist<T01,
        Typelist<T02,
        Typelist<T03,
        NullType
        > > >
        Type;
    };
    template<class T01,class T02,class T03,class T04>
    struct Seq<T01,T02,T03,T04>
    {
        typedef 
        Typelist<T01,
        Typelist<T02,
        Typelist<T03,
        Typelist<T04,
        NullType
        > > > >
        Type;
    };
    template<class T01,class T02,class T03,class T04,class T05>
    struct Seq<T01,T02,T03,T04,T05>
    {
        typedef 
        Typelist<T01,
        Typelist<T02,
        Typelist<T03,
        Typelist<T04,
        Typelist<T05,
        NullType
        > > > > >
        Type;
    };    
    template<class T01,class T02,class T03,class T04,class T05,
             class T06>
    struct Seq< T01,T02,T03,T04,T05,
                T06>
    {
        typedef 
        Typelist<T01,
        Typelist<T02,
        Typelist<T03,
        Typelist<T04,
        Typelist<T05,
        Typelist<T06,
        NullType
        > > > > >
        >
        Type;
    };    
    template<class T01,class T02,class T03,class T04,class T05,
             class T06,class T07>
    struct Seq< T01,T02,T03,T04,T05,
                T06,T07>
    {
        typedef 
        Typelist<T01,
        Typelist<T02,
        Typelist<T03,
        Typelist<T04,
        Typelist<T05,
        Typelist<T06,
        Typelist<T07,
        NullType
        > > > > >
        > >
        Type;
    };    
    template<class T01,class T02,class T03,class T04,class T05,
             class T06,class T07,class T08>
    struct Seq< T01,T02,T03,T04,T05,
                T06,T07,T08>
    {
        typedef 
        Typelist<T01,
        Typelist<T02,
        Typelist<T03,
        Typelist<T04,
        Typelist<T05,
        Typelist<T06,
        Typelist<T07,
        Typelist<T08,
        NullType
        > > > > >
        > > >
        Type;
    };    
    template<class T01,class T02,class T03,class T04,class T05,
             class T06,class T07,class T08,class T09>
    struct Seq< T01,T02,T03,T04,T05,
                T06,T07,T08,T09>
    {
        typedef 
        Typelist<T01,
        Typelist<T02,
        Typelist<T03,
        Typelist<T04,
        Typelist<T05,
        Typelist<T06,
        Typelist<T07,
        Typelist<T08,
        Typelist<T09,
        NullType
        > > > > >
        > > > > 
        Type;
    };    
    template<class T01,class T02,class T03,class T04,class T05,
             class T06,class T07,class T08,class T09,class T10>
    struct Seq< T01,T02,T03,T04,T05,
                T06,T07,T08,T09,T10>
    {
        typedef 
        Typelist<T01,
        Typelist<T02,
        Typelist<T03,
        Typelist<T04,
        Typelist<T05,
        Typelist<T06,
        Typelist<T07,
        Typelist<T08,
        Typelist<T09,
        Typelist<T10,
        NullType
        > > > > >
        > > > > >
        Type;
    };
    template<class T01,class T02,class T03,class T04,class T05,
             class T06,class T07,class T08,class T09,class T10,
             class T11>
    struct Seq< T01,T02,T03,T04,T05,
                T06,T07,T08,T09,T10,
                T11>
    {
        typedef 
        Typelist<T01,
        Typelist<T02,
        Typelist<T03,
        Typelist<T04,
        Typelist<T05,
        Typelist<T06,
        Typelist<T07,
        Typelist<T08,
        Typelist<T09,
        Typelist<T10,
        Typelist<T11,
        NullType
        > > > > >
        > > > > >
        > 
        Type;
    };    
    template<class T01,class T02,class T03,class T04,class T05,
             class T06,class T07,class T08,class T09,class T10,
             class T11,class T12>
    struct Seq< T01,T02,T03,T04,T05,
                T06,T07,T08,T09,T10,
                T11,T12>
    {
        typedef 
        Typelist<T01,
        Typelist<T02,
        Typelist<T03,
        Typelist<T04,
        Typelist<T05,
        Typelist<T06,
        Typelist<T07,
        Typelist<T08,
        Typelist<T09,
        Typelist<T10,
        Typelist<T11,
        Typelist<T12,
        NullType
        > > > > >
        > > > > >
        > > 
        Type;
    };    
    template<class T01,class T02,class T03,class T04,class T05,
             class T06,class T07,class T08,class T09,class T10,
             class T11,class T12,class T13>
    struct Seq< T01,T02,T03,T04,T05,
                T06,T07,T08,T09,T10,
                T11,T12,T13>
    {
        typedef 
        Typelist<T01,
        Typelist<T02,
        Typelist<T03,
        Typelist<T04,
        Typelist<T05,
        Typelist<T06,
        Typelist<T07,
        Typelist<T08,
        Typelist<T09,
        Typelist<T10,
        Typelist<T11,
        Typelist<T12,
        Typelist<T13,

        NullType
        > > > > >
        > > > > >
        > > >
        Type;
    };    
    template<class T01,class T02,class T03,class T04,class T05,
             class T06,class T07,class T08,class T09,class T10,
             class T11,class T12,class T13,class T14>
    struct Seq< T01,T02,T03,T04,T05,
                T06,T07,T08,T09,T10,
                T11,T12,T13,T14>
    {
        typedef 
        Typelist<T01,
        Typelist<T02,
        Typelist<T03,
        Typelist<T04,
        Typelist<T05,
        Typelist<T06,
        Typelist<T07,
        Typelist<T08,
        Typelist<T09,
        Typelist<T10,
        Typelist<T11,
        Typelist<T12,
        Typelist<T13,
        Typelist<T14,
        NullType
        > > > > >
        > > > > >
        > > > > 
        Type;
    };    
    template<class T01,class T02,class T03,class T04,class T05,
             class T06,class T07,class T08,class T09,class T10,
             class T11,class T12,class T13,class T14,class T15>
    struct Seq< T01,T02,T03,T04,T05,
                T06,T07,T08,T09,T10,
                T11,T12,T13,T14,T15>
    {
        typedef 
        Typelist<T01,
        Typelist<T02,
        Typelist<T03,
        Typelist<T04,
        Typelist<T05,
        Typelist<T06,
        Typelist<T07,
        Typelist<T08,
        Typelist<T09,
        Typelist<T10,
        Typelist<T11,
        Typelist<T12,
        Typelist<T13,
        Typelist<T14,
        Typelist<T15,
        NullType
        > > > > >
        > > > > >
        > > > > >
        Type;
    };    
    template<class T01,class T02,class T03,class T04,class T05,
             class T06,class T07,class T08,class T09,class T10,
             class T11,class T12,class T13,class T14,class T15,
             class T16>
    struct Seq< T01,T02,T03,T04,T05,
                T06,T07,T08,T09,T10,
                T11,T12,T13,T14,T15,
                T16>
    {
        typedef 
        Typelist<T01,
        Typelist<T02,
        Typelist<T03,
        Typelist<T04,
        Typelist<T05,
        Typelist<T06,
        Typelist<T07,
        Typelist<T08,
        Typelist<T09,
        Typelist<T10,
        Typelist<T11,
        Typelist<T12,
        Typelist<T13,
        Typelist<T14,
        Typelist<T15,
        Typelist<T16,
        NullType
        > > > > >
        > > > > >
        > > > > >
        > 
        Type;
    };    
    template<class T01,class T02,class T03,class T04,class T05,
             class T06,class T07,class T08,class T09,class T10,
             class T11,class T12,class T13,class T14,class T15,
             class T16,class T17>
    struct Seq< T01,T02,T03,T04,T05,
                T06,T07,T08,T09,T10,
                T11,T12,T13,T14,T15,
                T16,T17>
    {
        typedef 
        Typelist<T01,
        Typelist<T02,
        Typelist<T03,
        Typelist<T04,
        Typelist<T05,
        Typelist<T06,
        Typelist<T07,
        Typelist<T08,
        Typelist<T09,
        Typelist<T10,
        Typelist<T11,
        Typelist<T12,
        Typelist<T13,
        Typelist<T14,
        Typelist<T15,
        Typelist<T16,
        Typelist<T17,
        NullType
        > > > > >
        > > > > >
        > > > > >
        > > 
        Type;
    };    
    template<class T01,class T02,class T03,class T04,class T05,
             class T06,class T07,class T08,class T09,class T10,
             class T11,class T12,class T13,class T14,class T15,
             class T16,class T17,class T18>
    struct Seq< T01,T02,T03,T04,T05,
                T06,T07,T08,T09,T10,
                T11,T12,T13,T14,T15,
                T16,T17,T18>
    {
        typedef 
        Typelist<T01,
        Typelist<T02,
        Typelist<T03,
        Typelist<T04,
        Typelist<T05,
        Typelist<T06,
        Typelist<T07,
        Typelist<T08,
        Typelist<T09,
        Typelist<T10,
        Typelist<T11,
        Typelist<T12,
        Typelist<T13,
        Typelist<T14,
        Typelist<T15,
        Typelist<T16,
        Typelist<T17,
        Typelist<T18,
        NullType
        > > > > >
        > > > > >
        > > > > >
        > > >  
        Type;
    };    
    template<class T01,class T02,class T03,class T04,class T05,
             class T06,class T07,class T08,class T09,class T10,
             class T11,class T12,class T13,class T14,class T15,
             class T16,class T17,class T18,class T19>
    struct Seq< T01,T02,T03,T04,T05,
                T06,T07,T08,T09,T10,
                T11,T12,T13,T14,T15,
                T16,T17,T18,T19>
    {
        typedef 
        Typelist<T01,
        Typelist<T02,
        Typelist<T03,
        Typelist<T04,
        Typelist<T05,
        Typelist<T06,
        Typelist<T07,
        Typelist<T08,
        Typelist<T09,
        Typelist<T10,
        Typelist<T11,
        Typelist<T12,
        Typelist<T13,
        Typelist<T14,
        Typelist<T15,
        Typelist<T16,
        Typelist<T17,
        Typelist<T18,
        Typelist<T19,
        NullType
        > > > > >
        > > > > >
        > > > > >
        > > > > 
        Type;
    };    
    template<class T01,class T02,class T03,class T04,class T05,
             class T06,class T07,class T08,class T09,class T10,
             class T11,class T12,class T13,class T14,class T15,
             class T16,class T17,class T18,class T19,class T20>
    struct Seq
    {
        typedef 
        Typelist<T01,
        Typelist<T02,
        Typelist<T03,
        Typelist<T04,
        Typelist<T05,
        Typelist<T06,
        Typelist<T07,
        Typelist<T08,
        Typelist<T09,
        Typelist<T10,
        Typelist<T11,
        Typelist<T12,
        Typelist<T13,
        Typelist<T14,
        Typelist<T15,
        Typelist<T16,
        Typelist<T17,
        Typelist<T18,
        Typelist<T19,
        Typelist<T20,
        NullType
        > > > > >
        > > > > >
        > > > > >
        > > > > >
        Type;
    };    

}   // namespace Loki

#endif // LOKI_SEQUENCE_INC_

// $Log: Sequence.h,v $
// Revision 1.3  2006/01/16 19:05:09  rich_sposato
// Added cvs keywords.
//
