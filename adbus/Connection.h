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

#pragma once

#include "Common.h"
#include "Interface.h"
#include "Marshaller.h"
#include "Message.h"
#include "User.h"

ADBUS_API struct ADBusConnection* ADBusCreateConnection();
ADBUS_API void ADBusFreeConnection(struct ADBusConnection* connection);

ADBUS_API void ADBusSetSendCallback(
        struct ADBusConnection*     connection,
        ADBusSendCallback           callback,
        struct ADBusUser*           data);

ADBUS_API void ADBusSendMessage(
        struct ADBusConnection*     connection,
        struct ADBusMessage*        message);

ADBUS_API uint32_t ADBusNextSerial(
        struct ADBusConnection*     connection);

ADBUS_API int ADBusDispatch(
        struct ADBusConnection*     connection,
        struct ADBusMessage*        message);

ADBUS_API int ADBusRawDispatch(
        struct ADBusCallDetails*    details);

