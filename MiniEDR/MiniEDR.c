#include <fltKernel.h>
#include <dontuse.h>
#include <ntstrsafe.h>
#include "MiniEDR.h"

DRIVER_DATA DriverData;

CONST FLT_OPERATION_REGISTRATION Callbacks[] = {
    {IRP_MJ_CREATE,
     0,
     NULL,
     FileNotifyRoutine},
    {IRP_MJ_OPERATION_END}
};

CONST FLT_REGISTRATION FilterRegistration = {
    sizeof(FLT_REGISTRATION),           //  Size
    FLT_REGISTRATION_VERSION,           //  Version
    0,                                  //  Flags
    NULL,                               //  Context Registration.
    Callbacks,                          //  Operation callbacks
    FilterUnload,                       //  FilterUnload
    NULL,                               //  InstanceSetup
    NULL,                               //  InstanceQueryTeardown
    NULL,                               //  InstanceTeardownStart
    NULL,                               //  InstanceTeardownComplete
    NULL,                               //  GenerateFileName
    NULL,                               //  GenerateDestinationFileName
    NULL                                //  NormalizeNameComponent
};

// ����̹��� ����
NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
)
{
    UNREFERENCED_PARAMETER(RegistryPath);

    NTSTATUS Status = STATUS_SUCCESS;
    OBJECT_ATTRIBUTES PortAttributes;
    PSECURITY_DESCRIPTOR SecurityDescriptor;
    UNICODE_STRING ObjectName;

    DbgPrint("[MiniEDR] DriverEntry\n");

    try
    {
        // �̴����� ����̹� ���(+ ���� ��ƾ ���)
        Status = FltRegisterFilter(DriverObject,
                                   &FilterRegistration,
                                   &DriverData.Filter);
        CheckErrorLeaveOrSet(Status, L"DriverEntry, FltRegisterFilter error", DriverData.IsFileMonEnabled);
        
        // ���μ��� ��ƾ ���
        Status = PsSetCreateProcessNotifyRoutineEx(ProcessNotifyRoutine, FALSE);
        CheckErrorLeaveOrSet(Status, L"DriverEntry, PsSetCreateProcessNotifyRoutineEx error", DriverData.IsProcessMonEnabled);
        
        // ������ ��ƾ ���
        Status = PsSetCreateThreadNotifyRoutine(ThreadNotifyRoutine);
        CheckErrorLeaveOrSet(Status, L"DriverEntry, PsSetCreateThreadNotifyRoutine error", DriverData.IsThreadMonEnabled);
        
        // ������Ʈ�� ��ƾ ���
        RtlInitUnicodeString(&DriverData.Altitude, AltitudeString);
        Status = CmRegisterCallbackEx(RegistryNotifyRoutine,
                                      &DriverData.Altitude,
                                      DriverObject,
                                      NULL,
                                      &DriverData.Cookie,
                                      NULL);
        CheckErrorLeaveOrSet(Status, L"DriverEntry, CmRegisterCallbackEx error", DriverData.IsRegistryMonEnabled);
        
        // ���� ��� ���α׷����� ����� ���� ��Ʈ ����
        RtlInitUnicodeString(&ObjectName, PortName);
        Status = FltBuildDefaultSecurityDescriptor(&SecurityDescriptor, FLT_PORT_ALL_ACCESS);
        CheckErrorLeave(Status, L"DriverEntry, FltBuildDefaultSecurityDescriptor error");

        InitializeObjectAttributes(&PortAttributes,
                                   &ObjectName,
                                   OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                   NULL,
                                   SecurityDescriptor);

        Status = FltCreateCommunicationPort(DriverData.Filter,
                                            &DriverData.ServerPort,
                                            &PortAttributes,
                                            NULL,
                                            ConnectCallback,
                                            DisconnectCallback,
                                            NULL,
                                            1);
        FltFreeSecurityDescriptor(SecurityDescriptor);
        CheckErrorLeave(Status, L"DriverEntry, FltCreateCommunicationPort error");

        DbgPrint("[MiniEDR] Server port: 0x%p\n", DriverData.ServerPort);

        // ���͸� ����
        Status = FltStartFiltering(DriverData.Filter);
        CheckErrorLeave(Status, L"DriverEntry, FltStartFiltering error");
    }
    finally
    {
        if (!NT_SUCCESS(Status))
        {
            UnloadDriver();
        }
    }

    return Status;
}

