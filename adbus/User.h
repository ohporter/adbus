// vim: ts=2 sw=2 sts=2 et
//
// Copyright (c) 2009 James R. McKaskill
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//
// ----------------------------------------------------------------------------

#pragma once

#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

struct ADBusUser;

typedef void (*ADBusUserCloneFunction)(const struct ADBusUser*, struct ADBusUser*);
typedef void (*ADBusUserFreeFunction)(struct ADBusUser*);

struct ADBusUser
{
  void*  data;
  size_t size;
  ADBusUserCloneFunction clone;
  ADBusUserFreeFunction  free;
};

#define ADBusUserInit(pdata) memset(pdata, 0, sizeof(struct ADBusUser))
void ADBusUserClone(const struct ADBusUser* from, struct ADBusUser* to);
void ADBusUserFree(struct ADBusUser* data);

void ADBusUserCloneDefault(const struct ADBusUser* from, struct ADBusUser* to);
void ADBusUserFreeDefault(struct ADBusUser* data);

#ifdef __cplusplus
}
#endif
