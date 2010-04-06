/* vim: ts=4 sw=4 sts=4 et
 *
 * Copyright (c) 2009 James R. McKaskill
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * ----------------------------------------------------------------------------
 */

#ifndef ADBUSCPP_H
#define ADBUSCPP_H

#include <adbus.h>

#include <map>
#include <string>
#include <vector>

#include <assert.h>


#define ADBUSCPP_MULTI_MATCH        0
#define ADBUSCPP_MULTI_PROXY        1
#define ADBUSCPP_MULTI_CLASSES      2
#define ADBUSCPP_MULTI_INTERFACES   3
#define ADBUSCPP_MULTI_BIND         4
#define ADBUSCPP_MULTI_CALL         5

#define ADBUSCPP_NOT_COPYABLE(x) x(const x&); x& operator=(const x&)


namespace adbus
{
    class Error : public std::exception
    {
    public:
        Error(const char* name, const char* msg = NULL) : m_Name(name), m_Message(msg ? msg : "") {}
        ~Error() throw() {}
        virtual const char* name() const {return m_Name.c_str();}
        virtual const char* message() const {return m_Message.empty() ? NULL : m_Message.c_str();}
    private:
        std::string m_Name;
        std::string m_Message;
    };

    class ArgumentError : public Error
    {
    public:
        ArgumentError() : Error("nz.co.foobar.adbuscpp.ArgumentError") {}
    };

    class Buffer
    {
    public:
        Buffer(adbus_Buffer* b_) : b(b_) {}
        operator adbus_Buffer*() const {return b;}
        adbus_Buffer* b;
    };

    inline void operator>>(bool v, Buffer& b)
    { adbus_buf_bool(b, v); }
    inline void operator>>(uint8_t v, Buffer& b)
    { adbus_buf_u8(b, v); }
    inline void operator>>(int16_t v, Buffer& b)
    { adbus_buf_i16(b, v); }
    inline void operator>>(uint16_t v, Buffer& b)
    { adbus_buf_u16(b, v); }
    inline void operator>>(int32_t v, Buffer& b)
    { adbus_buf_i32(b, v); }
    inline void operator>>(uint32_t v, Buffer& b)
    { adbus_buf_u32(b, v); }
    inline void operator>>(int64_t v, Buffer& b)
    { adbus_buf_i64(b, v); }
    inline void operator>>(uint64_t v, Buffer& b)
    { adbus_buf_u64(b, v); }
    inline void operator>>(double v, Buffer& b)
    { adbus_buf_double(b, v); }
    inline void operator>>(const char* v, Buffer& b)
    { adbus_buf_string(b, v, -1); }
    inline void operator>>(const std::string& s, Buffer& b)
    { adbus_buf_string(b, s.c_str(), (int) s.size()); }

    template<class T>
    inline void operator>>(const std::vector<T>& v, Buffer& b)
    {
        adbus_BufArray a;
        adbus_buf_beginarray(b, &a);

        for (size_t i = 0; i < v.size(); ++i) {
            adbus_buf_arrayentry(b, &a);
            v[i] >> b;
        }

        adbus_buf_endarray(b, &a);
    }

    template<class K, class V>
    inline void operator>>(const std::map<K,V>& map, Buffer& b)
    {
        adbus_BufArray a;
        adbus_buf_beginarray(b, &a);

        typename std::map<K,V>::const_iterator ii;
        for (ii = map.begin(); ii != map.end(); ++ii) {
            adbus_buf_arrayentry(b, &a);
            adbus_buf_begindictentry(b);
            ii->first >> b;
            ii->second >> b;
            adbus_buf_enddictentry(b);
        }

        adbus_buf_endarray(b, &a);
    }




    struct MessageEnd{};

    class Iterator
    {
    public:
        Iterator()
        { i.data = i.sig = NULL; i.size = 0; }

        Iterator(adbus_Message* msg)
        { adbus_iter_args(&i, msg); }

