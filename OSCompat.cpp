/*
 * OSCompat.cpp - OS compatibility functions.
 *
 *   Copyright (c) 2009  Higepon(Taro Minowa)  <higepon@users.sourceforge.jp>
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 *
 *   1. Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 *   TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 *   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 *   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 *   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  $Id: OScompat.cpp 183 2008-07-04 06:19:28Z higepon $
 */

#include <stdlib.h>
#include "scheme.h"
#include "Object.h"
#include "Transcoder.h"
#include "UTF8Codec.h"
#include "ByteArrayBinaryInputPort.h"
#include "OSCompat.h"

using namespace scheme;

// N.B Dont't forget to add tests to OScompatTest.cpp.

ucs4string scheme::utf8ToUcs4(const char* s, int len)
{
    ByteArrayBinaryInputPort in((uint8_t *)s, len);
    UTF8Codec codec;
    Transcoder transcoderr(&codec, EolStyle(LF), ErrorHandlingMode(IGNORE_ERROR));
    return transcoderr.getString(&in);
}

ucs4char* scheme::getEnv(const ucs4string& key)
{
    const char* value = getenv(key.ascii_c_str());
    if (NULL == value) {
        return NULL;
    }
    return utf8ToUcs4(value, strlen(value)).strdup();
}

#ifdef _WIN32
#include <stdlib.h>
#define environ _environ
#else
extern char** environ;
#endif

Object scheme::getEnvAlist()
{
    Object ret = Object::Nil;
    char** env = environ;
    while(*env) {
        char* equalPostion = strchr(*env, '=');
        ucs4string key = utf8ToUcs4(*env, equalPostion - *env);
        ucs4string value = utf8ToUcs4(equalPostion + 1, strlen(equalPostion + 1));
        ret = Object::cons(Object::cons(Object::makeString(key),
                                        Object::makeString(value)),
                           ret);
        env++;
    }
    return ret;
}