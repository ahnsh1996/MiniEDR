#include <windows.h>
#include <sys/timeb.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <tlhelp32.h>

#include "Utility.h"

// ������ UTC �ð��� ���ڿ��� ��ȯ
std::wstring Utility::get_current_utctime()
{
    struct timeb utc_timeb;
    struct tm utc_tm;
    std::wstringstream utc_time;

    // ���� �ð��� utc_timeb�� ����
    ftime(&utc_timeb);

    // ���� �ð��� �̿��Ͽ� UTC �ð��� ���� utc_tm�� ����
    if (gmtime_s(&utc_tm, &utc_timeb.time))
    {
        std::cout << "gmtime_s error\n";
    }

    // UTC �ð��� ���˿� ���� ���ڿ��� ����
    utc_time << std::put_time(&utc_tm, L"%F %T") << ".";
    utc_time.fill('0');
    utc_time.width(3);
    utc_time << utc_timeb.millitm;

    return utc_time.str();
}

// process id�� UTC �ð��� �̿��ؼ� process guid�� ����
inline std::wstring Utility::get_process_guid(DWORD process_id, const std::wstring& utc_time)
{
    //return std::to_string(std::hash<std::wstring>{}(std::to_wstring(process_id))) + "-" + std::to_string(std::hash<std::wstring>{}(utc_time));
    return std::to_wstring(std::hash<std::wstring>{}(std::to_wstring(process_id) + utc_time));
}

// ���� ���� ���� ���μ����� ������ process map�� �߰���
void Utility::get_current_process_info(std::unordered_map<DWORD, std::tuple<std::wstring, std::wstring, int>>& process_map)
{
    HANDLE process_snapshot;
    PROCESSENTRY32 process_entry;
    WCHAR image_file_name[MAX_IMAGE_NAME_LEN + 1];
    DWORD name_length;
    HANDLE process_handle;

    //���� ���� ���� ���μ����� �������� ����
    process_snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (process_snapshot == INVALID_HANDLE_VALUE)
    {
        std::cout << "CreateToolhelp32Snapshot error\n";
        return;
    }

    // ���μ��� ���������� ù ��° ���μ��� �˻�
    process_entry.dwSize = sizeof(PROCESSENTRY32);
    if (!Process32First(process_snapshot, &process_entry))
    {
        std::cout << "Process32First error\n";
        return;
    }

    // ���μ����� ������ ���������� process map�� �߰�
    std::wstring current_utctime = Utility::get_current_utctime();
    do
    {
        name_length = MAX_IMAGE_NAME_LEN;
        process_handle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, process_entry.th32ProcessID);

        std::wstring current_image_name = (QueryFullProcessImageNameW(process_handle, 0, image_file_name, &name_length)) ? image_file_name : process_entry.szExeFile;
        process_map.emplace(process_entry.th32ProcessID,
                            std::make_tuple(Utility::get_process_guid(process_entry.th32ProcessID, current_utctime), current_image_name, process_entry.cntThreads));

        CloseHandle(process_handle);
    } while (Process32Next(process_snapshot, &process_entry));

    CloseHandle(process_snapshot);
}

// ����Ϸ��� PID ���� ������ �ִٸ� ���� ������ �Բ�, ���ٸ� PID�� �α�
void Utility::log_process_info(
    std::wofstream& log_file,
    std::unordered_map<DWORD, std::tuple<std::wstring, std::wstring, int>>& process_map,
    DWORD process_id,
    int mode
)
{
    std::wstring guid_string;
    std::wstring id_string;
    std::wstring image_name_string;

    switch (mode)
    {
    case 1:
        guid_string = L"ParentProcessGuid: ";
        id_string = L"ParentProcessId: ";
        image_name_string = L"ParentProcessImage: ";
        break;
    case 2:
        guid_string = L"SourceProcessGuid: ";
        id_string = L"SourceProcessId: ";
        image_name_string = L"SourceProcessImage: ";
        break;
    case 3:
        guid_string = L"TargetProcessGuid: ";
        id_string = L"TargetProcessId: ";
        image_name_string = L"TargetProcessImage: ";
        break;
    default:
        guid_string = L"ProcessGuid: ";
        id_string = L"ProcessId: ";
        image_name_string = L"ImageFileName: ";
    }

    if (process_map.find(process_id) != process_map.end())
    {
        log_file << guid_string << std::get<0>(process_map[process_id]) << "\n"
                 << id_string << process_id << "\n"
                 << image_name_string << std::get<1>(process_map[process_id]) << "\n";
    }
    else
    {
        log_file << id_string << process_id << "\n";
    }
}