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

#include "proxy.h"


/** \struct adbus_Proxy
 *
 *  \brief Helper class to access members of a specific remote object
 *
 *  adbus_Proxy acts as a simple wrapper around a specific remote object to
 *  ease calling methods, setting/getting properties, and adding matches for
 *  signals on that object.
 *
 *  All callbacks are registered with a provided adbus_State and their
 *  lifetime is tied to the state not the proxy. This means that the proxy can
 *  be modified, freed, reset, etc without losing the original callbacks.
 *
 *  The user can optionally set the interface, but this is required for
 *  accessing properties.
 *
 *  \warning Only one adbus_Call can be active at a time.
 *  \warning The proxy can only be used from the creating thread and must be
 *  on the same thread as the associated state.
 *
 *  \section methods Calling Methods
 *
 *  Method calling uses a helper class adbus_Call. The general workflow is:
 *  -# Create an adbus_Call on the stack.
 *  -# Call adbus_call_method() to setup the fields in the adbus_Call.
 *  -# Setup arguments and callbacks in the adbus_Call fields.
 *  -# Call adbus_call_send() to send off the message and register for any
 *  callbacks.
 *
 *  For example:
 *  \code
 *  struct my_state
 *  {
 *      adbus_State* state;
 *      adbus_Proxy* proxy;
 *  };
 *
 *  struct my_state* CreateMyState(adbus_Connection* c)
 *  {
 *      struct my_state* ret = (struct my_state*) calloc(sizeof(struct my_state), 1);
 *      ret->state = adbus_state_new();
 *      ret->proxy = adbus_proxy_new(ret->state);
 *
 *      adbus_proxy_init(ret->proxy, c, "com.example.ExampleService", -1, "/", -1);
 *
 *      // This is required for properties and signals, but not calling methods.
 *      adbus_proxy_setinterface(ret->proxy, "com.example.ExampleInterface", -1);
 *
 *      return ret;
 *  }
 *
 *  void DeleteMyState(struct my_state* s)
 *  {
 *      if (s) {
 *          adbus_proxy_free(s->proxy);
 *          adbus_state_free(s->state);
 *          free(s);
 *      }
 *  }
 *
 *  int Reply(adbus_CbData* d)
 *  {
 *      struct my_state* s = (struct my_state*) d->user1;
 *      // Do something
 *      return 0;
 *  }
 *
 *  void CallMethod(struct my_state* s)
 *  {
 *      adbus_Call call;
 *
 *      // Setup the call
 *      adbus_proxy_method(s->proxy, "ExampleMethod", -1);
 *
 *      // Append some arguments
 *      adbus_msg_setsig(call.msg, "b", -1);
 *      adbus_msg_bool(call.msg, 1);
 *
 *      // Setup callbacks
 *      call.callback = &Reply;
 *      call.cuser = s;
 *
 *      // Send off the call
 *      adbus_call_send(s->proxy, &call);
 *  }
 *  \endcode
 *
 *  \section properties Setting/Getting Properties
 *
 *  \note This requires that the interface be set with
 *  adbus_proxy_setinterface().
 *
 *  The workflow for getting/setting properties is very similar to calling
 *  methods:
 *  -# Create an adbus_Call on the stack
 *  -# Call adbus_call_setproperty() and adbus_call_getproperty() to setup the
 *  adbus_Call.
 *  -# Append the property to adbus_Call::msg for adbus_call_setproperty().
 *  Add callbacks for adbus_call_getproperty().
 *  -# Send off the call using adbus_call_send().
 *
 *  \note When appending the property to the message you should not set the
 *  argument signature.
 *
 *  For example:
 *  \code
 *  void SetProperty(struct my_state* s)
 *  {
 *      adbus_Call call;
 *
 *      // Setup the call
 *      adbus_proxy_setproperty(s->proxy, &call, "BooleanProperty", -1, "b", -1);
 *
 *      // Append the argument
 *      adbus_msg_bool(call.msg, 1);
 *
 *      // Send off the call
 *      adbus_call_send(s->proxy, &call);
 *  }
 *
 *  void GetProperty(struct my_state* s)
 *  {
 *      adbus_Call call;
 *
 *      // Setup the call
 *      adbus_proxy_getproperty(s->proxy, &call, "BooleanProperty", -1);
 *
 *      // Set the callback
 *      call.callback = &Reply;
 *      call.cuser = s;
 *
 *      // Send off the call
 *      adbus_call_send(s->proxy, &call);
 *  }
 *  \endcode
 *
 *  \section signals Setting up Signal Matches
 *
 *  The proxy can also add signal matches through to the state for this remote
 *  object.
 *
 *  The workflow is:
 *  -# Create a match on the stack and initialise it using adbus_match_init().
 *  -# Set the callback in the match
 *  -# Call adbus_proxy_signal() which will setup the remaining items and add
 *  it to the state.
 *
 *  For example:
 *  \code
 *  int OnSignal(adbus_CbData* d)
 *  {
 *      struct my_state* s = (struct my_state*) d->user1;
 *      // Do something
 *      return 0;
 *  }
 *  void AddSignalMatch(struct my_state* s)
 *  {
 *      adbus_Match m;
 *      adbus_match_init(&m);
 *      m.callback  = &OnSignal;
 *      m.cuser     = s;
 *      adbus_proxy_signal(s->proxy, &m, "ExampleSignal", -1);
 *  }
 *  \endcode
 *
 *  \sa adbus_State
 */

