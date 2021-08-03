#pragma once
#include <unordered_map>

#include "MiniEDR_User.h"

constexpr int MAX_IMAGE_NAME_LEN = 512;

namespace Utility
{
    // ������ UTC �ð��� ���ڿ��� ��ȯ
    std::wstring get_current_utctime();

    // process id�� UTC �ð��� �̿��ؼ� process guid�� ����
    inline std::wstring get_process_guid(DWORD process_id, const std::wstring& utc_time);

    // ���� ���� ���� ���μ����� ������ process map�� �߰���
    void get_current_process_info(std::unordered_map<DWORD, std::tuple<std::wstring, std::wstring, int>>& process_map);

    // ����Ϸ��� PID ���� ������ �ִٸ� ���� ������ �Բ�, ���ٸ� PID�� �α�
    void log_process_info(
        std::wofstream& log_file,
        std::unordered_map<DWORD, std::tuple<std::wstring, std::wstring, int>>& process_map,
        DWORD process_id,
        int mode
    );
}