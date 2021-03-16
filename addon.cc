#include <nan.h>
#include <tlhelp32.h>
#include <thread>
#include <psapi.h>
#pragma comment(lib, "psapi.lib")

namespace OverlayAddon
{
    using v8::Isolate;
    using v8::Context;
    using v8::FunctionCallbackInfo;
    using v8::Local;
    using v8::Object;
    using v8::String;
    using v8::Value;

    const DWORD mask = 0x1 << (std::thread::hardware_concurrency() - 1);

    std::string windowName;
    HWND window;
    PROCESSENTRY32 entry;

    void InitWindow(const FunctionCallbackInfo<Value> &args) 
    {
        unsigned char* bufferData = (unsigned char*)node::Buffer::Data(args[0].As<Object>());
        window = (HWND) *reinterpret_cast<unsigned long*>(bufferData);

        Isolate* isolate = args.GetIsolate();
        String::Utf8Value name(isolate, args[1]->ToString(isolate->GetCurrentContext()).ToLocalChecked());
        windowName = (std::string) *name;
    }

    void FindWindow(const FunctionCallbackInfo<Value>& args) {
        Isolate* isolate = args.GetIsolate();
        String::Utf8Value name(isolate, args[0]->ToString(isolate->GetCurrentContext()).ToLocalChecked());

        const HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        while (Process32Next(snap, &entry)) {
            if (strcmp(entry.szExeFile, ((std::string) *name).c_str()) == 0) {
                return args.GetReturnValue().Set(true);
            }
        }

        return args.GetReturnValue().Set(false);
    }

    void SetLowPriority(const FunctionCallbackInfo<Value> &args)
    {
        const HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

        while (Process32Next(snap, &entry)) {
            if (strcmp(entry.szExeFile, windowName.c_str()) == 0 && entry.pcPriClassBase != 4) {
                if (const HANDLE handle = OpenProcess(PROCESS_ALL_ACCESS, true, entry.th32ProcessID)) {
                    SetProcessAffinityMask(handle, mask);
                    SetPriorityClass(handle, IDLE_PRIORITY_CLASS);
                    SetProcessWorkingSetSize(handle, -1, -1);
                    CloseHandle(handle);
                }
            }
        }

        CloseHandle(snap);
    }

    void ReduceWorkingSet(const FunctionCallbackInfo<Value> &args)
    {
        const HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

        while (Process32Next(snap, &entry)) {
            if (strcmp(entry.szExeFile, windowName.c_str()) == 0) {
                if (const HANDLE handle = OpenProcess(PROCESS_ALL_ACCESS, true, entry.th32ProcessID)) {
                    EmptyWorkingSet(handle);
                    CloseHandle(handle);
                }
            }
        }

        CloseHandle(snap);
    }

    void MoveTop(const FunctionCallbackInfo<Value> &args) 
    {
        SetWindowPos(window, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
    }

    void Initialize(const Local<Object> exports)
    {
        entry.dwSize = sizeof(PROCESSENTRY32);

        NODE_SET_METHOD(exports, "InitWindow", InitWindow);
        NODE_SET_METHOD(exports, "FindWindow", FindWindow);
        NODE_SET_METHOD(exports, "SetLowPriority", SetLowPriority);
        NODE_SET_METHOD(exports, "ReduceWorkingSet", ReduceWorkingSet);
        NODE_SET_METHOD(exports, "MoveTop", MoveTop);
    }

    NODE_MODULE(NODE_GYP_MODULE_NAME, Initialize)
}