        operator adbus_Iterator*() {return &i;}
        adbus_Iterator i;

        int Check(char ch)
        {
            if (!i.sig || *i.sig != ch)
                throw ArgumentError();
            return 0;
        }
    };

    inline int operator<<(bool& v, Iterator& i)
    { return i.Check(ADBUS_BOOLEAN) || adbus_iter_bool(i, &v); }

    inline int operator<<(uint8_t& v, Iterator& i)
    { return i.Check(ADBUS_UINT8) || adbus_iter_u8(i, &v); }

    inline int operator<<(int16_t& v, Iterator& i)
    { return i.Check(ADBUS_INT16) || adbus_iter_i16(i, &v); }

    inline int operator<<(uint16_t& v, Iterator& i)
    { return i.Check(ADBUS_UINT16) || adbus_iter_u16(i, &v); }

    inline int operator<<(int32_t& v, Iterator& i)
    { return i.Check(ADBUS_INT32) || adbus_iter_i32(i, &v); }

    inline int operator<<(uint32_t& v, Iterator& i)
    { return i.Check(ADBUS_UINT32) || adbus_iter_u32(i, &v); }

    inline int operator<<(int64_t& v, Iterator& i)
    { return i.Check(ADBUS_INT64) || adbus_iter_i64(i, &v); }

    inline int operator<<(uint64_t& v, Iterator& i)
    { return i.Check(ADBUS_UINT64) || adbus_iter_u64(i, &v); }

    inline int operator<<(double& v, Iterator& i)
    { return i.Check(ADBUS_DOUBLE) || adbus_iter_double(i, &v); }

    inline int operator<<(const char*& v, Iterator& i)
    { return i.Check(ADBUS_STRING) || adbus_iter_string(i, &v, NULL); }

    inline int operator<<(std::string& v, Iterator& i)
    {
        size_t sz;
        const char* str;
        i.Check(ADBUS_STRING);
        if (adbus_iter_string(i, &str, &sz))
            return -1;
        v.assign(str, sz);
        return 0;
    }

    template<class T>
    inline int operator<<(std::vector<T>& v, Iterator& i)
    {
        adbus_IterArray a;
        i.Check(ADBUS_ARRAY);
        if (adbus_iter_beginarray(i, &a)) {
            return -1;
        }
        while (adbus_iter_inarray(i, &a)) {
            v.resize(v.size() + 1);
            if (v[v.size() - 1] << i)
                return -1;
        }
        return adbus_iter_endarray(i, &a);
    }

    template<class K, class V>
    inline int operator<<(std::map<K,V>& map, Iterator& i)
    {
        adbus_IterArray a;
        i.Check(ADBUS_ARRAY);
        if (adbus_iter_beginarray(i, &a)) {
            return -1;
        }
        while (adbus_iter_inarray(i, &a)) {
            K key;
            V val;
            i.Check(ADBUS_DICTENTRY_BEGIN);
            if (adbus_iter_begindictentry(i))
                return -1;
            if (key << i || val << i)
                return -1;
            i.Check(ADBUS_DICTENTRY_END);
            if (adbus_iter_enddictentry(i))
                return -1;
            map[key] = val;
        }
        return adbus_iter_endarray(i, &a);
    }

    template<class T>
    class Array
    {
    public:
        Array() : data(NULL), size(0) {}
        Array(adbus_IterArray a) : data((const T*) a.data), size(a.size / sizeof(T)) {}
        Array(const T* d, size_t s) : data(d), size(s) {}

        void operator>>(Buffer& b) const
        {
            adbus_BufArray a;
            adbus_buf_beginarray(b, &a);
            adbus_buf_append(b, (const char*) data, size * sizeof(T));
            adbus_buf_endarray(b, &a);
        }

        int operator<<(Iterator& i)
        {
            adbus_IterArray a;
            i.Check(ADBUS_ARRAY);
            if (adbus_iter_beginarray(i, &a) || adbus_iter_endarray(i, &a))
                return -1;
            data = (const T*) a.data;
            size = a.size / sizeof(T);
            return 0;
        }

