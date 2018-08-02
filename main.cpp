#pragma comment(lib,"Ws2_32.lib")

#include "helperClasses.h"
#include "helperFuncs.h"

int main(int argc, char** argv)
{
    cout << "DNS-Relay-Server v1.0. Build time: " << __DATE__ << " " << __TIME__ << endl
        << "Usage: DNS-Relay-Server.exe [-d | -dd] [<dns-server>] [<local-list-path>]" << endl
        << endl;

    //�����б��ַ
    string localPath;
    //�ⲿDNS��ַ
    string dnsServer;
    //�����������ȡֵΪ0��1��2
    //=0��ʲô�������
    //=1�����ʱ����Ϣ��DNS������е�������QCLASS��QTYPE
    //=2���ڡ�����1���Ļ����ϸ���FLAG
    int outputLevel;
    //�����в�������
    parseParam(argc, argv, localPath, dnsServer, outputLevel); //lxj

    //�洢
    deque<IpDomainNode> ipList;
    //��ʼ��IPת����
    loadLocalList(localPath, ipList, outputLevel);

    //��ʼ����̬���ӿ��õģ�֮��socket������
    WSADATA wsaData;
    //��ʼ��ws2_32.dll��̬���ӿ⡣��һ��2��ʾ���汾�ţ��ڶ����ű�ʾ���汾�ţ��ڶ���������������socket�İ汾��Ϣ
    //��һ��Ӧ�ó������WSAStartup����ʱ��
    //����ϵͳ���������Socket�汾��������Ӧ��Socket�⣬
    //Ȼ����ҵ���Socket�⵽��Ӧ�ó����С�
    //�Ժ�Ӧ�ó���Ϳ��Ե����������Socket���е�����Socket�����ˡ��ú���ִ�гɹ��󷵻�0��
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    //����DNS�׽���
    SOCKET localSocket;
    //�����׽��֣�
    //UDP����TCP/IP��,���ݱ�����������ͨ��Э��
    //����socket��ע�������SOCK_DGRAM
    //����������Ϊ0���Զ�ѡ��ڶ�������Ӧ��Э������
    if ((localSocket = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        LOGE("Failed to create socket! Error code = " << WSAGetLastError());
        getchar();
        exit(1);
    }

    //����DNS�׽��ֵ�ַ
    SOCKADDR_IN localAddr;
    //�����׽��֣�
    memset(&localAddr, 0, sizeof(localAddr));
    //ָ����ַ�壬��ʾ��UDPЭ������׽��֣�
    localAddr.sin_family = AF_INET;
    //ָ���˿ں�
    localAddr.sin_port = htons(DEFAULT_PORT);
    //��������IP����������
    //sin_addr����inet_addr()���ַ�����ʽ��IP��ַת����unsigned long�͵�����ֵ�����ø�s_addr��
    localAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    //�󶨱���DNS��������ַ
    if (bind(localSocket, (SOCKADDR*)&localAddr, sizeof(localAddr)))
    {
        LOGE("Failed to bind localSocket! Error code = " << WSAGetLastError());
        exit(-2);
    }
    else
    {
        LOGI("Successfully binded localSocket!");
    }

    //�ⲿDNS�׽��ֵ�ַ
    SOCKADDR_IN  dnsServerAddr;
    dnsServerAddr.sin_family = AF_INET;
    dnsServerAddr.sin_port = htons(DEFAULT_PORT);
    dnsServerAddr.sin_addr.s_addr = ::inet_addr(dnsServer.c_str());

    //������׽��ֵ�ַ
    SOCKADDR_IN clientAddr;
    int clientLen = sizeof(clientAddr);

    //�ɼ������ϵͳʱ���һ���ӿ�
    MySystemTime systime;

    //deque��˫�˶���
    //�洢�м�DNS����oldID����ԭ�����׽�����Ϣ
    deque<IDTransferNode> idtList;
    //��¼һ������Ľڵ�
    IDTransferNode idtNode;
    //��ʼʱ���л�׼Ϊ0��Ȼ�󳬹�1000ʱ��Ϊ500
    int idtListBase = 0;

    //����DNS������QCLASS, QTYPE����Ϣ
    unsigned short qclass, qtype;

    //���͡����յĻ�����
    char sendBuffer[512];
    char recvBuffer[512];
    int recvBufLen = sizeof(recvBuffer);
    //��ʼ�����ջ�������ȫ������
    memset(recvBuffer, 0, recvBufLen);

    //���յ���DNS���ĵ�ͷ��
    Flags rflags;

    LOGI("DNS Relay Server is now listening...");
    while (1)
    {
        //����DNS����
        //������
        //localSocket:��ʶһ���������׽ӿڵ������֣�
        //recvBuffer:�������ݻ�������
        //recvBufLen:����������
        //0:���ò�����ʽ
        //(SOCKADDR*)&clientAddr��ָ�룬ָ��װ��Դ��ַ�Ļ�����
        //&clientLen��ָ�룬ָ��clientAddr�ĳ���ֵ
        //
        //��;��
        //���ڴӣ������ӣ��׽ӿ��Ͻ������ݣ����������ݷ���Դ�ĵ�ַ
        //
        //����ֵ��
        //recvFlag=���յ����ַ���
        //������ֽ������޴�
        //0�������ж�
        //SOCKET_ERROR��û�ɹ����ա�
        //����UDP��Ϣ��sockaddr����clientAddr��
        //
        //������ʽ�ģ�һֱ�ȴ��ͻ�����������
        //
        int recvFlag = recvfrom(localSocket,
                                recvBuffer, recvBufLen,
                                0,
                                (SOCKADDR*)&clientAddr, &clientLen);

        //���յ���Ϣ����Ϊ0
        if (recvFlag == 0)
        {
            LOGE("recvfrom: Disconnected!");
            break;
        }
        //Socket����
        else if (recvFlag == SOCKET_ERROR)
        {
            if (outputLevel > OUTPUT_QUIET)
            {
                systime.print();
                LOGE("recvfrom: Failed!");
            }
            continue;
        }
        //����UDP��Ϣ�ɹ�
        else
        {
            //��ȡ�������õ�QTYPE��QCLASS
            string domain = getDomain(recvBuffer, recvFlag, qclass, qtype);

            //QCLASS=1ΪIPv4
            //������IPV4
            string queryResult;
            if (qclass != 1)
            {
                if (outputLevel > OUTPUT_QUIET)
                {
                    systime.print();
                    LOGW("getDomain: Not IPV4");
                }
                queryResult = LOCAL_NOT_FOUND;
                continue;
            }

            //recvBuffer[0]+recvBuffer[1]=ID
            //recvBuffer[2]=QR+OPCODE+AA+TC+RD
            //recvBuffer[3]=RA+Z+RCODE
            //recvBuffer[4]+recvBuffer[5]=QDCOUNT
            //recvBuffer[6]+recvBuffer[7]=ANCOUNT
            //recvBuffer[8]+recvBuffer[9]=NSCOUNT
            //recvBuffer[10]+recvBuffer[11]=ARCOUNT
            rflags.set(recvBuffer);

            if (outputLevel > OUTPUT_QUIET)
            {
                systime.print();
                LOGI("domain = " << domain);

                if (outputLevel > OUTPUT_NORMAL)
                {
                    LOGD("idtList.size() = " << idtList.size());
                    LOGD("QTYPE = " << qtype);
                    LOGD("QCLASS = " << qclass);
                    LOGD("Recv FLAGS = ");
                    rflags.print();
                    LOGD("recvBuffer HEX = ");
                    printBuf(recvBuffer, recvFlag);
                }
            }

            //QR=0�������ѯ
            if (rflags.QR == QR_QUERY)
            {
                queryResult = localQuery(domain, ipList);
                //�ڱ����ҵ������ӷ���������
                if (queryResult != LOCAL_NOT_FOUND)
                {
                    //������Ӧ���ķ���
                    //��ʼ��������һ�ݺ�����һ���ı���
                    for (int i = 0; i < recvFlag; ++i)
                    {
                        sendBuffer[i] = recvBuffer[i];
                    }

                    //char: 8bit
                    //short: 16bit
                    //int: 32bit
                    //long: 64bit
                    unsigned short us16bit;

                    //����DNS��ͷ��ʼ
                    //sendBuffer[0]��sendBuffer[1]ΪID����������һ��

                    //sendBuffer[2]��sendBuffer[3]ΪQR��RCODE�ı�ͷ
                    //���ñ�־��1000 0001 1000 00��00/11��
                    //QR = QueryResponse = 1,��ʾ��Ӧ
                    //OPCODE = 0,��ʾ��׼��ѯ
                    //AA = AuthoritativeAnswer = 0,��ʾ���ַ�������Ȩ�޷�����
                    //TC = TrunCated = 0���ǽض�
                    //RD = RecursionDesired = 1����ʾ�ͻ���ϣ���õ��ݹ�ش�
                    //RA = RecursionAvailable = 1����ʾ���Եõ��ݹ���Ӧ
                    //Z = Zero = 0�������ֶ�
                    //RCODE = 0���޲�������б����
                    //RCODE = 3���в�������б����Σ�
                    if (queryResult == "0.0.0.0")
                    {
                        us16bit = htons(0x8183);
                        if (outputLevel > OUTPUT_QUIET)
                        {
                            LOGI("Domain found in local list.");
                            LOGW("Domain forbidden!");
                        }
                    }
                    else
                    {
                        us16bit = htons(0x8180);
                        if (outputLevel > OUTPUT_QUIET)
                        {
                            LOGI("Domain found in local list.");
                            LOGI("Local returns " << queryResult);
                        }
                    }
                    memcpy(&sendBuffer[2], &us16bit, 2);

                    //sendBuffer[4]��sendBuffer[5]ΪQDCOUNT����ԭ������ͬ������

                    //sendBuffer[6]��sendBuffer[7]ΪANCOUNT���ش���
                    if (queryResult == "0.0.0.0")
                    {
                        //�������ι��ܣ������ֽ�ת�����ֽڣ�RR=0
                        us16bit = htons(0x0000);
                    }
                    else
                    {
                        //���ӷ��������� ,�����ֽ�ת�����ֽڡ�RR=1
                        us16bit = htons(0x0001);
                    }
                    memcpy(&sendBuffer[6], &us16bit, 2);

                    //sendBuffer[8]��sendBuffer[9]ΪNSCOUNT��������������

                    //sendBuffer[10]��sendBuffer[11]ΪARCOUNT�����Ӽ�¼��
                    //����DNS��ͷ����

                    //����DNS��ӦRR��ʼ
                    int sendLen = recvFlag;//���յ��ֽڳ���

                    //NAME�������������
                    us16bit = htons(0xc00c);
                    memcpy(&sendBuffer[sendLen], &us16bit, 2);
                    sendLen += 2;

                    //TYPE=1��ΪIPV4
                    us16bit = htons(0x0001);
                    memcpy(&sendBuffer[sendLen], &us16bit, 2);
                    sendLen += 2;

                    //CLASS=1��ΪIN����
                    us16bit = htons(0x0001);
                    memcpy(&sendBuffer[sendLen], &us16bit, 2);
                    sendLen += 2;

                    //TTL��ȷ��������ȡΪ273
                    unsigned long ul32bit;
                    ul32bit = htonl(0x00000111);
                    memcpy(&sendBuffer[sendLen], &ul32bit, 4);
                    sendLen += 4;

                    //RDLENGTH������������Ҫ4���ֽ�(IPv4��ַ�ĳ���)
                    us16bit = htons(0x0004);
                    memcpy(&sendBuffer[sendLen], &us16bit, 2);
                    sendLen += 2;

                    //RDATA��inet_addr()���ַ�����ʽ��IP��ַת����unsigned long�͵�����ֵ
                    ul32bit = (unsigned long)inet_addr(queryResult.c_str());
                    memcpy(&sendBuffer[sendLen], &ul32bit, 4);
                    sendLen += 4;

                    //����DNS��ӦRR����

                    //size_t x = queryResult.size();
                    //usTemp = htons(x);
                    //memcpy(&sendBuffer[sendLen - x - 2], &usTemp, 2);

                    //��ӡ�ոչ������Ӧ����
                    if (outputLevel > OUTPUT_NORMAL)
                    {
                        Flags sflags;

                        sflags.set(sendBuffer);

                        LOGD("Sent FLAGS = ");
                        sflags.print();
                        LOGD("sendBuffer HEX = ");
                        printBuf(sendBuffer, sendLen);
                    }

                    //������Ӧ����
                    int sendFlag = sendto(localSocket,
                                          sendBuffer, sendLen,
                                          0,
                                          (SOCKADDR*)&clientAddr, clientLen);
                    //���޴��������������������ݵ�����������Ļ�������SOCKET_ERROR����
                    if (sendFlag == SOCKET_ERROR && outputLevel > OUTPUT_QUIET)
                    {
                        LOGE("sendto: Failed to send to clients! Error code = " << WSAGetLastError());
                    }
                    else
                    {
                        LOGI("Successfully sent response to the client.");
                    }
                }
                //û���ڱ����ҵ��������м̹���
                else
                {
                    //��ȡ��ID
                    unsigned short *oldID = (unsigned short*)malloc(sizeof(unsigned short));
                    memcpy(oldID, recvBuffer, 2);

                    //��֤���в�Ϊ��
                    handleIDTList(idtList, idtListBase);

                    //Ϊȷ���м�DNS��id����Ψһ��
                    int newIDint = pos2id(idtList, idtListBase);
                    unsigned short newID = htons((unsigned short)(newIDint));

                    //����м�DNS����id��ʹ֮newIDΨһ,����¼oldID,cname
                    idtNode.oldID = ntohs(*oldID);
                    idtNode.clientAddr = clientAddr;
                    idtNode.processed = false;
                    idtList.push_back(idtNode);

                    if (outputLevel > OUTPUT_QUIET)
                    {
                        LOGI("Domain NOT found in local list. Relaying...");
                    }
                    //����ID�����µ������ĵ�ͷ��
                    memcpy(recvBuffer, &newID, 2);

                    //��recvBufferת�����ⲿDNS������
                    int sendFlag = sendto(localSocket, recvBuffer, recvFlag, 0,
                        (SOCKADDR*)&dnsServerAddr, sizeof(dnsServerAddr));

                    if (sendFlag == SOCKET_ERROR)
                    {
                        if (outputLevel > OUTPUT_QUIET)
                        {
                            LOGE("sendto: Failed to reach DNS server! Error code = " << WSAGetLastError());
                        }
                        continue;
                    }
                    else if (sendFlag == 0)
                    {
                        if (outputLevel > OUTPUT_QUIET)
                        {
                            LOGE("sendto: Disconected!");
                        }
                        break;
                    }

                    free(oldID);

                    if (outputLevel > OUTPUT_QUIET)
                    {
                        LOGI("Successfully relayed request to external DNS server.");
                    }
                }
            }

            //QR=1��������Ӧ
            else if (rflags.QR == QR_RESPONSE)
            {
                //��newIDתΪoldID�����ҵ���Ӧ��cname
                unsigned short *newID = (unsigned short*)malloc(sizeof(unsigned short));

                //��buffer��ȡ��ID
                memcpy(newID, recvBuffer, 2);

                //����IDת��int�ͣ�ntohs = net to host short������˳��ת������˳��
                int a = (int)ntohs(*newID);
                free(newID);

                //��ȡ��ID��Ӧ�ľ�ID���±�
                int pos = id2pos(a, idtListBase);

                //������query�Ѵ�����ֱ������
                if (idtList[pos].processed) continue;

                //������queryδ������ȡ��ID��htons = host to net short������˳��ת������˳��
                unsigned short oldID = htons(idtList[pos].oldID);

                //������Ӧ����ͷID��׼�����ͻؿͻ���
                memcpy(recvBuffer, &oldID, 2);

                //���Ϊ�Ѵ���
                idtList[pos].processed = true;

                //�����ͻ���
                int sendFlag = sendto(localSocket,
                                      recvBuffer, recvFlag,
                                      0,
                                      (SOCKADDR*)&idtList[pos].clientAddr, sizeof(idtList[pos].clientAddr));
                //�жϷ��ͽ��
                if (sendFlag == SOCKET_ERROR)
                {
                    if (outputLevel > OUTPUT_QUIET)
                    {
                        LOGE("sendto: Failed to send to the client! Error code = " << WSAGetLastError());
                    }
                    continue;
                }
                else if (sendFlag == 0)
                {
                    if (outputLevel > OUTPUT_QUIET)
                    {
                        LOGE("sendto: Disconnected!");
                    }
                    break;
                }

                if (outputLevel > OUTPUT_QUIET)
                {
                    LOGI("Successfully sent response to the client.");
                }
            }

            //�����QR�ֶ�
            else if (outputLevel > OUTPUT_QUIET)
            {
                LOGE("QR field incorrect!");
            }
        }
    }

    //�ر��׽���
    closesocket(localSocket);
    WSACleanup();

    return 0;
}