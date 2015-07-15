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

#include <adbus.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>

#define RECV_SIZE 64 * 1024

static int quit = 0;

static void log_it(const char *str, size_t size)
{
    printf("%s\n", str);

    return;
}

static int quit_it(adbus_CbData* data)
{
    adbus_check_end(data);
    quit = 1;

    return 0;
}

static int a_signal(adbus_CbData *d)
{
    printf("Received ASignal\n");

    return 0;
}

static adbus_ssize_t send_it(void* d, adbus_Message* m)
{
    return send(*(adbus_Socket*) d, m->data, m->size, 0);
}

int main(void)
{
    adbus_Buffer* buf;
    adbus_Socket s;

    adbus_set_logger((adbus_LogCallback)log_it);

    buf = adbus_buf_new();
    s = adbus_sock_connect(ADBUS_SESSION_BUS);
    if (s == ADBUS_SOCK_INVALID || adbus_sock_cauth(s, buf))
        abort();

    adbus_ConnectionCallbacks cbs = {};
    cbs.send_message = &send_it;

    adbus_Connection* c = adbus_conn_new(&cbs, &s);

    adbus_Interface* i = adbus_iface_new("com.konsulko.ucdbus.Simple", -1);
    adbus_Member* mbr = adbus_iface_addmethod(i, "Quit", -1);
    adbus_mbr_setmethod(mbr, &quit_it, NULL);
    mbr = adbus_iface_addsignal(i, "Status", -1);
    adbus_mbr_argsig(mbr, "u", -1);

    adbus_Bind b;
    adbus_bind_init(&b);
    b.interface = i;
    b.path      = "/";
    adbus_conn_bind(c, &b);

    adbus_conn_connect(c, NULL, NULL);

    /* Register the Simple service name on the message bus */
    adbus_State* state = adbus_state_new();
    adbus_Proxy* p = adbus_proxy_new(state);
    adbus_proxy_init(p, c, "org.freedesktop.DBus", -1, "/", -1);
    adbus_Call f;
    adbus_call_method(p, &f, "RequestName", -1);
    adbus_msg_setsig(f.msg, "su", -1);
    adbus_msg_string(f.msg, "com.konsulko.ucdbus.Simple", -1);
    adbus_msg_u32(f.msg, 0);
    adbus_call_send(p, &f);
    adbus_proxy_free(p);

    /* Register a callback for the inbound ASignal */
    adbus_Match match;
    adbus_match_init(&match);
    match.addMatchToBusDaemon = 1;
    match.type      = ADBUS_MSG_SIGNAL;
    match.path      = "/";
    match.member    = "ASignal";
    match.callback  = &a_signal;
    adbus_conn_addmatch(c, &match);

    /* Wait for incoming Quit method call */
    while(!quit) {
        char* dest = adbus_buf_recvbuf(buf, RECV_SIZE);
        adbus_ssize_t recvd = recv(s, dest, RECV_SIZE, 0);
        adbus_buf_recvd(buf, RECV_SIZE, recvd);
        if (recvd < 0)
            abort();

        if (adbus_conn_parse(c, buf))
            abort();
    }

    /* Emit Status signal, 0 is down */
    adbus_Signal *status = adbus_sig_new(mbr);
    adbus_sig_bind(status, c, "/", -1);
    adbus_MsgFactory* msg = adbus_sig_msg(status);
    adbus_msg_u32(msg, 0);
    adbus_sig_emit(status);

    adbus_buf_free(buf);
    adbus_iface_free(i);
    adbus_conn_free(c);

    return 0;
}

