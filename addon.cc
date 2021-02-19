#include <node.h>
#include <nan.h>
#include <windows.h>
#include <tlhelp32.h>
#include <thread>

namespace OverlayAddon
{
    using v8::FunctionCallbackInfo;
    using v8::Local;
    using v8::Object;
    using v8::String;
    using v8::Value;

    void Run(const FunctionCallbackInfo<Value> &args)
    {
        const String::Utf8Value name(args.GetIsolate(), args[0]);
        const HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        const DWORD mask = 0x1 << (std::thread::hardware_concurrency() - 1);
        PROCESSENTRY32 entry;
        
        while (Process32Next(snap, &entry))
            if (strcmp(entry.szExeFile, *name) == 0 && entry.pcPriClassBase != 4)
                if (const HANDLE handle = OpenProcess(PROCESS_ALL_ACCESS, true, entry.th32ProcessID))
                {
                    SetProcessAffinityMask(handle, mask);
                    while (!SetPriorityClass(handle, 0x40));
                    CloseHandle(handle);
                }
                
        CloseHandle(snap);
    }

    void MoveTop(const FunctionCallbackInfo<Value> &args) {
        unsigned char* bufferData = (unsigned char*)node::Buffer::Data(args[0].As<Object>());
        unsigned long handle = *reinterpret_cast<unsigned long*>(bufferData);
        SetWindowPos((HWND)handle, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
    }

    void Initialize(const Local<Object> exports)
    {
        NODE_SET_METHOD(exports, "Run", Run);
        NODE_SET_METHOD(exports, "MoveTop", MoveTop);
    }

    NODE_MODULE(NODE_GYP_MODULE_NAME, Initialize)
}