/* ------------------------------------------------------------------------- */

/** Create a new proxy tied to the provided state
 *  \relates adbus_Proxy
 */
adbus_Proxy* adbus_proxy_new(adbus_State* state)
{
    adbus_Proxy* p  = NEW(adbus_Proxy);
    p->state        = state;
    p->message      = adbus_msg_new();
    p->refConnection = 1;

    return p;
}

/* ------------------------------------------------------------------------- */

/** Frees the proxy
 *  \relates adbus_Proxy
 */
void adbus_proxy_free(adbus_Proxy* p)
{
    if (p) {
        if (p->refConnection) {
            adbus_conn_deref(p->connection);
        }

        adbus_msg_free(p->message);
        ds_free(&p->service);
        ds_free(&p->path);
        ds_free(&p->interface);
        free(p);
    }
}

/* ------------------------------------------------------------------------- */

/** Reset and reinitialise the proxy
 *  \relates adbus_Proxy
 */
void adbus_proxy_init(
        adbus_Proxy*        p,
        adbus_Connection*   connection,
        const char*         service,
        int                 ssize,
        const char*         path,
        int                 psize)
{
    if (ssize < 0)
        ssize = strlen(service);
    if (psize < 0)
        psize = strlen(path);

    p->connection = connection;

    if (p->refConnection) {
        adbus_conn_ref(p->connection);
    }

    ds_set_n(&p->service, service, ssize);
    ds_set_n(&p->path, path, psize);
    ds_clear(&p->interface);
}

/* ------------------------------------------------------------------------- */

/** Set the proxy interface
 *  \relates adbus_Proxy
 */
void adbus_proxy_setinterface(
        adbus_Proxy*        p,
        const char*         interface,
        int                 isize)
{
    if (isize < 0)
        isize = strlen(interface);

    ds_set_n(&p->interface, interface, isize);
}

/* ------------------------------------------------------------------------- */

/** Add a signal match
 *  \relates adbus_Proxy
 */
void adbus_proxy_signal(
        adbus_Proxy*        p,
        adbus_Match*        m,
        const char*         signal,
        int                 size)
{
    m->type                 = ADBUS_MSG_SIGNAL;
    m->addMatchToBusDaemon  = 1;
    m->member               = signal;
    m->memberSize           = size;
    m->sender               = ds_cstr(&p->service);
    m->senderSize           = ds_size(&p->service);
    m->path                 = ds_cstr(&p->path);
    m->pathSize             = ds_size(&p->path);

    if (ds_size(&p->interface) > 0) {
        m->interface            = ds_cstr(&p->interface);
        m->interfaceSize        = ds_size(&p->interface);
    }

    adbus_state_addmatch(p->state, p->connection, m);
}

