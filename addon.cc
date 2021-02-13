/*
    Переедем на более свежую версию NODE_MODULE_VERSION сразу же, 
    как выйдет Electron 12.0
*/

#include <node.h>
#include <windows.h>
#include <tlhelp32.h>
#include <thread>

namespace OverlayAddon
{
    using v8::Array;
    using v8::Context;
    using v8::FunctionCallbackInfo;
    using v8::Isolate;
    using v8::Local;
    using v8::NewStringType;
    using v8::Number;
    using v8::Object;
    using v8::String;
    using v8::Value;

    void ProcessList(const FunctionCallbackInfo<Value> &args)
    {
        Isolate *isolate = args.GetIsolate();
        String::Utf8Value name(isolate, args[0]);

        Local<Context> context = isolate->GetCurrentContext();
        Local<Array> arr = Array::New(isolate);

        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        PROCESSENTRY32 entry;

        int i = 0;
        while (Process32Next(snap, &entry))
            if (!stricmp(entry.szExeFile, *name))
                arr->Set(context, i++, Number::New(isolate, entry.th32ProcessID));

        CloseHandle(snap);
        args.GetReturnValue().Set(arr);
    }

    void SetLowPriority(const FunctionCallbackInfo<Value> &args)
    {
        Isolate *isolate = args.GetIsolate();
        String::Utf8Value pid(isolate, args[0]);

        if (HANDLE handle = OpenProcess(PROCESS_ALL_ACCESS, true, strtol(*pid, 0, 0)))
        {
            SetProcessAffinityMask(handle, (DWORD_PTR) 0x1 << (std::thread::hardware_concurrency() - 1));
            while (!SetPriorityClass(handle, 0x00000040));
            CloseHandle(handle);
        }
    }

    void Initialize(Local<Object> exports)
    {
        NODE_SET_METHOD(exports, "ProcessList", ProcessList);
        NODE_SET_METHOD(exports, "SetLowPriority", SetLowPriority);
    }

    NODE_MODULE(NODE_GYP_MODULE_NAME, Initialize)
}