        const T*    data;
        size_t      size;
    };

    class VariantRef
    {
    public:
        VariantRef()
        { 
            m_V.origsig = m_V.sig = m_V.data = NULL;
            m_V.size = 0;
        }

        void operator>>(Buffer& b) const
        {
            adbus_BufVariant v;
            adbus_buf_beginvariant(b, &v, m_V.sig, -1);
            adbus_buf_append(b, m_V.data, m_V.size);
            adbus_buf_endvariant(b, &v);
        }

        int operator<<(Iterator& i)
        {
            i.Check(ADBUS_VARIANT);
            if (adbus_iter_beginvariant(i, &m_V))
                return -1;
            if (adbus_iter_value(i))
                return -1;
            if (adbus_iter_endvariant(i, &m_V))
                return -1;

            return 0;
        }

        Iterator iterator() const
        {
            Iterator i;
            i.i.data = m_V.data;
            i.i.size = m_V.size;
            i.i.sig  = m_V.sig;
            return i;
        }

    private:
        adbus_IterVariant   m_V;
    };

    class Variant
    {
        ADBUSCPP_NOT_COPYABLE(Variant);
    public:
        Variant() : m_Buf(adbus_buf_new()) {}

        ~Variant() { adbus_buf_free(m_Buf.b); }

        void operator>>(Buffer& b) const
        {
            adbus_BufVariant v;
            adbus_buf_beginvariant(b, &v, adbus_buf_sig(m_Buf, NULL), -1);
            adbus_buf_append(b, adbus_buf_data(m_Buf), adbus_buf_size(m_Buf));
            adbus_buf_endvariant(b, &v);
        }

        int operator<<(Iterator& i)
        {
            adbus_IterVariant iv;
            adbus_buf_reset(m_Buf);
            i.Check(ADBUS_VARIANT);
            if (adbus_iter_beginvariant(i, &iv))
                return -1;
            if (adbus_iter_value(i))
                return -1;
            if (adbus_iter_endvariant(i, &iv))
                return -1;

            adbus_BufVariant bv;
            adbus_buf_setsig(m_Buf, "v", 1);
            adbus_buf_beginvariant(m_Buf, &bv, iv.sig, -1);
            adbus_buf_append(m_Buf, iv.data, iv.size);
            adbus_buf_endvariant(m_Buf, &bv);
            return 0;
        }

        Iterator iterator() const
        {
            Iterator i;
            adbus_iter_buffer(i, m_Buf);
            return i;
        }

        Buffer buffer()
        { return m_Buf; }

    private:
        Buffer m_Buf;
    };

    template<class T> inline
    int operator>>(const T& t, Variant& v)
    { return t >> v.buffer(); }

    class Any
    {
        ADBUSCPP_NOT_COPYABLE(Any);
    public:
        Any() : m_Buf(adbus_buf_new()) {}
        ~Any() {adbus_buf_free(m_Buf.b);}

        int operator<<(Iterator& i)
        {
            adbus_buf_reset(m_Buf);

            const char* bdata = i.i.data;
            const char* bsig  = i.i.sig;
            if (!bdata || !bsig)
                return -1;
            if (adbus_iter_align(i, *bsig))
                return -1;
            if (adbus_iter_value(i))
                return -1;

            const char* edata = i.i.data;
            const char* esig  = i.i.sig;
            assert(bdata && bsig && edata && esig && edata > bdata && esig > bsig);

            adbus_buf_setsig(m_Buf, bsig, (int) (esig - bsig));
            adbus_buf_append(m_Buf, bdata, edata - bdata);
            return 0;
        }

        Iterator iterator() const
        {
            Iterator i;
            adbus_iter_buffer(i, m_Buf);
            return i;
        }

        Buffer buffer()
        { return m_Buf; }

    private:
        Buffer m_Buf;
    };


    class Path
    {
    public:
      Path(){}

