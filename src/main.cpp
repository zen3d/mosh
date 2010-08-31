/*
 * main.cpp - Interpreter main.
 *
 *   Copyright (c) 2008  Higepon(Taro Minowa)  <higepon@users.sourceforge.jp>
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
 *  $Id: main.cpp 2013 2009-08-11 03:46:06Z higepon $
 */

#include <time.h>
#include <signal.h>
#include <stdio.h>
#ifdef _WIN32
#else
#endif // NOT _WIN32
#include "Object.h"
#include "Object-inl.h"
#include "Pair.h"
#include "Pair-inl.h"
#include "scheme.h"
#include "VM.h"
#include "SString.h"
#include "Symbol.h"
#include "getoptU.h"
#include "OSCompat.h"
#include "OSCompatThread.h"
#include "MultiVMProcedures.h"
#include "VMFactory.h"
#include "Gloc.h"
#include "Closure.h"
#include "VM-inl.h"

#ifdef WITH_NMOSH_DEFAULTS
#include "Bignum.h"
#include "ByteArrayBinaryInputPort.h"
#include "BinaryInputPort.h"
#include "SimpleStruct.h"
#include "FaslReader.h"
#include "ProcedureMacro.h"
#endif

bool debug_on;
using namespace scheme;

#ifdef MONA
VM* theVM;
#else
static VM* theVM;
#endif

#ifdef WITH_NMOSH_DEFAULTS
extern "C" const uint8_t* nmosh_dbg_image_ptr;
extern "C" unsigned int nmosh_dbg_image_size;
extern "C" const uint8_t* nmosh_image_ptr;
extern "C" const unsigned int nmosh_image_size;
#else
extern "C" const uint8_t* psyntax_mosh_image_ptr;
extern "C" unsigned int psyntax_mosh_image_size;
#endif

#ifdef WITH_NMOSH_DEFAULTS
Object
internalGetStackTraceObj(VM* vm, int argc, const Object* argv){
	//DeclareProcedureName("%get-stack-trace-obj");
	return vm->getStackTraceObj();
}
Object
internalGetNmoshDbgImage(VM* vm, int argc, const Object* argv){
    //DeclareProcedureName("%get-nmosh-dbg-image");
    return FaslReader(vm, new ByteArrayBinaryInputPort(nmosh_dbg_image_ptr, nmosh_dbg_image_size)).get();
}
#endif

Object argsToList(int argc, int optind, ucs4char** argvU)
{
    Object p = Object::Nil;
    for (int i = optind; i < argc; i++) {
        p = Object::cons(Object::makeString(argvU[i]), p);
    }
    return Pair::reverse(p);
}

void showVersion()
{
#ifdef GIT_BRANCH
    printf("Mosh R6RS scheme interpreter, version %s (revision %s %s %s) \n", PACKAGE_VERSION, GIT_BRANCH, GIT_COMMIT_DATE, GIT_COMMIT_REVISION);
#else
    printf("Mosh R6RS scheme interpreter, version %s\n", PACKAGE_VERSION);
#endif
    exit(0);
}

Object activateR6RSMode(VM* vm, bool isDebugExpand)
{
#ifdef WITH_NMOSH_DEFAULTS
    return vm->activateR6RSMode(nmosh_image_ptr, nmosh_image_size, isDebugExpand);
#else
    return vm->activateR6RSMode(psyntax_mosh_image_ptr, psyntax_mosh_image_size, isDebugExpand);
#endif
}

void showUsage()
{
    fprintf(stderr,
#ifndef WITH_NMOSH_DEFAULTS
            "Usage: mosh <options> [file]\n"
#else
            "Usage: nmosh <options> [file]\n"
#endif
            "options:\n"
            "  -5                Run with safe mode (Almost R5RS).\n"
            "  -V                Prints version and exits.\n"
            "  -v                Prints version and exits.\n"
            "  -h                Prints this help.\n"
#ifdef ENABLE_PROFILER
            "  -p                Executes with profiler.\n"
#endif
//            "  -t                Executes test.\n"
            "  --help            Prints this help.\n"
            "  --disable-acc     disables auto-compile-cache.\n"
            "  --clean-acc       cleans auto-compile-cache.\n"
            "  --verbose         Show library serialisation messages.\n"
#ifdef WITH_NMOSH_DEFAULTS
            "  --guru-mode       Provide guru-mediation for nmosh developer.\n"
#endif
            "  --loadpath=<path> Add library loadpath.\n\n"
            " MOSH_LOADPATH\n"
            "  You can add library loadpath by using environment variable MOSH_LOADPATH, with \':\'(use \';\' for Windows) separated paths.\n\n"
            "bug report:\n"
            "  http://code.google.com/p/mosh-scheme/issues\n"
            "  higepon@users.sourceforge.jp\n\n"
        );
    exit(EXIT_FAILURE);
}

#ifdef ENABLE_PROFILER
void signal_handler(int signo)
{
    if (signo == SIGPROF && theVM != NULL) {
        theVM->collectProfile();
    }
}
#endif

#ifndef _WIN32
#include <sys/types.h>
#include <sys/wait.h>
#endif

