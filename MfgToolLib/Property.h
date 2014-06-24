/*
 * Copyright (C) 2009-2013, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
// PropertyT.h
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include <map>
#include <vector>

namespace property
{
    ////////////////////////////////////////////////////////////////////////////////////////
    //
    //   Property base class
    //
    ////////////////////////////////////////////////////////////////////////////////////////
    class Property
    {
    public:
        typedef std::vector<Property*> PropertyList;
        typedef std::vector<Property*>::iterator PropertyIterator;

        Property(bool container = false) 
            : _container(container)
            , _owner(NULL)
            , _name(_T(""))
            , _desc(_T(""))
        {};

        virtual ~Property() 
        {
            _propertyList.erase(_propertyList.begin(), _propertyList.end());
        };

        void describe(Property* owner, CString name, CString desc = _T(""))
        {
            _name  = name;
            _desc  = desc;
            _owner = owner;
            if ( _owner != NULL )
                _owner->_propertyList.push_back(this);
        };

        CString& name() { return _name; };
        CString desc() { return _desc; };
        bool isContainer() { return _container; };
        PropertyList* getList() { return &_propertyList; };
        // the following functions are only need in derived classes
        // that have a _value member.
        //
        // virtual value_type get() {return _value;};
        // virtual void put(const value_type& val) {_value = val;}
        virtual CString ToString(LPCTSTR f=_T("%d")){return CString(_T(""));};
        
    protected:
        bool _container;
        CString _name;
        CString _desc;
        Property* _owner;
        PropertyList _propertyList;
        PropertyIterator _propertyIterator;
    };

    ////////////////////////////////////////////////////////////////////////////////////////
    //
    //   GUID Property class
    //
    ////////////////////////////////////////////////////////////////////////////////////////
    class GuidProperty : public Property
    {
    public:
        GuidProperty() : Property(false), _value(GUID_NULL) {};
        ~GuidProperty() {};

        virtual LPGUID get() { return(&_value); };
        virtual void put(const GUID& guid) { _value = guid; };
        virtual void put(const LPCGUID guid) { _value = *guid; };
        virtual CString ToString()
        {
            CString guidStr;
            LPOLESTR str;
        
            StringFromIID(*get(), &str);
            //guidStr = SSOLE2T(str);
			guidStr = str;

            CoTaskMemFree(str);

            return guidStr;
        };

    protected:
        GUID _value;
    };

    ////////////////////////////////////////////////////////////////////////////////////////
    //
    //   CString Property class
    //
    ////////////////////////////////////////////////////////////////////////////////////////
    class StringProperty : public Property
    {
    public:
        StringProperty() : Property(false), _value(_T("")) {};
        ~StringProperty() {};

        virtual CString get() { return(_value); };
        virtual void put(const CString& str) { _value = str; };
        virtual void put(LPCTSTR str) { _value = str; };
        virtual CString ToString(LPCTSTR) { return get(); };

    protected:
        CString _value;
    };

    ////////////////////////////////////////////////////////////////////////////////////////
    //
    //   int Property class
    //
    ////////////////////////////////////////////////////////////////////////////////////////
    class Int32Property : public Property
    {
    public:
        Int32Property(int val = 0) : Property(false), Value(val) {};
        ~Int32Property() {};

        virtual int get() const { return(Value); };
        virtual void put(int val) { Value = val; };
        virtual CString ToString(LPCTSTR f)
        {
//          CString str = f;
//          str.Format(f, get());
//          return str;

            CString str, fmt = f;
            if ( fmt.IsEmpty() )
            {
                fmt = _T("0x%08X");
            }

            if ( ValueList.empty() )
            {
                str.Format(fmt, Value);
            }
            else
            {
                std::map<int, CString>::const_iterator key;
                key = ValueList.find(Value);
                if ( key == ValueList.end() )
                    str = _T("Not found.");
                else
                    str = key->second;
            }

            return str;
        };

        std::map<int, CString> ValueList;
    protected:
        int Value;
    };

    // macros used to override Property::get() functions
//  #define INT32_PROPERTY(name) class name : public Int32Property { public: int get(); }_##name
//  #define UNT32_PROPERTY(name) class name : public Int32Property { public: UINT get(); }_##name
//  #define STRING_PROPERTY(name) class name : public StringProperty { public: CString get(); }_##name
//  #define GUID_PROPERTY(name) class name : public GuidProperty { public: LPGUID get(); }_##name
    // possible macro to be used in Property-derived class if get() override is needed
    // #define INT32_PROPERTY(name) class name : public Int32Property { public: int get(); }_##name
    //
    // declaration: INT32_PROPERTY(myIntProperty);
    //   creates a member variable: Int32Property _myIntProperty
    //
    // instantiation of override: 
    //     int MyPropertyContainerClass::myIntProperty::get()
    //     {
    //          MyPropertyContainerClass* container = dynamic_cast<MyPropertyContainerClass*>(_owner);
    //          ASSERT(container);
    //
    //         _value = container->SomeFunction();
    //         return _value;
    //     }

} // namespace property

using namespace property;