      Path(const char* p) : m_Path(p) {}
      Path(const char* p, size_t sz) : m_Path(p, sz) {}
      Path(const std::string& p) : m_Path(p) {}

      const char* c_str() const {return m_Path.c_str();}
      size_t size() const {return m_Path.size();}

      void operator>>(Buffer& b) const
      { adbus_buf_objectpath(b, c_str(), size()); }

      int operator<<(Iterator& i)
      {
        const char* str;
        size_t sz;
        i.Check(ADBUS_OBJECT_PATH);
        if (adbus_iter_objectpath(i, &str, &sz))
          return -1;
        m_Path.assign(str, sz);
        return 0;
      }

      Path operator/(const char* p) const
      {
          if (m_Path == "/")
              return Path(m_Path + p);
          else
              return Path(m_Path + "/" + p);
      }

      Path operator/(const std::string& p)const
      {
          if (m_Path == "/")
              return Path(m_Path + p);
          else
              return Path(m_Path + "/" + p);
      }

      bool operator==(const Path& p)const
      {return m_Path == p.m_Path;}

      bool operator!=(const Path& p)const
      {return m_Path != p.m_Path;}

      bool operator<(const Path& p)const
      {return m_Path < p.m_Path;}

      bool operator>(const Path& p)const
      {return m_Path > p.m_Path;}

      bool operator<=(const Path& p)const
      {return m_Path <= p.m_Path;}

      bool operator>=(const Path& p)const
      {return m_Path >= p.m_Path;}

    private:
      std::string m_Path;
    };

    template<class T> inline
    void* CreateUser()
    { return (T*) malloc(sizeof(T)); }

    template<class T> inline
    void* CreateUser(const T& data)
    {
        T* p = (T*) malloc(sizeof(T));
        *p = data;
        return p;
    }

    template<class T0, class T1> inline
    void* CreateUser2(const T0& data0, const T1& data1)
    { return CreateUser<std::pair<T0, T1> >(std::pair<T0,T1>(data0, data1)); }

#ifndef DOC
#define ADBUSCPP_MULTI ADBUSCPP_MULTI_BIND
#include "adbuscpp-multi.h"
#endif

    class Match : public adbus_Match
    {
    public:
        Match() {reset();}

        void reset() {adbus_match_init(this);}

#define ADBUSCPP_MULTI ADBUSCPP_MULTI_MATCH
#include "adbuscpp-multi.h"

    };


    namespace detail
    {
        template<class MF, class O, class T>
        static int GetCallback(adbus_CbData* d)
        {
            MF mf = *(MF*) d->user1;
            O* o  =  (O*)  d->user2;
            try {
                T t = (o->*mf)();

                Buffer b;
                b.b = d->getprop;
                t >> b;
            } catch (Error& e) {
                adbus_error(d, e.name(), -1, e.message(), -1);
            }
            return 0;
        }

        template<class MF, class O, class T>
        static int SetCallback(adbus_CbData* d)
        {
            MF mf = *(MF*) d->user1;
            O* o  =  (O*)  d->user2;
            try {
                Iterator i;
                i.i = d->setprop;

                T t;
                if (t << i)
                    return -1;

                (o->*mf)(t);

            } catch (Error& e) {
                adbus_error(d, e.name(), -1, e.message(), -1);
            }
            return 0;
        }

        template<class O, class T>
        class PropertyMember
        {
        public:
            PropertyMember(adbus_Member* m):m_M(m){}

            PropertyMember<O,T>& annotate(const std::string& name, const std::string& value)
            {
                adbus_mbr_annotate(m_M, name.c_str(), (int) name.size(), value.c_str(), (int) value.size());
                return *this;
            }

            template<class MF>
            PropertyMember<O,T>& setGetter(MF function)
            {
                adbus_MsgCallback cb = &detail::GetCallback<MF,O,T>;
                void* cuser = CreateUser<MF>(function);
                adbus_mbr_setgetter(m_M, cb, cuser);
                adbus_mbr_addrelease(m_M, &free, cuser);
                return *this;
            }

