#include"title.h"
#include"sql.h"
void server();//服务器端测试
void sendCtxt(const Ciphertext& Ctxt_x, SOCKET& s, int size);//密文发送
void recvCtxt(shared_ptr<seal::SEALContext> context, Ciphertext& Ctxt_x, SOCKET& s, int size);//密文接收
void mode_G(SOCKET& hsock, double x);//socket通信G模式（大小比较协议）
DWORD WINAPI threadpro(LPVOID pParam);

void recvsz(SOCKET s, double c[], int num) {
    stringstream stream;
    int len = 0;
    while (len != num) {
        memset(c, 0, 1024);
        len = recv(s, (char*)c, num, 0);
    }
}

//冒泡排序
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
        sendlen -= len;
        p += len;
        // cout << "总大小：" << str.length() << "  发送大小：" << len << "   " << "  剩余大小：" << sendlen << endl;
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
        }
        //cout << "接收大小:" << len << "累计接收:" << reclen << endl;
    }
    p = pt;
    for (int i = 0; i < size; i++) {
        str.append(1, p[i]);
    }
    stream.str(str);
    Ctxt_x.load(context, stream);
}


//socket通信G模式（大小比较协议）
void mode_G(SOCKET& hsock, double x) {
    //A端创建BFV加密环境
    EncryptionParameters parms(scheme_type::BFV);
    size_t poly_modulus_degree = 4096;
    parms.set_poly_modulus_degree(poly_modulus_degree);
    parms.set_coeff_modulus(CoeffModulus::BFVDefault(poly_modulus_degree));
    parms.set_plain_modulus(256);
    auto context = SEALContext::Create(parms);
    // key
    KeyGenerator keygen(context);
    PublicKey public_key = keygen.public_key();
    SecretKey secret_key = keygen.secret_key();
    // 用公钥加密
    Encryptor encryptorA(context, public_key);

    // 用私钥解密
    Decryptor decryptor(context, secret_key);
    // relinearization    
    auto relin_keys = keygen.relin_keys();
    // 在B端生成evaluator函数做计算
    Evaluator evaluator(context);
    cout << "进入密文大小比较协议" << endl;
    srand(time(0));
    default_random_engine e(time(0));
    uniform_real_distribution<double> u(0, 100);
    //生成随机下标
    int m = rand() % 20;
    double C1[20];
    double C3[40];
    for (int i = 0; i < 20; i++) {
        if (i == m)C1[i] = x;
        else C1[i] = rand() % 100;
    }
    sort(C1, 20);
    send(hsock, (char*)C1, sizeof(C1), 0);
    recvsz(hsock, C3, sizeof(C3));
    for (m = 0; C3[m] != x; m++);//重新记录下标
   //数组编码
    int C4[40];
    for (int i = 0; i < 40; i++) {
        if (i == m)C4[i] = 1;
        else C4[i] = 0;
    }

    //编码数组加密
    Plaintext plaintext[40];//明文多项式
    for (int i = 0; i < 40; i++) plaintext[i] = Plaintext(to_string(C4[i]));
    Ciphertext ciphertext[40], Ctxt_add;//密文空间
    for (int i = 0; i < 40; i++) encryptorA.encrypt(plaintext[i], ciphertext[i]);
    //将得到的密文空间传输到B端计算
    int mmmmm;
    for (int i = 0; i < 40; i++) {
        sendCtxt(ciphertext[i], hsock, BFV_n);
    }
    recvCtxt(context, Ctxt_add, hsock, BFV_n);
    Plaintext add_result;
    decryptor.decrypt(Ctxt_add, add_result);
    cout << add_result.to_string() << endl;
    send(hsock, (char*)(&add_result.to_string()), sizeof(add_result.to_string()), 0);
    std::cout << "比较大小协议结束" << endl;

}

