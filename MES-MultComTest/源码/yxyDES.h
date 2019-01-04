
//----------------------------------------------------------------*/
//---------------------------------------------------------------------------

#ifndef yxyDESH
#define yxyDESH

#include <string>
#include <cmath>
#include <fstream>
#include <stdio.h>
#include <windows.h>
using namespace std;
class yxyDES
{
public:
         yxyDES(); //类构造函数
        ~yxyDES(); //类析构函数
        //--------------------------------------------------------------
        void InitializeKey(string,bool);
        //功能:产生16个28位的key
        //参数:源8位的字符串(key)
        //结果:函数将调用private CreateSubKey将结果存于char SubKeys[16][48]
        //--------------------------------------------------------------

        //--------------------------------------------------------------
        void EncryptData(string,bool,bool);
        //功能:加密8位字符串
        //参数:8位字符串
        //结果:函数将加密后结果存放于private szCiphertext[16]
        //      用户通过属性Ciphertext得到
        //--------------------------------------------------------------

        //--------------------------------------------------------------
        void DecryptData(string,bool,bool);
        //功能:解密16位十六进制字符串
        //参数:16位十六进制字符串
        //结果:函数将解密候结果存放于private szPlaintext[8]
        //      用户通过属性Plaintext得到
        //--------------------------------------------------------------

        //--------------------------------------------------------------
        void EncryptAnyLength(string,bool);
        //功能:加密任意长度字符串
        //参数:任意长度字符串
        //结果:函数将加密后结果存放于private szFCiphertextAnyLength[8192]
        //      用户通过属性CiphertextAnyLength得到
        //--------------------------------------------------------------

        //--------------------------------------------------------------
        void DecryptAnyLength(string,bool);
        //功能:解密任意长度十六进制字符串
        //参数:任意长度字符串
        //结果:函数将加密后结果存放于private szFPlaintextAnyLength[4096]
        //      用户通过属性PlaintextAnyLength得到
        //--------------------------------------------------------------

        //--------------------------------------------------------------
        void TripleEncryptAnyLength(string);
        //功能:使用3DES加密任意长度字符串
        //参数:任意长度字符串
        //结果:函数将加密后结果存放于private szFCiphertextAnyLength[8192]
        //      用户通过属性CiphertextAnyLength得到
        //--------------------------------------------------------------

        //--------------------------------------------------------------
        void TripleDecryptAnyLength(string);
        //功能:使用3DES解密任意长度十六进制字符串
        //参数:任意长度字符串
        //结果:函数将加密后结果存放于private szFPlaintextAnyLength[4096]
        //      用户通过属性PlaintextAnyLength得到
        //--------------------------------------------------------------

        //--------------------------------------------------------------
        void TripleEncrypt(string,bool);
        //功能:使用3DES加密8位字符串
        //参数:8位字符串
        //结果:函数将加密后结果存放于private szCiphertext[16]
        //      用户通过属性Ciphertext得到
        //--------------------------------------------------------------

        //--------------------------------------------------------------
        void TripleDecrypt(string,bool);
        //功能:使用3DES解密16位十六进制字符串
        //参数:16位十六进制字符串
        //结果:函数将解密候结果存放于private szPlaintext[8]
        //      用户通过属性Plaintext得到
        //--------------------------------------------------------------

        //--------------------------------------------------------------
        bool FileEncrypt(string,bool);
        //功能:文件加密,尚有已知BUG请慎重使用
        //参数:文件path,是否使用3DES
        //结果:源文件名.yxyDES
        //--------------------------------------------------------------

        //--------------------------------------------------------------
        bool FileDecrypt(string,bool);
        //功能:文件解密,尚有已知BUG请慎重使用
        //参数:文件path(文件名.yxyDES),是否使用3DES
        //结果:源文件名
        //--------------------------------------------------------------

		//--------------------------------------------------------------
        void SetCiphertext(char* value);
        //属性Ciphertext的set函数
        //--------------------------------------------------------------

        //--------------------------------------------------------------
        char*  GetCiphertext();
        //属性Ciphertext的get函数
        //--------------------------------------------------------------

        //--------------------------------------------------------------
        void  SetPlaintext(char* value);
        //属性Plaintext的set函数
        //--------------------------------------------------------------

        //--------------------------------------------------------------
        char*  GetPlaintext();
        //属性Plaintext的get函数
        //--------------------------------------------------------------

        //--------------------------------------------------------------
        char*  GetCiphertextAnyLength();
        //属性CiphertextAnyLength的get函数
        //--------------------------------------------------------------