/* ------------------------------------------------------------------------- */

/** Setup an adbus_Call structure for a method call
 *  \relates adbus_Proxy
 */
void adbus_proxy_method(
        adbus_Proxy*        p,
        adbus_Call*         call,
        const char*         method,
        int                 size)
{
    adbus_MsgFactory* m = p->message;

    p->type = METHOD_CALL;
    memset(call, 0, sizeof(adbus_Call));
    call->msg = m;
    call->proxy = p;
    call->timeoutms = -1;

    adbus_msg_reset(m);
    adbus_msg_settype(m, ADBUS_MSG_METHOD);
    adbus_msg_setserial(m, adbus_conn_serial(p->connection));

	if (ds_size(&p->service) > 0) {
	    adbus_msg_setdestination(m, ds_cstr(&p->service), ds_size(&p->service));
	}

    adbus_msg_setpath(m, ds_cstr(&p->path), ds_size(&p->path));

	if (ds_size(&p->interface) > 0) {
        adbus_msg_setinterface(m, ds_cstr(&p->interface), ds_size(&p->interface));
	}

    adbus_msg_setmember(m, method, size);
}

/* ------------------------------------------------------------------------- */

/** Setup an adbus_Call structure to set a property
 *  \relates adbus_Proxy
 */
void adbus_proxy_setproperty(
        adbus_Proxy*        p,
        adbus_Call*         call,
        const char*         property,
        int                 propsize,
        const char*         type,
        int                 typesize)
{
    adbus_MsgFactory* m = p->message;

    assert(ds_size(&p->interface) > 0);
    p->type = SET_PROP_CALL;

    memset(call, 0, sizeof(adbus_Call));
    call->msg = m;
    call->proxy = p;
    call->timeoutms = -1;

    adbus_msg_reset(m);
    adbus_msg_settype(m, ADBUS_MSG_METHOD);
    adbus_msg_setserial(m, adbus_conn_serial(p->connection));
    adbus_msg_setdestination(m, ds_cstr(&p->service), ds_size(&p->service));
    adbus_msg_setpath(m, ds_cstr(&p->path), ds_size(&p->path));

    adbus_msg_setinterface(m, "org.freedesktop.DBus.Properties", -1);
    adbus_msg_setmember(m, "Set", -1);

    adbus_msg_setsig(m, "ssv", 3);
    adbus_msg_string(m, ds_cstr(&p->interface), ds_size(&p->interface));
    adbus_msg_string(m, property, propsize);
    adbus_msg_beginvariant(m, &p->variant, type, typesize);
}

/* ------------------------------------------------------------------------- */

/** Setup an adbus_Call structure to get a property
 *  \relates adbus_Proxy
 */
void adbus_proxy_getproperty(
        adbus_Proxy*        p,
        adbus_Call*         call,
        const char*         property,
        int                 size)
{
    adbus_MsgFactory* m = p->message;

    p->type = GET_PROP_CALL;
    assert(ds_size(&p->interface) > 0);

    memset(call, 0, sizeof(adbus_Call));
    call->msg = m;
    call->proxy = p;
    call->timeoutms = -1;

    adbus_msg_reset(m);
    adbus_msg_settype(m, ADBUS_MSG_METHOD);
    adbus_msg_setserial(m, adbus_conn_serial(p->connection));
    adbus_msg_setdestination(m, ds_cstr(&p->service), ds_size(&p->service));
    adbus_msg_setpath(m, ds_cstr(&p->path), ds_size(&p->path));

    adbus_msg_setinterface(m, "org.freedesktop.DBus.Properties", -1);
    adbus_msg_setmember(m, "Get", -1);

    adbus_msg_setsig(m, "ss", 2);
    adbus_msg_string(m, ds_cstr(&p->interface), ds_size(&p->interface));
    adbus_msg_string(m, property, size);
}

/* ------------------------------------------------------------------------- */