DWORD WINAPI threadpro1(LPVOID pParam)
{

    //政府段创建加密环境，每启动一次线程创建一次密钥
    EncryptionParameters parms(scheme_type::CKKS);
    size_t poly_modulus_degree = 8192;
    parms.set_poly_modulus_degree(poly_modulus_degree);
    parms.set_coeff_modulus(CoeffModulus::Create(
        poly_modulus_degree, { 60, 40, 40, 60 }));
    double scale = pow(2.0, 40);
    auto context = SEALContext::Create(parms);
    auto context2 = SEALContext::Create(parms);
    KeyGenerator keygen(context);//基础配置
    auto public_key = keygen.public_key();//公钥
    auto secret_key = keygen.secret_key();//私钥
    auto relin_keys = keygen.relin_keys();//重线性化
    Encryptor encryptor(context, public_key);//加密器
    Evaluator evaluator(context);//计算器
    Decryptor decryptor(context, secret_key);//解密器
    CKKSEncoder encoder(context);//编码器
    
    SOCKET hsock = (SOCKET)pParam;
    char buffer[1024];
    char sendBuffer[1024];
    if (hsock != INVALID_SOCKET)
        cout << "Start Receive!" << endl;
    ConnectDatabase("people");
    string ID;
    stringstream stream;
    recv(hsock, buffer, 1024, 0);
    stream << buffer;
    stream >> ID;
    cout << "ID:" << ID << endl;
    while (1)
    {
        //循环接收发送的内容
        int num = recv(hsock, buffer, 1024, 0);//阻塞函数，等待接收内容
        if (num >= 0)
            cout << "Receive form clinet!" << buffer << endl;
       
        if (!strcmp(buffer, "G")) {
            Plaintext Ptxt_G;
            Ciphertext Ctxt_G;
            vector<double>result0;
            recvCtxt(context, Ctxt_G, hsock, CKKS_n);
            //A端创建BFV加密环境
            decryptor.decrypt(Ctxt_G, Ptxt_G);
            encoder.decode(Ptxt_G, result0);
            cout << result0[0] << endl;
            mode_G(hsock, result0[0]);
        }
        else if (!strcmp(buffer, "J")) {//通信模式J数据库查询加密
            cout << "数据库查询后加密计算" << endl;
            stream << ID;
            Plaintext Ptxt_J;
            Ciphertext Ctxt_J;
            char s;;
            //memset(buffer, 0, 1024);
            cout<<recv(hsock, buffer, 1024, 0)<<endl;
            int num = buffer[0] - 'A' + 2;

            string qy = "select * from people where ID='" + ID + "'";
            mysql_query(&mysql, "set names gbk");
            if (mysql_query(&mysql, qy.c_str())) {
                printf("Query failed (%s)\n", mysql_error(&mysql));
            }
            else {
                printf("query success\n");
            }
            if (!(res = mysql_store_result(&mysql)))
            {
                printf("Couldn't get result from %s\n", mysql_error(&mysql));

            }
            while (column = mysql_fetch_row(res))
            {
                stringstream stream1;
                double x;
                stream1 << column[num];
                stream1 >> x;

                x /= 1000;
                cout << x << endl;
                encoder.encode(x, scale, Ptxt_J);
                encryptor.encrypt(Ptxt_J, Ctxt_J);
            }
            sendCtxt(Ctxt_J, hsock, CKKS_n);
            cout << "测试结束" << endl;
        }
        //返回两个密文 解密比较大小返回结果
        else if (!strcmp(buffer, "X")) {
            vector<double> result0;
            Plaintext Ptxt;
            Ciphertext Ctxt_X, Ctxt_Y;
            double x, y;
            char a;
            recvCtxt(context, Ctxt_X, hsock, CKKS_n);
            recvCtxt(context, Ctxt_Y, hsock, CKKS_n);
            decryptor.decrypt(Ctxt_X, Ptxt);
            encoder.decode(Ptxt, result0);
            x = result0[0];
            decryptor.decrypt(Ctxt_Y, Ptxt);
            encoder.decode(Ptxt, result0);
            y = result0[0];
            a = x > y ? '0' : '1';
            cout << a << endl;
            send(hsock, &a, 2, 0);
        }
        //解密一个密文,返回明文结果
        else if (!strcmp(buffer, "N")) {
            vector<double> result0;
            Plaintext Ptxt_N;
            Ciphertext Ctxt_N;
            recvCtxt(context, Ctxt_N, hsock, CKKS_n);
            cout << "接收成功" << endl;
            decryptor.decrypt(Ctxt_N, Ptxt_N);
            encoder.decode(Ptxt_N, result0);
            cout << result0[0] << endl;
            cout << send(hsock, (char*)(&result0[0]), sizeof(result0[0]), 0) << endl;
        }
        //退出线程
        else if (!strcmp(buffer, "exit"))
        {

            FreeConnect();
            cout << "Client Close" << endl;
            cout << "Server Process Close" << endl;
            return 0;
        }
        else
        {
            memset(sendBuffer, 0, 1024);
            strcpy(sendBuffer, "ERR");
            int ires = send(hsock, sendBuffer, sizeof(sendBuffer), 0);
            cout << "Send to client" << sendBuffer << endl;
        }

    }
    return 0;
}

//服务端socket测试
void server() {
    WSADATA wsd;//定义WSADATA对象
    WSAStartup(MAKEWORD(2, 2), &wsd);
    SOCKET m_SockServer;
    sockaddr_in serveraddr;
    sockaddr_in serveraddrfrom;
    SOCKET m_Server[20];

    serveraddr.sin_family = AF_INET;//设置服务器地址
    serveraddr.sin_port = htons(4600);//设置端口号
    serveraddr.sin_addr.S_un.S_addr = INADDR_ANY;
    m_SockServer = socket(AF_INET, SOCK_STREAM, 0);
    int i = ::bind(m_SockServer, (sockaddr*)&serveraddr, sizeof(serveraddr));
    cout << "政府端启动..." << endl;
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
            int ires = send(m_Server[iConnect], buf, sizeof(buf), 0);//发送字符过去
            cout << "发送消息：" << buf << endl;
            cout << "accept: " << ires << endl;//显示已经建立连接次数
            iConnect++;
            if (iConnect > iMaxConnect)
            {
                int ires = send(m_Server[iConnect], WarnBuf, sizeof(WarnBuf), 0);

            }
            else
            {
                HANDLE m_Handel;//线程句柄
                DWORD nThreadId = 0;//线程ID
                m_Handel = (HANDLE)::CreateThread(NULL, 0, threadpro1, (LPVOID)m_Server[--iConnect], 0, &nThreadId);
                //启动线程
                cout << "启动线程!" << endl;
            }
        }
    }
    WSACleanup();
}