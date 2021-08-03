#include <iostream>
#include <csignal>
#include <fstream>
#include <queue>
#include <thread>

#include "MiniEDR_User.h"
#include "Utility.h"

#pragma comment(lib, "FltLib.lib")

bool is_running = true; // 프로그램이 실행 중인지 여부

void interrupt_handler(int signal)
{
    std::cout << "프로그램 종료\n";
    is_running = false;
}

// 드라이버의 메시지를 수신하여 큐에 추가
void get_driver_message(std::queue<MESSAGE>& queue)
{
    HRESULT hr;
    HANDLE port;
    FILTER_MESSAGE filter_message;

    // MiniEDR 드라이버와의 통신 포트 연결
    hr = FilterConnectCommunicationPort(PortName,
                                        0,
                                        NULL,
                                        0,
                                        NULL,
                                        &port);
    if (IS_ERROR(hr)) {
        std::cout << "ERROR: Connecting to filter port: " << hr << "\n";
        is_running = false;
        return;
    }
    std::cout << "Port: " << port << "\n";

    while (is_running)
    {
        // 드라이버의 메시지 수신
        hr = FilterGetMessage(port,
                              &filter_message.message_header,
                              sizeof(FILTER_MESSAGE),
                              NULL);
        if (IS_ERROR(hr)) {
            std::cout << "FilterGetMessage Error: " << hr << "\n";
            is_running = false;
            break;
        }
        queue.push(filter_message.message);
    }

    CloseHandle(port);
}

void handle_event(
    std::queue<MESSAGE>& queue,
    std::unordered_map<DWORD, std::tuple<std::wstring, std::wstring, int>>& process_map,
    std::unordered_map<PVOID, std::wstring>& registry_map
)
{
    MESSAGE message;

    // 로그 파일 오픈
    std::wofstream log_file("MiniEDR_Log.txt", std::ios::app);
    log_file.imbue(std::locale("", std::locale::ctype));

    while (is_running)
    {
        if (!queue.empty()) {
            message = queue.front();

            // 현재의 UTC 시간 저장
            std::wstring current_utctime = Utility::get_current_utctime();

            // 이벤트 별로 적절한 처리 및 로깅
            switch (message.Event)
            {
            case ProcessCreate:
                process_map.emplace(message.PID, std::make_tuple(Utility::get_process_guid(message.PID, current_utctime), &message.StrContents_1[4], 0));
                
                log_file << "UtcTime: " << current_utctime << "\n"
                         << "Event: ProcessCreate" << "\n";
                Utility::log_process_info(log_file, process_map, message.PID, 0);
                log_file << "CommandLine: " << message.StrContents_2 << "\n";
                Utility::log_process_info(log_file, process_map, message.PPID, 1);
                log_file << "\n";
                     
                break;
            case ProcessTerminated:
                log_file << "UtcTime: " << current_utctime << "\n"
                         << "Event: ProcessTerminated" << "\n";
                Utility::log_process_info(log_file, process_map, message.PID, 0);
                log_file << "\n";

                process_map.erase(message.PID);
                break;
            case ThreadCreate:
                if (message.PID != message.DstPID && process_map.find(message.DstPID) != process_map.end())
                {
                    if (++std::get<2>(process_map[message.DstPID]) > 1 && (message.PID != 4 && message.DstPID != 4))
                    {
                        log_file << "UtcTime: " << current_utctime << "\n"
                                 << "Event: CreateRemoteThread" << "\n";
                        Utility::log_process_info(log_file, process_map, message.PID, 2);
                        Utility::log_process_info(log_file, process_map, message.DstPID, 3);
                        log_file  << "ThreadId: " << message.ThreadId << "\n\n";
                    }
                }
                break;
            case ThreadTerminated:
                if (process_map.find(message.PID) != process_map.end())
                {
                    --std::get<2>(process_map[message.PID]);
                }
                break;
            case FileCreate:
                log_file << "UtcTime: " << current_utctime << "\n"
                         << "Event: FileCreate" << "\n";
                Utility::log_process_info(log_file, process_map, message.PID, 0);
                log_file << "FileName: " << message.StrContents_1 << "\n\n";
                break;
            case RegistryCreateKey:
                log_file << "UtcTime: " << current_utctime << "\n"
                         << "Event: RegistryKeyCreate" << "\n";
                Utility::log_process_info(log_file, process_map, message.PID, 0);
                log_file << "Target: " << message.StrContents_1 << "\n\n";
                break;
            case RegistryPreDeleteKey:
                registry_map.emplace(message.RegistryKey, message.StrContents_1);
                break;
            case RegistryPostDeleteKey:
                if (message.StrContents_1[0] == 'O') // 키 삭제 성공 시
                {
                    log_file << "UtcTime: " << current_utctime << "\n"
                             << "Event: RegistryKeyDelete" << "\n";
                    Utility::log_process_info(log_file, process_map, message.PID, 0);
                    if (registry_map.find(message.RegistryKey) != registry_map.end())
                    {
                        log_file << "Target: " << registry_map[message.RegistryKey] << "\n\n";
                    }
                    else
                    {
                        log_file << "\n";
                    }
                }

                registry_map.erase(message.RegistryKey);
                break;
            case RegistrySetValueKey:
                log_file << "UtcTime: " << current_utctime << "\n"
                         << "Event: RegistryValueSet" << "\n";
                Utility::log_process_info(log_file, process_map, message.PID, 0);
                log_file << "Target: " << message.StrContents_1 << "\n"
                         << "Data: " << message.StrContents_2 << "\n\n";
                break;
            case RegistryDeleteValueKey:
                log_file << "UtcTime: " << current_utctime << "\n"
                         << "Event: RegistryValueDelete" << "\n";
                Utility::log_process_info(log_file, process_map, message.PID, 0);
                log_file << "Target: " << message.StrContents_1 << "\n\n";
                break;
            default:
                break;
            }

            queue.pop();
        }
    }

    log_file << "\n";
    log_file.close();
}

int main()
{
    // 드라이버에서 수신된 메시지를 저장하는 큐
    std::queue<MESSAGE> queue;

    // process id : (process guid, image file name, thread 수) 형태
    std::unordered_map<DWORD, std::tuple<std::wstring, std::wstring, int>> process_map;

    // 삭제할 레지스트리 키 : 레지스트리 키 이름 형태
    std::unordered_map<PVOID, std::wstring> registry_map;

    // 포트를 제대로 닫지 않고 종료 시 컴퓨터가 대기 상태에 빠질 수 있기 때문
    std::signal(SIGINT, interrupt_handler);

    // 드라이버의 메시지 수신하여 큐에 추가하는 스레드 실행
    std::thread message_thread(get_driver_message, std::ref(queue));

    // 현재 실행 중인 프로세스의 정보를 추가
    Utility::get_current_process_info(process_map);

    // 큐에서 메시지를 꺼내 기록하는 스레드 실행
    std::thread handle_thread(&handle_event, std::ref(queue), std::ref(process_map), std::ref(registry_map));
    
    std::string input;
    while (is_running && input != "exit")
    {
        std::cout << "종료하시려면 exit를 입력해주세요: ";
        std::cin >> input;
    }
    is_running = false;

    message_thread.join();
    handle_thread.join();

    return 0;
}