int adbus_call_send(adbus_Call* call)
{
    adbus_MsgFactory* msg = call->msg;
    adbus_Proxy* p = call->proxy;

    assert(p->message == msg);

    if (p->type == SET_PROP_CALL) {
        adbus_msg_endvariant(msg, &p->variant);
    }

    adbus_msg_end(msg);

    /* Add reply */
    if (call->callback || call->error) {
        adbus_Reply r;
        adbus_reply_init(&r);

        r.serial        = (uint32_t) adbus_msg_serial(msg);
        r.remote        = ds_cstr(&p->service);
        r.remoteSize    = ds_size(&p->service);
        r.callback      = call->callback;
        r.cuser         = call->cuser;
        r.error         = call->error;
        r.euser         = call->euser;
        r.release[0]    = call->release[0];
        r.ruser[0]      = call->ruser[0];
        r.release[1]    = call->release[1];
        r.ruser[1]      = call->ruser[1];

        adbus_state_addreply(p->state, p->connection, &r);

    } else {
        /* Why are you adding release callbacks, without a reply or error
         * callback?
         */
        assert(!call->cuser && !call->euser);
        assert(!call->release[0] && !call->ruser[0]);
        assert(!call->release[1] && !call->ruser[1]);
        adbus_msg_setflags(msg, adbus_msg_flags(msg) | ADBUS_MSG_NO_REPLY);
    }

    /* Send message */
    return adbus_msg_send(msg, p->connection);
}

/* ------------------------------------------------------------------------- */

struct ProxyBlockData
{
    adbus_Connection*   connection;
    void*               block;
    adbus_MsgCallback   callback;
    void*               cuser;
    adbus_MsgCallback   error;
    void*               euser;
    adbus_Callback      release[2];
    void*               ruser[2];
    int*                ret;
};

static void ProxyBlockRelease(void* user)
{
    struct ProxyBlockData* d = (struct ProxyBlockData*) user;

    /* No need to proxy this as we have already been proxied */
    if (d->release[0]) {
        d->release[0](d->ruser[0]);
    }
    if (d->release[1]) {
        d->release[1](d->ruser[1]);
    }

    free(d);
}

static int ProxyBlockCallback(adbus_CbData* d)
{
    struct ProxyBlockData* s = (struct ProxyBlockData*) d->user1;
    int ret = 0;

    /* No need to proxy this as we have already been proxied */
    if (s->callback) {
        d->user1 = s->cuser;
        ret = s->callback(d);
    }        
    adbus_conn_block(d->connection, ADBUS_UNBLOCK, &s->block, -1);

    return ret;
}

static int ProxyBlockError(adbus_CbData* d)
{
    struct ProxyBlockData* s = (struct ProxyBlockData*) d->user1;
    int ret = 0;

    /* No need to proxy this as we have already been proxied */
    if (s->error) {
        d->user1 = s->euser;
        ret = s->error(d);
    }        
    *s->ret = 1;
    adbus_conn_block(d->connection, ADBUS_UNBLOCK, &s->block, -1);

    return ret;
}

int adbus_call_block(adbus_Call* call)
{
    int ret = 0;
    struct ProxyBlockData* d = NEW(struct ProxyBlockData);

    d->callback         = call->callback;
    d->cuser            = call->cuser;
    d->error            = call->error;
    d->euser            = call->euser;
    d->release[0]       = call->release[0];
    d->release[1]       = call->release[1];
    d->ruser[0]         = call->ruser[0];
    d->ruser[1]         = call->ruser[1];

    call->callback      = &ProxyBlockCallback;
    call->error         = &ProxyBlockError;
    call->cuser         = d;
    call->euser         = d;
    call->release[0]    = &ProxyBlockRelease;
    call->ruser[0]      = d;
    call->release[1]    = NULL;
    call->ruser[1]      = NULL;

    d->connection       = call->proxy->connection;
    d->ret              = &ret;

    if (adbus_call_send(call))
        return -1;

    if (adbus_conn_block(d->connection, ADBUS_BLOCK, &d->block, call->timeoutms))
        return -1;

    /* Returns -1 on send, block, and timeout, 0 on success, 1 on dbus error */
    return ret;
}