int main(int argc, char *argv[])
{
    uint64_t s1 = MonAPI::Date::nowInMsec();
    // call this before any allocation.
    mosh_init();

    uint64_t s11 = MonAPI::Date::nowInMsec();
    ucs4char opt;
    int optionIndex = 0;
    bool isTestOption    = false;
    bool isCompileString = false;
    bool isProfilerOn      = false;
    bool isR6RSBatchMode = true;
    bool disableAcc = false;
    bool verbose = false;
    bool cleanAcc = false;
    bool isDebugExpand   = false; // show the result of psyntax expansion.
#ifdef WITH_NMOSH_DEFAULTS
    bool isGuruMode = false;
#endif
    ucs4char* initFile = NULL;
    ucs4char* loadPath = NULL;

   static struct optionU long_options[] = {
       {UC("loadpath"), optional_argument, 0, 'L'},
       {UC("help"), 0, 0, 'h'},
       {UC("disable-acc"), 0, 0, 'd'},
       {UC("clean-acc"), 0, 0, 'C'},
       {UC("verbose"), 0, 0, 'a'},
#ifdef WITH_NMOSH_DEFAULTS
       {UC("guru-mode"), 0, 0, 'G'},
#endif
       {0, 0, 0, 0}
   };

   ucs4char** argvU = getCommandLine(argc, argv);

   while ((opt = getopt_longU(argc, argvU, UC("htvpVcl:5rze"), long_options, &optionIndex)) != -1) {
        switch (opt) {
        case 'h':
            showUsage();
            break;
        case 'd':
            disableAcc = true;
            break;
        case 'l':
            initFile = optargU;
            break;
        case 'L':
            loadPath = optargU;
            break;
        case 'b':
            isR6RSBatchMode = true;
            break;
        case 'v':
            showVersion();
            break;
        case 'V':
            showVersion();
            break;
        case 't':
            isTestOption = true;
            break;
        case 'p':
            isProfilerOn = true;
            break;
        case 'c':
            isCompileString = true;
            break;
        case 'a':
            verbose = true;
            break;
        case 'C':
            cleanAcc = true;
            disableAcc = true;
            break;
        case 'e':
            isDebugExpand = true;
            break;
        case '5':
            isR6RSBatchMode = false;
            break;
#ifdef WITH_NMOSH_DEFAULTS
        case 'G':
            isGuruMode = true;
            break;
#endif
        default:
            fprintf(stderr, "invalid option %c", opt);
            showUsage();
            exit(EXIT_FAILURE);
        }
    }

    if (isProfilerOn && argc == optindU) {
        fprintf(stderr, "[file] not specified\n");
        showUsage();
        exit(EXIT_FAILURE);
    }


    // for Shell mode.
    // VM(=parent) ignores SIGINT, but child use default handler. (See %fork)
//    signal(SIGINT, SIG_IGN);

#if defined(_WIN32) || defined(MONA)
#else
    signal(SIGPIPE, SIG_IGN);
#endif

    VMFactory factory;
    const int INITIAL_STACK_SIZE = 10000;
    uint64_t s2 = MonAPI::Date::nowInMsec();
    // N.B.
    // We store the VM instance in thread specific storage.
    // Used for storing yylex and re2c which has only global interfaces.
    theVM = factory.create(INITIAL_STACK_SIZE, isProfilerOn);

    if (!setCurrentVM(theVM)) {
        fprintf(stderr, "fatal vm specific failure\n");
        exit(-1);
    }

    theVM->setValueString(UC("*command-line-args*"), argsToList(argc, optindU, argvU));
#ifdef WITH_NMOSH_DEFAULTS
    theVM->setValueString(UC("%get-stack-trace-obj"),Object::makeCProcedure(internalGetStackTraceObj));
    theVM->setValueString(UC("%get-nmosh-dbg-image"),Object::makeCProcedure(internalGetNmoshDbgImage));
    theVM->setValueString(UC("%nmosh-guru-mode"),Object::makeBool(isGuruMode));
#endif
    uint64_t s3 = MonAPI::Date::nowInMsec();
    if (isTestOption) {
        theVM->loadFileWithGuard(UC("all-tests.scm"));
//     } else if (isCompileString) {
//         ucs4string text
//         const Object port = Object::makeStringInputPort((const uint8_t*)argvU[optindU], strlen(argv[optindU]));
//         bool errorOccured = false;
//         const Object code = port.toTextualInputPort()->getDatum(errorOccured);
//         if (errorOccured) {
//             callLexicalViolationImmidiaImmediately(theVM, "read", port.toTextualInputPort()->error());
//         } else {
//             const Object compiled = theVM->compile(code);
//             theVM->currentOutputPort().toTextualOutputPort()->display(compiled);
//         }
    } else if (isR6RSBatchMode) {
        if (NULL == loadPath) {
            theVM->setValueString(UC("%loadpath"), Object::False);
        } else {
            theVM->setValueString(UC("%loadpath"), Object::makeString(loadPath));
        }
        theVM->setValueString(UC("%verbose"), Object::makeBool(verbose));
        theVM->setValueString(UC("%disable-acc"), Object::makeBool(disableAcc));
        theVM->setValueString(UC("%clean-acc"), Object::makeBool(cleanAcc));
        activateR6RSMode(theVM, isDebugExpand);
    } else if (optindU < argc) {
        theVM->setValueString(UC("debug-expand"), Object::makeBool(isDebugExpand));
        theVM->loadFileWithGuard(Object::makeString(argvU[optindU]).toString()->data());
    } else {
        showUsage();
    }
#ifdef ENABLE_PROFILER
    if (isProfilerOn) {
        const Object result = theVM->getProfileResult();
        theVM->callClosureByName(Symbol::intern(UC("show-profile")), result);
    }
#endif
    theVM->flushAllPorts();
    uint64_t s4 = MonAPI::Date::nowInMsec();
    logprintf("Mosh %d %d %d %d\n", (int)(s11 - s1), (int)(s2-s11), (int)(s3 - s2), (int)(s4 - s3));
    // N.B.
    // static destructor will be called.
    // this means that static member *can be freed*.
    // Don't rely on static initializer and destructor on multithreads.
    // See Symbol::symbols for more detailed information.
    exit(EXIT_SUCCESS);
}
