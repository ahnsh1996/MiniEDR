#pragma once

#include <windows.h>
#include <fltuser.h>

extern "C"
{
    #include "../MiniEDR_Shared/MiniEDR_shared.h"
}

// 드라이버로부터 메시지를 수신하기 위한 구조체
typedef struct _FILTER_MESSAGE {
    FILTER_MESSAGE_HEADER message_header;
    MESSAGE message; // 실제 메시지 부분
} FILTER_MESSAGE, * PFILTER_MESSAGE;