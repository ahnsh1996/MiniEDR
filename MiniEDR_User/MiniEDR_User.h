#pragma once

#include <windows.h>
#include <fltuser.h>

extern "C"
{
    #include "../MiniEDR_Shared/MiniEDR_shared.h"
}

// ����̹��κ��� �޽����� �����ϱ� ���� ����ü
typedef struct _FILTER_MESSAGE {
    FILTER_MESSAGE_HEADER message_header;
    MESSAGE message; // ���� �޽��� �κ�
} FILTER_MESSAGE, * PFILTER_MESSAGE;