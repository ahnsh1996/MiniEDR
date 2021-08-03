#pragma once

#define MAX_STRING_LEN  1024
#define MAX_BUFFER_SIZE (MAX_STRING_LEN * sizeof(WCHAR))

// ����̹����� �߻��� �̺�Ʈ ����
// ���� �α׷� ��ϵǴ� �̺�Ʈ�ʹ� �ణ�� ���̰� ����
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

// ���� ��� ���α׷����� �۽��ϴ� �޽���
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