            template<class MF>
            PropertyMember<O,T>& setSetter(MF function)
            {
                adbus_MsgCallback cb = &detail::SetCallback<MF,O,T>;
                void* cuser = CreateUser<MF>(function);
                adbus_mbr_setsetter(m_M, cb, cuser);
                adbus_mbr_addrelease(m_M, &free, cuser);
                return *this;
            }

        private:
            adbus_Member* m_M;
        };

        class SignalMember
        {
        public:
            SignalMember(adbus_Member* m) : m_M(m) {}

            SignalMember& annotate(const char* name, const char* value)
            { adbus_mbr_annotate(m_M, name, -1, value, -1); return *this; }

            SignalMember& argname(const char* name)
            { adbus_mbr_argname(m_M, name, -1); return *this; }

        private:
            adbus_Member* m_M;
        };

        class MethodMember
        {
        public:
            MethodMember(adbus_Member* m) : m_M(m) {}

            MethodMember& annotate(const char* name, const char* value)
            { adbus_mbr_annotate(m_M, name, -1, value, -1); return *this; }

            MethodMember& argname(const char* name)
            { adbus_mbr_argname(m_M, name, -1); return *this; }

            MethodMember& retname(const char* name)
            { adbus_mbr_retname(m_M, name, -1); return *this; }

        private:
            adbus_Member* m_M;
        };

    }



    template<class O>
    class Interface
    {
    public:
        Interface(const std::string& name) 
        : m_I(adbus_iface_new(name.c_str(), (int) name.size())) 
        { adbus_iface_ref(m_I); }

        Interface(const Interface<O>& r)
        : m_I(r.m_I)
        { adbus_iface_ref(m_I); }

        Interface<O>& operator=(const Interface<O>& r)
        {
            adbus_iface_ref(r.m_I);
            adbus_iface_deref(m_I);
            m_I = r.m_I;
            return *this;
        }

        ~Interface() 
        { adbus_iface_deref(m_I); }

        template<class T>
        detail::PropertyMember<O, T> addProperty(const std::string& name)
        {
            std::string type = adbus_type_string((T*) NULL);
            adbus_Member* mbr = adbus_iface_addproperty( m_I,
                    name.c_str(), (int) name.size(),
                    type.c_str(), (int) type.size());
            return detail::PropertyMember<O, T>(mbr);
        }

        adbus_Member* property(const std::string& name) {return adbus_iface_property(m_I, name.c_str(), (int) name.size());}
        adbus_Member* signal(const std::string& name) {return adbus_iface_signal(m_I, name.c_str(), (int) name.size());}
        adbus_Member* method(const std::string& name) {return adbus_iface_method(m_I, name.c_str(), (int) name.size());}

        const adbus_Interface* interface() const {return m_I;}
        adbus_Interface* interface() {return m_I;}

        operator const adbus_Interface*() const {return m_I;}
        operator adbus_Interface*() {return m_I;}

#define ADBUSCPP_MULTI ADBUSCPP_MULTI_INTERFACES
#include "adbuscpp-multi.h"

    private:
        adbus_Interface* m_I;
    };


    class State
    {
        ADBUSCPP_NOT_COPYABLE(State);
    public:
        State() : m_S(NULL) {}
        virtual ~State() {adbus_state_free(m_S);}

        void reset()
        {
            if (m_S) {
                adbus_state_reset(m_S);
            }
        }

        void deleteState()
        { adbus_state_free(m_S); }

        template<class O>
        void bind(adbus_Connection* c, const char* path, const Interface<O>& i, O* object)
        {
            adbus_Bind b;
            adbus_bind_init(&b);
            b.path      = path;
            b.interface = i;
            b.cuser2    = object;
            adbus_state_bind(state(), c, &b);
        }