// ����̹� ��ε� ��ƾ
NTSTATUS
FilterUnload(
    _In_ FLT_FILTER_UNLOAD_FLAGS Flags
)
{
    UNREFERENCED_PARAMETER(Flags);

    UnloadDriver();

    return STATUS_SUCCESS;
}

// �������� ����̹� ��ε� ��ƾ
VOID
UnloadDriver(
)
{
    NTSTATUS Status;

    DbgPrint("[MiniEDR] UnloadDriver\n");

    if (DriverData.ServerPort != NULL)
    {
        FltCloseCommunicationPort(DriverData.ServerPort);
    }

    if (DriverData.IsProcessMonEnabled)
    {
        Status = PsSetCreateProcessNotifyRoutineEx(ProcessNotifyRoutine, TRUE);
        CheckErrorOnlyPrint(Status, L"UnloadDriver, PsSetCreateProcessNotifyRoutineEx(remove) error");
    }

    if (DriverData.IsThreadMonEnabled)
    {
        Status = PsRemoveCreateThreadNotifyRoutine(ThreadNotifyRoutine);
        CheckErrorOnlyPrint(Status, L"UnloadDriver, PsRemoveCreateThreadNotifyRoutine error");
    }

    if (DriverData.IsRegistryMonEnabled)
    {
        Status = CmUnRegisterCallback(DriverData.Cookie);
        CheckErrorOnlyPrint(Status, L"UnloadDriver, CmUnRegisterCallback error");
    }

    if (DriverData.IsFileMonEnabled)
    {
        FltUnregisterFilter(DriverData.Filter);
    }
}

// ���� ���� ����� ���
NTSTATUS
ConnectCallback(
    _In_ PFLT_PORT ClientPort,
    _In_opt_ PVOID ServerPortCookie,
    _In_reads_bytes_opt_(SizeOfContext) PVOID ConnectionContext,
    _In_ ULONG SizeOfContext,
    _Outptr_result_maybenull_ PVOID* ConnectionCookie
)
{
    UNREFERENCED_PARAMETER(ServerPortCookie);
    UNREFERENCED_PARAMETER(ConnectionContext);
    UNREFERENCED_PARAMETER(SizeOfContext);
    UNREFERENCED_PARAMETER(ConnectionCookie = NULL);

    DriverData.ClientPort = ClientPort;

    DbgPrint("[MiniEDR] Connected, port = 0x%p\n", ClientPort);

    return STATUS_SUCCESS;
}

// ���� ���� ������ �����ǰų� ����̹��� ��ε�� ���
VOID
DisconnectCallback(
    _In_opt_ PVOID ConnectionCookie
)
{
    UNREFERENCED_PARAMETER(ConnectionCookie);
    DbgPrint("[MiniEDR] Disconnected, Port = 0x%p\n", DriverData.ClientPort);
    FltCloseClientPort(DriverData.Filter, &DriverData.ClientPort);
}

// ���� ��忡 �޽����� ������ �Լ�
VOID
SendMessage(
    _In_ PMESSAGE message
)
{
    NTSTATUS Status = STATUS_THREAD_IS_TERMINATING;

    if (DriverData.ClientPort == NULL) {
        return;
    }
    
    while (Status == STATUS_THREAD_IS_TERMINATING)
    {
        Status = FltSendMessage(DriverData.Filter,
                                &DriverData.ClientPort,
                                message,
                                sizeof(MESSAGE),
                                NULL,
                                NULL,
                                NULL);
        if (Status != STATUS_THREAD_IS_TERMINATING)
        {
            CheckErrorOnlyPrint(Status, L"SendMessage, FltSendMessage error");
        }
    }
}

