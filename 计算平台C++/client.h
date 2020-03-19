#pragma once
#include"seal/seal.h"
#include <stdio.h>
#include <iostream>
#include <cstring>
#include <fstream>
#include <winsock2.h>
#include<time.h>
#include<Windows.h>
#pragma comment(lib, "ws2_32.lib")
using namespace std;
using namespace seal;

SOCKET m_Client;
#define socket_v 4096//socket密文传输速率
#define Ctxt_num 50//多密文传输测试数目
#define CKKS_n 393321
#define BFV_n 131177
#define socket_n 1024
#define max(x,y) (x>=y? x:y)
#define min(x,y) (x<=y? x:y)

void recvsz(SOCKET s, double c[], int num) {
    stringstream stream;
    int len = 0;
    while (len != num) {
        memset(c, 0, 1024);
        len = recv(s, (char*)c, num, 0);
    }
}

void mysend(SOCKET& s, const char p[]) {
    char buffer[1024];
    memset(buffer, 0, 1024);
    strcpy(buffer, p);
    cout<<"发送字节："<<send(s, buffer, 1024, 0)<<endl;
}

typedef struct {
    int flag;//0代表double,1代表密文，乱码代表未赋值
    double x;//存储参数
    Ciphertext Ctxt;//存储密文
    void operator= (const double& n) {
        this->flag = 0;
        this->x = n;
    }
    void operator= (const Ciphertext& ctxt) {
        this->flag = 1;
        this->Ctxt = ctxt;
    }
    void operator+ (const double& n) {
        if (this->flag == 0)this->x += n;
    }
    void operator- (const double& n) {
        if (this->flag == 0)this->x -= n;
    }
    void operator* (const double& n) {
        if (this->flag == 0)this->x *= n;
    }
    void operator/ (const double& n) {
        if (this->flag == 0)this->x /= n;
    }
}Param;

void sort(double array[], int n) {
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n - 1 - i; j++)
            if (array[j + 1] < array[j]) {
                double temp = array[j + 1];
                array[j + 1] = array[j];
                array[j] = temp;
            }
}

void sendCtxt(const Ciphertext& Ctxt_x, SOCKET& s, int size) {
    stringstream stream;
    Ctxt_x.save(stream);
    string str = stream.str();
    char pt[400000];
    char* p = pt;
    str.copy(p, size);
    int sendlen = str.length();
    while (sendlen >= 0) {
        int len = send(s, p, socket_v, 0);
        if (len == -1) { cout << "ERROR" << endl; return; }
        if (len == socket_v) {
            sendlen -= len;
            p += len;
            // cout << "总大小：" << str.length() << "  发送大小：" << len << "   " << "  剩余大小：" << sendlen << endl;
        }
    }
}

void recvCtxt(shared_ptr<seal::SEALContext> context, Ciphertext& Ctxt_x, SOCKET& s, int size) {
    stringstream stream;
    string str;
    char pt[400000];
    char* p = pt;//pt永远指向首地址
    int reclen = 0;
    while (reclen <= size) {//经测试所有密文大小都是这么大
        int len = recv(s, p, socket_v, 0);
        if (len == -1) { cout << "ERROR" << endl; return; }
        if (len == socket_v) {
            p += len;
            reclen += len;
            //cout << "接收大小:" << len << "累计接收:" << reclen << endl;
        }
    }
    p = pt;
    for (int i = 0; i < size; i++) {
        str.append(1, p[i]);
    }
    stream.str(str);
    Ctxt_x.load(context, stream);
}
//密文的大小比较，如果Ctxt_x>Ctxt_y则返回0，反之返回1
int comCtxt(Ciphertext& Ctxt_x, Ciphertext& Ctxt_y, SOCKET& s) {
    stringstream stream;
    char a;
    int x;
    send(s, "X", 2, 0);
    sendCtxt(Ctxt_x, s, CKKS_n);
    sendCtxt(Ctxt_y, s, CKKS_n);
    recv(s, &a, 2, 0);
    stream << a;
    stream >> x;
    return x;
}
//socket中的G模式（大小比较协议）
int mode_G(SOCKET& s, Ciphertext& Ctxt, double& y) {
    sendCtxt(Ctxt, s, CKKS_n);
    EncryptionParameters parms(scheme_type::BFV);
    size_t poly_modulus_degree = 4096;
    parms.set_poly_modulus_degree(poly_modulus_degree);
    parms.set_coeff_modulus(CoeffModulus::BFVDefault(poly_modulus_degree));
    parms.set_plain_modulus(256);
    auto context = SEALContext::Create(parms);
    Evaluator evaluator(context);
    cout << "进入密文大小比较协议" << endl;
    srand(time(0));
    default_random_engine e(time(0));
    uniform_real_distribution<double> u(0, 100);
    //生成随机下标
    int n = rand() % 20;
    double C1[20];
    double C2[20];
    double C3[40];
    for (int i = 0; i < 20; i++) {
        if (i == n)C1[i] = y;
        else C1[i] = rand() % 100;
    }
    sort(C1, 20);
    recvsz(s, C2, sizeof(C2));
    int i_C1 = 0;
    int i_C2 = 0;
    int num = 0;
    while (i_C1 != 20 && i_C2 != 20) {
        if (C1[i_C1] <= C2[i_C2])C3[num++] = C1[i_C1++];
        else C3[num++] = C2[i_C2++];
    }
    while (i_C1 != 20) {
        C3[num++] = C1[i_C1++];
    }
    while (i_C2 != 20) {
        C3[num++] = C2[i_C2++];
    }
    for (n = 0; C3[n] != y; n++);
    send(s, (char*)C3, 1024, 0);
    Ciphertext Ctxt_p[40], Ctxt_add;
    for (int i = 0; i < 40; i++) {
        recvCtxt(context, Ctxt_p[i], s, BFV_n);
    }
    Ctxt_add = Ctxt_p[0];
    if (n > 0) {
        for (int i = 1; i < n; i++) {
            evaluator.add(Ctxt_add, Ctxt_p[i], Ctxt_add);
        }
    }
    sendCtxt(Ctxt_add, s, BFV_n);
    stringstream stream;
    int x;
    char i;
    recv(s, &i, sizeof(i), 0);
    cout << i << endl;
    
    stream << i;
    stream >> x;
    cout << "wait..." << endl;
    Sleep(500);
    cout << "比较大小协议结束" << endl;
    return x;
}

