#pragma once
#include <unordered_map>

#include "MiniEDR_User.h"

constexpr int MAX_IMAGE_NAME_LEN = 512;

namespace Utility
{
    // 현재의 UTC 시간을 문자열로 반환
    std::wstring get_current_utctime();

    // process id와 UTC 시간을 이용해서 process guid를 구함
    inline std::wstring get_process_guid(DWORD process_id, const std::wstring& utc_time);

    // 현재 실행 중인 프로세스의 정보를 process map에 추가함
    void get_current_process_info(std::unordered_map<DWORD, std::tuple<std::wstring, std::wstring, int>>& process_map);

    // 기록하려는 PID 관련 정보가 있다면 관련 정보를 함께, 없다면 PID만 로깅
    void log_process_info(
        std::wofstream& log_file,
        std::unordered_map<DWORD, std::tuple<std::wstring, std::wstring, int>>& process_map,
        DWORD process_id,
        int mode
    );
}