// ���μ��� �̺�Ʈ ���
VOID
ProcessNotifyRoutine(
    _Inout_ PEPROCESS Process,
    _In_ HANDLE ProcessId,
    _In_opt_ PPS_CREATE_NOTIFY_INFO CreateInfo
)
{
    UNREFERENCED_PARAMETER(Process);
    PMESSAGE Msg;
    
    Msg = ExAllocatePoolWithTag(NonPagedPool,
                                sizeof(MESSAGE),
                                MINIEDR_MESSAGE_TAG);
    
    if (Msg == NULL)
    {
        DbgPrint("[MiniEDR] ProcessNotifyRoutine, ExAllocatePoolWithTag error\n");
        return;
    }

    RtlZeroMemory(Msg, sizeof(MESSAGE));
    
    try
    {
        if (CreateInfo) // ���μ��� ���� ��
        {
            NTSTATUS Status;

            Msg->Event = ProcessCreate;
#pragma warning(push)
#pragma warning(disable:4311)
            Msg->PID = (DWORD)ProcessId;
            Msg->PPID = (DWORD)CreateInfo->ParentProcessId;
#pragma warning(pop)

            Status = RtlStringCbCopyUnicodeString(Msg->StrContents_1, MAX_BUFFER_SIZE, CreateInfo->ImageFileName);
            CheckErrorLeave(Status, L"ProcessNotifyRoutine, RtlStringCbCopyUnicodeString error");

            Status = RtlStringCbCopyUnicodeString(Msg->StrContents_2, MAX_BUFFER_SIZE, CreateInfo->CommandLine);
            CheckErrorLeave(Status, L"ProcessNotifyRoutine, RtlStringCbCopyUnicodeString error");
        }
        else // ���μ��� ���� ��
        {
            Msg->Event = ProcessTerminated;
#pragma warning(push)
#pragma warning(disable:4311)
            Msg->PID = (DWORD)ProcessId;
#pragma warning(pop)
        }

        SendMessage(Msg);
    }
    finally 
    {
        if(Msg != NULL)
        {
            ExFreePoolWithTag(Msg, MINIEDR_MESSAGE_TAG);
        }
    }
}

// ������ �̺�Ʈ ���
VOID
ThreadNotifyRoutine(
    _In_ HANDLE ProcessId,
    _In_ HANDLE ThreadId,
    _In_ BOOLEAN Create
)
{
    PMESSAGE Msg;
    
    Msg = ExAllocatePoolWithTag(NonPagedPool,
                                sizeof(MESSAGE),
                                MINIEDR_MESSAGE_TAG);

    if (Msg == NULL)
    {
        DbgPrint("[MiniEDR] ThreadNotifyRoutine, ExAllocatePoolWithTag error\n");
        return;
    }

    RtlZeroMemory(Msg, sizeof(MESSAGE));

    if (Create) // ������ ���� ��
    {
        Msg->Event = ThreadCreate;
#pragma warning(push)
#pragma warning(disable:4311)
        Msg->PID = (DWORD)PsGetCurrentProcessId(); // SrcPID
        Msg->ThreadId = (DWORD)ThreadId;
        Msg->DstPID = (DWORD)ProcessId;
#pragma warning(pop)
    }
    else // ������ ���� ��
    {
        Msg->Event = ThreadTerminated;
#pragma warning(push)
#pragma warning(disable:4311)
        Msg->PID = (DWORD)ProcessId;
#pragma warning(pop)
    }

    SendMessage(Msg);
    if (Msg != NULL)
    {
        ExFreePoolWithTag(Msg, MINIEDR_MESSAGE_TAG);
    }
}

