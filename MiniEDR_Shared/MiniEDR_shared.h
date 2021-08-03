#pragma once

#define MAX_STRING_LEN  1024
#define MAX_BUFFER_SIZE (MAX_STRING_LEN * sizeof(WCHAR))

// 드라이버에서 발생한 이벤트 종류
// 실제 로그로 기록되는 이벤트와는 약간의 차이가 있음
typedef enum _MINI_EDR_EVENT {
    ProcessCreate = 1,
    ProcessTerminated,
    ThreadCreate,
    ThreadTerminated,
    FileCreate,
    RegistryCreateKey,
    RegistryPreDeleteKey,
    RegistryPostDeleteKey,
    RegistrySetValueKey,
    RegistryDeleteValueKey
} MINI_EDR_EVENT;

// 유저 모드 프로그램으로 송신하는 메시지
typedef struct _MESSAGE {
    MINI_EDR_EVENT Event;
    DWORD PID; // SrcPID
    DWORD PPID;
    DWORD ThreadId;
    DWORD DstPID;
    PVOID RegistryKey;
    WCHAR StrContents_1[MAX_STRING_LEN];
    WCHAR StrContents_2[MAX_STRING_LEN];
} MESSAGE, *PMESSAGE;

const PCWSTR PortName = L"\\MiniEDR_Port";