        template<class O>
        void bind(adbus_Connection* c, const std::string& path, const Interface<O>& i, O* object)
        {
            adbus_Bind b;
            adbus_bind_init(&b);
            b.path      = path.c_str();
            b.pathSize  = (int) path.size();
            b.interface = i;
            b.cuser2    = object;
            adbus_state_bind(state(), c, &b);
        }

        template<class O>
        void bind(adbus_Connection* c, const Path& path, const Interface<O>& i, O* object)
        {
            adbus_Bind b;
            adbus_bind_init(&b);
            b.path      = path.c_str();
            b.pathSize  = (int) path.size();
            b.interface = i;
            b.cuser2    = object;
            adbus_state_bind(state(), c, &b);
        }

        void addMatch(adbus_Connection* c, adbus_Match* m)
        { adbus_state_addmatch(state(), c, m); }

        adbus_State* state() 
        {
            if (!m_S) {
                m_S = adbus_state_new();
            }
            return m_S;
        }

    private:
        adbus_State* m_S;
    };

    class BindPath;

    class Connection
    {
    public:
        Connection(const adbus_ConnVTable* vtable, void* obj)
        :   m_C(adbus_conn_new(vtable, obj)) 
        { adbus_conn_ref(m_C); }

        Connection(adbus_SendMsgCallback cb, void* obj)
        {
            adbus_ConnVTable vtable = {};
            vtable.send_message = cb;
            m_C = adbus_conn_new(&vtable, obj);
            adbus_conn_ref(m_C);
        }

        Connection(adbus_Connection* c)
        :   m_C(c)
        { if (m_C) adbus_conn_ref(m_C); }

        Connection(const Connection& c)
        :   m_C(c.m_C)
        { if (m_C) adbus_conn_ref(m_C); }

        Connection& operator=(const Connection& c)
        {
			if (c.m_C)
	            adbus_conn_ref(c.m_C);
			if (m_C)
	            adbus_conn_deref(m_C);
            m_C = c.m_C;
            return *this;
        }

        ~Connection() { if (m_C) adbus_conn_deref(m_C);}

        adbus_ConnMatch* addMatch(adbus_Match* m) {return adbus_conn_addmatch(m_C, m);}
        void removeMatch(adbus_ConnMatch* m) {adbus_conn_removematch(m_C, m);}
       
        BindPath path(const char* path);
        BindPath path(const std::string& path);

        uint32_t serial() const
        { return adbus_conn_serial(m_C); }

        int parse(const char* data, size_t size)
        { return adbus_conn_parse(m_C, data, size); }

        int step()
        { return adbus_conn_continue(m_C); }

        int dispatch(adbus_Message* msg)
        { return adbus_conn_dispatch(m_C, msg); }

        void connectToBus()
        { adbus_conn_connect(m_C, NULL, NULL); }

        void connectToBus(adbus_Callback callback, void* data)
        { adbus_conn_connect(m_C, callback, data); }

		int waitForConnected(int timeoutms = -1)
		{
			uintptr_t handle;
			return adbus_conn_block(m_C, ADBUS_WAIT_FOR_CONNECTED, &handle, timeoutms);
		}

		bool isValid() const {return m_C != NULL;}

        std::string uniqueName()
        {
            size_t sz;
            const char* str = adbus_conn_uniquename(m_C, &sz);
            return std::string(str, sz);
        }

        operator adbus_Connection*() const {return m_C;}
    private:
        adbus_Connection* m_C;
    };

    class BindPath
    {
    public:
        BindPath() : m_C(NULL) {}
        BindPath(adbus_Connection* c, const char* path) : m_C(c), m_Path(path) {}
        BindPath(adbus_Connection* c, const Path& path) : m_C(c), m_Path(path) {}

        template<class O>
        void bind(const Interface<O>& i, O* object) const
        { bind(i, object, *((State*) object)); }

        template<class O>
        void bind(const Interface<O>& i, O* object, adbus_State* state) const
        {
            adbus_Bind b;
            adbus_bind_init(&b);
            b.path      = m_Path.c_str();
            b.pathSize  = (int) m_Path.size();
            b.interface = i;
            b.cuser2    = object;
            adbus_state_bind(state, m_C, &b);
        }

