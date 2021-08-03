# MiniEDR
## 프로젝트 배경
그동안 사이버 위협에 대한 대응을 주로 예방(prevention)의 관점으로 시행해왔습니다. 
이러한 대표적인 보안 솔루션은 국내에서는 백신으로 유명한 안티 바이러스(Anti-Virus) 솔루션입니다. 
하지만 이와 같이 예방에 초점을 맞춘 솔루션은 알려진(Known) 악성코드에만 대응할 수 있다는 단점이 있습니다. 
과거 악성코드의 수가 상대적으로 적었을 당시에는 새롭게 생겨나는 악성코드에 대해 시그니처를 만들어서 예방의 관점으로도 대응할 수 있었지만, 
최근에는 하루에만 35만 개의 악성코드가 발견되어 이는 불가능에 가깝습니다.

<p align="center">
<image width="420" alt="total malware" src="https://user-images.githubusercontent.com/77680436/127827388-fb507c39-dfdc-498b-963d-ba83c7de2612.png"></br>
(출처: https://www.av-test.org/en/statistics/malware/)
</p>

이처럼 새롭게 생겨나는 악성코드의 절대적인 양이 늘어나고 있을 뿐만 아니라 파일리스 악성코드(Fileless Malware)와 같은 공격들은 파일을 기반으로 동작하는 안티 바이러스 솔루션으로는 탐지할 수 없습니다. 
따라서 최근의 보안 트렌드는 사이버 위협을 완벽히 예방하는 것은 불가능하다는 것을 인정하는 대신 
위협을 탐지하고 대응하는 것을 가능한 한 빠르게 하여 위협이 내부에 머무는 시간(Dwell time)을 최소화하는 것을 목표로 하고 있습니다.

이러한 대표적인 보안 솔루션이 EDR(Endpoint Detection and Response) 솔루션입니다. 
EDR 솔루션은 조직의 보안 전문가가 단말(Endpoint)에서 발생하는 의심스러운 행위를 빠르게 탐지하고 대응하여 조직의 환경을 안전하게 유지할 수 있도록 돕는 솔루션이라고 볼 수 있습니다. 
이를 위해서 EDR 솔루션은 단말에서 발생하는 행위 정보를 수집·분석하여 알려진 위협과 알려지지 않은 위협에 대한 가시성을 제공하여 탐지를 돕고, 
네트워크 격리나 프로세스 종료 등의 기능을 통해 보안 관리자의 대응을 도울 수 있습니다.

## 프로젝트 소개
MiniEDR 프로젝트는 EDR의 기능을 축소하여 직접 구현해보기 위한 프로젝트입니다. EDR은 보통 행위 정보 수집, 탐지, 대응의 부분으로 이루어져 있습니다. 
현재는 행위 정보를 수집하는 부분만 개발되었으며, 탐지와 대응은 개발 예정입니다.

## 행위 정보 수집 
단말에서 일어나는 행위를 수집하기 위해서 더 전역적이고 완벽한 모니터링을 위해 보통 EDR은 커널 레벨의 드라이버를 통한 행위 수집을 수행합니다. 
본 프로젝트 역시 행위를 모니터링하기 위해 드라이버가 개발되었습니다.

MiniEDR 프로젝트는 행위 정보를 수집해 로깅하기 위해 다음과 같은 구조로 설계되었습니다.
<p align="center">
<image width="420" alt="행위 정보 수집기 구조" src="https://user-images.githubusercontent.com/77680436/127832399-799ea1dc-9fcd-4134-9db8-6f41209a154e.png">
</p>

위와 같이 커널 영역에서 드라이버가 행위를 수집해 유저 레벨의 어플리케이션에 전송하면 유저 레벨의 어플리케이션은 이를 처리해 로깅하는 방식입니다.  

행위 정보를 수집해 로깅하는 대표적인 프로그램으로는 [Sysmon](https://docs.microsoft.com/en-us/sysinternals/downloads/sysmon)이 있습니다. 
실제로 본 프로젝트는 Sysmon을 어느 정도 모델로 진행하였으며, 로그의 내용과 형식 부분에서 많이 참고하였습니다.

MiniEDR에서 수집할 수 있는 행위 정보는 다음과 같습니다.

* <b>ProcessCreate</b>
```
<로그 예시>
UtcTime: 2021-08-02 06:24:07.163
Event: ProcessCreate
ProcessGuid: 10140982431117095939
ProcessId: 4084
ImageFileName: C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe
CommandLine: "C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe" --profile-directory=Default
ParentProcessGuid: 7014387256993649928
ParentProcessId: 4056
ParentProcessImage: C:\Windows\explorer.exe
```
프로세스의 생성 정보를 제공합니다. 위와 같이 부모 프로세스 정보를 통해 상관 관계를 파악할 수 있으며, 
CommandLine 정보를 알 수 있기 때문에 파워쉘 원 라이너 방식과 같은 공격을 파악하는 데에도 도움이 됩니다.

ProcessGuid는 로그 전역적으로 프로세스를 식별할 수 있는 ID 입니다. Process ID의 경우 프로세스가 종료되면 재사용 될 수 있기 때문에 
하나의 로그 파일 내에서 다른 프로세스지만 같은 Process ID를 가질 수 있습니다. 따라서 정확히 프로세스를 식별할 수 있도록 ProcessGuid 필드를 제공합니다.

* <b>ProcessTerminated</b>
```
<로그 예시>
UtcTime: 2021-08-02 06:25:09.415
Event: ProcessTerminated
ProcessGuid: 10140982431117095939
ProcessId: 4084
ImageFileName: C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe
```
프로세스의 종료 정보를 제공합니다.

* <b>CreateRemoteThread</b>
```
<로그 예시>
UtcTime: 2021-08-01 16:24:14.344
Event: CreateRemoteThread
SourceProcessGuid: 12478161380305138021
SourceProcessId: 4
SourceProcessImage: System
TargetProcessGuid: 5270953678262298707
TargetProcessId: 11112
TargetProcessImage: C:\Windows\System32\svchost.exe
ThreadId: 18240
```
리모트 스레드의 생성 정보를 제공합니다. 리모트 스레드의 경우 DLL Injection과 같은 공격에서 주로 사용됩니다.

위에서는 로그의 예시를 보여주기 위해 커널의 프로세스(System)가 리모트 스레드를 생성한 것을 보여주고 있지만 
실제로는 System 프로세스가 대상이 되는 경우는 신뢰할 수 있다고 생각하여 기록하지 않습니다.

유명한 [Sysmon 구성파일](https://github.com/SwiftOnSecurity/sysmon-config/blob/master/sysmonconfig-export.xml)을 살펴보면 
여기에서도 운영체제에서 사용하고 있는 프로세스는 예외로 둔 것을 확인할 수 있습니다.

* <b>FileCreate</b>
```
<로그 예시>
UtcTime: 2021-08-02 06:24:59.381
Event: FileCreate
ProcessGuid: 18283575540000956765
ProcessId: 6920
ImageFileName: C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe
FileName: C:\Users\ahn\Desktop\googlelogo_color_272x92dp.png
```
파일의 생성 정보를 제공합니다. 
일부 악성코드는 다른 악성코드를 다운받거나 생성하는 데에 집중합니다. 
파일 생성 정보는 이를 추적하는 데에 도움이 될 것입니다.

* <b>RegistryKeyCreate</b>
```
<로그 예시>
UtcTime: 2021-08-02 06:25:31.656
Event: RegistryKeyCreate
ProcessGuid: 14013579356189523975
ProcessId: 1860
ImageFileName: C:\Windows\regedit.exe
Target: \REGISTRY\MACHINE\SOFTWARE\Google\Chrome\새 키 #1
```
레지스트리 Key의 생성 정보를 제공합니다. 
악성코드는 공격의 지속성을 위해서 레지스트리를 이용하기도 하기 때문에 레지스트리 관련 정보는 이를 추적하는 데에 용이합니다.

* <b>RegistryKeyDelete</b>
```
<로그 예시>
UtcTime: 2021-08-02 06:25:35.856
Event: RegistryKeyDelete
ProcessGuid: 14013579356189523975
ProcessId: 1860
ImageFileName: C:\Windows\regedit.exe
Target: \REGISTRY\MACHINE\SOFTWARE\Google\Chrome\새 키 #1
```
레지스트리 Key의 삭제 정보를 제공합니다.

* <b>RegistryValueSet</b>
```
<로그 예시>
UtcTime: 2021-08-02 06:26:04.453
Event: RegistryValueSet
ProcessGuid: 14013579356189523975
ProcessId: 1860
ImageFileName: C:\Windows\regedit.exe
Target: \REGISTRY\MACHINE\SOFTWARE\Google\Chrome\새 값 #1
Data: (REG_QWORD) 0x1234567890123456
```
레지스트리 Value의 설정 정보를 제공합니다.
레지스트리 데이터 형식이 `REG_SZ`, `REG_EXPAND_SZ`, `REG_DWORD`, `REG_QWORD`인 경우 데이터의 내용까지 표시해주며, 그 이외의 타입에는 'Binary Data'라고 나타냅니다.

이는 Sysmon을 직접 설치하여 테스트 해본 결과 위와 같이 동작하였기 때문에, Sysmon을 모델로 한 MiniEDR 역시 동일하게 진행하였습니다.
(모든 데이터 타입을 테스트한 것은 아니지만 레지스트리 편집기에서 만들 수 있는 타입의 경우는 그러하였음)

* <b>RegistryValueDelete</b>
```
<로그 예시>
UtcTime: 2021-08-02 06:24:18.849
Event: RegistryValueDelete
ProcessGuid: 14013579356189523975
ProcessId: 1860
ImageFileName: C:\Windows\regedit.exe
Target: \REGISTRY\MACHINE\SOFTWARE\Google\Chrome\새 값 #1
```
레지스트리 Value 삭제 정보를 제공합니다.

## 탐지
이상탐지를 통한 탐지 개발 예정

## 대응
개발 예정

## 프로그램 실행 방법
**드라이버의 경우 커널 영역에서 돌아가기 때문에 문제가 발생 시 그대로 블루스크린이 발생합니다. 테스트의 경우 가상머신 환경에서 수행하기를 권장합니다.**


> 1. '드라이버 서명 적용 사용 안 함' 모드로 부팅

윈도우 10에서는 서명되지 않은 드라이버를 로드할 수 없습니다. 따라서 이를 잠시 해제하여야 하는데, 테스트 모드로 설정하여 우회할 수도 있겠지만 일회용으로는 '드라이버 서명 적용 사용 안 함' 모드를 이용한 방법이 
더 적합하기 때문에 이를 기준으로 설명합니다.

(1) [여기](https://support.microsoft.com/ko-kr/windows/windows-10%EC%97%90%EC%84%9C-%EC%95%88%EC%A0%84-%EB%AA%A8%EB%93%9C%EB%A1%9C-pc-%EC%8B%9C%EC%9E%91-92c27cff-db89-8644-1ce4-b3e5e56fe234)의 
방법으로 시작 설정에 진입합니다.  
(2) 7번을 눌러 '드라이버 서명 적용 사용 안 함' 모드로 부팅합니다.

이제 서명되지 않은 드라이버를 로드할 수 있게됩니다.

> 2. 드라이버 및 유저 모드 프로그램 실행

먼저 드라이버의 Release 폴더에서 'MiniEDR.inf' 파일을 우클릭하여 설치를 클릭해 설치를 진행합니다.  
이후 명령 프롬프트를 관리자 권한으로 실행한 후 'sc query MiniEDR'을 입력해 정상적으로 설치가 되었는지 확인합니다.

정상적으로 설치되었다면 드라이버를 로드하여 실행하기 위해서 'sc start MiniEDR`을 입력하여 드라이버를 다음과 같이 실행합니다.
<p align="center">
<image width="620" alt="실행 방법2-1" src="https://user-images.githubusercontent.com/77680436/127854454-c01f4ea3-ef79-4c18-ac3e-837c8aaecca4.png">
</p>

이후 유저 모드 프로그램이 존재하는 폴더로 이동하여 프로그램을 실행합니다.(명령 프롬프트가 관리자 권한이어야 함)
<p align="center">
<image width="620" alt="실행 방법2-2" src="https://user-images.githubusercontent.com/77680436/127854608-4ef9db06-2592-4a91-8b84-720c412ad133.png">
</p>

그러면 유저 모드 프로그램이 존재하는 폴더에 로그 파일(MiniEDR_Log.txt)이 생성되어 기록됩니다.(이미 존재한다면 이어서 기록)

> 3. 드라이버 및 유저 모드 프로그램 종료

프로그램을 종료하기 위해서 먼저 화면에 나타난 유저 모드 프로그램의 출력에 따라 exit를 입력해 MiniEDR_User.exe 프로그램을 종료한 후, 'sc stop MiniEDR'을 입력해 드라이버를 언로드합니다.
(비정상 종료 시 컴퓨터가 대기 상태에 빠질 수 있기 때문에 가능하면 exit를 입력하여 종료하여 주시길 바랍니다)
<p align="center">
<image width="620" alt="실행 방법3" src="https://user-images.githubusercontent.com/77680436/127855033-b8c507ba-73c5-4493-8935-5c25d9407d75.png">
</p>

> 4. 드라이버 삭제

'sc delete MiniEDR'을 입력하여 드라이버를 삭제할 수 있습니다.
<p align="center">
<image width="620" alt="실행 방법4" src="https://user-images.githubusercontent.com/77680436/127855827-111a72e1-f8e8-4e75-a845-5bffa9c7c37e.png">
</p>

## MiniEDR_LogView
파이썬 기반의 간단한 로그 뷰어입니다. 프로그램을 로그 파일(MiniEDR_Log.txt)이 존재하는 폴더에서 실행시키면 프로세스 별로(ProcessGuid 기준) 행위 로그를 확인할 수 있습니다.

<p align="center">
<image width=500 alt="로그 뷰어 예시" src="https://user-images.githubusercontent.com/77680436/127852335-2a6535f2-da83-4316-9f21-3b1d7bcbeb82.png">
</p>

## 기타
### 코딩 스타일
* 드라이버의 경우 완전히 C로 제작되었으며 기존의 드라이버 샘플 소스들이 파스칼 표기법을 사용한 경우가 많았기 때문에 이를 따라 코딩하였습니다.
* 유저 모드 프로그램의 경우 가능하면 C++의 특성을 이용하고자 하였기 때문에 되도록 스네이크 표기법을 사용하여 코딩하였습니다.
  
### 개발환경
* Microsoft Windows 10 Education 버전 10.0.19042 빌드 19042
* Microsoft Visual Studio Community 2019 버전 16.10.3
* Windows Driver Kit 10.0.19030.1000

### 테스트 환경
* VMware Workstation 16 Player 16.1.2 build-17966106
* Microsoft Windows 10 Home 10.0.19043 빌드 19043

### 주의사항
1. 드라이버가 64비트 운영체제를 대상으로 개발되었기 때문에 32비트 운영체제에서는 돌아가지 않을 것입니다. (테스트 해보지 않음)
  
2. MiniEDR_LogView의 경우 로그 파일의 마지막에 개행이 두 번 이상 존재하지 않으면 무한 루프에 빠져버립니다. 

실제로도 유저 모드 프로그램이 다음 로그를 위해서 종료 전 개행을 한 번 더 출력하여 개행이 두 번 이상이 됩니다. 
따라서 만약 로그 뷰어 프로그램이 실행은 됐지만 반응이 없다면 이를 확인하여 주시길 바랍니다.

### 버그
1. RegistryValueSet 행위 정보가 가끔 프로세스 종료 알림 이후에 수신되어 어떠한 ProcessGuid에도 속하지 못 하는 버그가 존재합니다.

아직까지는 시스템이 `\REGISTRY\MACHINE\SYSTEM\ControlSet001\Services\bam\State\UserSettings\*`에 대한 부분을 설정하는 로직에서만 발생하는 것을 확인하였는데, 
이는 유저 모드 프로그램에서 수신을 잘못된 순서로 수신한 것이 아니라 실제로 드라이버의 디버그 출력을 남겨본 결과 드라이버의 레지스트리 콜백 루틴 자체가 프로세스 루틴보다 더 늦게 호출되어 발생하는 것을 확인하였습니다.  
추후 더 정확한 원인을 파악한다면 수정할 예정입니다.

또한 콜백 루틴 자체가 비동기적이기 때문에 이와 같은 일이 발생할 가능성을 고려하여 로그 뷰어 프로그램에는 이러한 로그를 별도로 '예외' 항목에 모아서 보여줍니다.