        //--------------------------------------------------------------
        char*  GetPlaintextAnyLength();
        //属性PlaintextAnyLength的get函数
        //--------------------------------------------------------------

private:
        //--------------------------------------------------------------
        char SubKeys[16][48];//储存16组48位密钥
        char SubKeys2[16][48];//储存16组48位密钥
        char szCiphertext[16];//储存16位密文(十六进制字符串)
        char szPlaintext[8];//储存8位明文字符串
        char szFCiphertextAnyLength[8192];//任意长度密文(十六进制字符串)
        char szFPlaintextAnyLength[4096];//任意长度明文字符串
        //--------------------------------------------------------------

        //--------------------------------------------------------------
        void CreateSubKey(char*,char[16][48]);
        //功能:生成子密钥
        //参数:经过PC1变换的56位二进制字符串
        //结果:将保存于char SubKeys[16][48]
        //--------------------------------------------------------------

        //--------------------------------------------------------------
        void FunctionF(char*,char*,bool,int);
        //功能:DES中的F函数,
        //参数:左32位,右32位,key序号(0-15)
        //结果:均在变换左右32位
        //--------------------------------------------------------------

        //--------------------------------------------------------------
        void InitialPermuteData(string,char*,bool);
        //功能:IP变换
        //参数:待处理字符串,处理后结果存放指针,加密/解密(true加密,false解密)
        //结果:函数改变第二个参数的内容
        //--------------------------------------------------------------

        //--------------------------------------------------------------
        void ExpansionR(char* ,char*);
        //功能:将右32位进行扩展位48位,
        //参数:原32位字符串,扩展后结果存放指针
        //结果:函数改变第二个参数的内容
        //--------------------------------------------------------------

        //--------------------------------------------------------------
        void XOR(char* ,char* ,int ,char*);
        //功能:异或函数,
        //参数:待异或的操作字符串1,字符串2,操作数长度,处理后结果存放指针
        //结果: 函数改变第四个参数的内容
        //--------------------------------------------------------------

        //--------------------------------------------------------------
        string CompressFuncS(char* );
        //功能:S-BOX , 数据压缩,
        //参数:48位二进制字符串,
        //结果:返回结果:32位字符串
        //--------------------------------------------------------------

        //--------------------------------------------------------------
        void PermutationP(string ,char*);
        //功能:IP逆变换,
        //参数:待变换字符串,处理后结果存放指针
        //结果:函数改变第二个参数的内容
        //--------------------------------------------------------------

        //--------------------------------------------------------------
        string FillToEightBits(string);
        //功能:当明文不足8位,使用'$'进行填充,
        //参数:原始字符串,
        //结果:返回8位字符串
        //--------------------------------------------------------------

        //--------------------------------------------------------------
        void CleanPlaintextMark(int);
        //将不足8位而补齐的明文处理还原
        //函数将处理szFPlaintextAnyLength
        //结果: 例如123$$$$$ 处理后将变为 123'\0'
        //--------------------------------------------------------------

        //--------------------------------------------------------------
        string HexCharToBinary(char);
        //功能:16进制字符('0'-'F')到2进制字符串的转换
        //参数:十六进制字符('0'-'F')
        //结果:返回二进制字符串("0000"-"1111")
        //--------------------------------------------------------------

        //--------------------------------------------------------------
        string HexIntToBinary(int );
        //功能:16进制整数(0-15)到2进制字符串的转换
        //参数:十六进制整数(0-15)
        //结果:返回二进制字符串("0000"-"1111")
        //--------------------------------------------------------------

        //--------------------------------------------------------------
        string BinaryToString(char*,int,bool);
        //功能:二进制串到字符串的转换,
        //参数:源二进制字符串,二进制字符串长度,类型(true为二进制到hex,false为二进制到ANSCII char),
        //结果:返回处理后结果
        //--------------------------------------------------------------

        //--------------------------------------------------------------
        int SingleCharToBinary(char);
        //功能:单个char '0'或'1' 到int 0或1的变换
        //参数: '0'或'1'
        //结果:0或1
        //--------------------------------------------------------------

        //--------------------------------------------------------------
        char SingleBinaryToChar(int);
        //功能:将int类型的0或1转换为char类型的0或1
        //参数:0或1
        //返回:'0'或'1'
        //--------------------------------------------------------------
        void TranslateBytes2Bits(string, char*);
        void TranslateBits2Bytes(char*, char*);
};

//---------------------------------------------------------------------------
#endif
 