// ���� �̺�Ʈ ���
FLT_POSTOP_CALLBACK_STATUS
FileNotifyRoutine(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_opt_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
)
{
    UNREFERENCED_PARAMETER(CompletionContext);
    UNREFERENCED_PARAMETER(Flags);
    UNREFERENCED_PARAMETER(FltObjects);

    HANDLE ProcessId = PsGetCurrentProcessId();
    UNICODE_STRING DosName;
    PMESSAGE Msg;
    
    Msg = ExAllocatePoolWithTag(NonPagedPool,
                                sizeof(MESSAGE),
                                MINIEDR_MESSAGE_TAG);

    if (Msg == NULL)
    {
        DbgPrint("[MiniEDR] FileNotifyRoutine, ExAllocatePoolWithTag error\n");
        return FLT_POSTOP_FINISHED_PROCESSING;
    }

    RtlZeroMemory(Msg, sizeof(MESSAGE));

    try
    {
        if (Data->IoStatus.Information == FILE_CREATED) // ���� ���� ��
        {
            PUNICODE_STRING FileName = &Data->Iopb->TargetFileObject->FileName;
            NTSTATUS Status;
            
            Status = IoVolumeDeviceToDosName(Data->Iopb->TargetFileObject->DeviceObject, &DosName); // 'C:'�� ���� ����̺� ���ڸ� ������
            CheckErrorLeave(Status, L"FileNotifyRoutine, IoVolumeDeviceToDosName error");

            Msg->Event = FileCreate;
#pragma warning(push)
#pragma warning(disable:4311)
            Msg->PID = (DWORD)ProcessId;
#pragma warning(pop)

            Status = RtlStringCbCopyUnicodeString(Msg->StrContents_1, MAX_BUFFER_SIZE, &DosName);
            CheckErrorLeave(Status, L"FileNotifyRoutine, RtlStringCbCopyUnicodeString error");
            Status = RtlStringCbCatNW(Msg->StrContents_1, MAX_BUFFER_SIZE, FileName->Buffer, FileName->Length);
            CheckErrorLeave(Status, L"FileNotifyRoutine, RtlStringCbCatNW error");

            SendMessage(Msg);
        }
    }
    finally
    {
        if (Msg != NULL)
        {
            ExFreePoolWithTag(Msg, MINIEDR_MESSAGE_TAG);
        }
        return FLT_POSTOP_FINISHED_PROCESSING;
    }
}