        const adbus::Connection& connection() const {return m_C;}
        const Path& path() const {return m_Path;}

        void operator>>(Buffer& b) const
        { adbus_buf_objectpath(b, c_str(), size()); }

        const char* c_str() const {return m_Path.c_str();}
        size_t size() const {return m_Path.size();}

        BindPath operator/(const char* p) const
        { return BindPath(m_C, m_Path / p); }

        BindPath operator/(const std::string& p)const
        { return BindPath(m_C, m_Path / p); }

    private:
        adbus::Connection m_C;
        Path              m_Path;
    };

       
    inline BindPath Connection::path(const char* path)
    { return BindPath(*this, path); }

    inline BindPath Connection::path(const std::string& path)
    { return BindPath(*this, path); }

#ifdef DOC

    template<class A0...>
    class SignalX
    {
        ADBUSCPP_NOT_COPABLE(SignalX<A0...>);
    public:
        SignalX();

        void bind(adbus_Path* path, adbus_Member* mbr);

        void emit(const A0& a0...);
        void operator()(const A0& a0...);
    private:
        adbus_Signal*       m_Signal;
    };

#else
#define ADBUSCPP_MULTI ADBUSCPP_MULTI_CLASSES
#include "adbuscpp-multi.h"
#endif


    namespace detail
    {
        template<class MF, class O>
        static int ErrorCallback(adbus_CbData* d)
        {
            MF mf = ((std::pair<MF,O*>*) d->user1)->first;
            O* o  = ((std::pair<MF,O*>*) d->user1)->second;

            try {
                Iterator i;
                adbus_iter_args(i, d->msg);

                const char* msg = NULL;
                if (i.i.sig && strcmp(i.i.sig, "s") == 0 && (msg << i))
                    return -1;

                (o->*mf)(d->msg->error, msg);

            } catch (Error&e) {
                (void) e;
            }
            return 0;
        }

        template<class MF, class O>
        static int BusCallback(adbus_CbData* d)
        {
            MF mf = ((std::pair<MF,O*>*) d->user1)->first;
            O* o  = ((std::pair<MF,O*>*) d->user1)->second;

            try {
                (o->*mf)(d->msg->type == ADBUS_MSG_RETURN);

            } catch (Error&e) {
                (void) e;
            }
            return 0;
        }
    }

    class Call : public adbus_Call
    {
    public:
        Call() 
        {
            this->timeoutms = -1;
            this->cuser = NULL;
            this->euser = NULL;
        }

        void send()
        { adbus_call_send(this); }

        template<class T>
        Call& arg(const T& arg)
        {
            Buffer b = adbus_msg_argbuffer(this->msg);

            std::string sig = adbus_type_string((T*) NULL);
            adbus_buf_appendsig(b, sig.c_str(), (int) sig.size());

            arg >> b;

            return *this;
        }

        template<class MF, class O>
        Call& setError(MF mf, O* o)
        {
            assert(!error && !euser && !release[1] && !ruser[1]);
            this->error         = &detail::ErrorCallback<MF, O>;
            this->euser         = CreateUser2<MF,O*>(mf, o);
            this->release[1]    = &free;
            this->ruser[1]      = this->euser;
            return *this;
        }

#define ADBUSCPP_MULTI ADBUSCPP_MULTI_CALL
#include "adbuscpp-multi.h"

    };

    class Proxy
    {
        ADBUSCPP_NOT_COPYABLE(Proxy);
    public:
        Proxy(State* o) 
        { m_Proxy = adbus_proxy_new(o->state()); }

        Proxy(adbus_State* state) 
        { m_Proxy = adbus_proxy_new(state); }

        ~Proxy()
        { adbus_proxy_free(m_Proxy); }

        void init(adbus_Connection* connection,
                  const std::string& service,
                  const std::string& path)
        {
            adbus_proxy_init(m_Proxy, connection, service.c_str(), (int) service.size(), path.c_str(), (int) path.size());
        }

