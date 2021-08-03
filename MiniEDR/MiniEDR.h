#pragma once

// 유저 모드와 공통적으로 사용하는 헤더
#include "../MiniEDR_Shared/MiniEDR_shared.h"

#define MINIEDR_MESSAGE_TAG 'gatM'
#define MINIEDR_KEYNAME_TAG 'gatK'

typedef struct _DRIVER_DATA {
    PFLT_FILTER Filter;
    PFLT_PORT ServerPort;
    PFLT_PORT ClientPort;
    LARGE_INTEGER Cookie;
    UNICODE_STRING Altitude;
    BOOLEAN IsProcessMonEnabled;
    BOOLEAN IsThreadMonEnabled;
    BOOLEAN IsFileMonEnabled;
    BOOLEAN IsRegistryMonEnabled;
} DRIVER_DATA, * PDRIVER_DATA;

const PWSTR AltitudeString = L"381234";

//
// 드라이버 기본 골격
//

NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
);

NTSTATUS
FilterUnload(
    _In_ FLT_FILTER_UNLOAD_FLAGS Flags
);

VOID
UnloadDriver(
);

//
// 유저모드 프로그램과의 통신 관련
//

NTSTATUS
ConnectCallback(
    _In_ PFLT_PORT ClientPort,
    _In_opt_ PVOID ServerPortCookie,
    _In_reads_bytes_opt_(SizeOfContext) PVOID ConnectionContext,
    _In_ ULONG SizeOfContext,
    _Outptr_result_maybenull_ PVOID* ConnectionCookie
);

VOID
DisconnectCallback(
    _In_opt_ PVOID ConnectionCookie
);

VOID
SendMessage(
    _In_ PMESSAGE message
);

//
// 프로세스 관련
//

VOID
ProcessNotifyRoutine(
    _Inout_ PEPROCESS Process,
    _In_ HANDLE ProcessId,
    _In_opt_ PPS_CREATE_NOTIFY_INFO CreateInfo
);

//
// 스레드 관련
//

VOID
ThreadNotifyRoutine(
    _In_ HANDLE ProcessId,
    _In_ HANDLE ThreadId,
    _In_ BOOLEAN Create
);

//
// 파일 관련
//

FLT_POSTOP_CALLBACK_STATUS
FileNotifyRoutine(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_opt_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
);

//
// 레지스트리 관련
//

NTSTATUS
RegistryNotifyRoutine(
    _In_     PVOID CallbackContext,
    _In_opt_ PVOID Argument1,
    _In_opt_ PVOID Argument2
);

//
// 에러 관련
//

VOID
PrintError(
    NTSTATUS Status,
    PCWSTR ErrorString
)
{
    DbgPrint("[MiniEDR] %S, status is %X\n", ErrorString, Status);
}

#define CheckErrorReturn(Status, ErrorString)           if (!NT_SUCCESS(Status)) {PrintError(Status, ErrorString); return Status;}
#define CheckErrorLeave(Status, ErrorString)            if (!NT_SUCCESS(Status)) {PrintError(Status, ErrorString); leave;}
#define CheckErrorLeaveOrSet(Status, ErrorString, Flag) if (!NT_SUCCESS(Status)) {PrintError(Status, ErrorString); leave;} else {Flag = TRUE;}
#define CheckErrorOnlyPrint(Status, ErrorString)        if (!NT_SUCCESS(Status)) {PrintError(Status, ErrorString);}