// ������Ʈ�� �̺�Ʈ ���
NTSTATUS
RegistryNotifyRoutine(
    _In_ PVOID CallbackContext,
    _In_opt_ PVOID Argument1,
    _In_opt_ PVOID Argument2
)
{
    UNREFERENCED_PARAMETER(CallbackContext);

    REG_NOTIFY_CLASS NotifyClass = (REG_NOTIFY_CLASS)(ULONG_PTR)Argument1;
    BOOLEAN IsPost = TRUE;

    if (Argument2 == NULL)
    {
        DbgPrint("[MiniEDR] RegistryNotifyRoutine, Argument2 is NULL\n");
        return STATUS_SUCCESS;
    }

    // ����� �Ǵ� ������ �ƴ� �͵��� �ٷ� ��ȯ
    switch (NotifyClass)
    {
    case RegNtPostDeleteKey:
    case RegNtPostCreateKeyEx:
    case RegNtPostDeleteValueKey:
    case RegNtPostSetValueKey:
        break;
    case RegNtPreDeleteKey:
        IsPost = FALSE;
        break;
    default:
        return STATUS_SUCCESS;
    }

    PREG_POST_OPERATION_INFORMATION PostInfo = (IsPost)? (PREG_POST_OPERATION_INFORMATION)Argument2 : NULL;
    
    // ������Ʈ�� ���� ����� ������ ��쿡�� ����ϱ� ����
    // DeleteKey�� ��� ���� ��忡�� �Ǵ�
    if (NotifyClass != RegNtPreDeleteKey && NotifyClass != RegNtPostDeleteKey)
    {
        if (PostInfo->Status != STATUS_SUCCESS || PostInfo->Object == NULL)
        {
            return STATUS_SUCCESS;
        }
    }

    PVOID KeyObject = (IsPost) ? PostInfo->Object : ((PREG_DELETE_KEY_INFORMATION)Argument2)->Object;
    HANDLE ProcessId = PsGetCurrentProcessId();
    POBJECT_NAME_INFORMATION NameInfo = NULL;
    ULONG NeededLength = 0;
    NTSTATUS Status;
    PMESSAGE Msg;

    Msg = ExAllocatePoolWithTag(NonPagedPool,
                                sizeof(MESSAGE),
                                MINIEDR_MESSAGE_TAG);

    if (Msg == NULL)
    {
        DbgPrint("[MiniEDR] RegistryNotifyRoutine, ExAllocatePoolWithTag error\n");
        return STATUS_SUCCESS;
    }

    RtlZeroMemory(Msg, sizeof(MESSAGE));

    try
    {
        // Ű �̸��� �����ϴ� ���
        if (NotifyClass != RegNtPostDeleteKey)
        {
            // ������Ʈ�� Ű�� ���̸� ������
            Status = ObQueryNameString(KeyObject,
                                       NULL,
                                       0,
                                       &NeededLength);

            // ���̸�ŭ �޸� �Ҵ�
            if (Status == STATUS_INFO_LENGTH_MISMATCH) {
                NameInfo = ExAllocatePoolWithTag(NonPagedPool,
                                                 NeededLength,
                                                 MINIEDR_KEYNAME_TAG);
                if (NameInfo == NULL)
                {
                    DbgPrint("[MiniEDR] RegistryNotifyRoutine, ExAllocatePoolWithTag error\n");
                    leave;
                }
            }
            else
            {
                CheckErrorLeave(Status, L"RegistryNotifyRoutine, ObQueryNameString error");
            }

            // ������Ʈ�� Ű �̸��� ������
            ULONG NameInfoSize = NeededLength;
            Status = ObQueryNameString(KeyObject,
                                       NameInfo,
                                       NameInfoSize,
                                       &NeededLength);
            CheckErrorLeave(Status, L"RegistryNotifyRoutine, ObQueryNameString error");

            Status = RtlStringCbCopyUnicodeString(Msg->StrContents_1, MAX_BUFFER_SIZE, &NameInfo->Name);
            CheckErrorLeave(Status, L"RegistryNotifyRoutine, RtlStringCbCopyUnicodeString error");
        }

        // ������Ʈ�� ���� �� ó��
        switch (NotifyClass)
        {
        case RegNtPostCreateKeyEx:
            Msg->Event = RegistryCreateKey;
            break;

        case RegNtPreDeleteKey:
            Msg->Event = RegistryPreDeleteKey;
            Msg->RegistryKey = KeyObject;
            break;

        case RegNtPostDeleteKey:
            Msg->Event = RegistryPostDeleteKey;
            Msg->RegistryKey = KeyObject;
            
            Status = RtlStringCbCatW(Msg->StrContents_1, MAX_BUFFER_SIZE, (PostInfo->Status == STATUS_SUCCESS)? L"O" :L"X"); // ���� ����
            CheckErrorLeave(Status, L"RegistryNotifyRoutine, RtlStringCbCatW error");
            break;

        case RegNtPostDeleteValueKey:
            Msg->Event = RegistryDeleteValueKey;
            
            PUNICODE_STRING ValueName = ((PREG_DELETE_VALUE_KEY_INFORMATION)PostInfo->PreInformation)->ValueName;
            Status = RtlStringCbCatW(Msg->StrContents_1, MAX_BUFFER_SIZE, L"\\");
            CheckErrorLeave(Status, L"RegistryNotifyRoutine, RtlStringCbCatW error");
            Status = RtlStringCbCatNW(Msg->StrContents_1, MAX_BUFFER_SIZE, ValueName->Buffer, ValueName->Length);
            CheckErrorLeave(Status, L"RegistryNotifyRoutine, RtlStringCbCatNW error");
            break;
            
        case RegNtPostSetValueKey:
            Msg->Event = RegistrySetValueKey;
            UNICODE_STRING Data;
            WCHAR DataBuffer[40];
            
            Data.Buffer = DataBuffer;
            Data.Length = 0;
            Data.MaximumLength = 40 * sizeof(WCHAR);

            PREG_SET_VALUE_KEY_INFORMATION PreInfo = (PREG_SET_VALUE_KEY_INFORMATION)PostInfo->PreInformation;

            Status = RtlStringCbCatW(Msg->StrContents_1, MAX_BUFFER_SIZE, L"\\");
            CheckErrorLeave(Status, L"RegistryNotifyRoutine, RtlStringCbCatW error");

            Status = RtlStringCbCatNW(Msg->StrContents_1, MAX_BUFFER_SIZE, PreInfo->ValueName->Buffer, PreInfo->ValueName->Length);
            CheckErrorLeave(Status, L"RegistryNotifyRoutine, RtlStringCbCatNW error");

            switch (PreInfo->Type) // ������Ʈ�� value�� data ���
            {
            case REG_SZ:
                Status = RtlStringCbCatW(Msg->StrContents_2, MAX_BUFFER_SIZE, L"(REG_SZ) ");
                CheckErrorLeave(Status, L"RegistryNotifyRoutine, RtlStringCbCatW error");

                RtlInitUnicodeString(&Data, PreInfo->Data);
                break;
            case REG_EXPAND_SZ:
                Status = RtlStringCbCatW(Msg->StrContents_2, MAX_BUFFER_SIZE, L"(REG_EXPAND_SZ) ");
                CheckErrorLeave(Status, L"RegistryNotifyRoutine, RtlStringCbCatW error");

                RtlInitUnicodeString(&Data, PreInfo->Data);
                break;
            case REG_DWORD:
                Status = RtlStringCbCatW(Msg->StrContents_2, MAX_BUFFER_SIZE, L"(REG_DWORD) 0x");
                CheckErrorLeave(Status, L"RegistryNotifyRoutine, RtlStringCbCatW error");

                Status = RtlIntegerToUnicodeString(*(PULONG)PreInfo->Data, 16, &Data);
                CheckErrorLeave(Status, L"RegistryNotifyRoutine, RtlIntegerToUnicodeString error");
                break;
            case REG_QWORD:
                Status = RtlStringCbCatW(Msg->StrContents_2, MAX_BUFFER_SIZE, L"(REG_QWORD) 0x");
                CheckErrorLeave(Status, L"RegistryNotifyRoutine, RtlStringCbCatW error");

                Status = RtlInt64ToUnicodeString(*(PULONG64)PreInfo->Data, 16, &Data);
                CheckErrorLeave(Status, L"RegistryNotifyRoutine, RtlInt64ToUnicodeString error");
                break;
            default:
                Status = RtlUnicodeStringCatString(&Data, L"Binary Data");
                CheckErrorLeave(Status, L"RegistryNotifyRoutine, RtlUnicodeStringCatString error");
                break;
            }
            Status = RtlStringCbCatNW(Msg->StrContents_2, MAX_BUFFER_SIZE, Data.Buffer, Data.Length);
            CheckErrorLeave(Status, L"RegistryNotifyRoutine, RtlStringCbCatNW error");
            break;
        }
#pragma warning(push)
#pragma warning(disable:4311)
        Msg->PID = (DWORD)ProcessId;
#pragma warning(pop)
        SendMessage(Msg);
    }
    finally
    {
        if (NameInfo != NULL)
        {
            ExFreePoolWithTag(NameInfo, MINIEDR_KEYNAME_TAG);
        }
        if (Msg != NULL)
        {
            ExFreePoolWithTag(Msg, MINIEDR_MESSAGE_TAG);
        }
    }

    return STATUS_SUCCESS;
}