        void init(adbus_Connection* connection,
                  const std::string& service,
                  const Path& path)
        {
            adbus_proxy_init(m_Proxy, connection, service.c_str(), (int) service.size(), path.c_str(), (int) path.size());
        }

        void init(adbus_Connection* connection,
                  const std::string& service,
                  const std::string& path,
                  const std::string& interface)
        {
            init(connection, service, path);
            adbus_proxy_setinterface(m_Proxy, interface.c_str(), (int) interface.size());
        }

        void init(adbus_Connection* connection,
                  const std::string& service,
                  const Path& path,
                  const std::string& interface)
        {
            init(connection, service, path);
            adbus_proxy_setinterface(m_Proxy, interface.c_str(), (int) interface.size());
        }

        void init(adbus_Connection* connection,
                  const char* service,
                  const char* path)
        {
            adbus_proxy_init(m_Proxy, connection, service, -1, path, -1);
        }

        void init(adbus_Connection* connection,
                  const char* service,
                  const char* path,
                  const char* interface)
        {
            init(connection, service, path);
            adbus_proxy_setinterface(m_Proxy, interface, -1);
        }

        template <class T>
        Call setProperty(const char* property, const T& arg)
        {
            Call call;

            std::string signature = adbus_type_string((T*) NULL);
            adbus_proxy_setproperty(m_Proxy, &call, property, -1, signature.c_str(), -1);

            Buffer b = adbus_msg_argbuffer(call.msg);
            arg >> b;

            return call;
        }

        Call getProperty(const char* property)
        {
            Call call;
            adbus_proxy_getproperty(m_Proxy, &call, property, -1);
            return call;
        }

#define ADBUSCPP_MULTI ADBUSCPP_MULTI_PROXY
#include "adbuscpp-multi.h"

    private:
        adbus_Proxy*        m_Proxy;
    };







}


inline const char* adbus_type_string(bool*)             {return "b";}
inline const char* adbus_type_string(uint8_t*)          {return "y";}
inline const char* adbus_type_string(int16_t*)          {return "n";}
inline const char* adbus_type_string(uint16_t*)         {return "q";}
inline const char* adbus_type_string(int32_t*)          {return "i";}
inline const char* adbus_type_string(uint32_t*)         {return "u";}
inline const char* adbus_type_string(int64_t*)          {return "x";}
inline const char* adbus_type_string(uint64_t*)         {return "t";}
inline const char* adbus_type_string(double*)           {return "d";}
inline const char* adbus_type_string(char**)            {return "s";}
inline const char* adbus_type_string(const char**)      {return "s";}
inline const char* adbus_type_string(std::string*)      {return "s";}
inline const char* adbus_type_string(adbus::Variant*)   {return "v";}
inline const char* adbus_type_string(adbus::VariantRef*){return "v";}
inline const char* adbus_type_string(adbus::Path*)      {return "o";}
inline const char* adbus_type_string(adbus::BindPath*)  {return "o";}

template<size_t size> inline
const char* adbus_type_string(char (*)[size])           {return "s";}

template<size_t size> inline
const char* adbus_type_string(const char (*)[size])     {return "s";}

template<class T> inline
std::string adbus_type_string(std::vector<T>*)
{return std::string("a") + adbus_type_string((T*) NULL);}

template<class K, class V> inline
std::string adbus_type_string(std::map<K,V>*)
{return std::string("a{") + adbus_type_string((K*) NULL) + adbus_type_string((V*) NULL) + "}";}

template<class T> inline
std::string adbus_type_string(adbus::Array<T>*)
{return std::string("a") + adbus_type_string((T*) NULL);}

template<class T1, class T2> inline
std::string adbus_type_string(std::pair<T1,T2>*)
{return std::string(adbus_type_string((T1*) NULL)) + adbus_type_string((T2*) NULL);}




#endif /* ADBUSCPP_H */