string client(const double *param,string tID) {
    //加密

    EncryptionParameters parms(scheme_type::CKKS);
    size_t poly_modulus_degree = 8192;
    parms.set_poly_modulus_degree(poly_modulus_degree);
    parms.set_coeff_modulus(CoeffModulus::Create(
        poly_modulus_degree, { 60, 40, 40, 60 }));
    double scale = pow(2.0, 40);
    auto context2 = SEALContext::Create(parms);
    KeyGenerator keygen(context2);//基础配置
    auto public_key = keygen.public_key();//公钥
    auto secret_key = keygen.secret_key();//私钥
    auto relin_keys = keygen.relin_keys();//重线性化
    Encryptor encryptor(context2, public_key);//加密器
    Evaluator evaluator(context2);//计算器
    CKKSEncoder encoder(context2);//编码器
    Decryptor decryptor(context2, secret_key);//解密器

    WSADATA wsd;//定义WSADATA对象
    WSAStartup(MAKEWORD(2, 2), &wsd);
    SOCKET m_SockClient;
    sockaddr_in clientaddr;
    clientaddr.sin_family = AF_INET;//设置服务器地址
    clientaddr.sin_port = htons(4600);//设置服务器端口号
    clientaddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
    m_SockClient = socket(AF_INET, SOCK_STREAM, 0);
    int i = connect(m_SockClient, (sockaddr*)&clientaddr, sizeof(clientaddr));//连接超时
    std::cout << "connect:" << i << endl;

    char* buffer;
    buffer = new char[1024];
    char inBuf[1024];
    int num;
    num = recv(m_SockClient, buffer, 1024, 0);//阻塞
    if (num > 0)
    {
        cout << "Receive from server:" << buffer << endl;//欢迎信息
        cout << "公式测试" << endl;
        
        char p;
        string ID = tID;
        double a = param[0], b = param[1], c = param[2];
        double q = param[3] / 1000, w = param[4] / 1000, e;
        vector<double> result;
        Param p_a, p_b;
        Plaintext PJ_a, PJ_b, PJ_c;
        Ciphertext CJ_A, CJ_B;
        mysend(m_SockClient, ID.c_str());
        mysend(m_SockClient, "J");
        mysend(m_SockClient, "A");
       /* send(m_SockClient, "J", 2, 0);
        send(m_SockClient, "A", 2, 0);*/
        recvCtxt(context2, CJ_A, m_SockClient, CKKS_n);
        /*send(m_SockClient, "J", 2, 0);
        send(m_SockClient, "B", 2, 0);*/
        mysend(m_SockClient, "J");
        mysend(m_SockClient, "B");
        recvCtxt(context2, CJ_B, m_SockClient, CKKS_n);
        encoder.encode(a, scale, PJ_a);
        encoder.encode(b, scale, PJ_b);
        encoder.encode(c, scale, PJ_c);
        p_a = CJ_A;
        p_b = CJ_B;
        evaluator.multiply_plain(p_a.Ctxt, PJ_a, p_a.Ctxt);
        evaluator.multiply_plain(p_b.Ctxt, PJ_b, p_b.Ctxt);
        if (comCtxt(p_a.Ctxt, p_b.Ctxt, m_SockClient) == 1)
            p_a = p_b;
        evaluator.multiply_plain(p_a.Ctxt, PJ_c, p_a.Ctxt);;
        mysend(m_SockClient, "G");
        if (mode_G(m_SockClient, p_a.Ctxt, q) == 0)p_a = q;
        if (p_a.flag == 0) p_a = p_a.x < w ? p_a.x : w;
        else {
            mysend(m_SockClient, "G");
            if (mode_G(m_SockClient, p_a.Ctxt, w) == 1)
                p_a = w;
        }
        if (p_a.flag == 1) {
            mysend(m_SockClient, "N");
            sendCtxt(p_a.Ctxt, m_SockClient, CKKS_n);
            int len = recv(m_SockClient, (char*)(&p_a.x), 1024, 0);
            cout <<len <<endl;
            if(len != sizeof(p_a.x) && len != 1024)
                recv(m_SockClient, (char*)(&p_a.x), sizeof(p_a.x), 0);
        }
        cout << p_a.x << endl;
        send(m_SockClient, "exit", sizeof("exit"), 0);
        stringstream stream;
        stream << p_a.x;
        string ppp = stream.str();
        return stream.str();
        closesocket(m_SockClient);
    }
}

