#include <nan.h>
#include <tlhelp32.h>
#include <thread>
#include <psapi.h>
#include <list>
#pragma comment(lib, "psapi.lib")

namespace OverlayAddon
{
    using v8::Isolate;
    using v8::Context;
    using v8::FunctionCallbackInfo;
    using v8::Local;
    using v8::Array;
    using v8::Object;
    using v8::String;
    using v8::Value;

    const DWORD mask = 0x1 << (std::thread::hardware_concurrency() - 1);

    char* windowName;
    HWND window;
    PROCESSENTRY32 entry;

    void InitWindowA(const FunctionCallbackInfo<Value> &args)
    {
        Isolate* isolate = args.GetIsolate();
        String::Utf8Value name(isolate, args[1]->ToString(isolate->GetCurrentContext()).ToLocalChecked());
        windowName = new char[((std::string) *name).length()];
        std::strcpy(windowName, ((std::string) *name).c_str());
    }

    void InitWindow(const FunctionCallbackInfo<Value> &args) 
    {
        unsigned char* bufferData = (unsigned char*)node::Buffer::Data(args[0].As<Object>());
        window = (HWND) *reinterpret_cast<unsigned long*>(bufferData);
        InitWindowA(args);
    }

    std::list<int> GetPidsA(const char *szExeFile) {
        std::list<int> result;

        const HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        while (Process32Next(snap, &entry)) {
            if (strcmp(entry.szExeFile, szExeFile) == 0) {
                result.push_back(entry.th32ProcessID);
            }
        }

        CloseHandle(snap);
        return result;
    }
    
    bool FindFirst(const char* szExeFile) {
        const HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

        while (Process32Next(snap, &entry)) {
            if (strcmp(entry.szExeFile, szExeFile) == 0) {
                CloseHandle(snap);
                return true;
            }
        }

        CloseHandle(snap);
        return false;
    }

    void GetPids(const FunctionCallbackInfo<Value> &args) {
        std::list<int> pids = GetPidsA(windowName);

        Isolate *isolate = args.GetIsolate();
        Local <Array> arr = Array::New(isolate);

        for (int i = 0; i < pids.size(); i++) {
            arr->Set(isolate->GetCurrentContext(), i, v8::Number::New(isolate, *std::next(pids.begin(), i)));
        }

        return args.GetReturnValue().Set(arr);
    }

    void FindWindow(const FunctionCallbackInfo<Value>& args) {
        Isolate* isolate = args.GetIsolate();
        String::Utf8Value name(isolate, args[0]->ToString(isolate->GetCurrentContext()).ToLocalChecked());
        return args.GetReturnValue().Set(FindFirst(((std::string) *name).c_str()));
    }

    void SetLowPriority(const FunctionCallbackInfo<Value> &args)
    {
        const HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

        while (Process32Next(snap, &entry)) {
            if (strcmp(entry.szExeFile, windowName) == 0 && entry.pcPriClassBase != 4) {
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
        std::list<int> pids = GetPidsA(windowName);

        for (int i = 0; i < pids.size(); i++) {
            if (const HANDLE handle = OpenProcess(PROCESS_ALL_ACCESS, true, *std::next(pids.begin(), i))) {
                EmptyWorkingSet(handle);
                CloseHandle(handle);
            }
        }
    }

    void MoveTop(const FunctionCallbackInfo<Value> &args) 
    {
        SetWindowPos(window, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
    }

    void Initialize(const Local<Object> exports)
    {
        entry.dwSize = sizeof(PROCESSENTRY32);
           
        NODE_SET_METHOD(exports, "InitWindowA", InitWindowA);
        NODE_SET_METHOD(exports, "InitWindow", InitWindow);
        NODE_SET_METHOD(exports, "GetPids", GetPids);
        NODE_SET_METHOD(exports, "FindWindow", FindWindow);
        NODE_SET_METHOD(exports, "SetLowPriority", SetLowPriority);
        NODE_SET_METHOD(exports, "ReduceWorkingSet", ReduceWorkingSet);
        NODE_SET_METHOD(exports, "MoveTop", MoveTop);
    }

    NODE_MODULE(NODE_GYP_MODULE_NAME, Initialize)
}