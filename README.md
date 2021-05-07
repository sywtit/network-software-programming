# network-software-programming
네트워크 소프트웨어 설계 수업 실습


### 설계 요약
-----

**사용기술** : c++, AWS-EC2, Winsock, wpcap


#### 개인프로젝트
1. CL based programming (ARQ)
  * UDP client - CL programming 에서 client 가 udp 방법으로 server와 통신하면서 stop-and-wait ARQ protocol 를 이용한 프로그램
  * UDP server - CL programming 에서 server가 client 와 통신하면서 client 의 packet loss를 이론적으로 가정하여 discard 하는 과정을 담은 프로그램
  
   
2. CO programming
  * CO echo Client - CO socket programming 과정에서 client가 여러 형식의 데이터를 보내고 echo message 를 받는 프로그램
  * CO echo Server - CO socket programming 과정에서 server가 여러 형식의 데이터을 받고 echo message 를 보내는 프로그램
  
   
3. Hybrid P2P desgin programming
  * client - TCPserver와 통신을 해서 원하는 정보를 등록하거나 얻어오고 다른 client끼리 UDP 통신을 하는 프로그램
  * server - client의 아이디와 비밀번호 정보, client의 IP, port number를 저장해서 이를 client과 TCP 통신을 이용해서 데이터를 교환하는 프로그램

   
#### 팀프로젝트
1. Packet capture programming (SNMP)
   매니저 - 관리 장치(agent) - 공격 장치(attacker)
   1. 매니저가 매 1초마다 관리 장치에 정보를 request하면 관리장치에서 매니저에게 패킷 정보 MIB 형식으로 response
   2. 공격 장치가 SYN Flooding 패킷을 생성하면 관리 장치가 공격을 확인 후 서버에게 Trap 메시지 전송
   </br>
   
  * 맡은 부분 - 관리 장치 코드 작성, MIB 설계
  * MIB 구조를 이용한 패킷 캡쳐 응용 설계
   <center><img src = "https://user-images.githubusercontent.com/52434154/117517489-234cd780-afd7-11eb-8611-1619fce9c9e1.PNG" width = "400" height = "200"></center>
  
   
2. Platform bench marking project (HUNGRY-MAN) 
   높은 접근성과 매칭 서비스를 기반으로 한 플랫폼 서비스를 벤치마킹하여 가게와 고객의 연결하는 플랫폼 서버 구조를 구현
   </br>
   
  * 맡은 부분 - server - client 연결 구조 디자인, store server & client 코드 작성
  * 시스템 구조도
   <center><img src = "https://user-images.githubusercontent.com/52434154/117518024-31035c80-afd9-11eb-951f-18539ac72032.png" width = "400" height = "200"></center>
   
