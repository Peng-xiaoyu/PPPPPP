#include"client.h"
//int main() {
//	client();
//	return 0;
//}

// C++socket.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//


//#include "stdafx.h"
#include<thread>
#include <sstream>
#include <stdio.h>
#include <iostream>
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
using namespace std;

//通信线程
DWORD WINAPI threadpro(LPVOID pParam) {
    SOCKET hsock = (SOCKET)pParam;
    char buffer[1024];
    char sendBuffer[1024];
    if (hsock != INVALID_SOCKET)
        cout << "Start Receive!" << endl;
    int num = recv(hsock, buffer, 1024, 0);//阻塞函数，等待接收内容
    if (num >= 0)
        cout << "Receive form clinet!" << buffer << endl;
    double *p = new double[10];
    string ID;
    int i = 0; int j = 0;
    while (buffer[i] != '*') {
        stringstream stream;
        while (buffer[i] != '#') {
            stream << buffer[i++];
        }
        i++;
        stream >> p[j++];
    }
    i++;
    stringstream stream;
    while (buffer[i] != '*') {
        stream << buffer[i++];
    }
    ID = stream.str();

    
    //计算以及和政府端通信
    string result = client(p, ID);
    //随便返回了一个字符，实际将计算的结果返回
    //char result = '1';
    cout <<"发送字节：" <<send(hsock, (char*)(&result), result.length(), 0) << endl;
    closesocket(hsock);
    cout << "通信结束" << endl;
    return 0;
}

void test() {
    WSADATA wsd;//定义WSADATA对象
    WSAStartup(MAKEWORD(2, 2), &wsd);
    SOCKET m_SockServer;
    sockaddr_in serveraddr;
    sockaddr_in serveraddrfrom;
    SOCKET m_Server[20];

    serveraddr.sin_family = AF_INET;//设置服务器地址
    serveraddr.sin_port = htons(8866);//设置端口号
    serveraddr.sin_addr.S_un.S_addr = INADDR_ANY;
    m_SockServer = socket(AF_INET, SOCK_STREAM, 0);
    int i = ::bind(m_SockServer, (sockaddr*)&serveraddr, sizeof(serveraddr));
    cout << "计算平台启动..." << endl;
    cout << "bind:" << i << endl;

    int iMaxConnect = 20;//最大连接数
    int iConnect = 0;
    int iLisRet;
    char buf[] = "This is Server\0";//向客户端发送的内容
    char WarnBuf[] = "It is over Max connect\0";
    int len = sizeof(sockaddr);
    while (1)
    {
        iLisRet = listen(m_SockServer, 0);//进行监听
        m_Server[iConnect] = accept(m_SockServer, (sockaddr*)&serveraddrfrom, &len);
        //同意连接
        if (m_Server[iConnect] != INVALID_SOCKET)
        {
            //int ires = send(m_Server[iConnect], buf, sizeof(buf), 0);//发送字符过去
            cout << "发送消息：" << buf << endl;
            //cout << "accept: " << ires << endl;//显示已经建立连接次数
            iConnect++;
            if (iConnect > iMaxConnect)
            {
                //int ires = send(m_Server[iConnect], WarnBuf, sizeof(WarnBuf), 0);

            }
            else
            {
                HANDLE m_Handel;//线程句柄
                DWORD nThreadId = 0;//线程ID
                m_Handel = (HANDLE)::CreateThread(NULL, 0, threadpro, (LPVOID)m_Server[--iConnect], 0, &nThreadId);
                //启动线程
                cout << "启动线程!" << endl;
            }
        }
    }
    WSACleanup();
}

//主函数启动计算端服务器，监听端口
int main(int argc, char* argv[]) {
    test();
    //clienttest();
    return   0;
}
