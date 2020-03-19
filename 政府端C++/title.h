#pragma once
#include"seal/seal.h"
#include<iostream>
//client
#include <iostream>
#include <stdio.h>
#include <iostream>
#include <cstring>
#include <fstream>
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
//#pragma comment(linker,"/subsystem:windows /entry:mainCRTStartup")
using namespace std;
using namespace seal;
int sock;
bool going = true;
SOCKET m_Client;

#define max(x,y) (x>=y? x:y)
#define min(x,y) (x<=y? x:y)
#define socket_v 4096//socket密文传输速率
#define Ctxt_num 50//多密文传输测试数目
#define CKKS_n 393321
#define BFV_n 131177


#include <mysql.h>    //引入mysql头文件(一种方式是在vc++目录里面设置，一种是文件夹拷到工程目录，然后这样包含)  
#include <Windows.h>  
#pragma comment(lib,"libmysql.lib")

