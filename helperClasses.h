#pragma once

#include "header.h"

class IpDomainNode
{
public:
    string ip;
    string domain;

    void set(string pip, string pdomain)
    {
        ip = pip;
        domain = pdomain;
    }
};

//IDת����
//�м�DNS���������ǰ�������Ϣ
class IDTransferNode
{
public:
    //��ID
    unsigned short oldID;

    //�������׽��ֵ�ַ
    SOCKADDR_IN clientAddr;

    //����Ƿ��Ѿ���ɽ���
    bool processed;
};

class MySystemTime
{
public:
    SYSTEMTIME t;
    void print()
    {
        GetLocalTime(&t);
        cout << endl
            << dec << t.wYear << "-" << t.wMonth << "-" << t.wDay
            << ' ' << t.wHour << ':' << t.wMinute << ':' << t.wSecond << ":" << t.wMilliseconds << endl;
    }
};

class Flags
{
public:
    //(2�ֽ�)ID��
    unsigned short id;

    //(1����)queryOrResponse����ѯ/��Ӧ�ı�־λ��1Ϊ��Ӧ��0Ϊ��ѯ
    int QR;

    //(4����)operationCode�������ѯ����Ӧ������(��Ϊ0���ʾ�Ǳ�׼�ģ���Ϊ1���Ƿ���ģ���Ϊ2���Ƿ�����״̬����)��
    unsigned short OPCODE;

    //(1����)authoritativeAnswer����Ȩ�ش�ı�־λ����λ����Ӧ��������Ч��1��ʾ���ַ�������Ȩ�޷�����
    int AA;

    //(1����)trunCated���ضϱ�־λ��1��ʾ��Ӧ�ѳ���512�ֽڲ��ѱ��ض�
    int TC;

    //(1����)recursionDesired����λΪ1��ʾ�ͻ���ϣ���õ��ݹ�ش�
    int RD;

    //(1����)recursionAvailable����λΪ1��ʾ�ͻ���ϣ���õ��ݹ�ش�
    int RA;

    //(4����)responseCode�������룬��ʾ��Ӧ�Ĳ��״̬��0�޲��3�в��
    unsigned short RCODE;

    //RR=Resource Record����Դ��¼
    //questionCount��������
    unsigned short QDCOUNT;

    //answerCount���ش���
    unsigned short ANCOUNT;

    //nameServerCount��Ȩ����������
    unsigned short NSCOUNT;

    //additionalRecordCount�����Ӽ�¼��
    unsigned short ARCOUNT;

    void set(char* buf)
    {
        //�ӱ���ͷ��ǰ16bit���ID
        unsigned short * us = (unsigned short *)malloc(sizeof(unsigned short));
        memcpy(us, buf, sizeof(unsigned short));
        id = ntohs(*us);

        //����ڶ��У���16bit��
        //�ڶ���ǰ8�����أ�ע�ⷴ�����
        bitset<8> bs8(buf[2]);
        QR = bs8[7];
        OPCODE = 0;
        OPCODE += bs8[6] * 8;
        OPCODE += bs8[5] * 4;
        OPCODE += bs8[4] * 2;
        OPCODE += bs8[3];
        AA = bs8[2];
        TC = bs8[1];
        RD = bs8[0];

        //�ڶ��к�8�����أ�ע�ⷴ�����
        bs8 = buf[3];
        RA = bs8[7];
        RCODE = 0;
        RCODE += bs8[3] * 8;
        RCODE += bs8[2] * 4;
        RCODE += bs8[1] * 2;
        RCODE += bs8[0];

        //������
        memcpy(us, &buf[4], 2);
        QDCOUNT = ntohs(*us);

        //������
        memcpy(us, &buf[6], 2);
        ANCOUNT = ntohs(*us);

        //������
        memcpy(us, &buf[8], 2);
        NSCOUNT = ntohs(*us);

        //������
        memcpy(us, &buf[10], 2);
        ARCOUNT = ntohs(*us);

        free(us);
    }

    void print()
    {
        cout << "ID = " << id << endl
            << "QR = " << QR << endl
            << "OPCODE = " << OPCODE << endl
            << "AA = " << AA << endl
            << "TC = " << TC << endl
            << "RD = " << RD << endl
            << "RA = " << RA << endl
            << "RCODE = " << RCODE << endl
            << "QDCOUNT = " << QDCOUNT << endl
            << "ANCOUNT = " << ANCOUNT << endl
            << "NSCOUNT = " << NSCOUNT << endl
            << "ARCOUNT = " << ARCOUNT << endl;
    }
};
