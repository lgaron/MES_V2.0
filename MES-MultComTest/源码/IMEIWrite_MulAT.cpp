// IMEIWrite_MulAT.cpp : 实现文件
//
/*
void LogShow_exchange(CEdit* m_Result,CEdit* Final_Result_Control,int State,CString Msg_Log,int HandleNum,CString Category="-1",CString ChipRfIDbg=""); //重点看
void WriteLogIni(int LogNum,CString ChipRfIDbg="");//写LOG信息
BOOL GetExistReturnCode(CAdoInterface& myado,int DataUpNum,CEdit* m_Result,CEdit* Final_Result_Control,CString *ExistMessage,CString ChipRfIDbg="");
BOOL Data_UpdatePara(CAdoInterface& myado,int DataUpNum,CEdit* m_Result,CEdit* Final_Result_Control,BOOL ErrorUpEnable=TRUE,CString ChipRfIDbg="");
BOOL Data_UpdatePara2(CAdoInterface& myado,int DataUpNum,CEdit* m_Result,CEdit* Final_Result_Control,BOOL ErrorUpEnable=TRUE,CString ChipRfIDbg="");			//SMT测试
BOOL Data_UpdateError(CAdoInterface& myado,int DataUpNum,CEdit* m_Result,CEdit* Final_Result_Control,CString Message,CString Category,CString ChipRfIDbg="");
BOOL GetExistError(CAdoInterface& myado,int DataUpNum,CEdit* m_Result,CEdit* Final_Result_Control,CString *ExistMessage,CString ChipRfIDbg="");
BOOL HEX2JPG3(CString str,CString& m_sPath,CString NightDay,int HandleNum,CString ChipRfIDbg="");
*/
#include "stdafx.h"
#include "WriteIMEI.h"
#include "IMEIWrite_MulAT.h"
#include "Login2.h"
#include <Windows.h>
//---------------------------------------------------//
#include <stdio.h>
#include <wtypes.h>
#include <string.h>
#include <algorithm>



extern "C" int _imp___vsnprintf(
	char *buffer,
	size_t count,
	const char *format,
	va_list argptr
	)
{
	return vsnprintf(buffer, count, format, argptr);
}
//---------------------------------------------------//报vsnprintf错误请加这一段


//三合一系统消息函数宏定义
#define WM_SimpleMessage WM_USER+1001

//三合一全局变量
map<CString, int>IMEIWrite_MulAT::PortStatusMap;

//图的一个根据值找到键的方法，模板类函数的定义和声明要放在同一个文件中
template<typename _MapType>
auto get_map_key_value(const _MapType& input_map, const decltype(input_map.begin()->second)& mapped_value) -> decltype(input_map.begin()->first)
{
	auto iter = std::find_if(input_map.begin(), input_map.end(), [mapped_value](const std::map<CString, int>::value_type item) {
		return (item.second == mapped_value);
	});

	if (iter == input_map.end())
	{
		return decltype(input_map.begin()->first)();
	}
	return iter->first;
}


//文件复制
#include "shlwapi.h"

#define GPSNUMBERMAX  16
#define SETTING_ITEM_MAX 72
// IMEIWrite_MulAT 对话框

IMPLEMENT_DYNAMIC(IMEIWrite_MulAT, CDialog)

IMEIWrite_MulAT::IMEIWrite_MulAT(CWnd* pParent /*=NULL*/)
: CResizableDialog(IMEIWrite_MulAT::IDD, pParent)
, IMEI_InputA(_T(""))
, IMEI_InputB(_T(""))
, IMEI_InputC(_T(""))
, IMEI_InputD(_T(""))
, IMEI_InputE(_T(""))
, IMEI_InputF(_T(""))
, SuccessCount(0)
, FailCount(0)
, CheckSMTChoose(TRUE)
, RTestChoose(FALSE)
, GetPicChoose(TRUE)
, m_GdiplusToken(0)
, BGConfirmChoose(TRUE)
, GPSDataTypeChoose(FALSE)
, PicStaticChoose(TRUE)
, GetBluetoothCheckValue(FALSE)
, BLEGetSettingFlag(-1)
, BLEGetSettingEndFlag(0)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	GdiplusStartup(&m_GdiplusToken, &m_GdiplusStartupInput, NULL);

	//如果是三合一，这四个都要设置为FALSE
	if (g_ADCTFlag == 1)
	{
		CheckSMTChoose = FALSE;
		GetPicChoose = FALSE;
		BGConfirmChoose = FALSE;
		PicStaticChoose = FALSE;
	}

}

IMEIWrite_MulAT::~IMEIWrite_MulAT()
{
	DeleteCriticalSection(&GETPORT);
	DeleteCriticalSection(&CPUCOMMUNICATE);
	DeleteCriticalSection(&SHOWONEPIC);
	DeleteCriticalSection(&UseScanResult);
	DeleteCriticalSection(&SPEAKVOICE);

	DeleteCriticalSection(&SUCCFAILCOUNT);
	for (int i = 0; i<THREAD_NUM / 4; i++)
	{
		DeleteCriticalSection(&UPDATEDB[i]);
	}
	for (int j = 0; j<THREAD_NUM / 2; j++)
	{
		DeleteCriticalSection(&WIFICOMMUNICATE[j]);
	}
	FreeLibrary(password_Handle);
	GdiplusShutdown(m_GdiplusToken);
}

void IMEIWrite_MulAT::DoDataExchange(CDataExchange* pDX)
{
	CResizableDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO1, m_Port1);
	DDX_Control(pDX, IDC_COMBO2, m_Baud1);
	DDX_Control(pDX, IDC_FINAL_RESULT1, Final_Result_Control1);
	DDX_Control(pDX, IDC_RESULT, m_Result1);
	DDX_Text(pDX, IDC_IMEIA, IMEI_InputA);
	DDX_Control(pDX, IDC_IMEIA, IMEI_InputA_Control);
	DDX_Control(pDX, IDC_FINAL_RESULT2, Final_Result_Control2);
	DDX_Control(pDX, IDC_RESULT2, m_Result2);
	DDX_Text(pDX, IDC_IMEIA2, IMEI_InputB);
	DDX_Control(pDX, IDC_IMEIA2, IMEI_InputB_Control);
	DDX_Control(pDX, IDC_COMBO4, m_Port2);
	DDX_Control(pDX, IDC_COMBO5, m_Baud2);
	DDX_Control(pDX, IDC_FINAL_RESULT3, Final_Result_Control3);
	DDX_Control(pDX, IDC_RESULT3, m_Result3);
	DDX_Control(pDX, IDC_COMBO6, m_Port3);
	DDX_Control(pDX, IDC_COMBO7, m_Baud3);
	DDX_Text(pDX, IDC_IMEIA3, IMEI_InputC);
	DDX_Control(pDX, IDC_IMEIA3, IMEI_InputC_Control);
	DDX_Control(pDX, IDC_FINAL_RESULT4, Final_Result_Control4);
	DDX_Control(pDX, IDC_RESULT4, m_Result4);
	DDX_Control(pDX, IDC_COMBO8, m_Port4);
	DDX_Control(pDX, IDC_COMBO9, m_Baud4);
	DDX_Text(pDX, IDC_IMEIA4, IMEI_InputD);
	DDX_Control(pDX, IDC_IMEIA4, IMEI_InputD_Control);
	DDX_Control(pDX, IDC_FINAL_RESULT5, Final_Result_Control5);
	DDX_Control(pDX, IDC_RESULT5, m_Result5);
	DDX_Control(pDX, IDC_COMBO10, m_Port5);
	DDX_Control(pDX, IDC_COMBO11, m_Baud5);
	DDX_Text(pDX, IDC_IMEIA5, IMEI_InputE);
	DDX_Control(pDX, IDC_IMEIA5, IMEI_InputE_Control);
	DDX_Control(pDX, IDC_FINAL_RESULT6, Final_Result_Control6);
	DDX_Control(pDX, IDC_RESULT6, m_Result6);
	DDX_Control(pDX, IDC_COMBO12, m_Port6);
	DDX_Control(pDX, IDC_COMBO13, m_Baud6);
	DDX_Text(pDX, IDC_IMEIA6, IMEI_InputF);
	DDX_Control(pDX, IDC_IMEIA6, IMEI_InputF_Control);
	DDX_Control(pDX, IDC_BUTTON1, StartA_Control);
	DDX_Control(pDX, IDC_BUTTON14, StartB_Control);
	DDX_Control(pDX, IDC_BUTTON16, StartC_Control);
	DDX_Control(pDX, IDC_BUTTON18, StartD_Control);
	DDX_Control(pDX, IDC_BUTTON20, StartE_Control);
	DDX_Control(pDX, IDC_BUTTON22, StartF_Control);
	DDX_Control(pDX, IDC_COMBO14, ScanGunPort1);
	DDX_Control(pDX, IDC_COMBO18, ScanGunPort2);
	DDX_Control(pDX, IDC_COMBO20, ScanGunPort3);
	DDX_Control(pDX, IDC_COMBO16, ScanGunPort4);
	DDX_Control(pDX, IDC_COMBO22, ScanGunPort5);
	DDX_Control(pDX, IDC_COMBO24, ScanGunPort6);
	DDX_Control(pDX, IDC_COMBO15, ScanGunBaud1);
	DDX_Control(pDX, IDC_COMBO19, ScanGunBaud2);
	DDX_Control(pDX, IDC_COMBO21, ScanGunBaud3);
	DDX_Control(pDX, IDC_COMBO17, ScanGunBaud4);
	DDX_Control(pDX, IDC_COMBO23, ScanGunBaud5);
	DDX_Control(pDX, IDC_COMBO25, ScanGunBaud6);
	DDX_Control(pDX, IDC_COMBO26, ScanGunTpye);
	DDX_Control(pDX, IDC_COMBO27, PrintSelect);
	DDX_Control(pDX, IDC_COMBO28, PortType);
	DDX_Control(pDX, IDC_FINAL_RESULT7, Final_Result_Control7);
	DDX_Control(pDX, IDC_FINAL_RESULT8, Final_Result_Control8);
	DDX_Control(pDX, IDC_FINAL_RESULT9, Final_Result_Control9);
	DDX_Control(pDX, IDC_FINAL_RESULT10, Final_Result_Control10);
	DDX_Control(pDX, IDC_FINAL_RESULT11, Final_Result_Control11);
	DDX_Control(pDX, IDC_FINAL_RESULT12, Final_Result_Control12);
	DDX_Control(pDX, IDC_FINAL_RESULT13, Final_Result_Control13);
	DDX_Control(pDX, IDC_FINAL_RESULT14, Final_Result_Control14);
	DDX_Control(pDX, IDC_FINAL_RESULT15, Final_Result_Control15);
	DDX_Control(pDX, IDC_FINAL_RESULT16, Final_Result_Control16);
	DDX_Control(pDX, IDC_RESULT7, m_Result7);
	DDX_Control(pDX, IDC_RESULT8, m_Result8);
	DDX_Control(pDX, IDC_RESULT9, m_Result9);
	DDX_Control(pDX, IDC_RESULT10, m_Result10);
	DDX_Control(pDX, IDC_RESULT11, m_Result11);
	DDX_Control(pDX, IDC_RESULT12, m_Result12);
	DDX_Control(pDX, IDC_RESULT13, m_Result13);
	DDX_Control(pDX, IDC_RESULT14, m_Result14);
	DDX_Control(pDX, IDC_RESULT15, m_Result15);
	DDX_Control(pDX, IDC_RESULT16, m_Result16);
	DDX_Control(pDX, IDC_COMBO29, m_Port7);
	DDX_Control(pDX, IDC_COMBO31, m_Port8);
	DDX_Control(pDX, IDC_COMBO33, m_Port9);
	DDX_Control(pDX, IDC_COMBO35, m_Port10);
	DDX_Control(pDX, IDC_COMBO37, m_Port11);
	DDX_Control(pDX, IDC_COMBO39, m_Port12);
	DDX_Control(pDX, IDC_COMBO41, m_Port13);
	DDX_Control(pDX, IDC_COMBO43, m_Port14);
	DDX_Control(pDX, IDC_COMBO45, m_Port15);
	DDX_Control(pDX, IDC_COMBO47, m_Port16);
	DDX_Control(pDX, IDC_COMBO30, m_Baud7);
	DDX_Control(pDX, IDC_COMBO32, m_Baud8);
	DDX_Control(pDX, IDC_COMBO34, m_Baud9);
	DDX_Control(pDX, IDC_COMBO36, m_Baud10);
	DDX_Control(pDX, IDC_COMBO38, m_Baud11);
	DDX_Control(pDX, IDC_COMBO40, m_Baud12);
	DDX_Control(pDX, IDC_COMBO42, m_Baud13);
	DDX_Control(pDX, IDC_COMBO44, m_Baud14);
	DDX_Control(pDX, IDC_COMBO46, m_Baud15);
	DDX_Control(pDX, IDC_COMBO48, m_Baud16);
	DDX_Control(pDX, IDC_BUTTONSTART7, Start7_Control);
	DDX_Control(pDX, IDC_BUTTONSTART8, Start8_Control);
	DDX_Control(pDX, IDC_BUTTONSTART9, Start9_Control);
	DDX_Control(pDX, IDC_BUTTONSTART10, Start10_Control);
	DDX_Control(pDX, IDC_BUTTONSTART11, Start11_Control);
	DDX_Control(pDX, IDC_BUTTONSTART12, Start12_Control);
	DDX_Control(pDX, IDC_BUTTONSTART13, Start13_Control);
	DDX_Control(pDX, IDC_BUTTONSTART14, Start14_Control);
	DDX_Control(pDX, IDC_BUTTONSTART15, Start15_Control);
	DDX_Control(pDX, IDC_BUTTONSTART16, Start16_Control);
	DDX_Control(pDX, IDC_COMBO49, CPUControlPort);
	DDX_Control(pDX, IDC_COMBO50, CPUControlBaud);
	DDX_Control(pDX, IDC_COMBO51, OrderNumbersControl);
	DDX_Control(pDX, IDOK, StartALL_Control);
	DDX_Control(pDX, IDCANCEL, StopALL_Control);
	DDX_Control(pDX, IDC_COMBO52, GPS_SNRLimit);
	DDX_Text(pDX, IDC_EDITSUCCOUNT, SuccessCount);
	DDX_Text(pDX, IDC_EDITFAILCOUNT, FailCount);
	DDX_Control(pDX, IDC_COMBO53, ReturnCodeControl);
	DDX_Control(pDX, IDC_COMBO54, VIControlPort);
	DDX_Control(pDX, IDC_COMBO55, VIControlBaud);
	DDX_Control(pDX, IDC_COMBO56, WorkStationControl);
	DDX_Check(pDX, IDC_CHECK39, CheckSMTChoose);
	DDX_Check(pDX, IDC_CHECK40, RTestChoose);
	DDX_Check(pDX, IDC_CHECK41, GetPicChoose);
	DDX_Check(pDX, IDC_CHECK42, BGConfirmChoose);
	DDX_Check(pDX, IDC_CHECK43, GPSDataTypeChoose);
	DDX_Control(pDX, IDC_COMBO57, WIFI_RSSILimit);
	DDX_Check(pDX, IDC_CHECK44, PicStaticChoose);
	DDX_Check(pDX, IDC_BLUETOOTH_CHECK, GetBluetoothCheckValue);
}


BEGIN_MESSAGE_MAP(IMEIWrite_MulAT, CResizableDialog)
	ON_WM_CTLCOLOR()//改变颜色
	ON_WM_PAINT()
	ON_WM_SYSCOMMAND()//PostMessage

	ON_WM_CLOSE()

	ON_WM_RBUTTONUP()   //右键弹起
	ON_WM_CONTEXTMENU() //菜单

	ON_COMMAND(ID_FRESH_32771, OnCbnSelchangeCombo2_Fresh)//刷新端口
	ON_COMMAND(ID_32776, OnGetWebSetting)//获取网络配置
	ON_COMMAND(ID_32772, OnBnClickedButton6)

	ON_COMMAND(ID_32773START, OnBnClickedButton42)
	ON_COMMAND(ID_32777STOP, OnBnClickedButton40)

	ON_WM_TIMER()   //定时器

	ON_EN_CHANGE(IDC_IMEIA, &IMEIWrite_MulAT::OnEnChangeImeia)
	ON_BN_CLICKED(IDC_BUTTON1, &IMEIWrite_MulAT::OnBnClickedButton1)
	ON_BN_CLICKED(IDC_BUTTON2, &IMEIWrite_MulAT::OnBnClickedButton2)
	ON_EN_CHANGE(IDC_IMEIA2, &IMEIWrite_MulAT::OnEnChangeImeia2)
	ON_BN_CLICKED(IDC_BUTTON14, &IMEIWrite_MulAT::OnBnClickedButton14)
	ON_BN_CLICKED(IDC_BUTTON15, &IMEIWrite_MulAT::OnBnClickedButton15)
	ON_EN_CHANGE(IDC_IMEIA3, &IMEIWrite_MulAT::OnEnChangeImeia3)
	ON_BN_CLICKED(IDC_BUTTON16, &IMEIWrite_MulAT::OnBnClickedButton16)
	ON_BN_CLICKED(IDC_BUTTON17, &IMEIWrite_MulAT::OnBnClickedButton17)
	ON_EN_CHANGE(IDC_IMEIA4, &IMEIWrite_MulAT::OnEnChangeImeia4)
	ON_BN_CLICKED(IDC_BUTTON18, &IMEIWrite_MulAT::OnBnClickedButton18)
	ON_BN_CLICKED(IDC_BUTTON19, &IMEIWrite_MulAT::OnBnClickedButton19)
	ON_EN_CHANGE(IDC_IMEIA5, &IMEIWrite_MulAT::OnEnChangeImeia5)
	ON_BN_CLICKED(IDC_BUTTON20, &IMEIWrite_MulAT::OnBnClickedButton20)
	ON_BN_CLICKED(IDC_BUTTON21, &IMEIWrite_MulAT::OnBnClickedButton21)
	ON_EN_CHANGE(IDC_IMEIA6, &IMEIWrite_MulAT::OnEnChangeImeia6)
	ON_BN_CLICKED(IDC_BUTTON22, &IMEIWrite_MulAT::OnBnClickedButton22)
	ON_BN_CLICKED(IDC_BUTTON23, &IMEIWrite_MulAT::OnBnClickedButton23)
	ON_BN_CLICKED(IDC_BUTTON6, &IMEIWrite_MulAT::OnBnClickedButton6)
	ON_BN_CLICKED(IDC_BUTTON7, &IMEIWrite_MulAT::OnBnClickedButton7)
	ON_BN_CLICKED(IDC_BUTTON3, &IMEIWrite_MulAT::OnBnClickedButton3)
	ON_BN_CLICKED(IDC_BUTTON25, &IMEIWrite_MulAT::OnBnClickedButton25)
	ON_BN_CLICKED(IDC_BUTTON26, &IMEIWrite_MulAT::OnBnClickedButton26)
	ON_BN_CLICKED(IDC_BUTTON27, &IMEIWrite_MulAT::OnBnClickedButton27)
	ON_BN_CLICKED(IDC_BUTTON28, &IMEIWrite_MulAT::OnBnClickedButton28)
	ON_BN_CLICKED(IDC_BUTTON29, &IMEIWrite_MulAT::OnBnClickedButton29)
	ON_BN_CLICKED(IDC_BUTTON30, &IMEIWrite_MulAT::OnBnClickedButton30)
	ON_BN_CLICKED(IDC_BUTTON31, &IMEIWrite_MulAT::OnBnClickedButton31)
	ON_BN_CLICKED(IDC_BUTTON32, &IMEIWrite_MulAT::OnBnClickedButton32)
	ON_BN_CLICKED(IDC_BUTTON33, &IMEIWrite_MulAT::OnBnClickedButton33)
	ON_BN_CLICKED(IDC_BUTTON34, &IMEIWrite_MulAT::OnBnClickedButton34)
	ON_BN_CLICKED(IDC_BUTTON35, &IMEIWrite_MulAT::OnBnClickedButton35)
	ON_BN_CLICKED(IDC_BUTTON10, &IMEIWrite_MulAT::OnBnClickedButton10)
	ON_BN_CLICKED(IDC_BUTTON37, &IMEIWrite_MulAT::OnBnClickedButton37)
	ON_BN_CLICKED(IDC_BUTTON38, &IMEIWrite_MulAT::OnBnClickedButton38)
	ON_BN_CLICKED(IDOK, &IMEIWrite_MulAT::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &IMEIWrite_MulAT::OnBnClickedCancel)
	ON_BN_CLICKED(IDC_BUTTON40, &IMEIWrite_MulAT::OnBnClickedButton40)
	ON_BN_CLICKED(IDC_BUTTON42, &IMEIWrite_MulAT::OnBnClickedButton42)
	ON_BN_CLICKED(IDC_BUTTONSTART7, &IMEIWrite_MulAT::OnBnClickedButtonstart7)
	ON_BN_CLICKED(IDC_BUTTONSTOP7, &IMEIWrite_MulAT::OnBnClickedButtonstop7)
	ON_BN_CLICKED(IDC_BUTTONSTART8, &IMEIWrite_MulAT::OnBnClickedButtonstart8)
	ON_BN_CLICKED(IDC_BUTTONSTART9, &IMEIWrite_MulAT::OnBnClickedButtonstart9)
	ON_BN_CLICKED(IDC_BUTTONSTART10, &IMEIWrite_MulAT::OnBnClickedButtonstart10)
	ON_BN_CLICKED(IDC_BUTTONSTART11, &IMEIWrite_MulAT::OnBnClickedButtonstart11)
	ON_BN_CLICKED(IDC_BUTTONSTART12, &IMEIWrite_MulAT::OnBnClickedButtonstart12)
	ON_BN_CLICKED(IDC_BUTTONSTART13, &IMEIWrite_MulAT::OnBnClickedButtonstart13)
	ON_BN_CLICKED(IDC_BUTTONSTART14, &IMEIWrite_MulAT::OnBnClickedButtonstart14)
	ON_BN_CLICKED(IDC_BUTTONSTART15, &IMEIWrite_MulAT::OnBnClickedButtonstart15)
	ON_BN_CLICKED(IDC_BUTTONSTART16, &IMEIWrite_MulAT::OnBnClickedButtonstart16)
	ON_BN_CLICKED(IDC_BUTTONSTOP8, &IMEIWrite_MulAT::OnBnClickedButtonstop8)
	ON_BN_CLICKED(IDC_BUTTONSTOP9, &IMEIWrite_MulAT::OnBnClickedButtonstop9)
	ON_BN_CLICKED(IDC_BUTTONSTOP10, &IMEIWrite_MulAT::OnBnClickedButtonstop10)
	ON_BN_CLICKED(IDC_BUTTONSTOP11, &IMEIWrite_MulAT::OnBnClickedButtonstop11)
	ON_BN_CLICKED(IDC_BUTTONSTOP12, &IMEIWrite_MulAT::OnBnClickedButtonstop12)
	ON_BN_CLICKED(IDC_BUTTONSTOP13, &IMEIWrite_MulAT::OnBnClickedButtonstop13)
	ON_BN_CLICKED(IDC_BUTTONSTOP14, &IMEIWrite_MulAT::OnBnClickedButtonstop14)
	ON_BN_CLICKED(IDC_BUTTONSTOP15, &IMEIWrite_MulAT::OnBnClickedButtonstop15)
	ON_BN_CLICKED(IDC_BUTTONSTOP16, &IMEIWrite_MulAT::OnBnClickedButtonstop16)
	ON_BN_CLICKED(IDC_BUTTONCONNCPU, &IMEIWrite_MulAT::OnBnClickedButtonconncpu)
	ON_BN_CLICKED(IDC_BUTTONDISCONNCPU, &IMEIWrite_MulAT::OnBnClickedButtondisconncpu)
	ON_BN_CLICKED(IDC_BUTTONCONNCPU2, &IMEIWrite_MulAT::OnBnClickedButtonconncpu2)
	ON_BN_CLICKED(IDC_BUTTONDISCONNCPU2, &IMEIWrite_MulAT::OnBnClickedButtondisconncpu2)
	ON_BN_CLICKED(IDC_CHECK39, &IMEIWrite_MulAT::OnBnClickedCheck39)
	ON_BN_CLICKED(IDC_CHECK40, &IMEIWrite_MulAT::OnBnClickedCheck40)
	ON_CBN_SELCHANGE(IDC_COMBO53, &IMEIWrite_MulAT::OnCbnSelchangeCombo53)
	ON_BN_CLICKED(IDC_CHECK41, &IMEIWrite_MulAT::OnBnClickedCheck41)
	ON_BN_CLICKED(IDC_CHECK42, &IMEIWrite_MulAT::OnBnClickedCheck42)
	ON_BN_CLICKED(IDC_CHECK43, &IMEIWrite_MulAT::OnBnClickedCheck43)
	ON_CBN_SELCHANGE(IDC_COMBO56, &IMEIWrite_MulAT::OnCbnSelchangeCombo56)

	//三合一系统消息函数宏定义
	ON_MESSAGE(WM_SimpleMessage, &IMEIWrite_MulAT::MSG_GetSimpleMessage)
	ON_WM_COPYDATA()
	ON_WM_CLOSE()
	ON_BN_CLICKED(IDC_BLUETOOTH_CHECK, &IMEIWrite_MulAT::OnBnClickedBluetoothCheck)
END_MESSAGE_MAP()


// IMEIWrite_MulAT 消息处理程序

BOOL IMEIWrite_MulAT::OnInitDialog()//初始化程序
{
	CResizableDialog::OnInitDialog();

	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	COM_IniDone = FALSE;
	for (int j = 0; j<THREAD_NUM; j++)
	{
		hPort[j] = NULL;
		hScanGun[j] = NULL;
		hPicPort[j] = NULL;
		Vaule_Return_Count_CS[j] = "";

		BGShowPicTime[j] = "7000";
		BGShowPicCount[j] = 50;
	}
	hPort[THREAD_NUM] = NULL;
	for (int j = 0; j<PORTS_NUM; j++)
	{
		COM_Number[j] = "";
	}
	InitCOM(&m_Port1, &m_Baud1, 0);//初始化串口
	InitCOM(&m_Port2, &m_Baud2, 1);//初始化串口--LBC
	InitCOM(&m_Port3, &m_Baud3, 2);
	InitCOM(&m_Port4, &m_Baud4, 3);
	InitCOM(&m_Port5, &m_Baud5, 4);
	InitCOM(&m_Port6, &m_Baud6, 5);
	InitCOM(&m_Port7, &m_Baud7, 6);
	InitCOM(&m_Port8, &m_Baud8, 7);
	InitCOM(&m_Port9, &m_Baud9, 8);
	InitCOM(&m_Port10, &m_Baud10, 9);
	InitCOM(&m_Port11, &m_Baud11, 10);
	InitCOM(&m_Port12, &m_Baud12, 11);
	InitCOM(&m_Port13, &m_Baud13, 12);
	InitCOM(&m_Port14, &m_Baud14, 13);
	InitCOM(&m_Port15, &m_Baud15, 14);
	InitCOM(&m_Port16, &m_Baud16, 15);

	InitCOM(&CPUControlPort, &CPUControlBaud, 16);
	InitCOM(&VIControlPort, &VIControlBaud, 17);

	InitCOM(&ScanGunPort1, &ScanGunBaud1, 18);//初始化扫描枪串口
	InitCOM(&ScanGunPort2, &ScanGunBaud2, 1);
	InitCOM(&ScanGunPort3, &ScanGunBaud3, 2);
	InitCOM(&ScanGunPort4, &ScanGunBaud4, 3);
	InitCOM(&ScanGunPort5, &ScanGunBaud5, 4);
	InitCOM(&ScanGunPort6, &ScanGunBaud6, 5);

	ScanGunTpye.SetCurSel(0);
	PrintSelect.SetCurSel(0);

	HBRUSH mBrush = NULL;//画刷 ScanGunPort1

	font = new CFont;
	font->CreateFont(50, 0, 0, 0, 600, 0, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS,
		CLIP_DEFAULT_PRECIS,
		DEFAULT_QUALITY,
		DEFAULT_PITCH | FF_DONTCARE,
		"arial");

	fontsp = new CFont;
	fontsp->CreateFontA(30, 0, 0, 0, 600, 0, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS,
		CLIP_DEFAULT_PRECIS,
		DEFAULT_QUALITY,
		DEFAULT_PITCH | FF_DONTCARE,
		"arial");

	GetDlgItem(IDC_FINAL_RESULT1)->SetFont(font);
	GetDlgItem(IDC_FINAL_RESULT2)->SetFont(font);//--LBC
	GetDlgItem(IDC_FINAL_RESULT3)->SetFont(font);
	GetDlgItem(IDC_FINAL_RESULT4)->SetFont(font);
	GetDlgItem(IDC_FINAL_RESULT5)->SetFont(font);
	GetDlgItem(IDC_FINAL_RESULT6)->SetFont(font);
	GetDlgItem(IDC_FINAL_RESULT7)->SetFont(font);
	GetDlgItem(IDC_FINAL_RESULT8)->SetFont(font);
	GetDlgItem(IDC_FINAL_RESULT9)->SetFont(font);
	GetDlgItem(IDC_FINAL_RESULT10)->SetFont(font);
	GetDlgItem(IDC_FINAL_RESULT11)->SetFont(font);
	GetDlgItem(IDC_FINAL_RESULT12)->SetFont(font);
	GetDlgItem(IDC_FINAL_RESULT13)->SetFont(font);
	GetDlgItem(IDC_FINAL_RESULT14)->SetFont(font);
	GetDlgItem(IDC_FINAL_RESULT15)->SetFont(font);
	GetDlgItem(IDC_FINAL_RESULT16)->SetFont(font);


	//set the IMEI input boxfont
	CFont* ffont;
	ffont = new CFont;
	ffont->CreateFont(20, 0, 0, 0, 500, 0, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS,
		CLIP_DEFAULT_PRECIS,
		DEFAULT_QUALITY,
		DEFAULT_PITCH | FF_DONTCARE,
		"arial");
	//GetDlgItem(IDC_IMEIA)->SetFont(ffont);		//IMEI1
	//GetDlgItem(IDC_IMEIA2)->SetFont(ffont);		//IMEI2--LBC
	//GetDlgItem(IDC_IMEIA3)->SetFont(ffont);
	//GetDlgItem(IDC_IMEIA4)->SetFont(ffont);
	//GetDlgItem(IDC_IMEIA5)->SetFont(ffont);
	//GetDlgItem(IDC_IMEIA6)->SetFont(ffont);
	GetDlgItem(IDC_EDITSUCCOUNT)->SetFont(ffont);
	GetDlgItem(IDC_EDITFAILCOUNT)->SetFont(ffont);

	AddAnchor(IDC_BUTTON42, TOP_LEFT);
	AddAnchor(IDC_BUTTON40, TOP_LEFT);
	AddAnchor(IDOK, TOP_LEFT);
	AddAnchor(IDCANCEL, TOP_LEFT);
	AddAnchor(IDC_BUTTON7, TOP_LEFT);
	AddAnchor(IDC_BUTTON6, TOP_LEFT);

	AddAnchor(IDC_COMBOTESTYYPE2, TOP_LEFT);
	AddAnchor(IDC_COMBOTESTYYPE3, TOP_LEFT);
	AddAnchor(IDC_COMBOTESTYYPE4, TOP_LEFT);

	AddAnchor(IDC_STATICSCAN, TOP_LEFT);
	AddAnchor(IDC_COMBO49, TOP_LEFT);
	AddAnchor(IDC_COMBO50, TOP_LEFT);
	AddAnchor(IDC_COMBOTESTYYPE, TOP_LEFT);
	AddAnchor(IDC_BUTTONCONNCPU, TOP_LEFT);
	AddAnchor(IDC_BUTTONDISCONNCPU, TOP_LEFT);
	AddAnchor(IDC_STATICCPU, TOP_LEFT);
	AddAnchor(IDC_COMBO14, TOP_LEFT);
	AddAnchor(IDC_COMBO15, TOP_LEFT);
	AddAnchor(IDC_SCANDATA_EDIT, TOP_LEFT);
	AddAnchor(IDC_SCANDATA_STATIC, TOP_LEFT);
	AddAnchor(IDC_BUTTON3, TOP_LEFT);
	AddAnchor(IDC_BUTTON25, TOP_LEFT);
	AddAnchor(IDC_STATICCONFIG, TOP_LEFT);

	AddAnchor(IDC_STATICCPU2, TOP_LEFT);
	AddAnchor(IDC_COMBO54, TOP_LEFT);
	AddAnchor(IDC_COMBO55, TOP_LEFT);
	AddAnchor(IDC_BUTTONCONNCPU2, TOP_LEFT);
	AddAnchor(IDC_BUTTONDISCONNCPU2, TOP_LEFT);

	AddAnchor(IDC_STATICPIC, TOP_CENTER);
	AddAnchor(IDC_STATICORDER, TOP_CENTER);
	AddAnchor(IDC_COMBO51, TOP_CENTER);
	AddAnchor(IDC_COMBO28, TOP_CENTER);

	AddAnchor(IDC_STATICRETURN, TOP_CENTER);
	AddAnchor(IDC_COMBO53, TOP_CENTER);

	AddAnchor(IDC_COMBO56, TOP_CENTER);
	AddAnchor(IDC_CHECK39, TOP_CENTER);
	AddAnchor(IDC_CHECK40, TOP_CENTER);
	AddAnchor(IDC_CHECK41, TOP_CENTER);
	AddAnchor(IDC_CHECK42, TOP_CENTER);
	AddAnchor(IDC_CHECK44, TOP_CENTER);
	AddAnchor(IDC_BLUETOOTH_CHECK, TOP_CENTER);
	AddAnchor(IDC_CHECK43, TOP_LEFT4);//AddAnchor(IDC_CHECK43, TOP_CENTER);

	AddAnchor(IDC_STATICSUCCOUNT, TOP_CENTER);
	AddAnchor(IDC_STATICFAILCOUNT, TOP_CENTER);
	AddAnchor(IDC_EDITSUCCOUNT, TOP_CENTER);
	AddAnchor(IDC_EDITFAILCOUNT, TOP_CENTER);

	AddAnchor(IDC_STATICSNR, TOP_LEFT4);
	AddAnchor(IDC_COMBO52, TOP_LEFT4, TOP_RIGHT);

	AddAnchor(IDC_STATICSNR2, TOP_LEFT4);
	AddAnchor(IDC_COMBO57, TOP_LEFT4, TOP_RIGHT);

	//外框
	AddAnchor(IDC_STATICTHREAD1, TOP_LEFT, MIDDLE_LEFT2);
	AddAnchor(IDC_STATICTHREAD2, TOP_LEFT2, MIDDLE_LEFT3);
	AddAnchor(IDC_STATICTHREAD3, TOP_LEFT3, MIDDLE_LEFT4);
	AddAnchor(IDC_STATICTHREAD4, TOP_LEFT4, MIDDLE_LEFT5);
	AddAnchor(IDC_STATICTHREAD5, TOP_LEFT6, MIDDLE_LEFT7);
	AddAnchor(IDC_STATICTHREAD6, TOP_LEFT7, MIDDLE_LEFT8);
	AddAnchor(IDC_STATICTHREAD7, TOP_LEFT8, MIDDLE_LEFT9);
	AddAnchor(IDC_STATICTHREAD8, TOP_LEFT9, MIDDLE_LEFT10);
	AddAnchor(IDC_STATICTHREAD9, MIDDLE_LEFT, BOTTOM_LEFT2);
	AddAnchor(IDC_STATICTHREAD10, MIDDLE_LEFT2, BOTTOM_LEFT3);
	AddAnchor(IDC_STATICTHREAD11, MIDDLE_LEFT3, BOTTOM_LEFT4);
	AddAnchor(IDC_STATICTHREAD12, MIDDLE_LEFT4, BOTTOM_LEFT5);
	AddAnchor(IDC_STATICTHREAD13, MIDDLE_LEFT6, BOTTOM_LEFT7);
	AddAnchor(IDC_STATICTHREAD14, MIDDLE_LEFT7, BOTTOM_LEFT8);
	AddAnchor(IDC_STATICTHREAD15, MIDDLE_LEFT8, BOTTOM_LEFT9);
	AddAnchor(IDC_STATICTHREAD16, MIDDLE_LEFT9, BOTTOM_LEFT10);

	//最终结果
	AddAnchor(IDC_FINAL_RESULT1, TOP_LEFT, TOP_LEFT2);
	AddAnchor(IDC_FINAL_RESULT2, TOP_LEFT2, TOP_LEFT3);
	AddAnchor(IDC_FINAL_RESULT3, TOP_LEFT3, TOP_LEFT4);
	AddAnchor(IDC_FINAL_RESULT4, TOP_LEFT4, TOP_LEFT5);
	AddAnchor(IDC_FINAL_RESULT5, TOP_LEFT6, TOP_LEFT7);
	AddAnchor(IDC_FINAL_RESULT6, TOP_LEFT7, TOP_LEFT8);
	AddAnchor(IDC_FINAL_RESULT7, TOP_LEFT8, TOP_LEFT9);
	AddAnchor(IDC_FINAL_RESULT8, TOP_LEFT9, TOP_LEFT10);
	AddAnchor(IDC_FINAL_RESULT9, MIDDLE_LEFT, MIDDLE_LEFT2);
	AddAnchor(IDC_FINAL_RESULT10, MIDDLE_LEFT2, MIDDLE_LEFT3);
	AddAnchor(IDC_FINAL_RESULT11, MIDDLE_LEFT3, MIDDLE_LEFT4);
	AddAnchor(IDC_FINAL_RESULT12, MIDDLE_LEFT4, MIDDLE_LEFT5);
	AddAnchor(IDC_FINAL_RESULT13, MIDDLE_LEFT6, MIDDLE_LEFT7);
	AddAnchor(IDC_FINAL_RESULT14, MIDDLE_LEFT7, MIDDLE_LEFT8);
	AddAnchor(IDC_FINAL_RESULT15, MIDDLE_LEFT8, MIDDLE_LEFT9);
	AddAnchor(IDC_FINAL_RESULT16, MIDDLE_LEFT9, MIDDLE_LEFT10);

	//LOG显示
	AddAnchor(IDC_RESULT, TOP_LEFT, MIDDLE_LEFT2);
	AddAnchor(IDC_RESULT2, TOP_LEFT2, MIDDLE_LEFT3);
	AddAnchor(IDC_RESULT3, TOP_LEFT3, MIDDLE_LEFT4);
	AddAnchor(IDC_RESULT4, TOP_LEFT4, MIDDLE_LEFT5);
	AddAnchor(IDC_RESULT5, TOP_LEFT6, MIDDLE_LEFT7);
	AddAnchor(IDC_RESULT6, TOP_LEFT7, MIDDLE_LEFT8);
	AddAnchor(IDC_RESULT7, TOP_LEFT8, MIDDLE_LEFT9);
	AddAnchor(IDC_RESULT8, TOP_LEFT9, MIDDLE_LEFT10);
	AddAnchor(IDC_RESULT9, MIDDLE_LEFT, BOTTOM_LEFT2);
	AddAnchor(IDC_RESULT10, MIDDLE_LEFT2, BOTTOM_LEFT3);
	AddAnchor(IDC_RESULT11, MIDDLE_LEFT3, BOTTOM_LEFT4);
	AddAnchor(IDC_RESULT12, MIDDLE_LEFT4, BOTTOM_LEFT5);
	AddAnchor(IDC_RESULT13, MIDDLE_LEFT6, BOTTOM_LEFT7);
	AddAnchor(IDC_RESULT14, MIDDLE_LEFT7, BOTTOM_LEFT8);
	AddAnchor(IDC_RESULT15, MIDDLE_LEFT8, BOTTOM_LEFT9);
	AddAnchor(IDC_RESULT16, MIDDLE_LEFT9, BOTTOM_LEFT10);

	//端口
	AddAnchor(IDC_STATICPORT1, TOP_LEFT);
	AddAnchor(IDC_STATICPORT2, TOP_LEFT2);
	AddAnchor(IDC_STATICPORT3, TOP_LEFT3);
	AddAnchor(IDC_STATICPORT4, TOP_LEFT4);
	AddAnchor(IDC_STATICPORT5, TOP_LEFT6);
	AddAnchor(IDC_STATICPORT6, TOP_LEFT7);
	AddAnchor(IDC_STATICPORT7, TOP_LEFT8);
	AddAnchor(IDC_STATICPORT8, TOP_LEFT9);
	AddAnchor(IDC_STATICPORT9, MIDDLE_LEFT);
	AddAnchor(IDC_STATICPORT10, MIDDLE_LEFT2);
	AddAnchor(IDC_STATICPORT11, MIDDLE_LEFT3);
	AddAnchor(IDC_STATICPORT12, MIDDLE_LEFT5);
	AddAnchor(IDC_STATICPORT13, MIDDLE_LEFT6);
	AddAnchor(IDC_STATICPORT14, MIDDLE_LEFT7);
	AddAnchor(IDC_STATICPORT15, MIDDLE_LEFT8);
	AddAnchor(IDC_STATICPORT16, MIDDLE_LEFT9);

	AddAnchor(IDC_STATICBAUD1, TOP_LEFT);
	AddAnchor(IDC_STATICBAUD2, TOP_LEFT2);
	AddAnchor(IDC_STATICBAUD3, TOP_LEFT3);
	AddAnchor(IDC_STATICBAUD4, TOP_LEFT4);
	AddAnchor(IDC_STATICBAUD5, TOP_LEFT6);
	AddAnchor(IDC_STATICBAUD6, TOP_LEFT7);
	AddAnchor(IDC_STATICBAUD7, TOP_LEFT8);
	AddAnchor(IDC_STATICBAUD8, TOP_LEFT9);
	AddAnchor(IDC_STATICBAUD9, MIDDLE_LEFT);
	AddAnchor(IDC_STATICBAUD10, MIDDLE_LEFT2);
	AddAnchor(IDC_STATICBAUD11, MIDDLE_LEFT3);
	AddAnchor(IDC_STATICBAUD12, MIDDLE_LEFT5);
	AddAnchor(IDC_STATICBAUD13, MIDDLE_LEFT6);
	AddAnchor(IDC_STATICBAUD14, MIDDLE_LEFT7);
	AddAnchor(IDC_STATICBAUD15, MIDDLE_LEFT8);
	AddAnchor(IDC_STATICBAUD16, MIDDLE_LEFT9);

	AddAnchor(IDC_COMBO1, TOP_LEFT);
	AddAnchor(IDC_COMBO2, TOP_LEFT);
	AddAnchor(IDC_COMBO4, TOP_LEFT2);
	AddAnchor(IDC_COMBO5, TOP_LEFT2);
	AddAnchor(IDC_COMBO6, TOP_LEFT3);
	AddAnchor(IDC_COMBO7, TOP_LEFT3);
	AddAnchor(IDC_COMBO8, TOP_LEFT4);
	AddAnchor(IDC_COMBO9, TOP_LEFT4);
	AddAnchor(IDC_COMBO10, TOP_LEFT6);
	AddAnchor(IDC_COMBO11, TOP_LEFT6);
	AddAnchor(IDC_COMBO12, TOP_LEFT7);
	AddAnchor(IDC_COMBO13, TOP_LEFT7);
	AddAnchor(IDC_COMBO29, TOP_LEFT8);
	AddAnchor(IDC_COMBO30, TOP_LEFT8);
	AddAnchor(IDC_COMBO31, TOP_LEFT9);
	AddAnchor(IDC_COMBO32, TOP_LEFT9);
	AddAnchor(IDC_COMBO33, MIDDLE_LEFT);
	AddAnchor(IDC_COMBO34, MIDDLE_LEFT);
	AddAnchor(IDC_COMBO35, MIDDLE_LEFT2);
	AddAnchor(IDC_COMBO36, MIDDLE_LEFT2);
	AddAnchor(IDC_COMBO37, MIDDLE_LEFT3);
	AddAnchor(IDC_COMBO38, MIDDLE_LEFT3);
	AddAnchor(IDC_COMBO39, MIDDLE_LEFT5);
	AddAnchor(IDC_COMBO40, MIDDLE_LEFT5);
	AddAnchor(IDC_COMBO41, MIDDLE_LEFT6);
	AddAnchor(IDC_COMBO42, MIDDLE_LEFT6);
	AddAnchor(IDC_COMBO43, MIDDLE_LEFT7);
	AddAnchor(IDC_COMBO44, MIDDLE_LEFT7);
	AddAnchor(IDC_COMBO45, MIDDLE_LEFT8);
	AddAnchor(IDC_COMBO46, MIDDLE_LEFT8);
	AddAnchor(IDC_COMBO47, MIDDLE_LEFT9);
	AddAnchor(IDC_COMBO48, MIDDLE_LEFT9);


	AddAnchor(IDC_BUTTON1, TOP_LEFT);
	AddAnchor(IDC_BUTTON2, TOP_LEFT);
	AddAnchor(IDC_BUTTON14, TOP_LEFT2);
	AddAnchor(IDC_BUTTON15, TOP_LEFT2);
	AddAnchor(IDC_BUTTON16, TOP_LEFT3);
	AddAnchor(IDC_BUTTON17, TOP_LEFT3);
	AddAnchor(IDC_BUTTON18, TOP_LEFT4);
	AddAnchor(IDC_BUTTON19, TOP_LEFT4);
	AddAnchor(IDC_BUTTON20, TOP_LEFT6);
	AddAnchor(IDC_BUTTON21, TOP_LEFT6);
	AddAnchor(IDC_BUTTON22, TOP_LEFT7);
	AddAnchor(IDC_BUTTON23, TOP_LEFT7);
	AddAnchor(IDC_BUTTONSTART7, TOP_LEFT8);
	AddAnchor(IDC_BUTTONSTOP7, TOP_LEFT8);
	AddAnchor(IDC_BUTTONSTART8, TOP_LEFT9);
	AddAnchor(IDC_BUTTONSTOP8, TOP_LEFT9);
	AddAnchor(IDC_BUTTONSTART9, MIDDLE_LEFT);
	AddAnchor(IDC_BUTTONSTOP9, MIDDLE_LEFT);
	AddAnchor(IDC_BUTTONSTART10, MIDDLE_LEFT2);
	AddAnchor(IDC_BUTTONSTOP10, MIDDLE_LEFT2);
	AddAnchor(IDC_BUTTONSTART11, MIDDLE_LEFT3);
	AddAnchor(IDC_BUTTONSTOP11, MIDDLE_LEFT3);
	AddAnchor(IDC_BUTTONSTART12, MIDDLE_LEFT5);
	AddAnchor(IDC_BUTTONSTOP12, MIDDLE_LEFT5);
	AddAnchor(IDC_BUTTONSTART13, MIDDLE_LEFT6);
	AddAnchor(IDC_BUTTONSTOP13, MIDDLE_LEFT6);
	AddAnchor(IDC_BUTTONSTART14, MIDDLE_LEFT7);
	AddAnchor(IDC_BUTTONSTOP14, MIDDLE_LEFT7);
	AddAnchor(IDC_BUTTONSTART15, MIDDLE_LEFT8);
	AddAnchor(IDC_BUTTONSTOP15, MIDDLE_LEFT8);
	AddAnchor(IDC_BUTTONSTART16, MIDDLE_LEFT9);
	AddAnchor(IDC_BUTTONSTOP16, MIDDLE_LEFT9);

	AT_LoadConfig();

	if (IMEI_Setdlg.Initial_Connect_DB == TRUE)
	{
		EnableWindow_StartALL(FALSE);
	}
	else
	{

	}
	UI_LoadConfig();

	for (int i = 0; i<THREAD_NUM; i++)
	{
		gColor[i] = RGB(60, 190, 190);

		if (IMEI_Setdlg.Initial_Connect_DB == TRUE)
		{
			if (1)
			{
				if (i == 0)
					LogShow_exchange(&m_Result1, &Final_Result_Control1, 251, "初始化完成！\r\n", i);
				if (i == 1)
					LogShow_exchange(&m_Result2, &Final_Result_Control2, 251, "初始化完成！\r\n", i);
				if (i == 2)
					LogShow_exchange(&m_Result3, &Final_Result_Control3, 251, "初始化完成！\r\n", i);
				if (i == 3)
					LogShow_exchange(&m_Result4, &Final_Result_Control4, 251, "初始化完成！\r\n", i);
				if (i == 4)
					LogShow_exchange(&m_Result5, &Final_Result_Control5, 251, "初始化完成！\r\n", i);
				if (i == 5)
					LogShow_exchange(&m_Result6, &Final_Result_Control6, 251, "初始化完成！\r\n", i);

				if (i == 6)
					LogShow_exchange(&m_Result7, &Final_Result_Control7, 251, "初始化完成！\r\n", i);
				if (i == 7)
					LogShow_exchange(&m_Result8, &Final_Result_Control8, 251, "初始化完成！\r\n", i);
				if (i == 8)
					LogShow_exchange(&m_Result9, &Final_Result_Control9, 251, "初始化完成！\r\n", i);
				if (i == 9)
					LogShow_exchange(&m_Result10, &Final_Result_Control10, 251, "初始化完成！\r\n", i);
				if (i == 10)
					LogShow_exchange(&m_Result11, &Final_Result_Control11, 251, "初始化完成！\r\n", i);
				if (i == 11)
					LogShow_exchange(&m_Result12, &Final_Result_Control12, 251, "初始化完成！\r\n", i);
				if (i == 12)
					LogShow_exchange(&m_Result13, &Final_Result_Control13, 251, "初始化完成！\r\n", i);
				if (i == 13)
					LogShow_exchange(&m_Result14, &Final_Result_Control14, 251, "初始化完成！\r\n", i);
				if (i == 14)
					LogShow_exchange(&m_Result15, &Final_Result_Control15, 251, "初始化完成！\r\n", i);
				if (i == 15)
					LogShow_exchange(&m_Result16, &Final_Result_Control16, 251, "初始化完成！\r\n", i);
			}
		}
		else
		{
			if (i == 0)
				LogShow_exchange(&m_Result1, &Final_Result_Control1, 250, "没勾选连接数据库！\r\n", i);
			if (i == 1)
				LogShow_exchange(&m_Result2, &Final_Result_Control2, 250, "没勾选连接数据库！\r\n", i);
			if (i == 2)
				LogShow_exchange(&m_Result3, &Final_Result_Control3, 250, "没勾选连接数据库！\r\n", i);
			if (i == 3)
				LogShow_exchange(&m_Result4, &Final_Result_Control4, 250, "没勾选连接数据库！\r\n", i);
			if (i == 4)
				LogShow_exchange(&m_Result5, &Final_Result_Control5, 250, "没勾选连接数据库！\r\n", i);
			if (i == 5)
				LogShow_exchange(&m_Result6, &Final_Result_Control6, 250, "没勾选连接数据库！\r\n", i);

			if (i == 6)
				LogShow_exchange(&m_Result7, &Final_Result_Control7, 250, "没勾选连接数据库！\r\n", i);
			if (i == 7)
				LogShow_exchange(&m_Result8, &Final_Result_Control8, 250, "没勾选连接数据库！\r\n", i);
			if (i == 8)
				LogShow_exchange(&m_Result9, &Final_Result_Control9, 250, "没勾选连接数据库！\r\n", i);
			if (i == 9)
				LogShow_exchange(&m_Result10, &Final_Result_Control10, 250, "没勾选连接数据库！\r\n", i);
			if (i == 10)
				LogShow_exchange(&m_Result11, &Final_Result_Control11, 250, "没勾选连接数据库！\r\n", i);
			if (i == 11)
				LogShow_exchange(&m_Result12, &Final_Result_Control12, 250, "没勾选连接数据库！\r\n", i);
			if (i == 12)
				LogShow_exchange(&m_Result13, &Final_Result_Control13, 250, "没勾选连接数据库！\r\n", i);
			if (i == 13)
				LogShow_exchange(&m_Result14, &Final_Result_Control14, 250, "没勾选连接数据库！\r\n", i);
			if (i == 14)
				LogShow_exchange(&m_Result15, &Final_Result_Control15, 250, "没勾选连接数据库！\r\n", i);
			if (i == 15)
				LogShow_exchange(&m_Result16, &Final_Result_Control16, 250, "没勾选连接数据库！\r\n", i);
		}
		Sleep(20);
		Thread_State[i] = FALSE;
		COM_State[i] = FALSE;

		TestResult[i] = FALSE;
	}
	COM_State[THREAD_NUM] = FALSE;
	EnableWindow_Choose(0);
	EnableWindow_ALL(FALSE);

	PrintOn = FALSE;
	for (int p = 0; p<THREAD_NUM; p++)
	{
		Data3_FromDB[p] = "";
		Data2_FromDB[p] = "";
		Data1_FromDB[p] = "";
		StopSign[p] = FALSE;
	}

	StopAutoStart = TRUE;

	InitializeCriticalSection(&GETPORT);
	InitializeCriticalSection(&CPUCOMMUNICATE);
	InitializeCriticalSection(&SHOWONEPIC);
	InitializeCriticalSection(&UseScanResult);
	InitializeCriticalSection(&SPEAKVOICE);

	InitializeCriticalSection(&SUCCFAILCOUNT);
	for (int i = 0; i<THREAD_NUM / 4; i++)
	{
		InitializeCriticalSection(&UPDATEDB[i]);
	}

	for (int i = 0; i<THREAD_NUM / 2; i++)
	{
		InitializeCriticalSection(&WIFICOMMUNICATE[i]);
	}

	PortType.SetCurSel(1);

	TestCheck = FALSE;
	CompareIMEI = FALSE;
	UpdateData(FALSE);

	CMenu m_menu;
	m_menu.LoadMenu(IDR_SYSMENU);
	SetMenu(&m_menu);
	Voice_Ini();
	Voice_Speak("欢迎使用");
	SetTimer(31, 100, NULL);

	LoadDll();
	WorkStationControl.SetCurSel(1);

	//三合一新增功能
	SetUnhandledExceptionFilter((LPTOP_LEVEL_EXCEPTION_FILTER)CallBackCrashHandler); //声明调用回调函数，程序崩溃时发送消息给主控

	//整机测试新增功能
	GetBleOrder();//获取蓝牙配置文件里额指令

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void IMEIWrite_MulAT::OnPaint()
{
	if (1)
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
		dc.FillSolidRect(rect, RGB(210, 227, 199));
	}
	else
	{
		CDialog::OnPaint();
	}
}

void IMEIWrite_MulAT::Delay_Clock(UINT mSecond)//定时器延时
{
	RunCommandList1.ResetEvent();//等待
	WaitForSingleObject(RunCommandList1.m_hObject, mSecond + 500);
	return;
}
void IMEIWrite_MulAT::UI_LoadConfig()
{

	for (int j = 0; j<ITEMMAX; j++)
	{
		paraArray[j].ifCheck = false;          //没选中 
		paraArray[j].Low_Limit_Value = "";	//期望上下限值
		paraArray[j].High_Limit_Value = "";	//期望上下限值
		paraArray[j].Other_ITEM = "";
	}
	int nCommandNum = 0;

	CString strCommand = _T("");
	CString strValue = _T("");

	CString strName = _T("");
	CStringArray strCommandAry;



	char szName[MAX_PATH] = _T("");
	CString  szPath = _T("");
	GetModuleFileName(NULL, szName, MAX_PATH);
	szPath = szName;
	szPath = szPath.Mid(0, szPath.ReverseFind('\\') + 1);

	CString strLoadFile = szPath + STRING_PATH_BLE;
	CIniFile tmpread(strLoadFile);
	CString fSectionName = tmpread.GetParaName();
	if (fSectionName == "BLE")
	{
		// 读入INI文件
		ASSERT(!strLoadFile.IsEmpty());
		CIniFile m_fIniFile(strLoadFile);
		int i = 0;
		char aText[140] = { 0 };

		int nCommand = 0;
		int nCommandNum = 0;
		RVITEM rvi;

		CString strCommand = _T("");
		CString strValue = _T("");//测试上下限值
		CString Check_item = _T("");//是否选中
		CString strValue_low = _T("");//测试下限值
		CString strValue_high = _T("");//测试上限值
		CString strValue_OtherITEM = _T("");//其他自定义


		CStringArray strCommandAry;

		m_fIniFile.ReadSection(STRING_SECTION_BLE, strCommandAry);
		nCommandNum = strCommandAry.GetSize();

		if (nCommandNum == 0)
		{
			CString strInfo;
			strInfo = _T("Imported script file don't have setting data!");
			AfxMessageBox(strInfo);
			return;
		}
		else
		{
			for (i = 0; i< nCommandNum; i++)         //对象数组的数量
			{
				strCommand = strCommandAry[i];      //编号
				paraArray[i].showName = strCommand;   //具体测试项

				strValue = m_fIniFile.ReadString(STRING_SECTION_BLE, strCommand, "");//编号对应的内容

				int npos = strValue.Find("@@");
				if (npos >= 0)
				{
					Check_item = strValue.Left(npos);
					strValue = strValue.Mid(npos + 2);
				}
				npos = strValue.Find("@@");
				if (npos >= 0)
				{
					strValue_low = strValue.Left(npos);
					strValue = strValue.Mid(npos + 2);
				}
				npos = strValue.Find("@@");
				if (npos >= 0)
				{
					strValue_high = strValue.Left(npos);
					strValue_OtherITEM = strValue.Mid(npos + 2);
				}
				else
				{
					strValue_high = strValue;
					strValue_OtherITEM = "null";
				}

				paraArray[i].Low_Limit_Value = strValue_low;	//期望上下限值
				paraArray[i].High_Limit_Value = strValue_high;	//期望上下限值
				paraArray[i].Other_ITEM = strValue_OtherITEM;	//其他自定义
				if (Check_item == "true")
					paraArray[i].ifCheck = true;           //选中       
				else
					paraArray[i].ifCheck = false;          //没选中       

			}
		}
	}
	UpdateData(false);
}
void IMEIWrite_MulAT::AT_LoadConfig()
{
	int nCommandNum = 0;

	CString strCommand = _T("");
	CStringArray strCommandAry;

	char szName[MAX_PATH] = _T("");
	CString  szPath = _T("");
	GetModuleFileName(NULL, szName, MAX_PATH);
	szPath = szName;
	szPath = szPath.Mid(0, szPath.ReverseFind('\\') + 1);

	CString strLoadFile = szPath + STRING_PATH_CONFIG;
	CIniFile m_fIniFile(strLoadFile);

	m_fIniFile.ReadSection(STRING_SECTION_CONFIG, strCommandAry);
	nCommandNum = strCommandAry.GetSize();
	CString INI_MSG_TEMP;
	for (int i = 0; i<nCommandNum; i++)
	{
		strCommand = strCommandAry[i];//编号
		if (strCommand == "Initial_Connect_DB_M")
		{
			INI_MSG_TEMP = m_fIniFile.ReadString(STRING_SECTION_CONFIG, strCommand, "");
			if (INI_MSG_TEMP == "true")
				IMEI_Setdlg.Initial_Connect_DB = TRUE;
			else
				IMEI_Setdlg.Initial_Connect_DB = FALSE;
		}
		//三合一的，好像是因为单机版设置的影响，这里暂且都用IP为240的全局变量来代替
		else if (strCommand == "m_server_M")
		{
			//IMEI_Setdlg.m_server = m_fIniFile.ReadString(STRING_SECTION_CONFIG, strCommand, "");
			IMEI_Setdlg.m_server = g_BackDoorIP;
		}
		else if (strCommand == "m_db_M")
		{
			//IMEI_Setdlg.m_db = m_fIniFile.ReadString(STRING_SECTION_CONFIG, strCommand, "");
			IMEI_Setdlg.m_db = g_BackDoorDatabase;
		}
		else if (strCommand == "m_user_M")
		{
			//IMEI_Setdlg.m_user = m_fIniFile.ReadString(STRING_SECTION_CONFIG, strCommand, "");
			IMEI_Setdlg.m_user = g_BackDoorUser;
		}
		else if (strCommand == "m_pwd_M")
		{
			//IMEI_Setdlg.m_pwd = m_fIniFile.ReadString(STRING_SECTION_CONFIG, strCommand, "");
			IMEI_Setdlg.m_pwd = g_BackDoorPassword;
		}
		else if (strCommand == "DestFile_M")
		{
			IMEI_Setdlg.DestFile = m_fIniFile.ReadString(STRING_SECTION_CONFIG, strCommand, "");
		}
	}
}


BOOL IMEIWrite_MulAT::OnGetport()
{
	// TODO: Add your control notification handler code here
	EnterCriticalSection(&GETPORT);
	HKEY hKey;
	for (int j = 0; j<PORTS_NUM; j++)
	{
		COM_Number[j] = "";
	}
	if (::RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Hardware\\DeviceMap\\SerialComm",
		NULL, KEY_READ, &hKey) == ERROR_SUCCESS) // 打开串口注册表
	{
		int i = 0;
		char portName[256], commName[256];
		DWORD dwLong, dwSize;
		while (1)
		{
			dwLong = dwSize = sizeof(portName);
			if (::RegEnumValue(hKey, i, portName, &dwLong, NULL, NULL, (PUCHAR)commName,
				&dwSize) == ERROR_NO_MORE_ITEMS) // 枚举串口
				break;

			COM_Number[i] = commName;
			i++;
			if (i >= PORTS_NUM)
				break;
		}
		if (i == 0)
		{
			LeaveCriticalSection(&GETPORT);
			return FALSE;
		}
		RegCloseKey(hKey);
	}
	else
	{
		LeaveCriticalSection(&GETPORT);
		return FALSE;
	}
	LeaveCriticalSection(&GETPORT);
	return TRUE;
}
void IMEIWrite_MulAT::InitCOM(CComboBox* m_Port, CComboBox* m_Baud, int num)//初始化串口
{
	//init baud
	m_Baud->AddString("460800");
	m_Baud->AddString("512000");
	m_Baud->AddString("600000");
	m_Baud->AddString("2400");
	m_Baud->AddString("4800");
	m_Baud->AddString("9600");
	m_Baud->AddString("14400");
	m_Baud->AddString("19200");

	m_Baud->AddString("38400");
	m_Baud->AddString("56000");
	m_Baud->AddString("57600");
	m_Baud->AddString("115200");
	m_Baud->AddString("128000");

	m_Baud->AddString("256000");

	m_Baud->SetCurSel(0);

	//init port
	//int Count=0;
	BOOL COM1_EXIST = FALSE;
	if (COM_IniDone == FALSE)
	{
		HKEY hKey;
		if (::RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Hardware\\DeviceMap\\SerialComm",
			NULL, KEY_READ, &hKey) == ERROR_SUCCESS) // 打开串口注册表
		{
			int i = 0;
			char portName[256], commName[256];
			DWORD dwLong, dwSize;
			while (1)
			{
				dwLong = dwSize = sizeof(portName);
				if (::RegEnumValue(hKey, i, portName, &dwLong, NULL, NULL, (PUCHAR)commName,
					&dwSize) == ERROR_NO_MORE_ITEMS)	// 枚举串口
					break;

				m_Port->AddString(commName);			// commName就是串口名字
				COM_Number[i] = commName;


				if (COM_Number[i] == "COM1")
					COM1_EXIST = TRUE;

				i++;
				if (i >= PORTS_NUM)
					break;
			}
			if (m_Port->GetCount() == 0)
			{
				::AfxMessageBox("在HKEY_LOCAL_MACHINE:Hardware\\DeviceMap\\SerialComm里找不到串口!!!");
			}
			RegCloseKey(hKey);
		}
		COM_IniDone = TRUE;
	}
	else
	{
		for (int j = 0; j<PORTS_NUM; j++)
		{
			if (COM_Number[j] != "")
				m_Port->AddString(COM_Number[j]);
			if (COM_Number[j] == "COM1")
				COM1_EXIST = TRUE;
		}
	}
	if (COM1_EXIST == TRUE)
		m_Port->SetCurSel(num + 1);
	else
		m_Port->SetCurSel(num);
}

HBRUSH IMEIWrite_MulAT::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)//改变颜色
{
	HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
	switch (pWnd->GetDlgCtrlID())
	{
	case(IDC_EDITSUCCOUNT) :
		pDC->SetTextColor(RGB(0, 255, 0));
		break;
	case(IDC_EDITFAILCOUNT) :
		pDC->SetTextColor(RGB(255, 0, 0));
		break;
	case(IDC_FINAL_RESULT1) :
		pDC->SetTextColor(gColor[0]);
		break;
	case(IDC_FINAL_RESULT2) :
		pDC->SetTextColor(gColor[1]);
		break;//--LBC
	case(IDC_FINAL_RESULT3) :
		pDC->SetTextColor(gColor[2]);
		break;
	case(IDC_FINAL_RESULT4) :
		pDC->SetTextColor(gColor[3]);
		break;
	case(IDC_FINAL_RESULT5) :
		pDC->SetTextColor(gColor[4]);
		break;
	case(IDC_FINAL_RESULT6) :
		pDC->SetTextColor(gColor[5]);
		break;
	case(IDC_FINAL_RESULT7) :
		pDC->SetTextColor(gColor[6]);
		break;
	case(IDC_FINAL_RESULT8) :
		pDC->SetTextColor(gColor[7]);
		break;
	case(IDC_FINAL_RESULT9) :
		pDC->SetTextColor(gColor[8]);
		break;
	case(IDC_FINAL_RESULT10) :
		pDC->SetTextColor(gColor[9]);
		break;
	case(IDC_FINAL_RESULT11) :
		pDC->SetTextColor(gColor[10]);
		break;
	case(IDC_FINAL_RESULT12) :
		pDC->SetTextColor(gColor[11]);
		break;
	case(IDC_FINAL_RESULT13) :
		pDC->SetTextColor(gColor[12]);
		break;
	case(IDC_FINAL_RESULT14) :
		pDC->SetTextColor(gColor[13]);
		break;
	case(IDC_FINAL_RESULT15) :
		pDC->SetTextColor(gColor[14]);
		break;
	case(IDC_FINAL_RESULT16) :
		pDC->SetTextColor(gColor[15]);
		break;

	case(IDC_RESULT) :
		pDC->SetTextColor(RGB(0, 0, 0));
		pDC->SetBkColor(RGB(210, 227, 199));
		break;
	case(IDC_RESULT2) :
		pDC->SetTextColor(RGB(0, 0, 0));
		pDC->SetBkColor(RGB(210, 227, 199));
		break;//--LBC
	case(IDC_RESULT3) :
		pDC->SetTextColor(RGB(0, 0, 0));
		pDC->SetBkColor(RGB(210, 227, 199));
		break;
	case(IDC_RESULT4) :
		pDC->SetTextColor(RGB(0, 0, 0));
		pDC->SetBkColor(RGB(210, 227, 199));
		break;
	case(IDC_RESULT5) :
		pDC->SetTextColor(RGB(0, 0, 0));
		pDC->SetBkColor(RGB(210, 227, 199));
		break;
	case(IDC_RESULT6) :
		pDC->SetTextColor(RGB(0, 0, 0));
		pDC->SetBkColor(RGB(210, 227, 199));
		break;
	case(IDC_RESULT7) :
		pDC->SetTextColor(RGB(0, 0, 0));
		pDC->SetBkColor(RGB(210, 227, 199));
		break;
	case(IDC_RESULT8) :
		pDC->SetTextColor(RGB(0, 0, 0));
		pDC->SetBkColor(RGB(210, 227, 199));
		break;
	case(IDC_RESULT9) :
		pDC->SetTextColor(RGB(0, 0, 0));
		pDC->SetBkColor(RGB(210, 227, 199));
		break;
	case(IDC_RESULT10) :
		pDC->SetTextColor(RGB(0, 0, 0));
		pDC->SetBkColor(RGB(210, 227, 199));
		break;
	case(IDC_RESULT11) :
		pDC->SetTextColor(RGB(0, 0, 0));
		pDC->SetBkColor(RGB(210, 227, 199));
		break;
	case(IDC_RESULT12) :
		pDC->SetTextColor(RGB(0, 0, 0));
		pDC->SetBkColor(RGB(210, 227, 199));
		break;
	case(IDC_RESULT13) :
		pDC->SetTextColor(RGB(0, 0, 0));
		pDC->SetBkColor(RGB(210, 227, 199));
		break;
	case(IDC_RESULT14) :
		pDC->SetTextColor(RGB(0, 0, 0));
		pDC->SetBkColor(RGB(210, 227, 199));
		break;
	case(IDC_RESULT15) :
		pDC->SetTextColor(RGB(0, 0, 0));
		pDC->SetBkColor(RGB(210, 227, 199));
		break;
	case(IDC_RESULT16) :
		pDC->SetTextColor(RGB(0, 0, 0));
		pDC->SetBkColor(RGB(210, 227, 199));
		break;

	}
	return hbr;

	/*
	可以根据不同的控件类型返回不同的画刷，来实现不同控件颜色的设置
	CTLCOLOR_BTN                按钮控件
	CTLCOLOR_DLG                对话框
	CTLCOLOR_EDIT               编辑框
	CTLCOLOR_LISTBOX            列表控件
	CTLCOLOR_MSGBOX             消息控件
	CTLCOLOR_SCROLLBAR          滚动条控件
	CTLCOLOR_STATIC             静态控件
	*/
}



void IMEIWrite_MulAT::OnCbnSelchangeCombo2_Fresh()
{
	//int Date_Count;//时间点计数
	//Date_Count=m_Port.GetCount();
	UpdateData(TRUE);
	for (int del = 0; del<64; del++)
	{
		m_Port1.DeleteString(0);//
		m_Port2.DeleteString(0);//
		m_Port3.DeleteString(0);//
		m_Port4.DeleteString(0);//
		m_Port5.DeleteString(0);//
		m_Port6.DeleteString(0);//

		m_Baud1.DeleteString(0);//
		m_Baud2.DeleteString(0);//
		m_Baud3.DeleteString(0);//
		m_Baud4.DeleteString(0);//
		m_Baud5.DeleteString(0);//
		m_Baud6.DeleteString(0);//

		m_Port7.DeleteString(0);
		m_Port8.DeleteString(0);
		m_Port9.DeleteString(0);

		m_Port10.DeleteString(0);
		m_Port11.DeleteString(0);
		m_Port12.DeleteString(0);
		m_Port13.DeleteString(0);
		m_Port14.DeleteString(0);
		m_Port15.DeleteString(0);
		m_Port16.DeleteString(0);

		m_Baud7.DeleteString(0);
		m_Baud8.DeleteString(0);
		m_Baud9.DeleteString(0);

		m_Baud10.DeleteString(0);
		m_Baud11.DeleteString(0);
		m_Baud12.DeleteString(0);
		m_Baud13.DeleteString(0);
		m_Baud14.DeleteString(0);
		m_Baud15.DeleteString(0);
		m_Baud16.DeleteString(0);

		CPUControlPort.DeleteString(0);
		CPUControlBaud.DeleteString(0);

		VIControlPort.DeleteString(0);
		VIControlBaud.DeleteString(0);

		ScanGunPort1.DeleteString(0);//
		ScanGunPort2.DeleteString(0);//
		ScanGunPort3.DeleteString(0);//
		ScanGunPort4.DeleteString(0);//
		ScanGunPort5.DeleteString(0);//
		ScanGunPort6.DeleteString(0);//

		ScanGunBaud1.DeleteString(0);//
		ScanGunBaud2.DeleteString(0);//
		ScanGunBaud3.DeleteString(0);//
		ScanGunBaud4.DeleteString(0);//
		ScanGunBaud5.DeleteString(0);//
		ScanGunBaud6.DeleteString(0);//


	}
	COM_IniDone = FALSE;
	for (int j = 0; j<PORTS_NUM; j++)
	{
		COM_Number[j] = "";
	}
	InitCOM(&m_Port1, &m_Baud1, 0);//初始化串口
	InitCOM(&m_Port2, &m_Baud2, 1);//初始化串口--LBC
	InitCOM(&m_Port3, &m_Baud3, 2);
	InitCOM(&m_Port4, &m_Baud4, 3);
	InitCOM(&m_Port5, &m_Baud5, 4);
	InitCOM(&m_Port6, &m_Baud6, 5);

	InitCOM(&m_Port7, &m_Baud7, 6);
	InitCOM(&m_Port8, &m_Baud8, 7);
	InitCOM(&m_Port9, &m_Baud9, 8);
	InitCOM(&m_Port10, &m_Baud10, 9);
	InitCOM(&m_Port11, &m_Baud11, 10);
	InitCOM(&m_Port12, &m_Baud12, 11);
	InitCOM(&m_Port13, &m_Baud13, 12);
	InitCOM(&m_Port14, &m_Baud14, 13);
	InitCOM(&m_Port15, &m_Baud15, 14);
	InitCOM(&m_Port16, &m_Baud16, 15);

	InitCOM(&CPUControlPort, &CPUControlBaud, 16);
	InitCOM(&VIControlPort, &VIControlBaud, 17);

	InitCOM(&ScanGunPort1, &ScanGunBaud1, 18);//初始化扫描枪串口
	InitCOM(&ScanGunPort2, &ScanGunBaud2, 1);
	InitCOM(&ScanGunPort3, &ScanGunBaud3, 2);
	InitCOM(&ScanGunPort4, &ScanGunBaud4, 3);
	InitCOM(&ScanGunPort5, &ScanGunBaud5, 4);
	InitCOM(&ScanGunPort6, &ScanGunBaud6, 5);
	AfxMessageBox("刷新端口成功！");
	UpdateData(FALSE);
}



void IMEIWrite_MulAT::OnContextMenu(CWnd*, CPoint point)//菜单
{
	if (point.x == -1 && point.y == -1)
	{
		//keystroke invocation
		CRect rect;
		GetClientRect(rect);
		ClientToScreen(rect);

		point = rect.TopLeft();
		point.Offset(5, 5);
	}

	CMenu menu;
	VERIFY(menu.LoadMenu(IDR_MENU1));

	CMenu* pPopup = menu.GetSubMenu(0);

	if (EnableMenu)
		pPopup->EnableMenuItem(ID_FRESH_32771, MF_ENABLED);
	else
		pPopup->EnableMenuItem(ID_FRESH_32771, MF_DISABLED | MF_GRAYED);


	ASSERT(pPopup != NULL);
	CWnd* pWndPopupOwner = this;

	while (pWndPopupOwner->GetStyle() & WS_CHILD)
		pWndPopupOwner = pWndPopupOwner->GetParent();

	pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON, point.x, point.y,
		pWndPopupOwner);


}
void IMEIWrite_MulAT::OnRButtonUp(UINT nFlags, CPoint point) //右键弹起
{
	// TODO: Add your message handler code here and/or call default

	CDialog::OnRButtonUp(nFlags, point);
}

//关机必须正常关机，截获关机，判断是否修改了时间。 没有正常关机会有一个标志位，重启也需要管理账号
LRESULT IMEIWrite_MulAT::DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam)  //截获关机信息
{
	// TODO: Add your specialized code here and/or call the base class 
	switch (message)
	{
	case WM_CLOSE:
		if ((Thread_State[0] == TRUE) || (Thread_State[1] == TRUE) || (Thread_State[2] == TRUE) || (Thread_State[3] == TRUE) || (Thread_State[4] == TRUE) || (Thread_State[5] == TRUE) || \
			(Thread_State[6] == TRUE) || (Thread_State[7] == TRUE) || (Thread_State[8] == TRUE) || (Thread_State[9] == TRUE) || (Thread_State[10] == TRUE) || (Thread_State[11] == TRUE) || \
			(Thread_State[12] == TRUE) || (Thread_State[13] == TRUE) || (Thread_State[14] == TRUE) || (Thread_State[15] == TRUE))
		{
			AfxMessageBox("请先等待线程结束！");
			return TRUE;
		}
		else
		{
			OnBnClickedButton25();
			OnBnClickedButtondisconncpu();
			OnOK();
		}

		break;
	}
	return CDialog::DefWindowProc(message, wParam, lParam);
}

BOOL IMEIWrite_MulAT::CheckConnect_Thread(CComboBox* m_Port, CComboBox* m_Baud, int HandleNum, CEdit* m_Result, CEdit* Final_Result_Control)
{
	CString sPort;
	m_Port->GetWindowText(sPort);

	CString PortType_CS;
	PortType.GetWindowTextA(PortType_CS);

	if (sPort == "")
		return FALSE;
	//return TRUE;

	//static char* Serial_Order_Return;
	BOOL Return;
	int Port_OK_Try = 0;



	for (int num = 0; num<5; num++)
	{
		if (OnGetport() == TRUE)
			break;
		else
		{
			//LogShow_exchange(m_Result,Final_Result_Control,5,"设备端口已断开",HandleNum);
			return FALSE;
		}
		Sleep(150);
	}
	BOOL Port_OK = FALSE;

	for (int num1 = 0; num1<PORTS_NUM; num1++)
	{
		if ((COM_Number[num1] == sPort) && (sPort != ""))
		{
			Port_OK = TRUE;
			break;
		}
	}
	if (Port_OK == FALSE)
	{
		Port_OK_Try++;
		//LogShow_exchange(m_Result,Final_Result_Control,5,"设备端口已断开",HandleNum);
		return FALSE;
	}
	else
	{

	}
	Sleep(100);


	if (PortType_CS == "终端跳动端口")
		return TRUE;

	if (OPen_Serial_Port(m_Port, m_Baud, HandleNum) == TRUE)//CheckConnect_Thread
	{
		//打开串口成功
	}
	else
	{
		//打开串口失败
		//LogShow_exchange(m_Result,Final_Result_Control,5,"设备已断开连接，可以测试下一台",HandleNum);
		return FALSE;
	}
	if (hPort[HandleNum] != NULL)
		Return = CloseHandle(hPort[HandleNum]);								//测试成功后
	hPort[HandleNum] = NULL;
	if (Return == TRUE)
	{

	}
	else
	{
		LogShow_exchange(m_Result, Final_Result_Control, 5, "关闭该串口失败，该端口异常！！请重启测试工具！！！！", HandleNum);
		return FALSE;													//关闭串口失败，程序将异常
	}
	return TRUE;
}

BOOL IMEIWrite_MulAT::OPen_Serial_Port(CComboBox* m_Port, CComboBox* m_Baud, int HandleNum, BOOL CPUChoose)		//打开串口
{
	CString sPort, sBaud;
	int port, baud;
	//get port
	m_Port->GetWindowText(sPort);
	if (sPort == "")
	{
		AfxMessageBox("请先选择端口！");
		return FALSE;
	}

	sPort = sPort.Right(sPort.GetLength() - 3);
	port = atoi(sPort);
	sPort.Format(_T("\\\\.\\COM%d"), port);

	//get baud
	m_Baud->GetWindowText(sBaud);
	baud = atoi(sBaud);

	if (hPort[HandleNum] != NULL)
	{
		CloseHandle(hPort[HandleNum]);
		hPort[HandleNum] = NULL;
	}
	//open com port打开端口
	hPort[HandleNum] = CreateFile(sPort, GENERIC_READ | GENERIC_WRITE, 0, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hPort[HandleNum] == INVALID_HANDLE_VALUE)
	{
		CString csTemp;
		m_Port->GetWindowText(csTemp);
		//AfxMessageBox("Can't open "+csTemp);
		hPort[HandleNum] = NULL;
		return FALSE;
	}
	//LogShow_exchange(m_Result,Final_Result_Control,0,"成功创建串口句柄\r\n",HandleNum);

	//config the com port
	DCB dcb;
	dcb.DCBlength = sizeof(DCB);
	GetCommState(hPort[HandleNum], &dcb);
	SetupComm(hPort[HandleNum], PICDATAMAX, 4096);
	PurgeComm(hPort[HandleNum], PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);
	dcb.Parity = NOPARITY;
	dcb.ByteSize = 8;
	dcb.StopBits = ONESTOPBIT;
	dcb.BaudRate = baud;													//57600(MT6205B), 115200 (MT6218B)			
	dcb.fBinary = TRUE;
	dcb.fParity = FALSE;
	dcb.fOutxCtsFlow = FALSE;
	dcb.fOutxDsrFlow = FALSE;
	dcb.fDtrControl = DTR_CONTROL_DISABLE;
	dcb.fDsrSensitivity = FALSE;
	dcb.fTXContinueOnXoff = FALSE;
	dcb.fOutX = FALSE;
	dcb.fInX = FALSE;
	dcb.fErrorChar = FALSE;
	dcb.fNull = FALSE;
	dcb.fRtsControl = RTS_CONTROL_ENABLE;
	dcb.fAbortOnError = FALSE;
	dcb.XonChar = 0;
	dcb.XoffChar = 0;
	dcb.ErrorChar = 0;
	dcb.EofChar = 0;
	dcb.EvtChar = 0;
	SetCommState(hPort[HandleNum], &dcb);

	//set time out struct
	COMMTIMEOUTS timeouts;
	timeouts.ReadIntervalTimeout = 0xFFFFFFFF;
	//timeouts.ReadTotalTimeoutMultiplier = 0;
	//timeouts.ReadTotalTimeoutConstant = 0;
	/*
	ReadTotalTimeoutConstant：一次读取串口数据的固定超时。
	所以在一次读取串口的操作中，其超时为ReadTotalTimeoutMultiplier乘以读取的字节数再加上 ReadTotalTimeoutConstant。
	将ReadIntervalTimeout设置为MAXDWORD，并将ReadTotalTimeoutMultiplier 和ReadTotalTimeoutConstant设置为0，表示读取操作将立即返回存放在输入缓冲区的字符。
	*/
	timeouts.ReadTotalTimeoutMultiplier = 0;
	timeouts.ReadTotalTimeoutConstant = 10;//OPen_Serial_Port
	//timeouts.ReadTotalTimeoutConstant = 500;
	timeouts.WriteTotalTimeoutMultiplier = 0;
	timeouts.WriteTotalTimeoutConstant = 2000;
	SetCommTimeouts(hPort[HandleNum], &timeouts);

	//如果是蓝牙串口获取配置的时候，到这步就直接返回
	if (BLEGetSettingFlag == 0 || BLEGetSettingFlag == 1)
	{
		return TRUE;
	}

	//SetCommMask(hPort[HandleNum], EV_RXCHAR|EV_TXEMPTY );							//设置事件驱动的类型

	//read and write the com port
	BOOL bReadStatus, bWriteStat;
	DWORD dwBytesWritten, dwBytesRead;
	char *buffer;
	char *p = NULL;
	CString command;
	CString ReturnOK;
	char buf[1024];
	int timeout = 1;
	int i;


	//write "AT"
	for (i = 0; i<3; i++)
	{
		command.Empty();
		if (CPUChoose == FALSE)
		{
			if (paraArray[3].Low_Limit_Value != "")
			{
				int pos = paraArray[3].Low_Limit_Value.Find("**");
				if (pos >= 0)
				{
					command = paraArray[3].Low_Limit_Value.Left(pos) + "\r\n";
					ReturnOK = paraArray[3].Low_Limit_Value.Mid(pos + 2);
				}
				else
				{
					command.Format("AT^GT_CM=TEST\r\n");
					ReturnOK = "TEST_OK";
				}
			}
			else
			{
				command.Format("AT^GT_CM=TEST\r\n");
				ReturnOK = "TEST_OK";
			}
		}
		else
		{
			//////
			CString CpuOrderTemp = "AT01a#^GT_CM=TEST";
			CpuOrderTemp = CpuOrderTemp.Mid(2);
			MD5 mb;
			mb.update(CpuOrderTemp, strlen(CpuOrderTemp));
			CString CpuOrderMD5 = (mb.toString()).c_str();

			command = "AT" + CpuOrderTemp + CpuOrderMD5.Left(2) + "\r\n";
			ReturnOK = "TEST_OK";
			/////
		}


		if (ReturnOK == "")
			ReturnOK = "\r";
		buffer = command.GetBuffer(command.GetLength());
		bWriteStat = WriteFile(hPort[HandleNum], buffer, command.GetLength(), &dwBytesWritten, NULL);
		if (dwBytesWritten != command.GetLength())
		{
			Sleep(500);
			continue;
		}

		CString WaitTimeCS;
		int wpos = paraArray[3].showName.Find("DL");
		if (wpos != -1)
			WaitTimeCS = paraArray[3].showName.Mid(wpos + 2);
		else
			WaitTimeCS = "0";
		//receive response
		Sleep(300 + atoi(WaitTimeCS));
		memset(buf, 0, sizeof(buf));
		buffer = buf;
		bReadStatus = ReadFile(hPort[HandleNum], buffer, 256, &dwBytesRead, NULL);
		if (dwBytesRead != 0)
		{
			//p=strstr(buffer,"\r");
			//p=strstr(buffer,"TEST_OK");
			p = strstr(buffer, (LPSTR)(LPCTSTR)ReturnOK);
			if (p)
			{
				timeout = 0;
				break;
			}
		}
		Sleep(100);
	}

	if (timeout == 1)													//check if failed
	{
		if (hPort[HandleNum] != NULL)
			CloseHandle(hPort[HandleNum]);									//打开串口时，接收失败
		hPort[HandleNum] = NULL;
		return FALSE;
	}
	//make sure rx data cleaned
	Sleep(100);
	bReadStatus = ReadFile(hPort[HandleNum], buffer, 256, &dwBytesRead, NULL);


	return TRUE;														//返回TRUE，打开串口正常
}
BOOL IMEIWrite_MulAT::OPen_Serial_PortReadConstant(CComboBox* m_Port, CComboBox* m_Baud, int HandleNum, BOOL CPUChoose)		//打开串口
{
	CString sPort, sBaud;
	int port, baud;
	//get port
	m_Port->GetWindowText(sPort);
	if (sPort == "")
	{
		AfxMessageBox("请先选择端口！");
		return FALSE;
	}

	sPort = sPort.Right(sPort.GetLength() - 3);
	port = atoi(sPort);
	sPort.Format(_T("\\\\.\\COM%d"), port);

	//get baud
	m_Baud->GetWindowText(sBaud);
	baud = atoi(sBaud);

	//open com port打开端口
	hPort[HandleNum] = CreateFile(sPort, GENERIC_READ | GENERIC_WRITE, 0, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hPort[HandleNum] == INVALID_HANDLE_VALUE)
	{
		CString csTemp;
		m_Port->GetWindowText(csTemp);
		//AfxMessageBox("Can't open "+csTemp);
		hPort[HandleNum] = NULL;
		return FALSE;
	}
	//LogShow_exchange(m_Result,Final_Result_Control,0,"成功创建串口句柄\r\n",HandleNum);

	//config the com port
	DCB dcb;
	dcb.DCBlength = sizeof(DCB);
	GetCommState(hPort[HandleNum], &dcb);
	SetupComm(hPort[HandleNum], 4096, 4096);
	PurgeComm(hPort[HandleNum], PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);
	dcb.Parity = NOPARITY;
	dcb.ByteSize = 8;
	dcb.StopBits = ONESTOPBIT;
	dcb.BaudRate = baud;													//57600(MT6205B), 115200 (MT6218B)			
	dcb.fBinary = TRUE;
	dcb.fParity = FALSE;
	dcb.fOutxCtsFlow = FALSE;
	dcb.fOutxDsrFlow = FALSE;
	dcb.fDtrControl = DTR_CONTROL_DISABLE;
	dcb.fDsrSensitivity = FALSE;
	dcb.fTXContinueOnXoff = FALSE;
	dcb.fOutX = FALSE;
	dcb.fInX = FALSE;
	dcb.fErrorChar = FALSE;
	dcb.fNull = FALSE;
	dcb.fRtsControl = RTS_CONTROL_ENABLE;
	dcb.fAbortOnError = FALSE;
	dcb.XonChar = 0;
	dcb.XoffChar = 0;
	dcb.ErrorChar = 0;
	dcb.EofChar = 0;
	dcb.EvtChar = 0;
	SetCommState(hPort[HandleNum], &dcb);

	//set time out struct
	COMMTIMEOUTS timeouts;
	timeouts.ReadIntervalTimeout = 0xFFFFFFFF;
	//timeouts.ReadTotalTimeoutMultiplier = 0;
	//timeouts.ReadTotalTimeoutConstant = 0;
	/*
	ReadTotalTimeoutConstant：一次读取串口数据的固定超时。
	所以在一次读取串口的操作中，其超时为ReadTotalTimeoutMultiplier乘以读取的字节数再加上 ReadTotalTimeoutConstant。
	将ReadIntervalTimeout设置为MAXDWORD，并将ReadTotalTimeoutMultiplier 和ReadTotalTimeoutConstant设置为0，表示读取操作将立即返回存放在输入缓冲区的字符。
	*/
	timeouts.ReadTotalTimeoutMultiplier = 0;
	timeouts.ReadTotalTimeoutConstant = 300;//OPen_Serial_PortReadConstant
	timeouts.WriteTotalTimeoutMultiplier = 0;
	timeouts.WriteTotalTimeoutConstant = 2000;
	SetCommTimeouts(hPort[HandleNum], &timeouts);

	//SetCommMask(hPort[HandleNum], EV_RXCHAR|EV_TXEMPTY );							//设置事件驱动的类型

	//read and write the com port
	BOOL bReadStatus, bWriteStat;
	DWORD dwBytesWritten, dwBytesRead;
	char *buffer;
	char *p = NULL;
	CString command;
	CString ReturnOK;
	char buf[1024];
	int timeout = 1;
	int i;


	//write "AT"
	for (i = 0; i<3; i++)
	{
		command.Empty();
		if (CPUChoose == FALSE)
		{
			if (paraArray[3].Low_Limit_Value != "")
			{
				int pos = paraArray[3].Low_Limit_Value.Find("**");
				if (pos >= 0)
				{
					command = paraArray[3].Low_Limit_Value.Left(pos) + "\r\n";
					ReturnOK = paraArray[3].Low_Limit_Value.Mid(pos + 2);
				}
				else
				{
					command.Format("AT^GT_CM=TEST\r\n");
					ReturnOK = "TEST_OK";
				}
			}
			else
			{
				command.Format("AT^GT_CM=TEST\r\n");
				ReturnOK = "TEST_OK";
			}
		}
		else
		{
			//////
			CString CpuOrderTemp = "AT01a#^GT_CM=TEST";
			CpuOrderTemp = CpuOrderTemp.Mid(2);
			MD5 mb;
			mb.update(CpuOrderTemp, strlen(CpuOrderTemp));
			CString CpuOrderMD5 = (mb.toString()).c_str();

			command = "AT" + CpuOrderTemp + CpuOrderMD5.Left(2) + "\r\n";
			ReturnOK = "TEST_OK";
			/////
		}


		if (ReturnOK == "")
			ReturnOK = "\r";
		buffer = command.GetBuffer(command.GetLength());
		bWriteStat = WriteFile(hPort[HandleNum], buffer, command.GetLength(), &dwBytesWritten, NULL);
		if (dwBytesWritten != command.GetLength())
		{
			Sleep(500);
			continue;
		}

		CString WaitTimeCS;
		int wpos = paraArray[3].showName.Find("DL");
		if (wpos != -1)
			WaitTimeCS = paraArray[3].showName.Mid(wpos + 2);
		else
			WaitTimeCS = "0";
		//receive response
		Sleep(300 + atoi(WaitTimeCS));
		memset(buf, 0, sizeof(buf));
		buffer = buf;
		bReadStatus = ReadFile(hPort[HandleNum], buffer, 256, &dwBytesRead, NULL);
		if (dwBytesRead != 0)
		{
			//p=strstr(buffer,"\r");
			//p=strstr(buffer,"TEST_OK");
			p = strstr(buffer, (LPSTR)(LPCTSTR)ReturnOK);
			if (p)
			{
				timeout = 0;
				break;
			}
		}
		Sleep(100);
	}

	if (timeout == 1)													//check if failed
	{
		if (hPort[HandleNum] != NULL)
			CloseHandle(hPort[HandleNum]);									//打开串口时，接收失败
		hPort[HandleNum] = NULL;
		return FALSE;
	}
	//make sure rx data cleaned
	Sleep(100);
	bReadStatus = ReadFile(hPort[HandleNum], buffer, 256, &dwBytesRead, NULL);


	return TRUE;														//返回TRUE，打开串口正常
}
char*  IMEIWrite_MulAT::Send_Serial_Order(CString* Vaule_Return, CString strCommand_Vaule, int HandleNum, char* EndSign, char* StartSign, int WaitTime)//通过串口发送命令
{
	int Vaule_Return_Count = -1;															//参数的个数
	BOOL bReadStatus, bWriteStat;
	DWORD dwBytesWritten, dwBytesRead;
	char *buffer;
	char *p = NULL;
	CString command;
	char buf[1024];//char buf[PICDATAMAX];

	CString strCommand_Vaule_Return;													//串口返回值

	if (strCommand_Vaule == "IDLE")														//处理延时
	{
		*Vaule_Return = "DELAY_SUCCESS";
		return "DELAY_SUCCESS";

	}
	Sleep(10);
	for (int i = 0; i<1; i++)//多次发送
	{
		//send command
		command.Empty();
		command = strCommand_Vaule + "\r\n";//拼接命令

		buffer = command.GetBuffer(command.GetLength());
		bWriteStat = WriteFile(hPort[HandleNum], buffer, command.GetLength(), &dwBytesWritten, NULL);
		if (dwBytesWritten != command.GetLength())
		{
			Sleep(300);
			continue;
		}

		//receive response
		Sleep(atoi(paraArray[1].Low_Limit_Value) + WaitTime);
		CString Port_Temp = "";//串口读取数据 的缓存HexGroupToString
		for (int r = 0; r<6; r++)
		{
			memset(buf, 0, sizeof(buf));
			buffer = buf;
			bReadStatus = ReadFile(hPort[HandleNum], buffer, 512, &dwBytesRead, NULL);//bReadStatus = ReadFile( hPort[HandleNum], buffer, PICDATAMAX, &dwBytesRead, NULL);//Send_Serial_Order
			if (dwBytesRead != 0)
			{
				if (strstr(EndSign, "NULL"))
					p = strstr(buffer, "\r");
				else
					p = strstr(buffer, EndSign);

				strCommand_Vaule_Return = Port_Temp + CString(buffer);
				if ((p) || (strCommand_Vaule_Return.Find(EndSign) != -1))//if((p)||(Port_Temp.Find(EndSign)!=-1))
				{
					CString  selPort;
					selPort = strCommand_Vaule_Return;

					Vaule_Return_Count_CS[HandleNum] = selPort;

					*Vaule_Return = "Analysis_SUCCESS";
					return "Analysis_SUCCESS";
				}
				else
				{
					Port_Temp += CString(buffer);
					Sleep(200 * r);
					continue;
				}
			}
			else
			{
				Sleep(200 * r);
				continue;
			}
		}

	}
	Vaule_Return_Count_CS[HandleNum] = strCommand_Vaule_Return;	//串口返回值
	//CloseHandle(hPort[HandleNum]);							//成功后不关闭串口---发送完AT指令后
	*Vaule_Return = "FAIL";
	return "FAIL";
}


void IMEIWrite_MulAT::LogShow_exchange(CEdit* m_Result, CEdit* Final_Result_Control, int State, CString Msg_Log, int HandleNum, CString Category, CString ChipRfIDbg)
{
	CString ChipRfIDCurrent;
	if (ChipRfIDbg != "")
	{
		ChipRfIDCurrent = ChipRfIDbg;
	}
	else
	{
		ChipRfIDCurrent = ChipRfID[HandleNum];
	}
	CTime showtime = CTime::GetCurrentTime();

	CString showtimeCS;
	showtimeCS.Format("%02d:%02d:%02d", showtime.GetHour(), showtime.GetMinute(), showtime.GetSecond());

	CString LOG_Show_Old;
	m_Result->GetWindowText(LOG_Show_Old);
	m_Result->SetWindowText(Msg_Log + showtimeCS + "\r\n" + LOG_Show_Old.Left(8092));
	m_Result->UpdateWindow();

	CString HandleNumCS;
	HandleNumCS.Format("%02d", HandleNum + 1);
	if (State == 0)
	{
		if (ChipRfIDbg != "")
			gColor[HandleNum + 8] = RGB(0, 155, 100);
		else
			gColor[HandleNum] = RGB(0, 155, 100);
		Final_Result_Control->SetWindowTextA("运行" + HandleNumCS);
		//三合一传递信息
		if (g_ADCTFlag == TRUE)
		{
			SendADCTHint(get_map_key_value(PortStatusMap, HandleNum), "测试", 0);
			SendADCTHint(get_map_key_value(PortStatusMap, HandleNum), Msg_Log, 1);/*.Left(Msg_Log.GetLength()-8)*/
		}

		Final_Result_Control->UpdateWindow();
		Thread_State[HandleNum] = TRUE;
	}
	else if (State == 128)
	{
		Final_Result_Control->SetFont(font);

		if (ChipRfIDbg != "")
			gColor[HandleNum + 8] = RGB(255, 0, 0);
		else
			gColor[HandleNum] = RGB(255, 0, 0);
		Final_Result_Control->SetWindowTextA("失败" + HandleNumCS);
		//三合一传递信息
		if (g_ADCTFlag == TRUE)
		{
			SendADCTHint(get_map_key_value(PortStatusMap, HandleNum), "失败", 0);
			SendADCTHint(get_map_key_value(PortStatusMap, HandleNum), Msg_Log, 2);
		}


		Final_Result_Control->UpdateWindow();
		//Thread_State[HandleNum]=FALSE;
		if (Category != "-1")
			Data_UpdateError(IMEI_Setdlg.Mulmyado[HandleNum], HandleNum, m_Result, Final_Result_Control, Msg_Log, Category, ChipRfIDCurrent);

		BOOL FaillResult = CPUCommunication("失败", HandleNum, "", "", COM_State[THREAD_NUM], m_Result, Final_Result_Control);
		if (RecoverVoltage == TRUE)CPUCommunication("外电1开", HandleNum, "", "", COM_State[THREAD_NUM], m_Result, Final_Result_Control);

		FaillResult = CPUCommunication("关闭通路", HandleNum, "", "", COM_State[THREAD_NUM], m_Result, Final_Result_Control);

		CString VoiceMessage;
		VoiceMessage.Format("%s失败", HandleNumCS);
		Voice_Speak(VoiceMessage);

		if (ChipRfIDbg == "")
		{
			ChipRfID[HandleNum] = "";
			//Software_Version[HandleNum]="";//这个数据无需清理，因为软件版本是一样的
		}

		EnterCriticalSection(&SUCCFAILCOUNT);
		FailCount++;
		CString FailCountCS;
		if (BGConfirmChoose == TRUE)
			FailCountCS.Format("%d", FailCount / 2);
		else
			FailCountCS.Format("%d", FailCount);
		GetDlgItem(IDC_EDITFAILCOUNT)->SetWindowTextA(FailCountCS);
		LeaveCriticalSection(&SUCCFAILCOUNT);
	}
	else if (State == 255)
	{
		Final_Result_Control->SetFont(font);

		WriteLogIni(HandleNum, ChipRfIDCurrent);
		CString LogCS;
		LogCS = ChipRfIDCurrent + ";" + IMEIRecord[HandleNum] + ";" + BTMac[HandleNum] + ";" + SimNumber[HandleNum] + ";" + NteTemperature[HandleNum] + ";" + GPSSnrAve[HandleNum] + ";" + InchargeI[HandleNum] + ";" + BTTestResult[HandleNum] + ";";
		WriteLog(LogCS + "成功！", HandleNum);
		WriteLogEnd(HandleNum);
		if (ChipRfIDbg != "")
			gColor[HandleNum + 8] = RGB(0, 255, 0);
		else
			gColor[HandleNum] = RGB(0, 255, 0);

		//Thread_State[HandleNum]=FALSE;
		BOOL SuccessResult = CPUCommunication("成功", HandleNum, "", "", COM_State[THREAD_NUM], m_Result, Final_Result_Control);
		if (RecoverVoltage == TRUE)CPUCommunication("外电1开", HandleNum, "", "", COM_State[THREAD_NUM], m_Result, Final_Result_Control);
		SuccessResult &= CPUCommunication("关闭通路", HandleNum, "", "", COM_State[THREAD_NUM], m_Result, Final_Result_Control);

		Final_Result_Control->SetWindowTextA("成功" + HandleNumCS);

		//三合一传递信息
		if (g_ADCTFlag == TRUE)
		{
			SendADCTHint(get_map_key_value(PortStatusMap, HandleNum), "成功", 0);
		}

		Final_Result_Control->UpdateWindow();
		if (SuccessResult == FALSE)
		{
			if (ChipRfIDbg != "")
				gColor[HandleNum + 8] = RGB(255, 0, 0);
			else
				gColor[HandleNum] = RGB(255, 0, 0);
			Final_Result_Control->SetWindowTextA("失败" + HandleNumCS);
			Final_Result_Control->UpdateWindow();
			//三合一传递信息
			if (g_ADCTFlag == TRUE)
			{
				SendADCTHint(get_map_key_value(PortStatusMap, HandleNum), "失败", 0);
			}

			CString VoiceMessage;
			VoiceMessage.Format("%s失败", HandleNumCS);
			Voice_Speak(VoiceMessage);

			EnterCriticalSection(&SUCCFAILCOUNT);
			FailCount++;
			CString FailCountCS;
			if (BGConfirmChoose == TRUE)
				FailCountCS.Format("%d", FailCount / 2);
			else
				FailCountCS.Format("%d", FailCount);
			GetDlgItem(IDC_EDITFAILCOUNT)->SetWindowTextA(FailCountCS);
			LeaveCriticalSection(&SUCCFAILCOUNT);
		}
		else
		{
			CString VoiceMessage;
			VoiceMessage.Format("%s成功", HandleNumCS);
			Voice_Speak(VoiceMessage);

			EnterCriticalSection(&SUCCFAILCOUNT);
			SuccessCount++;
			CString SuccessCountCS;
			if (BGConfirmChoose == TRUE)
				SuccessCountCS.Format("%d", SuccessCount / 2);
			else
				SuccessCountCS.Format("%d", SuccessCount);
			GetDlgItem(IDC_EDITSUCCOUNT)->SetWindowTextA(SuccessCountCS);
			LeaveCriticalSection(&SUCCFAILCOUNT);
		}
		if (ChipRfIDbg == "")
		{
			ChipRfID[HandleNum] = "";
			//Software_Version[HandleNum]="";//这个数据无需清理，因为软件版本是一样的
		}
	}
	else if (State == 2550)
	{
		Final_Result_Control->SetFont(font);

		if (ChipRfIDbg != "")
			gColor[HandleNum + 8] = RGB(0, 255, 0);
		else
			gColor[HandleNum] = RGB(0, 255, 0);

		//Thread_State[HandleNum]=FALSE;
		BOOL SuccessResult = CPUCommunication("成功", HandleNum, "", "", COM_State[THREAD_NUM], m_Result, Final_Result_Control);
		if (RecoverVoltage == TRUE)CPUCommunication("外电1开", HandleNum, "", "", COM_State[THREAD_NUM], m_Result, Final_Result_Control);
		SuccessResult &= CPUCommunication("关闭通路", HandleNum, "", "", COM_State[THREAD_NUM], m_Result, Final_Result_Control);

		Final_Result_Control->SetWindowTextA("");

		Final_Result_Control->UpdateWindow();
		if (SuccessResult == FALSE)
		{
			if (ChipRfIDbg != "")
				gColor[HandleNum + 8] = RGB(255, 0, 0);
			else
				gColor[HandleNum] = RGB(255, 0, 0);
			Final_Result_Control->SetWindowTextA("失败" + HandleNumCS);
			//三合一传递信息
			if (g_ADCTFlag == TRUE)
			{
				SendADCTHint(get_map_key_value(PortStatusMap, HandleNum), "失败", 0);
			}
			Final_Result_Control->UpdateWindow();
			CString VoiceMessage;
			VoiceMessage.Format("%s失败", HandleNumCS);
			Voice_Speak(VoiceMessage);

			EnterCriticalSection(&SUCCFAILCOUNT);
			FailCount++;
			CString FailCountCS;
			if (BGConfirmChoose == TRUE)
				FailCountCS.Format("%d", FailCount / 2);
			else
				FailCountCS.Format("%d", FailCount);
			GetDlgItem(IDC_EDITFAILCOUNT)->SetWindowTextA(FailCountCS);
			LeaveCriticalSection(&SUCCFAILCOUNT);
		}
		else
		{
			CString VoiceMessage;
			VoiceMessage.Format("%s已成功", HandleNumCS);
			Voice_Speak(VoiceMessage);

			EnterCriticalSection(&SUCCFAILCOUNT);
			SuccessCount++;
			CString SuccessCountCS;
			if (BGConfirmChoose == TRUE)
				SuccessCountCS.Format("%d", SuccessCount / 2);
			else
				SuccessCountCS.Format("%d", SuccessCount);
			GetDlgItem(IDC_EDITSUCCOUNT)->SetWindowTextA(SuccessCountCS);
			LeaveCriticalSection(&SUCCFAILCOUNT);
		}
		if (ChipRfIDbg == "")
		{
			ChipRfID[HandleNum] = "";
			//Software_Version[HandleNum]="";//这个数据无需清理，因为软件版本是一样的
		}
	}
	else if (State == 250)
	{
		if (ChipRfIDbg != "")
			gColor[HandleNum + 8] = RGB(60, 190, 190);
		else
			gColor[HandleNum] = RGB(60, 190, 190);
		Final_Result_Control->SetWindowTextA("W");
		Final_Result_Control->UpdateWindow();
		Thread_State[HandleNum] = FALSE;
		if (COM_State[HandleNum] == FALSE)
			EnableWindow_Start(HandleNum);
	}
	else if (State == 251)
	{
		if (ChipRfIDbg != "")
			gColor[HandleNum + 8] = RGB(60, 190, 190);
		else
			gColor[HandleNum] = RGB(60, 190, 190);
		Final_Result_Control->SetWindowTextA("W");
		Final_Result_Control->UpdateWindow();
		Thread_State[HandleNum] = FALSE;
		//EnableWindow_Start(HandleNum);
	}
	else if (State == 5)//继续测试，状态不更新
	{
		Thread_State[HandleNum] = TRUE;
		if (g_ADCTFlag == TRUE)
		{
			SendADCTHint(get_map_key_value(PortStatusMap, HandleNum), Msg_Log, 1);
		}
	}
	else if (State == 6)//仅更新LOG信息，其他不变
	{

	}
	else if (State == 256)
	{
		Thread_State[HandleNum] = FALSE;
		Final_Result_Control->SetWindowTextA("W");
		Final_Result_Control->UpdateWindow();
		SetFocus_Choose(HandleNum);
	}
	else
	{
		if (ChipRfIDbg != "")
			gColor[HandleNum + 8] = RGB(0, 155, 100);
		else
			gColor[HandleNum] = RGB(0, 155, 100);
		Final_Result_Control->SetWindowTextA("运行" + HandleNumCS);
		//三合一传递信息
		if (g_ADCTFlag == TRUE)
		{
			SendADCTHint(get_map_key_value(PortStatusMap, HandleNum), "测试", 0);
		}
		Final_Result_Control->UpdateWindow();
		Thread_State[HandleNum] = TRUE;
	}

}
CString IMEIWrite_MulAT::GetData(char* Serial_Order_Return, CString Start, CString End, int Count, int HandleNum)
{
	CString DataGet;
	//CString DataGetTemp;
	int pos = -1;
	CString Serial_Order_Return_CS;
	Serial_Order_Return_CS.Format("%s", Serial_Order_Return);
	for (int i = 0; i<Count; i++)
	{
		if (i == Count - 1)
		{
			if ((Start == "NULL") || (Start == ""))
			{
				pos = Serial_Order_Return_CS.Find(End);
				if (pos >= 0)
				{
					DataGet = Serial_Order_Return_CS.Left(pos);
				}
				else
				{
					return Serial_Order_Return_CS;
				}
			}
			else
			{
				pos = Serial_Order_Return_CS.Find(Start);
				if (pos >= 0)
				{
					Serial_Order_Return_CS = Serial_Order_Return_CS.Mid(pos + Start.GetLength());
					pos = Serial_Order_Return_CS.Find(End);
					if (pos >= 0)
					{
						DataGet = Serial_Order_Return_CS.Left(pos);
					}
					else
					{
						return Serial_Order_Return_CS;//return "NULL";
					}
				}
				else
				{
					return "NULL";
				}
			}
		}
		else
		{
			if ((Start == "NULL") || (Start == ""))
			{
				continue;
			}
			else
			{
				pos = Serial_Order_Return_CS.Find(Start);
				if (pos >= 0)
				{
					Serial_Order_Return_CS = Serial_Order_Return_CS.Mid(pos + Start.GetLength());
					continue;
				}
				else
				{
					return "NULL";
				}
			}
		}

		return DataGet;
	}
	return "NULL";
}
bool IMEIWrite_MulAT::IMEI_Function_Judge(int i, CString IMEI_FT_Item, char* Serial_Order_Return, int HandleNum, CEdit* m_Result, CEdit* Final_Result_Control)
{
	int IMEI_FT_Item_Int = 0;
	CString Serial_Order_Return_CS;
	Serial_Order_Return_CS.Format("%s", Serial_Order_Return);
	int pos = -1;


	CString GpsID[GPSNUMBERMAX] = { "" };
	CString GpsSNR[GPSNUMBERMAX] = { "" };
	CString GpsSNRTemp[GPSNUMBERMAX] = { "" };
	CString GpsIDTemp[GPSNUMBERMAX] = { "" };
	int GpsNumber = -1;
	int GpsMessage = -1;
	int GpsMessageFirst = -1;
	CString GpsSNRStandardLow[GPSNUMBERMAX] = { "" };
	CString GpsSNRStandardHigh[GPSNUMBERMAX] = { "" };
	CString SNRSetting;
	//重启变量
	CString RestartCount;
	CString RestartCountLow;
	CString RestartCountHigh;
	//充电变量
	CString VBat;
	CString IBat;
	CString VCharge;

	//CString VBatSetting;
	//CString IBatSetting;
	//CString VChargeSetting;
	CString BBCSettingLow[3 * 2];
	CString BBCSettingHigh[3 * 2];
	CString BBCSetting;

	if (IMEI_FT_Item.Find("打开GPS测试") != -1)
	{
		IMEI_FT_Item_Int = 1;

		pos = Serial_Order_Return_CS.Find("GPGSV");
		if (pos >= 0)
		{
			Serial_Order_Return_CS = Serial_Order_Return_CS.Mid(pos + 5);
			GpsMessage = atoi(GetData((LPSTR)(LPCTSTR)Serial_Order_Return_CS, ",", ",", 1, HandleNum));//有效GPS数据条数 
			LogShow_exchange(m_Result, Final_Result_Control, 0, "解析GPS信息数量===" + GetData((LPSTR)(LPCTSTR)Serial_Order_Return_CS, ",", ",", 1, HandleNum), HandleNum);

			GpsMessageFirst = atoi(GetData((LPSTR)(LPCTSTR)Serial_Order_Return_CS, ",", ",", 2, HandleNum));//有效GPS数据条数 
			if (GpsMessageFirst != 1)
			{
				LogShow_exchange(m_Result, Final_Result_Control, 0, "解析GPS当前信息非第一条===" + GetData((LPSTR)(LPCTSTR)Serial_Order_Return_CS, ",", ",", 2, HandleNum), HandleNum);
				return false;
			}
			else
			{
				LogShow_exchange(m_Result, Final_Result_Control, 0, "解析GPS当前信息是第一条===" + GetData((LPSTR)(LPCTSTR)Serial_Order_Return_CS, ",", ",", 2, HandleNum) + " 开始解析", HandleNum);
			}

			GpsNumber = atoi(GetData((LPSTR)(LPCTSTR)Serial_Order_Return_CS, ",", ",", 3, HandleNum));//卫星数量
			LogShow_exchange(m_Result, Final_Result_Control, 0, "解析GPS数据:搜到卫星数量===" + GetData((LPSTR)(LPCTSTR)Serial_Order_Return_CS, ",", ",", 3, HandleNum), HandleNum);
			for (int j = 0; j<GpsNumber; j++)
			{
				if (GPSDataTypeChoose == TRUE)
				{
					if ((GpsMessage >= 2) && ((j >= 4) && (j <= 7)))//第2条信息
					{
						GpsID[j] = GetData((LPSTR)(LPCTSTR)Serial_Order_Return_CS, ",", ",", 4 + 4 * (j + 1), HandleNum);//4+===>每条语句，最后一颗星有4个逗号乘以1
						GpsSNR[j] = GetData((LPSTR)(LPCTSTR)Serial_Order_Return_CS, ",", ",", 4 + 4 * (j + 1) + 3, HandleNum);
					}
					else if ((GpsMessage >= 3) && ((j >= 8) && (j <= 11)))//第3条信息
					{
						GpsID[j] = GetData((LPSTR)(LPCTSTR)Serial_Order_Return_CS, ",", ",", 8 + 4 * (j + 1), HandleNum);//8+===>每条语句，最后一颗星有4个逗号乘以2
						GpsSNR[j] = GetData((LPSTR)(LPCTSTR)Serial_Order_Return_CS, ",", ",", 8 + 4 * (j + 1) + 3, HandleNum);
					}
					else if ((GpsMessage >= 4) && ((j >= 12) && (j <= 15)))//第4条信息
					{
						GpsID[j] = GetData((LPSTR)(LPCTSTR)Serial_Order_Return_CS, ",", ",", 12 + 4 * (j + 1), HandleNum);//12+===>每条语句，最后一颗星有4个逗号乘以3
						GpsSNR[j] = GetData((LPSTR)(LPCTSTR)Serial_Order_Return_CS, ",", ",", 12 + 4 * (j + 1) + 3, HandleNum);
					}
					else if ((GpsMessage >= 1) && (j <= 3))
					{
						GpsID[j] = GetData((LPSTR)(LPCTSTR)Serial_Order_Return_CS, ",", ",", 0 + 4 * (j + 1), HandleNum);//4+===>每条语句，最后一颗星有4个逗号乘以0
						GpsSNR[j] = GetData((LPSTR)(LPCTSTR)Serial_Order_Return_CS, ",", ",", 0 + 4 * (j + 1) + 3, HandleNum);
					}
				}
				else//正常GPS数据格式
				{
					if ((GpsMessage >= 2) && ((j >= 4) && (j <= 7)))//第2条信息
					{
						if ((GpsNumber <= 8) && (j == GpsNumber - 1))
						{
							GpsID[j] = GetData((LPSTR)(LPCTSTR)Serial_Order_Return_CS, ",", ",", 3 + 4 * (j + 1), HandleNum);//4+===>$GPGSV,2,2,7,				//3+===>每条语句，最后一颗星有3个逗号
							GpsSNR[j] = GetData((LPSTR)(LPCTSTR)Serial_Order_Return_CS, ",", "*", 3 + 4 * (j + 1) + 3, HandleNum);
						}
						else if ((GpsNumber>8) && (j == 7))
						{
							GpsID[j] = GetData((LPSTR)(LPCTSTR)Serial_Order_Return_CS, ",", ",", 3 + 4 * (j + 1), HandleNum);//4+===>$GPGSV,2,2,7,
							GpsSNR[j] = GetData((LPSTR)(LPCTSTR)Serial_Order_Return_CS, ",", "*", 3 + 4 * (j + 1) + 3, HandleNum);
						}
						else
						{
							GpsID[j] = GetData((LPSTR)(LPCTSTR)Serial_Order_Return_CS, ",", ",", 3 + 4 * (j + 1), HandleNum);//4+===>$GPGSV,2,2,7,
							GpsSNR[j] = GetData((LPSTR)(LPCTSTR)Serial_Order_Return_CS, ",", ",", 3 + 4 * (j + 1) + 3, HandleNum);
						}
					}
					else if ((GpsMessage >= 3) && ((j >= 8) && (j <= 11)))//第3条信息
					{
						if ((GpsNumber <= 12) && (j == GpsNumber - 1))
						{
							GpsID[j] = GetData((LPSTR)(LPCTSTR)Serial_Order_Return_CS, ",", ",", 6 + 4 * (j + 1), HandleNum);									//6+===>每条语句，最后一颗星有3个逗号乘以2
							GpsSNR[j] = GetData((LPSTR)(LPCTSTR)Serial_Order_Return_CS, ",", "*", 6 + 4 * (j + 1) + 3, HandleNum);
						}
						else if ((GpsNumber>12) && (j == 11))
						{
							GpsID[j] = GetData((LPSTR)(LPCTSTR)Serial_Order_Return_CS, ",", ",", 6 + 4 * (j + 1), HandleNum);
							GpsSNR[j] = GetData((LPSTR)(LPCTSTR)Serial_Order_Return_CS, ",", "*", 6 + 4 * (j + 1) + 3, HandleNum);
						}
						else
						{
							GpsID[j] = GetData((LPSTR)(LPCTSTR)Serial_Order_Return_CS, ",", ",", 6 + 4 * (j + 1), HandleNum);
							GpsSNR[j] = GetData((LPSTR)(LPCTSTR)Serial_Order_Return_CS, ",", ",", 6 + 4 * (j + 1) + 3, HandleNum);
						}
					}
					else if ((GpsMessage >= 4) && ((j >= 12) && (j <= 15)))//第4条信息
					{
						if ((GpsNumber <= 16) && (j == GpsNumber - 1))
						{
							GpsID[j] = GetData((LPSTR)(LPCTSTR)Serial_Order_Return_CS, ",", ",", 9 + 4 * (j + 1), HandleNum);									//9+===>每条语句，最后一颗星有3个逗号乘以3
							GpsSNR[j] = GetData((LPSTR)(LPCTSTR)Serial_Order_Return_CS, ",", "*", 9 + 4 * (j + 1) + 3, HandleNum);
						}
						else if ((GpsNumber>16) && (j == 15))
						{
							GpsID[j] = GetData((LPSTR)(LPCTSTR)Serial_Order_Return_CS, ",", ",", 9 + 4 * (j + 1), HandleNum);
							GpsSNR[j] = GetData((LPSTR)(LPCTSTR)Serial_Order_Return_CS, ",", "*", 9 + 4 * (j + 1) + 3, HandleNum);
						}
						else
						{
							GpsID[j] = GetData((LPSTR)(LPCTSTR)Serial_Order_Return_CS, ",", ",", 9 + 4 * (j + 1), HandleNum);
							GpsSNR[j] = GetData((LPSTR)(LPCTSTR)Serial_Order_Return_CS, ",", ",", 9 + 4 * (j + 1) + 3, HandleNum);
						}
					}
					else if ((GpsMessage >= 1) && (j <= 3))
					{
						if ((GpsNumber <= 4) && (j == GpsNumber - 1))
						{
							GpsID[j] = GetData((LPSTR)(LPCTSTR)Serial_Order_Return_CS, ",", ",", 4 * (j + 1), HandleNum);	//除了GPGSV,第4个逗号
							GpsSNR[j] = GetData((LPSTR)(LPCTSTR)Serial_Order_Return_CS, ",", "*", 4 * (j + 1) + 3, HandleNum);//除了GPGSV第7个逗号
						}
						else if ((GpsNumber>4) && (j == 3))
						{
							GpsID[j] = GetData((LPSTR)(LPCTSTR)Serial_Order_Return_CS, ",", ",", 4 * (j + 1), HandleNum);	//除了GPGSV,第4个逗号
							GpsSNR[j] = GetData((LPSTR)(LPCTSTR)Serial_Order_Return_CS, ",", "*", 4 * (j + 1) + 3, HandleNum);//除了GPGSV第7个逗号
						}
						else
						{
							GpsID[j] = GetData((LPSTR)(LPCTSTR)Serial_Order_Return_CS, ",", ",", 4 * (j + 1), HandleNum);//除了GPGSV,第4个逗号
							GpsSNR[j] = GetData((LPSTR)(LPCTSTR)Serial_Order_Return_CS, ",", ",", 4 * (j + 1) + 3, HandleNum);//除了GPGSV第7个逗号
						}
					}
				}
				//LogShow_exchange(m_Result,Final_Result_Control,0,"解析GPS数据:卫星编码= "+GpsID[j]+" 信噪比= "+GpsSNR[j],HandleNum);
			}
			//按信号强弱排序
			for (int j = 0; j<GpsNumber; j++)
			{
				for (int i = j + 1; i<GpsNumber; i++)
				{
					if (atoi(GpsSNR[j])<atoi(GpsSNR[i]))
					{
						GpsSNRTemp[j] = GpsSNR[j];
						GpsIDTemp[j] = GpsID[j];

						GpsSNR[j] = GpsSNR[i];
						GpsID[j] = GpsID[i];

						GpsSNR[i] = GpsSNRTemp[j];
						GpsID[i] = GpsIDTemp[j];
					}
				}
				LogShow_exchange(m_Result, Final_Result_Control, 0, "排序后GPS数据:卫星编码= " + GpsID[j] + " 信噪比= " + GpsSNR[j], HandleNum);
			}
			//按信号强弱排序
		}
		else
		{
			return false;
		}
	}
	else if (IMEI_FT_Item.Find("关闭GPS测试") != -1)
	{
		IMEI_FT_Item_Int = 2;
	}
	else if (IMEI_FT_Item.Find("152功能测试") != -1)
	{
		CString DataCountCS;
		int DataCount = Serial_Order_Return_CS.GetLength();
		DataCountCS.Format("%d", DataCount);
		LogShow_exchange(m_Result, Final_Result_Control, 0, "152功能测试数据量=" + DataCountCS, HandleNum);

		if (DataCount<atoi(paraArray[2].High_Limit_Value))
		{
			LogShow_exchange(m_Result, Final_Result_Control, 0, "重启次数异常！！！数据量不够，参考(>" + paraArray[2].High_Limit_Value, HandleNum);
			return false;
		}

		RestartCount = GetData((LPSTR)(LPCTSTR)Serial_Order_Return_CS, paraArray[i].Low_Limit_Value, "Recorded", 1, HandleNum);
		if (RestartCount == "NULL")
			RestartCount = GetData((LPSTR)(LPCTSTR)Serial_Order_Return_CS, paraArray[i].Low_Limit_Value, "\r", 1, HandleNum);
		RestartCount.Replace("\r", "");
		RestartCount.Replace("\n", "");
		LogShow_exchange(m_Result, Final_Result_Control, 0, "重启次数 " + RestartCount, HandleNum);
		IMEI_FT_Item_Int = 3;
	}
	else if (IMEI_FT_Item.Find("电池电压") != -1)//charge in, vBat=4.13V,iChr=0.20A,vChr=4.79V
	{

		if (Serial_Order_Return_CS.Find("VBAT") != -1)
		{
			VBat = GetData((LPSTR)(LPCTSTR)Serial_Order_Return_CS, "VBAT=", "V,", 1, HandleNum);
			IBat = GetData((LPSTR)(LPCTSTR)Serial_Order_Return_CS, "ICHR=", "A,", 1, HandleNum);
			VCharge = GetData((LPSTR)(LPCTSTR)Serial_Order_Return_CS, "VCHR=", "V", 1, HandleNum);
		}
		else if (Serial_Order_Return_CS.Find("EADC") != -1)
		{
			CString Temp = Serial_Order_Return_CS.Mid(Serial_Order_Return_CS.Find("EADC:") + 5, 25);

			Temp.Replace("\r", "");
			Temp.Replace("\n", "");
			Temp.Replace(" ", "");
			LogShow_exchange(m_Result, Final_Result_Control, 0, "Temp= " + Temp, HandleNum);
			VBat = GetData((LPSTR)(LPCTSTR)Temp, "NULL", ",", 1, HandleNum);
			IBat = GetData((LPSTR)(LPCTSTR)Temp, ",", ",", 3, HandleNum);
			VCharge = GetData((LPSTR)(LPCTSTR)Temp, ",", "NULL", 4, HandleNum);
		}
		else
		{
			VBat = GetData((LPSTR)(LPCTSTR)Serial_Order_Return_CS, "vBat=", "V,", 1, HandleNum);
			IBat = GetData((LPSTR)(LPCTSTR)Serial_Order_Return_CS, "iChr=", "A,", 1, HandleNum);
			VCharge = GetData((LPSTR)(LPCTSTR)Serial_Order_Return_CS, "vChr=", "V", 1, HandleNum);
		}
		LogShow_exchange(m_Result, Final_Result_Control, 0, "VBat= " + VBat, HandleNum);
		LogShow_exchange(m_Result, Final_Result_Control, 0, "IBat= " + IBat, HandleNum);
		LogShow_exchange(m_Result, Final_Result_Control, 0, "VCharge= " + VCharge, HandleNum);
		IMEI_FT_Item_Int = 4;
	}
	else if (IMEI_FT_Item.Find("IMEI检查") != -1)
	{
		int pos = Serial_Order_Return_CS.Find(':');
		if (pos >= 0)
			Serial_Order_Return_CS = Serial_Order_Return_CS.Mid(pos + 1);

		pos = Serial_Order_Return_CS.Find('"');
		if (pos >= 0)
		{
			Serial_Order_Return_CS = Serial_Order_Return_CS.Mid(pos + 1);
			Serial_Order_Return_CS = Serial_Order_Return_CS.Left(Serial_Order_Return_CS.Find('"'));
		}
		else
		{
		}
		IMEIRecord[HandleNum] = Serial_Order_Return_CS;
		if (CompareIMEI == TRUE)
		{
			if (IMEI_Input[HandleNum] != IMEIRecord[HandleNum])
			{
				LogShow_exchange(m_Result, Final_Result_Control, 0, "IMEI比对失败！！！！！！！！！\r\n" + IMEI_Input[HandleNum] + "!=" + IMEIRecord[HandleNum], HandleNum);
				return false;
			}
			else
			{
				LogShow_exchange(m_Result, Final_Result_Control, 0, "IMEI比对成功！！\r\n" + IMEI_Input[HandleNum] + "==" + IMEIRecord[HandleNum], HandleNum);
			}
		}
		IMEI_FT_Item_Int = 8;
	}
	else if (IMEI_FT_Item.Find("芯片ID") != -1)
	{
		SIMisOK[HandleNum] = FALSE;

		IMEI_FT_Item_Int = 9;
		Serial_Order_Return_CS.Replace(" ", "");
		Serial_Order_Return_CS.Replace("\r", "");
		Serial_Order_Return_CS.Replace("\n", "");
		//int pos=Serial_Order_Return_CS.Find("ChipRID:");
		int pos = Serial_Order_Return_CS.Find(":");
		if (pos >= 0)
			Serial_Order_Return_CS = Serial_Order_Return_CS.Mid(pos + 1);//Serial_Order_Return_CS=Serial_Order_Return_CS.Mid(pos+8);


		if (IMEI_FT_Item.Find("芯片IDCounterX") != -1)
		{
		}
		else if (IMEI_FT_Item.Find("芯片IDCounter") != -1)
		{
			Serial_Order_Return_CS = Serial_Order_Return_CS.Left(atoi(IMEI_FT_Item.Right(2)));
			if (Serial_Order_Return_CS.GetLength() != atoi(IMEI_FT_Item.Right(2)))
				return false;
		}
		else
		{
			Serial_Order_Return_CS = Serial_Order_Return_CS.Left(32);
			if (Serial_Order_Return_CS.GetLength() != 32)
				return false;
		}
		ChipRfID[HandleNum] = Serial_Order_Return_CS;//线程0、1
	}
	else if (IMEI_FT_Item.Find("软件版本") != -1)
	{
		pos = Serial_Order_Return_CS.Find(':');
		if (pos >= 0)
		{
			Serial_Order_Return_CS = Serial_Order_Return_CS.Mid(pos + 1);

			pos = Serial_Order_Return_CS.Find(' ');
			if (pos >= 0)
				Serial_Order_Return_CS = Serial_Order_Return_CS.Left(pos);
		}
		else
		{
			pos = Serial_Order_Return_CS.Find(']');
			if (pos >= 0)
				Serial_Order_Return_CS = Serial_Order_Return_CS.Mid(pos + 1);

			pos = Serial_Order_Return_CS.Find('[');
			if (pos >= 0)
				Serial_Order_Return_CS = Serial_Order_Return_CS.Left(pos);
		}

		Software_Version[HandleNum] = Software_VersionFull;
	}
	else if (IMEI_FT_Item.Find("蓝牙地址") != -1)
	{
		pos = Serial_Order_Return_CS.Find('"');
		if (pos >= 0)
		{
			Serial_Order_Return_CS = Serial_Order_Return_CS.Mid(pos + 1);
			Serial_Order_Return_CS = Serial_Order_Return_CS.Left(Serial_Order_Return_CS.Find('"'));
		}
		else
		{
		}
		BTMac[HandleNum] = Serial_Order_Return_CS;
	}
	else if (IMEI_FT_Item.Find("BT测试") != -1)
	{
		Serial_Order_Return_CS.Replace("\r", "");
		Serial_Order_Return_CS.Replace("\n", "");
		pos = Serial_Order_Return_CS.Find('"');
		if (pos >= 0)
		{
			Serial_Order_Return_CS = Serial_Order_Return_CS.Mid(pos + 1);
			Serial_Order_Return_CS = Serial_Order_Return_CS.Left(Serial_Order_Return_CS.Find('"'));
		}
		else
		{
		}
		BTTestResult[HandleNum] = Serial_Order_Return_CS;
	}
	else if (IMEI_FT_Item.Find("查询IMSI") != -1)
	{
		pos = Serial_Order_Return_CS.Find('"');
		if (pos >= 0)
		{
			Serial_Order_Return_CS = Serial_Order_Return_CS.Mid(pos + 1);
			Serial_Order_Return_CS = Serial_Order_Return_CS.Left(Serial_Order_Return_CS.Find('"'));
		}
		else
		{
		}
		SimNumber[HandleNum] = Serial_Order_Return_CS;
	}
	else if (IMEI_FT_Item.Find("温度测试") != -1)
	{
		Serial_Order_Return_CS.Replace("\r", "");
		Serial_Order_Return_CS.Replace("\n", "");
		//IMEI_FT_Item_Int=10;
		CString Temperature = GetData((LPSTR)(LPCTSTR)Serial_Order_Return_CS, paraArray[i].Low_Limit_Value, " C", 1, HandleNum);
		CString CountLow = GetData((LPSTR)(LPCTSTR)paraArray[i].High_Limit_Value, "NULL", ",", 1, HandleNum);
		CString CountHigh = GetData((LPSTR)(LPCTSTR)paraArray[i].High_Limit_Value, ",", ",", 1, HandleNum);
		if ((atof(Temperature)<atof(CountLow)) || (atof(Temperature)>atof(CountHigh)))
		{
			LogShow_exchange(m_Result, Final_Result_Control, 0, "温度异常！！！参考(>" + CountLow + " <" + CountHigh, HandleNum);
			return false;
		}
		else
		{
			LogShow_exchange(m_Result, Final_Result_Control, 0, "温度正常..", HandleNum);
		}
		NteTemperature[HandleNum] = Serial_Order_Return_CS;
	}
	else if (IMEI_FT_Item.Find("充电测试") != -1)
	{
		Serial_Order_Return_CS.Replace("\r", "");
		Serial_Order_Return_CS.Replace("\n", "");
		//IMEI_FT_Item_Int=11;
		CString iu = GetData((LPSTR)(LPCTSTR)Serial_Order_Return_CS, paraArray[i].Low_Limit_Value, " A", 1, HandleNum);
		CString CountLow = GetData((LPSTR)(LPCTSTR)paraArray[i].High_Limit_Value, "NULL", ",", 1, HandleNum);
		CString CountHigh = GetData((LPSTR)(LPCTSTR)paraArray[i].High_Limit_Value, ",", ",", 1, HandleNum);
		if ((atof(iu)<atof(CountLow)) || (atof(iu)>atof(CountHigh)))
		{
			LogShow_exchange(m_Result, Final_Result_Control, 0, "充电电流异常！！！参考(>" + CountLow + " <" + CountHigh, HandleNum);
			return false;
		}
		else
		{
			LogShow_exchange(m_Result, Final_Result_Control, 0, "充电电流正常..", HandleNum);
		}
		InchargeI[HandleNum] = Serial_Order_Return_CS;
	}
	else if (IMEI_FT_Item.Find("模拟电流") != -1)
	{
		SimulationCurrent[HandleNum].Replace("\r", "");
		SimulationCurrent[HandleNum].Replace("\n", "");
		CString iu = GetData((LPSTR)(LPCTSTR)SimulationCurrent[HandleNum], "", " A", 1, HandleNum);
		CString CountLow = GetData((LPSTR)(LPCTSTR)paraArray[i].High_Limit_Value, "NULL", ",", 1, HandleNum);
		CString CountHigh = GetData((LPSTR)(LPCTSTR)paraArray[i].High_Limit_Value, ",", ",", 1, HandleNum);
		if ((atof(iu)<atof(CountLow)) || (atof(iu)>atof(CountHigh)))
		{
			LogShow_exchange(m_Result, Final_Result_Control, 0, SimulationCurrent[HandleNum] + "模拟电流异常！！！" + iu + "参考(>" + CountLow + " <" + CountHigh, HandleNum);
			SimulationCurrent[HandleNum] = "";
			return false;
		}
		else
		{
			LogShow_exchange(m_Result, Final_Result_Control, 0, "模拟电流正常.." + iu, HandleNum);
			SimulationCurrent[HandleNum] = "";
		}
		InchargeI[HandleNum] = Serial_Order_Return_CS;
	}
	else if (IMEI_FT_Item.Find("对比电压") != -1)
	{
		//IMEI_FT_Item_Int=12;
		CString Vext;
		if (Serial_Order_Return_CS.Find("Vdc") == -1)
			Vext = GetData((LPSTR)(LPCTSTR)Serial_Order_Return_CS, '(', ')', 1, HandleNum);
		else
			Vext = GetData((LPSTR)(LPCTSTR)Serial_Order_Return_CS, ':', "Vdc", 1, HandleNum);
		CString CountLow = GetData((LPSTR)(LPCTSTR)paraArray[i].High_Limit_Value, "NULL", ",", 1, HandleNum);
		CString CountHigh = GetData((LPSTR)(LPCTSTR)paraArray[i].High_Limit_Value, ",", ",", 1, HandleNum);
		if ((atof(Vext)<atof(CountLow)) || (atof(Vext)>atof(CountHigh)))
		{
			LogShow_exchange(m_Result, Final_Result_Control, 0, "外电电压异常！！！参考(>" + CountLow + " <" + CountHigh, HandleNum);
			return false;
		}
		else
		{
			LogShow_exchange(m_Result, Final_Result_Control, 0, "外电电压正常..参考(>" + CountLow + " <" + CountHigh, HandleNum);
		}
	}
	else if (IMEI_FT_Item.Find("只有电压") != -1)
	{
		//IMEI_FT_Item_Int=12;
		CString Vext;
		if (Serial_Order_Return_CS.Find(')') != -1)
			Vext = GetData((LPSTR)(LPCTSTR)Serial_Order_Return_CS, paraArray[i].Low_Limit_Value, ')', 1, HandleNum);
		else if (Serial_Order_Return_CS.Find('V') != -1)
			Vext = GetData((LPSTR)(LPCTSTR)Serial_Order_Return_CS, paraArray[i].Low_Limit_Value, 'V', 1, HandleNum);
		CString CountLow = GetData((LPSTR)(LPCTSTR)paraArray[i].High_Limit_Value, "NULL", ",", 1, HandleNum);
		CString CountHigh = GetData((LPSTR)(LPCTSTR)paraArray[i].High_Limit_Value, ",", ",", 1, HandleNum);
		if ((atof(Vext)<atof(CountLow)) || (atof(Vext)>atof(CountHigh)))
		{
			LogShow_exchange(m_Result, Final_Result_Control, 0, "只有电压异常！！！参考(>" + CountLow + " <" + CountHigh, HandleNum);
			return false;
		}
		else
		{
			LogShow_exchange(m_Result, Final_Result_Control, 0, "只有电压正常..参考(>" + CountLow + " <" + CountHigh, HandleNum);
		}
	}
	else if (IMEI_FT_Item.Find("RSSI:[") != -1)
	{
		//IMEI_FT_Item_Int=12;
		CString Vext = GetData((LPSTR)(LPCTSTR)Serial_Order_Return_CS, paraArray[i].Low_Limit_Value, ']', 1, HandleNum);
		CString CountLow = GetData((LPSTR)(LPCTSTR)paraArray[i].High_Limit_Value, "NULL", ",", 1, HandleNum);
		CString CountHigh = GetData((LPSTR)(LPCTSTR)paraArray[i].High_Limit_Value, ",", ",", 1, HandleNum);
		if ((atof(Vext)<atof(CountLow)) || (atof(Vext)>atof(CountHigh)))
		{
			LogShow_exchange(m_Result, Final_Result_Control, 0, "RSSI异常！！！参考(>" + CountLow + " <" + CountHigh, HandleNum);
			return false;
		}
		else
		{
			LogShow_exchange(m_Result, Final_Result_Control, 0, "RSSI正常..参考(>" + CountLow + " <" + CountHigh, HandleNum);
		}
	}
	else if (IMEI_FT_Item.Find("SSIDRSSI") != -1)//SSIDRSSI
	{
		CString LowDataMul[THREAD_NUM];
		CString HighDataMul[THREAD_NUM];

		CString LimitSetting;
		if (WIFI_RSSILimitCS == "")
			LimitSetting = paraArray[i].High_Limit_Value;
		else
			LimitSetting = WIFI_RSSILimitCS;
		//CString LimitSetting=paraArray[i].High_Limit_Value;
		for (int j = 0; j<THREAD_NUM; j++)
		{
			if (j == 0)
			{
				LowDataMul[j] = GetData((LPSTR)(LPCTSTR)LimitSetting, "NULL", ",", 1, HandleNum);
				HighDataMul[j] = GetData((LPSTR)(LPCTSTR)LimitSetting, ",", ",", 1, HandleNum);
			}
			else
			{
				LowDataMul[j] = GetData((LPSTR)(LPCTSTR)LimitSetting, ",", ",", 2 * j, HandleNum);
				HighDataMul[j] = GetData((LPSTR)(LPCTSTR)LimitSetting, ",", ",", 2 * j + 1, HandleNum);
			}
		}

		//IMEI_FT_Item_Int=12;
		CString Vext = GetData((LPSTR)(LPCTSTR)Serial_Order_Return_CS, paraArray[i].Low_Limit_Value, ')', 1, HandleNum);
		//CString CountLow=GetData((LPSTR)(LPCTSTR)paraArray[i].High_Limit_Value,"NULL",   ",",1,HandleNum);
		//CString CountHigh=GetData((LPSTR)(LPCTSTR)paraArray[i].High_Limit_Value,",",   ",",1,HandleNum);
		//if((atof(Vext)<atof(CountLow))||(atof(Vext)>atof(CountHigh)))
		if ((atof(Vext)<atof(LowDataMul[HandleNum])) || (atof(Vext)>atof(HighDataMul[HandleNum])))
		{
			LogShow_exchange(m_Result, Final_Result_Control, 0, "RSSI异常！！！参考(>" + LowDataMul[HandleNum] + " <" + HighDataMul[HandleNum], HandleNum);
			return false;
		}
		else
		{
			LogShow_exchange(m_Result, Final_Result_Control, 0, "RSSI正常..参考(>" + LowDataMul[HandleNum] + " <" + HighDataMul[HandleNum], HandleNum);
		}
	}
	else if (IMEI_FT_Item.Find("RSSI") != -1)
	{

		CString Vext = GetData((LPSTR)(LPCTSTR)Serial_Order_Return_CS, paraArray[i].Low_Limit_Value, ')', 1, HandleNum);
		CString CountLow = GetData((LPSTR)(LPCTSTR)paraArray[i].High_Limit_Value, "NULL", ",", 1, HandleNum);
		CString CountHigh = GetData((LPSTR)(LPCTSTR)paraArray[i].High_Limit_Value, ",", ",", 1, HandleNum);
		if ((atof(Vext)<atof(CountLow)) || (atof(Vext)>atof(CountHigh)))
		{
			LogShow_exchange(m_Result, Final_Result_Control, 0, "RSSI异常！！！参考(>" + CountLow + " <" + CountHigh, HandleNum);
			return false;
		}
		else
		{
			LogShow_exchange(m_Result, Final_Result_Control, 0, "RSSI正常..参考(>" + CountLow + " <" + CountHigh, HandleNum);
		}
	}
	else if (IMEI_FT_Item.Find("重力传感器") != -1)
	{
		pos = Serial_Order_Return_CS.Find(paraArray[i].High_Limit_Value);
		if (pos >= 0)
		{
			return false;
		}
		else
		{
		}
	}
	else if (IMEI_FT_Item.Find("截取括号") != -1)
	{
		//IMEI_FT_Item_Int=12;
		CString Vext = GetData((LPSTR)(LPCTSTR)Serial_Order_Return_CS, paraArray[i].Low_Limit_Value, ')', 1, HandleNum);
		CString CountLow = GetData((LPSTR)(LPCTSTR)paraArray[i].High_Limit_Value, "NULL", ",", 1, HandleNum);
		CString CountHigh = GetData((LPSTR)(LPCTSTR)paraArray[i].High_Limit_Value, ",", ",", 1, HandleNum);
		if ((atof(Vext)<atof(CountLow)) || (atof(Vext)>atof(CountHigh)))
		{
			LogShow_exchange(m_Result, Final_Result_Control, 0, IMEI_FT_Item + "异常！！！参考(>" + CountLow + " <" + CountHigh, HandleNum);
			return false;
		}
		else
		{
			LogShow_exchange(m_Result, Final_Result_Control, 0, IMEI_FT_Item + "正常..参考(>" + CountLow + " <" + CountHigh, HandleNum);
		}
	}
	else if (IMEI_FT_Item.Find("逗号一") != -1)
	{
		//IMEI_FT_Item_Int=12;
		CString Vext = GetData((LPSTR)(LPCTSTR)Serial_Order_Return_CS, paraArray[i].Low_Limit_Value, ',', 1, HandleNum);
		CString CountLow = GetData((LPSTR)(LPCTSTR)paraArray[i].High_Limit_Value, "NULL", ",", 1, HandleNum);
		CString CountHigh = GetData((LPSTR)(LPCTSTR)paraArray[i].High_Limit_Value, ",", ",", 1, HandleNum);
		if ((atof(Vext)<atof(CountLow)) || (atof(Vext)>atof(CountHigh)))
		{
			LogShow_exchange(m_Result, Final_Result_Control, 0, IMEI_FT_Item + "异常！！！参考(>" + CountLow + " <" + CountHigh, HandleNum);
			return false;
		}
		else
		{
			LogShow_exchange(m_Result, Final_Result_Control, 0, IMEI_FT_Item + "正常..参考(>" + CountLow + " <" + CountHigh, HandleNum);
		}
	}
	else if (IMEI_FT_Item.Find("蜂鸣器量化") != -1)
	{
		CString Vext = GetData((LPSTR)(LPCTSTR)Serial_Order_Return_CS, paraArray[i].Low_Limit_Value, 'T', 1, HandleNum);
		CString CountLow = GetData((LPSTR)(LPCTSTR)paraArray[i].High_Limit_Value, "NULL", ",", 1, HandleNum);
		CString CountHigh = GetData((LPSTR)(LPCTSTR)paraArray[i].High_Limit_Value, ",", ",", 1, HandleNum);
		if ((atof(Vext)<atof(CountLow)) || (atof(Vext)>atof(CountHigh)))
		{
			LogShow_exchange(m_Result, Final_Result_Control, 0, IMEI_FT_Item + "异常！！！参考(>" + CountLow + " <" + CountHigh, HandleNum);
			return false;
		}
		else
		{
			LogShow_exchange(m_Result, Final_Result_Control, 0, IMEI_FT_Item + "正常..参考(>" + CountLow + " <" + CountHigh, HandleNum);
		}
	}
	else if (IMEI_FT_Item.Find("SIM卡检测") != -1)
	{
		SIMisOK[HandleNum] = TRUE;
	}
	else if (IMEI_FT_Item.Find("串口获取16进制字符串") != -1)//"串口获取图片"
	{
		char OutStringBuffer[PICDATAMAX] = { 0 };
		CString RecString;
		HexGroupToString(OutStringBuffer, (unsigned char *)(LPSTR)(LPCTSTR)Serial_Order_Return_CS, Serial_Order_Return_CS.GetLength());
		RecString.Format("%s", OutStringBuffer);//--------------------------------------------------------------注意看是否需要清理空格
		StartShowPic(HandleNum, m_Result, Final_Result_Control, RecString);
	}
	else if (IMEI_FT_Item.Find("值量化") != -1)//**
	{
		CString strH = GetData((LPSTR)(LPCTSTR)paraArray[i].High_Limit_Value, "**", ",", 1, HandleNum);
		CString Vext = GetData((LPSTR)(LPCTSTR)Serial_Order_Return_CS, paraArray[i].Low_Limit_Value, strH, 1, HandleNum);

		CString CountLow = GetData((LPSTR)(LPCTSTR)paraArray[i].High_Limit_Value, "NULL", ",", 1, HandleNum);
		CString CountHigh = GetData((LPSTR)(LPCTSTR)paraArray[i].High_Limit_Value, ",", ",", 1, HandleNum);
		if ((atof(Vext)<atof(CountLow)) || (atof(Vext)>atof(CountHigh)))
		{
			LogShow_exchange(m_Result, Final_Result_Control, 0, IMEI_FT_Item + "异常！！！参考(>" + CountLow + " <" + CountHigh, HandleNum);
			return false;
		}
		else
		{
			LogShow_exchange(m_Result, Final_Result_Control, 0, IMEI_FT_Item + "正常..参考(>" + CountLow + " <" + CountHigh, HandleNum);
		}
	}
	else
		IMEI_FT_Item_Int = 32;

	BOOL GPSresult = TRUE;
	switch (IMEI_FT_Item_Int)
	{
	case 1://GPS-30,-40,   -30,-40,   -30,-40,   -30,-40,   -30,-40,   -30,-40,   -30,-40,   //1,1    2,3   4,5  6,7
		if (GpsNumber<atoi(paraArray[3].High_Limit_Value))//卫星数量不够
		{
			LogShow_exchange(m_Result, Final_Result_Control, 0, "搜索到的卫星数量不足" + paraArray[3].High_Limit_Value + " 个,继续...", HandleNum);
			return false;
		}
		if (GPS_SNRLimitCS == "")
			SNRSetting = paraArray[i].High_Limit_Value;
		else
			SNRSetting = GPS_SNRLimitCS;
		LogShow_exchange(m_Result, Final_Result_Control, 0, SNRSetting, HandleNum);
		SNRSetting.Replace(" ", "");
		for (int j = 0; j<THREAD_NUM; j++)
		{
			if (j == 0)
			{
				GpsSNRStandardLow[j] = GetData((LPSTR)(LPCTSTR)SNRSetting, "NULL", ",", 1, HandleNum);
				GpsSNRStandardHigh[j] = GetData((LPSTR)(LPCTSTR)SNRSetting, ",", ",", 1, HandleNum);
			}
			else
			{
				GpsSNRStandardLow[j] = GetData((LPSTR)(LPCTSTR)SNRSetting, ",", ",", 2 * j, HandleNum);
				GpsSNRStandardHigh[j] = GetData((LPSTR)(LPCTSTR)SNRSetting, ",", ",", 2 * j + 1, HandleNum);
			}
		}
		for (int j = 0; j<atoi(paraArray[3].High_Limit_Value); j++)
		{
			if ((atoi(GpsSNR[j])<atoi(GpsSNRStandardLow[HandleNum])) || (atoi(GpsSNR[j])>atoi(GpsSNRStandardHigh[HandleNum])))//GPS不达标
			{
				GPSresult = FALSE;
				LogShow_exchange(m_Result, Final_Result_Control, 0, "卫星号" + GpsID[j] + " 信噪比异常！！！参考(>" + GpsSNRStandardLow[HandleNum] + " <" + GpsSNRStandardHigh[HandleNum], HandleNum);
			}
			else
			{
				LogShow_exchange(m_Result, Final_Result_Control, 0, "卫星号" + GpsID[j] + " 对应的信噪比正常..", HandleNum);
			}
		}
		if (GPSresult == FALSE)
			return false;
		GPSSnrAve[HandleNum] = GpsSNR[0];
		break;
	case 2:
		//关闭GPS测试
		break;
	case 3:
		RestartCountLow = GetData((LPSTR)(LPCTSTR)paraArray[i].High_Limit_Value, "NULL", ",", 1, HandleNum);
		RestartCountHigh = GetData((LPSTR)(LPCTSTR)paraArray[i].High_Limit_Value, ",", ",", 1, HandleNum);
		if ((atoi(RestartCount)<atoi(RestartCountLow)) || (atoi(RestartCount)>atoi(RestartCountHigh)))
		{
			LogShow_exchange(m_Result, Final_Result_Control, 0, "重启次数异常！！！参考(>" + RestartCountLow + " <" + RestartCountHigh, HandleNum);
			return false;
		}
		else
		{
			LogShow_exchange(m_Result, Final_Result_Control, 0, "重启次数正常..", HandleNum);
		}
		break;
	case 4:
		//charge in, vBat=4.13V,iChr=0.20A,vChr=4.79V
		BBCSetting = paraArray[i].High_Limit_Value;
		BBCSetting.Replace(" ", "");
		for (int j = 0; j<6; j++)
		{
			if (j == 0)
			{
				BBCSettingLow[j] = GetData((LPSTR)(LPCTSTR)BBCSetting, "NULL", ",", 1, HandleNum);
				BBCSettingHigh[j] = GetData((LPSTR)(LPCTSTR)BBCSetting, ",", ",", 1, HandleNum);
			}
			else
			{
				BBCSettingLow[j] = GetData((LPSTR)(LPCTSTR)BBCSetting, ",", ",", 2 * j, HandleNum);
				BBCSettingHigh[j] = GetData((LPSTR)(LPCTSTR)BBCSetting, ",", ",", 2 * j + 1, HandleNum);
			}
		}
		if ((atof(VBat)<atof(BBCSettingLow[0])) || (atof(VBat)>atof(BBCSettingHigh[0])) \
			|| (atof(IBat)<atof(BBCSettingLow[1])) || (atof(IBat)>atof(BBCSettingHigh[1])) \
			|| (atof(VCharge)<atof(BBCSettingLow[2])) || (atof(VCharge)>atof(BBCSettingHigh[2])) \
			)
		{
			LogShow_exchange(m_Result, Final_Result_Control, 0, "电池电压、电流、充电电压异常！！！参考1(>" + BBCSettingLow[0] + " <" + BBCSettingHigh[0] + "(>" + BBCSettingLow[1] + " <" + BBCSettingHigh[1] + "(>" + BBCSettingLow[2] + " <" + BBCSettingHigh[2], HandleNum);
			if ((atof(VBat)<atof(BBCSettingLow[0 + 3])) || (atof(VBat)>atof(BBCSettingHigh[0 + 3])) \
				|| (atof(IBat)<atof(BBCSettingLow[1 + 3])) || (atof(IBat)>atof(BBCSettingHigh[1 + 3])) \
				|| (atof(VCharge)<atof(BBCSettingLow[2 + 3])) || (atof(VCharge)>atof(BBCSettingHigh[2 + 3])) \
				)
			{
				LogShow_exchange(m_Result, Final_Result_Control, 0, "电池电压、电流、充电电压异常！！！参考2(>" + BBCSettingLow[0 + 3] + " <" + BBCSettingHigh[0 + 3] + "(>" + BBCSettingLow[1 + 3] + " <" + BBCSettingHigh[1 + 3] + "(>" + BBCSettingLow[2 + 3] + " <" + BBCSettingHigh[2 + 3], HandleNum);
				return false;
			}
			else
			{
				LogShow_exchange(m_Result, Final_Result_Control, 0, "电池电压、电流、充电电压正常..参考2(>" + BBCSettingLow[0 + 3] + " <" + BBCSettingHigh[0 + 3] + "(>" + BBCSettingLow[1 + 3] + " <" + BBCSettingHigh[1 + 3] + "(>" + BBCSettingLow[2 + 3] + " <" + BBCSettingHigh[2 + 3], HandleNum);
			}

		}
		else
		{
			LogShow_exchange(m_Result, Final_Result_Control, 0, "电池电压、电流、充电电压正常..参考1(>" + BBCSettingLow[0] + " <" + BBCSettingHigh[0] + "(>" + BBCSettingLow[1] + " <" + BBCSettingHigh[1] + "(>" + BBCSettingLow[2] + " <" + BBCSettingHigh[2], HandleNum);
		}

		break;
	default:
		return true;
		break;
	}
	return true;
}
BOOL IMEIWrite_MulAT::WriteIMEIFunction_Thread(CComboBox* m_Port, CComboBox* m_Baud, int HandleNum, CEdit* m_Result, CEdit* Final_Result_Control, CEdit* Data_Input_Control)
{
	//---------------------------11
	CString PortType_CS;
	PortType.GetWindowTextA(PortType_CS);
	CTime showtime;
	while (CheckConnect_Thread(m_Port, m_Baud, HandleNum, m_Result, Final_Result_Control) != TRUE)//
	{
		showtime = CTime::GetCurrentTime();

		CString showtimeCS;
		showtimeCS.Format("%02d:%02d:%02d", showtime.GetHour(), showtime.GetMinute(), showtime.GetSecond());

		if (StopSign[HandleNum] == TRUE)
			return FALSE;
		if (showtime.GetSecond() % 5>2)
			LogShow_exchange(m_Result, Final_Result_Control, 0, "等待连接", HandleNum);
		//LogShow_exchange(m_Result,Final_Result_Control,0,"等待连接"+showtimeCS,HandleNum);
		Sleep(1000);
	}

	CTime TestTime = CTime::GetCurrentTime();
	TestTimeStart[HandleNum] = TestTime.GetHour() * 3600 + TestTime.GetMinute() * 60 + TestTime.GetSecond();

	CString DB_FAIL = "";

	CString ScanGunTpyeCS;
	ScanGunTpye.GetWindowTextA(ScanGunTpyeCS);


	CString Serial_Order_Return;//参数

	int Port_OK_Try = 0;
	LogShow_exchange(m_Result, Final_Result_Control, 0, "正在获取端口。。。", HandleNum);
GETPort:
	for (int num = 0; num<8; num++)
	{
		if (OnGetport() == TRUE)
			break;
		Sleep(250);
	}
	BOOL Port_OK = FALSE;
	CString sPort;
	m_Port->GetWindowText(sPort);
	for (int num1 = 0; num1<PORTS_NUM; num1++)
	{
		if ((COM_Number[num1] == sPort) && (sPort != ""))
		{
			Port_OK = TRUE;
			break;
		}
	}
	if (Port_OK == FALSE)
	{
		Port_OK_Try++;
		if (Port_OK_Try<5)
		{
			LogShow_exchange(m_Result, Final_Result_Control, 0, "正在获取端口" + sPort + "...", HandleNum);
			Sleep(1000);
			goto GETPort;
		}
		else
		{
			LogShow_exchange(m_Result, Final_Result_Control, 128, "当前端口:" + sPort + "不存在，请检查硬件连接", HandleNum);
			//return FALSE;
			goto OVERDONE;
		}
	}
	else
	{
		LogShow_exchange(m_Result, Final_Result_Control, 0, "成功获取端口" + sPort + "...", HandleNum);
	}
	if (Port_OK_Try>0)
		Sleep(1000);
	else
		Sleep(500);

	/////////////////////////////////////////////
	//扫描枪
	if (CompareIMEI == TRUE)
	{
		EnterCriticalSection(&UseScanResult);
		if (TestCheck == TRUE)//串口接通
		{
			//正常状态
			IMEI_Input[HandleNum] = PublicIMEI;
			LogShow_exchange(m_Result, Final_Result_Control, 0, "IMEI------------->OK！\r\n", HandleNum);
			SetTimer(30, 1000, NULL);
		}
		else
		{
			COM_State[HandleNum] = FALSE;
			LogShow_exchange(m_Result, Final_Result_Control, 128, "没有扫描到IMEI！！！\r\n", HandleNum);
			LeaveCriticalSection(&UseScanResult);
			goto OVERDONE;//打开串口失败
		}
		TestCheck = FALSE;
		LeaveCriticalSection(&UseScanResult);
	}
	//扫描枪
	/////////////////////////////////////////////
	if ((paraArray[1].High_Limit_Value == "ON") && (COM_State[HandleNum] == TRUE))//仅打开一次串口，后续保持一直打开
	{
		LogShow_exchange(m_Result, Final_Result_Control, 5, "串口已经打开", HandleNum);
	}
	else
	{
		if (OPen_Serial_Port(m_Port, m_Baud, HandleNum) == TRUE)//仅打开一次串口，后续保持一直打开
		{
			//打开串口成功
			COM_State[HandleNum] = TRUE;
			LogShow_exchange(m_Result, Final_Result_Control, 0, "打开串口成功！\r\n", HandleNum);													//第一次显示“测试中”
		}
		else
		{
			//打开串口失败
			COM_State[HandleNum] = FALSE;
			LogShow_exchange(m_Result, Final_Result_Control, 128, "打开串口失败！\r\n", HandleNum);
			//return FALSE;
			goto OVERDONE;//打开串口失败
		}
	}
	/////////////////////////////////////////////////

	int Try_Time = 0;																												//尝试次数（结果判断）
	int All_Result[ITEMMAX] = { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0    \
		, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

	int Try_Time_Real = atoi(paraArray[0].High_Limit_Value);
	for (int i = 1; i<ITEMMAX; i++)
	{
		if (StopSign[HandleNum] == TRUE)
		{
			LogShow_exchange(m_Result, Final_Result_Control, 128, "人为终止,立即停止！！！", HandleNum, "功能测试:4-BORT");
			goto OVERDONE;//测试失败且有关闭串口选择处理
		}
		Try_Time_Real = atoi(paraArray[0].High_Limit_Value);

		Vaule_Return_Count_CS[HandleNum] = "";
		do
		{
			CString WaitCOMTimeCS;
			WaitCOMTimeCS = paraArray[0].Low_Limit_Value;//超时时限
			int posw = WaitCOMTimeCS.Find("(");
			if (posw >= 0)
			{
				WaitCOMTimeCS = WaitCOMTimeCS.Mid(posw + 1);

				posw = WaitCOMTimeCS.Find(")");
				if (posw >= 0)
					WaitCOMTimeCS = WaitCOMTimeCS.Left(posw);
			}
			else
				WaitCOMTimeCS = "-1";
			if (atoi(WaitCOMTimeCS)<0)
				WaitCOMTimeCS = "-1";

			CTime ScanTime = CTime::GetCurrentTime();
			int TimeDiff = (ScanTime.GetHour() * 3600 + ScanTime.GetMinute() * 60 + ScanTime.GetSecond() - TestTimeStart[HandleNum]) * 1;
			if ((WaitCOMTimeCS != "-1") && (TimeDiff>atoi(WaitCOMTimeCS)))//扫描后等待COM超时
			{
				LogShow_exchange(m_Result, Final_Result_Control, 128, "测试超时,立即停止！！！", HandleNum, "功能测试:4-BORT");
				goto OVERDONE;//测试失败且有关闭串口选择处理
			}


			//////////////////////////////////////////
			if (StopSign[HandleNum] == TRUE)
			{
				LogShow_exchange(m_Result, Final_Result_Control, 128, "人为终止,立即停止！！！", HandleNum, "功能测试:4-BORT");
				goto OVERDONE;//测试失败且有关闭串口选择处理
			}
			if (paraArray[i].ifCheck != true)																						//没有勾选
			{
				All_Result[i] = 1;
				break;																											//跳出do while
			}
			if ((paraArray[i].showName.Find("写测试标志") != -1) && (All_Result[0] != 1))
				break;																											//跳出do while//测试项不全部OK，不写功能测试标志
			int tr = paraArray[i].showName.Find("TR");
			if (tr >= 0)
				Try_Time_Real = atoi(paraArray[i].showName.Left(2));
			if (Try_Time_Real<1)
				Try_Time_Real = atoi(paraArray[0].High_Limit_Value);

			CString WaitTimeCS;
			int wpos = paraArray[i].showName.Find("DL");
			if (wpos != -1)
				WaitTimeCS = paraArray[i].showName.Mid(wpos + 2);
			else
				WaitTimeCS = "0";

			if ((paraArray[i].showName.Find("按键") != -1) || (paraArray[i].showName.Find("开蜂鸣器") != -1) || (paraArray[i].showName.Find("震动") != -1) || (paraArray[i].showName.Find("确认标识") != -1) || (paraArray[i].showName.Find("控制板状态") != -1))//
			{
				Final_Result_Control->SetFont(fontsp);
				gColor[HandleNum] = RGB(205, 0, 205);
				Final_Result_Control->SetWindowTextA(paraArray[i].showName);
				Final_Result_Control->UpdateWindow();
				//三合一传递提示信息
				if (g_ADCTFlag == TRUE)
				{
					if (paraArray[i].showName.Find("按键") != -1)
					{
						SendADCTHint(get_map_key_value(PortStatusMap, HandleNum), "按键", 0);
					}
					else if (paraArray[i].showName.Find("开蜂鸣器") != -1)
					{
						SendADCTHint(get_map_key_value(PortStatusMap, HandleNum), "开蜂鸣器", 0);
					}
					else if (paraArray[i].showName.Find("震动") != -1)
					{
						SendADCTHint(get_map_key_value(PortStatusMap, HandleNum), "震动", 0);
					}
					else if (paraArray[i].showName.Find("确认标识") != -1)
					{
						SendADCTHint(get_map_key_value(PortStatusMap, HandleNum), "确认标识", 0);
					}
					else if (paraArray[i].showName.Find("控制板状态") != -1)
					{
						SendADCTHint(get_map_key_value(PortStatusMap, HandleNum), "控制板状态", 0);
					}
				}
				if (paraArray[i].showName.Find("开蜂鸣器") != -1)
					Voice_Speak("滴答");
				if (paraArray[i].showName.Find("控制板状态") != -1)
				{
					TestResult[HandleNum] = TRUE;
					CString VoiceM;
					VoiceM.Format("请确认%d", HandleNum);
					Final_Result_Control->SetWindowTextA(VoiceM);
					Final_Result_Control->UpdateWindow();
					Voice_Speak(VoiceM);
				}
			}
			else
			{
				Final_Result_Control->SetFont(font);
				LogShow_exchange(m_Result, Final_Result_Control, 0, "测试中...", HandleNum);
			}

			if ((paraArray[i].showName.Find("开蜂鸣器") != -1) || (paraArray[i].showName.Find("关蜂鸣器") != -1) || (paraArray[i].showName.Find("按键") != -1) || (paraArray[i].showName.Find("震动") != -1)\
				|| (paraArray[i].showName.Find("控制板") != -1) || (paraArray[i].showName.Find("模拟电流") != -1) || (paraArray[i].showName.Find("通路") != -1) || (paraArray[i].showName.Find("照明灯") != -1)\
				|| (paraArray[i].showName.Find("控制板状态") != -1) || (paraArray[i].showName.Find("外电") != -1))
			{
				BOOL Result = CPUCommunication(paraArray[i].showName, HandleNum, "", "", COM_State[THREAD_NUM], m_Result, Final_Result_Control);
				if (paraArray[i].showName.Find("控制板状态") != -1)//查询控制板相关状态的时候，要多次循环
				{
					if (Result == FALSE)
					{
						Try_Time++;
						LogShow_exchange(m_Result, Final_Result_Control, 5, "测试项目:" + paraArray[i].showName + "无效继续...", HandleNum);	//控制板轮序获取状态
						if ((Try_Time<Try_Time_Real) && (((GetPicChoose == TRUE) && (PicUnionOK[HandleNum] == TRUE)) || (BGConfirmChoose == TRUE)))//次数限制，同时满足采集的WIFI信号正常（假如后台确认则稍等）
						{
							if (BGConfirmChoose == TRUE)//后台确认也不能等太久
							{
								if ((Try_Time>5) && ((GetPicChoose == TRUE) && (PicUnionOK[HandleNum] != TRUE)))
								{
									LogShow_exchange(m_Result, Final_Result_Control, 128, "获取图片失败,立即停止！！！", HandleNum, "功能测试:2-TEST");
									goto OVERDONE;
								}
							}
							Sleep(atoi(WaitTimeCS));
							continue;//循环do while
						}
						else
						{
							LogShow_exchange(m_Result, Final_Result_Control, 128, "控制板状态变化超时,立即停止！！！", HandleNum, "功能测试:1-CPU");
							goto OVERDONE;
						}
					}
					else
					{
						LogShow_exchange(m_Result, Final_Result_Control, 5, "开始显示图片。。。", HandleNum);//待处理，假如图片数据有异常，则跳到失败
						//TestResult[HandleNum]=TRUE;//测试成功,图片线程可能会比测试线程快，所以要放在前面
						DoorWait[HandleNum].SetEvent();
					}
				}
				else if (Result == FALSE)
				{
					LogShow_exchange(m_Result, Final_Result_Control, 128, "与控制板通信异常,立即停止！！！", HandleNum, "功能测试:1-CPU");
					goto OVERDONE;//测试失败且有关闭串口选择处理
				}
				else
				{
					if (COM_State[THREAD_NUM] == TRUE)
						LogShow_exchange(m_Result, Final_Result_Control, 5, "与控制板通信正常:" + paraArray[i].showName, HandleNum);
				}
			}
			else if ((paraArray[i].showName.Find("WIFI发送") != -1))
			{
				LogShow_exchange(m_Result, Final_Result_Control, 5, "等待上一次确认...", HandleNum);
				EnterCriticalSection(&WIFICOMMUNICATE[HandleNum]);
				LogShow_exchange(m_Result, Final_Result_Control, 5, "上一次确认结束", HandleNum);
				LeaveCriticalSection(&WIFICOMMUNICATE[HandleNum]);
			}
			else if ((paraArray[i].showName.Find("WIFI接收图片") != -1))
			{
				if (GetPicChoose == FALSE)//没有勾选获取图片
				{
					LogShow_exchange(m_Result, Final_Result_Control, 128, "没有勾选勾选<T9-T16获取图片>", HandleNum, "功能测试:2-TEST");
					goto OVERDONE;
				}
				CEdit* m_ResultPic;
				CEdit* Final_Result_ControlPic;

				if (HandleNum == 0)
				{
					m_ResultPic = &m_Result9;
					Final_Result_ControlPic = &Final_Result_Control9;
				}
				else if (HandleNum == 1)
				{
					m_ResultPic = &m_Result10;
					Final_Result_ControlPic = &Final_Result_Control10;
				}
				else if (HandleNum == 2)
				{
					m_ResultPic = &m_Result11;
					Final_Result_ControlPic = &Final_Result_Control11;
				}
				else if (HandleNum == 3)
				{
					m_ResultPic = &m_Result12;
					Final_Result_ControlPic = &Final_Result_Control12;
				}
				else if (HandleNum == 4)
				{
					m_ResultPic = &m_Result13;
					Final_Result_ControlPic = &Final_Result_Control13;
				}
				else if (HandleNum == 5)
				{
					m_ResultPic = &m_Result14;
					Final_Result_ControlPic = &Final_Result_Control14;
				}
				else if (HandleNum == 6)
				{
					m_ResultPic = &m_Result15;
					Final_Result_ControlPic = &Final_Result_Control15;
				}
				else if (HandleNum == 7)
				{
					m_ResultPic = &m_Result16;
					Final_Result_ControlPic = &Final_Result_Control16;
				}

				StartGetPic(HandleNum, m_ResultPic, Final_Result_ControlPic);
			}
			else if ((paraArray[i].showName.Find("串口获取图片") != -1))
			{
				if (GetPicChoose == FALSE)//没有勾选获取图片
				{
					LogShow_exchange(m_Result, Final_Result_Control, 128, "没有勾选勾选<T9-T16获取图片>", HandleNum, "功能测试:2-TEST");
					goto OVERDONE;
				}
				CEdit* m_ResultPic;
				CEdit* Final_Result_ControlPic;

				if (HandleNum == 0)
				{
					m_ResultPic = &m_Result9;
					Final_Result_ControlPic = &Final_Result_Control9;
				}
				else if (HandleNum == 1)
				{
					m_ResultPic = &m_Result10;
					Final_Result_ControlPic = &Final_Result_Control10;
				}
				else if (HandleNum == 2)
				{
					m_ResultPic = &m_Result11;
					Final_Result_ControlPic = &Final_Result_Control11;
				}
				else if (HandleNum == 3)
				{
					m_ResultPic = &m_Result12;
					Final_Result_ControlPic = &Final_Result_Control12;
				}
				else if (HandleNum == 4)
				{
					m_ResultPic = &m_Result13;
					Final_Result_ControlPic = &Final_Result_Control13;
				}
				else if (HandleNum == 5)
				{
					m_ResultPic = &m_Result14;
					Final_Result_ControlPic = &Final_Result_Control14;
				}
				else if (HandleNum == 6)
				{
					m_ResultPic = &m_Result15;
					Final_Result_ControlPic = &Final_Result_Control15;
				}
				else if (HandleNum == 7)
				{
					m_ResultPic = &m_Result16;
					Final_Result_ControlPic = &Final_Result_Control16;
				}
				DayTimeState[HandleNum] = TRUE;
				hPicPort[HandleNum] = hPort[HandleNum];//串口句柄赋值

				BOOL bWriteStat;
				DWORD dwBytesWritten;
				char *buffer;
				CString command = paraArray[i].Other_ITEM + "\r\n";//拼接命令
				buffer = command.GetBuffer(command.GetLength());
				bWriteStat = WriteFile(hPort[HandleNum], buffer, command.GetLength(), &dwBytesWritten, NULL);//
				if (dwBytesWritten != command.GetLength())
				{
					Sleep(300);
					continue;
				}
				//receive response
				Sleep(atoi(WaitTimeCS));

				SerialPicOver[HandleNum] = FALSE;
				SerialPic[HandleNum].ResetEvent();
				DoorWait[HandleNum].ResetEvent();
				StartGetPic(HandleNum, m_ResultPic, Final_Result_ControlPic);
				WaitForSingleObject(SerialPic[HandleNum].m_hObject, 60 * 1000);//最多等待60秒INFINITE
				if (SerialPicOver[HandleNum] == TRUE)
				{
					LogShow_exchange(m_Result, Final_Result_Control, 5, "测试项目:" + paraArray[i].showName + "达标！========该项测试成功", HandleNum);			//继续测试，状态不更新
					All_Result[i] = 1;//跳出do while 
					break;
				}
				else
				{
					LogShow_exchange(m_Result, Final_Result_Control, 128, "测试项目:" + paraArray[i].showName + "失败,立即停止！！！", HandleNum, "功能测试:2-TEST");
					goto OVERDONE;//测试失败且有关闭串口选择处理
				}
			}
			else if ((paraArray[i].showName.Find("WIFI确认标识") != -1))
			{
				if (GetPicChoose == FALSE)//没有勾选获取图片
				{
					LogShow_exchange(m_Result, Final_Result_Control, 128, "没有勾选勾选<T9-T16获取图片>", HandleNum, "功能测试:2-TEST");
					goto OVERDONE;
				}

				if (BGConfirmChoose == TRUE)//后台确认，则不弹出确认界面
				{
					All_Result[i] = 1;
					BGShowPicTime[HandleNum] = WaitTimeCS;
					BGShowPicCount[HandleNum] = Try_Time_Real;
					break; //跳出do while 
				}

				CEdit* m_ResultPic;
				CEdit* Final_Result_ControlPic;

				if (HandleNum == 0)
				{
					m_ResultPic = &m_Result9;
					Final_Result_ControlPic = &Final_Result_Control9;
				}
				else if (HandleNum == 1)
				{
					m_ResultPic = &m_Result10;
					Final_Result_ControlPic = &Final_Result_Control10;
				}
				else if (HandleNum == 2)
				{
					m_ResultPic = &m_Result11;
					Final_Result_ControlPic = &Final_Result_Control11;
				}
				else if (HandleNum == 3)
				{
					m_ResultPic = &m_Result12;
					Final_Result_ControlPic = &Final_Result_Control12;
				}
				else if (HandleNum == 4)
				{
					m_ResultPic = &m_Result13;
					Final_Result_ControlPic = &Final_Result_Control13;
				}
				else if (HandleNum == 5)
				{
					m_ResultPic = &m_Result14;
					Final_Result_ControlPic = &Final_Result_Control14;
				}
				else if (HandleNum == 6)
				{
					m_ResultPic = &m_Result15;
					Final_Result_ControlPic = &Final_Result_Control15;
				}
				else if (HandleNum == 7)
				{
					m_ResultPic = &m_Result16;
					Final_Result_ControlPic = &Final_Result_Control16;
				}

				////////////////////////////////////////////////
				//失败按钮
				BOOL Result = CPUCommunication("确认失败", HandleNum, "", "", COM_State[THREAD_NUM], m_Result, Final_Result_Control);
				if (Result == TRUE)
				{
					LogShow_exchange(m_Result, Final_Result_Control, 128, "确认失败", HandleNum, "功能测试:2-TEST");
					goto OVERDONE;
				}
				//失败按钮
				////////////////////////////////////////////////


				if (paraArray[i].showName.Find("确认标识") != -1)
				{
					Final_Result_Control->SetFont(fontsp);
					gColor[HandleNum] = RGB(205, 0, 205);
					Final_Result_Control->SetWindowTextA(paraArray[i].showName);
					Final_Result_Control->UpdateWindow();
				}
				if (Get_Wifi_Data(m_ResultPic, Final_Result_ControlPic, HandleNum, "", "RESET") != TRUE)
				{
					Picdlg[HandleNum].ShowPicTime = atoi(WaitTimeCS);
					/*if(UnionPicPath[HandleNum]=="")
					{
					LogShow_exchange(m_Result,Final_Result_Control,128,"测试项目:"+paraArray[i].showName+"失败,立即停止！！！",HandleNum,"功能测试:2-TEST");
					goto OVERDONE;
					}
					Picdlg[HandleNum].UnionPicPath=UnionPicPath[HandleNum];*/
					Picdlg[HandleNum].PicPath0 = PicPath[0][HandleNum];
					Picdlg[HandleNum].PicPath1 = PicPath[1][HandleNum];
					Picdlg[HandleNum].DoModal();

					Try_Time++;
					LogShow_exchange(m_Result, Final_Result_Control, 5, "测试项目:" + paraArray[i].showName + "无效继续...", HandleNum);	//控制板轮序获取状态
					if ((Try_Time<Try_Time_Real) && ((GetPicChoose == TRUE) && (PicUnionOK[HandleNum] == TRUE)))//次数限制，同时满足采集的WIFI信号正常
					{

						continue;//循环do while
					}
					else
					{
						LogShow_exchange(m_Result, Final_Result_Control, 128, "测试项目:" + paraArray[i].showName + "失败,立即停止！！！", HandleNum, "功能测试:2-TEST");
						goto OVERDONE;
					}
				}
				else
				{
					ResetButtonWIFI[HandleNum] = TRUE;
				}

			}
			/*if((paraArray[i].showName.Find("按键")!=-1)||(paraArray[i].showName.Find("开蜂鸣器")!=-1)||(paraArray[i].showName.Find("震动")!=-1)||(paraArray[i].showName.Find("确认标识")!=-1)||(paraArray[i].showName.Find("控制板状态")!=-1))//
			{
			Final_Result_Control->SetFont(fontsp);
			gColor[HandleNum] = RGB(205,0,205);
			Final_Result_Control->SetWindowTextA(paraArray[i].showName);
			Final_Result_Control->UpdateWindow();
			if(paraArray[i].showName.Find("开蜂鸣器")!=-1)
			Voice_Speak("滴答");
			if(paraArray[i].showName.Find("控制板状态")!=-1)
			{
			CString VoiceM;
			VoiceM.Format("请确认%d",HandleNum);
			Voice_Speak(VoiceM);
			}
			}
			else
			{
			Final_Result_Control->SetFont(font);
			LogShow_exchange(m_Result,Final_Result_Control,0,"测试中...",HandleNum);
			}*/

			CString PortOrder;
			PortOrder = paraArray[i].Other_ITEM;

			/*CString WaitTimeCS;
			int wpos=paraArray[i].showName.Find("DL");
			if(wpos!=-1)
			WaitTimeCS=paraArray[i].showName.Mid(wpos+2);
			else
			WaitTimeCS="0";*/

			if (paraArray[i].showName.Find("集成版本") != -1)//芯片ID
				PortOrder += ShowPassWord(ChipRfID[HandleNum], m_Result, Final_Result_Control, HandleNum);
			else if (paraArray[i].showName.Find("XX") != -1)
			{
				CString NewData;
				NewData.Format("%s%d", Ipaddress.Right(1), HandleNum);
				PortOrder.Replace("XX", NewData);
			}
			Send_Serial_Order(&Serial_Order_Return, PortOrder, HandleNum, (LPSTR)(LPCTSTR)paraArray[i].Low_Limit_Value, (LPSTR)(LPCTSTR)paraArray[i].High_Limit_Value, atoi(WaitTimeCS));
			CString Serial_Order_Return_CS_Show;

			Serial_Order_Return_CS_Show = Vaule_Return_Count_CS[HandleNum];
			LogShow_exchange(m_Result, Final_Result_Control, 5, "串口返回值:" + Serial_Order_Return_CS_Show, HandleNum);				//继续测试，状态不更新


			bool Judge_Return;																									//判断结果返回值，false为不正常
			if (Serial_Order_Return != "FAIL")
			{
				//成功后数据处理
				if (Serial_Order_Return == "DELAY_SUCCESS")																		//延时
				{
					All_Result[i] = 1;
					break; //跳出do while 
				}
				if (Serial_Order_Return == "Analysis_SUCCESS")																		//数据接收正常
				{
					Serial_Order_Return = Vaule_Return_Count_CS[HandleNum];
					Judge_Return = IMEI_Function_Judge(i, paraArray[i].showName, (LPTSTR)(LPCTSTR)Serial_Order_Return, HandleNum, m_Result, Final_Result_Control);
				}
				if (Judge_Return == true)
				{
					//结果达标
					LogShow_exchange(m_Result, Final_Result_Control, 5, "测试项目:" + paraArray[i].showName + "达标！========该项测试成功", HandleNum);			//继续测试，状态不更新
					All_Result[i] = 1;

					/*
					测试顺序
					芯片ID
					集成版本
					软件版本
					*/
					if (paraArray[i].showName.Find("软件版本") != -1)//if(paraArray[i].showName.Find("芯片ID")!=-1)
					{
						BOOL SMT_Check = FALSE;
						BOOL DBResult = Data_UpdateParaPre(IMEI_Setdlg.Mulmyado[HandleNum], HandleNum, m_Result, Final_Result_Control, &SMT_Check);
						if ((DBResult == TRUE) && (RTestChoose == FALSE))
						{
							if (COM_State[HandleNum] == TRUE)
							{

								if (paraArray[1].High_Limit_Value == "ON")
								{
									LogShow_exchange(m_Result, Final_Result_Control, 5, "保持当前串口打开", HandleNum);
								}
								else
								{
									//蓝牙整机测试的话，成功之后就直接停止
									if (GetBluetoothCheckValue == TRUE)
									{
										Sleep(2000);
										BleStopPortTest(HandleNum);
									}

									BOOL Return;
									if (hPort[HandleNum] != NULL)
										Return = CloseHandle(hPort[HandleNum]);				//测试成功后
									hPort[HandleNum] = NULL;
									if (Return == TRUE)
										LogShow_exchange(m_Result, Final_Result_Control, 5, "关闭该串口成功！！", HandleNum);
									else
									{
										LogShow_exchange(m_Result, Final_Result_Control, 5, "关闭该串口失败！！", HandleNum);
										//return FALSE; //关闭串口失败
									}
									COM_State[HandleNum] = FALSE;
								}
							}
							if (SMT_Check == TRUE)LogShow_exchange(m_Result, Final_Result_Control, 2550, "成功", HandleNum);

							if (g_ADCTFlag == FALSE)
							{
								while (CheckConnect_Thread(m_Port, m_Baud, HandleNum, m_Result, Final_Result_Control) == TRUE)//
								{
									if (StopSign[HandleNum] == TRUE)
										return FALSE;
									Sleep(300);
								}
							}

							EnableWindow_Start(HandleNum);
							LogShow_exchange(m_Result, Final_Result_Control, 256, "循环开始", HandleNum);
							return TRUE;
						}
					}
					break; //跳出do while
				}
				else
				{
					//失败返回
					Try_Time++;
					if ((paraArray[0].ifCheck == true) && (Try_Time >= Try_Time_Real))							//失败立即返回
					{
						LogShow_exchange(m_Result, Final_Result_Control, 128, "测试项目:" + paraArray[i].showName + "失败,立即停止！！！", HandleNum, "功能测试:2-TEST");
						////////////////////////////////////
						/////////////////////////////////////
						//return FALSE; //跳出整个循环for
						goto OVERDONE;//测试失败且有关闭串口选择处理
					}
					else
					{
						LogShow_exchange(m_Result, Final_Result_Control, 5, "测试项目:" + paraArray[i].showName + "失败！重试或者继续下一项目...", HandleNum);	//继续测试，状态不更新
						continue;//循环do while
					}
				}
			}
			else
			{
				//失败返回
				Try_Time++;
				if ((paraArray[0].ifCheck == true) && (Try_Time >= Try_Time_Real))//失败立即返回
				{
					LogShow_exchange(m_Result, Final_Result_Control, 128, "测试项目:" + paraArray[i].showName + "失败,立即停止！！！", HandleNum, "功能测试:2-TEST");
					////////////////////////////////////
					/////////////////////////////////////
					//return FALSE; //跳出整个循环for
					goto OVERDONE;//测试失败且有关闭串口选择处理
				}
				else
				{
					LogShow_exchange(m_Result, Final_Result_Control, 5, "测试项目:" + paraArray[i].showName + "失败！重试或者继续下一项目...", HandleNum);//继续测试，状态不更新
					continue;//循环do while
				}
			}
			////////////////////////////////////
			//虚拟串口的处理
			if (PortType_CS == "终端跳动端口")
			{
				int t = 0;
				BOOL Port_OK = FALSE;
				do
				{
					if (OnGetport() == TRUE)
					{
						//BOOL Port_OK=FALSE;
						//CString sPort;
						m_Port->GetWindowText(sPort);
						for (int num1 = 0; num1<PORTS_NUM; num1++)
						{
							if ((COM_Number[num1] == sPort) && (sPort != ""))
							{
								Port_OK = TRUE;
								break;
							}
						}
					}
					if (Port_OK == FALSE)
						Sleep(1000);


					t++;
				} while ((Port_OK == FALSE) && (t<5));

				if ((t>1) && (hPort[HandleNum] != NULL))				//串口不稳定的时候
				{
					CloseHandle(hPort[HandleNum]);				//重新打开串口
					OPen_Serial_Port(m_Port, m_Baud, HandleNum);	//串口不稳定的时候
				}
			}
			//虚拟串口的处理
			////////////////////////////////////

		} while (Try_Time<Try_Time_Real);


		Try_Time = 0;//将单项尝试次数清零
		All_Result[0] &= All_Result[i];
	}


	if (All_Result[0] == 1)
	{
		CString m_dbNew;
		//WorkStationControl.GetWindowTextA(WorkStationCS);
		//if(WorkStationCS=="")WorkStationControl.GetWindowTextA(WorkStationCS);
		if ((SIMisOK[HandleNum] != TRUE) && ((WorkStationCS != "组装测试") && (WorkStationCS != "研发测试")))
		{
			//三合一测试的时候可以先把这里注释掉
			LogShow_exchange(m_Result, Final_Result_Control, 128, "漏测SIM卡，必须测试SIM卡！！！", HandleNum, "功能测试:2-TEST");
			goto OVERDONE;//测试失败且有关闭串口选择处理
		}
		//上传数据库
		BOOL DBResult = FALSE;
		if (BGConfirmChoose == TRUE)//后台确认，同时数据库不立即上传，给到后台处理
		{
			DBResult = TRUE;
		}
		else
		{
			EnterCriticalSection(&UPDATEDB[HandleNum % (THREAD_NUM / 8)]);
			if (WorkStationCS == "SMT测试")
			{
				DBResult = Data_UpdatePara2(IMEI_Setdlg.Mulmyado[HandleNum], HandleNum, m_Result, Final_Result_Control, FALSE);
				if (DBResult == FALSE)
					DBResult = Data_UpdatePara2(IMEI_Setdlg.Mulmyado[HandleNum], HandleNum, m_Result, Final_Result_Control);
			}
			else if (WorkStationCS == "组装测试")
			{
				DBResult = Data_UpdatePara(IMEI_Setdlg.Mulmyado[HandleNum], HandleNum, m_Result, Final_Result_Control, FALSE);
				if (DBResult == FALSE)
				{
					Sleep(3000);
					DBResult = Data_UpdatePara(IMEI_Setdlg.Mulmyado[HandleNum], HandleNum, m_Result, Final_Result_Control);
				}
			}
			else if (WorkStationCS == "研发测试")
				DBResult = TRUE;
			else
			{
				LeaveCriticalSection(&UPDATEDB[HandleNum % (THREAD_NUM / 8)]);
				LogShow_exchange(m_Result, Final_Result_Control, 128, "请先选择站位！！！", HandleNum, "功能测试:2-TEST");
				goto OVERDONE;//测试失败且有关闭串口选择处理
			}
			LeaveCriticalSection(&UPDATEDB[HandleNum % (THREAD_NUM / 8)]);
		}
		//////////////////////////////////////////////
		//////////////////////////////////////////////




		if (DBResult == FALSE)
		{
			LogShow_exchange(m_Result, Final_Result_Control, 5, "DBResult==FALSE", HandleNum);
			goto OVERDONE;//测试失败且有关闭串口选择处理
		}
		else
		{
			if (COM_State[HandleNum] == TRUE)
			{

				if (paraArray[1].High_Limit_Value == "ON")
				{
					LogShow_exchange(m_Result, Final_Result_Control, 5, "保持当前串口打开", HandleNum);
				}
				else
				{
					//蓝牙整机测试的话，成功之后就直接停止
					if (GetBluetoothCheckValue == TRUE)
					{
						Sleep(2000);
						BleStopPortTest(HandleNum);
					}

					BOOL Return;
					if (hPort[HandleNum] != NULL)
						Return = CloseHandle(hPort[HandleNum]);				//测试成功后
					hPort[HandleNum] = NULL;
					if (Return == TRUE)
						LogShow_exchange(m_Result, Final_Result_Control, 5, "关闭该串口成功！！", HandleNum);
					else
					{
						LogShow_exchange(m_Result, Final_Result_Control, 5, "关闭该串口失败！！", HandleNum);
						//return FALSE; //关闭串口失败
					}
					COM_State[HandleNum] = FALSE;
				}
			}
			LogShow_exchange(m_Result, Final_Result_Control, 255, "成功", HandleNum);

			if (BGConfirmChoose == TRUE)//后台确认，而且要让当前线程结束，好开始新的线程
			{
			}
			else
			{
				if (g_ADCTFlag == FALSE)
				{
					while (CheckConnect_Thread(m_Port, m_Baud, HandleNum, m_Result, Final_Result_Control) == TRUE)//
					{
						if (StopSign[HandleNum] == TRUE)
							return FALSE;
						Sleep(300);
					}
				}
			}
			EnableWindow_Start(HandleNum);
			LogShow_exchange(m_Result, Final_Result_Control, 256, "循环开始", HandleNum);
			return TRUE;
		}
	}
	else
	{
		LogShow_exchange(m_Result, Final_Result_Control, 5, "All_Result[0]!=1", HandleNum);
		goto OVERDONE;//测试结果失败
	}
OVERDONE:
	DoorWait[HandleNum].SetEvent();
	LogShow_exchange(m_Result, Final_Result_Control, 5, "DoorWait[HandleNum].SetEvent()2", HandleNum);
	if (COM_State[HandleNum] == TRUE)
	{

		if (paraArray[1].High_Limit_Value == "ON")
		{
			LogShow_exchange(m_Result, Final_Result_Control, 5, "保持当前串口打开", HandleNum);
		}
		else
		{
			//蓝牙整机测试的话，失败之后就直接停止
			if (GetBluetoothCheckValue == TRUE)
			{
				Sleep(2000);
				BleStopPortTest(HandleNum);
			}

			BOOL Return;
			if (hPort[HandleNum] != NULL)
				Return = CloseHandle(hPort[HandleNum]);				//测试成功后
			hPort[HandleNum] = NULL;
			if (Return == TRUE)
				LogShow_exchange(m_Result, Final_Result_Control, 5, "关闭该串口成功！！", HandleNum);
			else
			{
				LogShow_exchange(m_Result, Final_Result_Control, 128, "关闭该串口失败！！", HandleNum, "功能测试:0-端口");
				//return FALSE; //关闭串口失败
			}
			COM_State[HandleNum] = FALSE;
		}
	}
	//int FailRestart=0;


	if (g_ADCTFlag == FALSE)
	{
		while (CheckConnect_Thread(m_Port, m_Baud, HandleNum, m_Result, Final_Result_Control) == TRUE)//
		{
			//if(FailRestart>10)
			if (StopSign[HandleNum] == TRUE)
				return FALSE;
			Sleep(300);
			//FailRestart++;
		}
	}
	EnableWindow_Start(HandleNum);
	LogShow_exchange(m_Result, Final_Result_Control, 256, "循环开始", HandleNum);
	return FALSE;
	//---------------------------11
}

__int64 IMEIWrite_MulAT::ComputeCD(__int64 nImei)
{
	int nTime = 0;
	__int64 nCD = 0;
	while (nImei != 0)
	{
		__int64 d = nImei % 10;
		if (nTime % 2 == 0)//将IMEI高14位的偶数位  乘以2，计算个位数和十位数之和，
		{
			d *= 2;
			if (d >= 10)
			{
				d = d % 10 + d / 10;
			}
		}
		nCD += d;   //再加上奇位数
		nTime++;
		nImei /= 10;
	}
	nCD %= 10;      //算出上述和 的个位数
	if (nCD != 0)    //该个位数是0 ，则校验位为0，否则为10-该个位数
	{
		nCD = 10 - nCD;
	}
	return nCD;
}
BOOL IMEIWrite_MulAT::CheckIMEI(CString	m_IMEI)
{
	__int64 int64IMEI = _ttoi64(m_IMEI);
	__int64 int64CD = ComputeCD(int64IMEI / 10);
	__int64 int64IMEICD = int64IMEI % 10;
	if (int64IMEICD != int64CD)
	{
		return FALSE;
	}
	return TRUE;
}

void IMEIWrite_MulAT::EnableWindow_ALL(BOOL Choose)
{
	GetDlgItem(IDC_COMBOTESTYYPE2)->EnableWindow(Choose);
	GetDlgItem(IDC_COMBOTESTYYPE3)->EnableWindow(Choose);
	GetDlgItem(IDC_COMBOTESTYYPE4)->EnableWindow(Choose);

	GetDlgItem(IDC_COMBOTESTYYPE)->EnableWindow(Choose);
	ReturnCodeControl.EnableWindow(Choose);

	WorkStationControl.EnableWindow(Choose);
	GetDlgItem(IDC_CHECK39)->EnableWindow(Choose);
	GetDlgItem(IDC_BLUETOOTH_CHECK)->EnableWindow(Choose);
	GetDlgItem(IDC_CHECK40)->EnableWindow(Choose);
	GetDlgItem(IDC_CHECK41)->EnableWindow(Choose);
	GetDlgItem(IDC_CHECK42)->EnableWindow(Choose);
	GetDlgItem(IDC_CHECK43)->EnableWindow(Choose);
	GetDlgItem(IDC_CHECK44)->EnableWindow(Choose);

	VIControlPort.EnableWindow(Choose);
	VIControlBaud.EnableWindow(Choose);
	WIFI_RSSILimit.EnableWindow(Choose);
	if (Choose == TRUE)//使能所有端口
	{
		m_Port1.EnableWindow(TRUE);
		m_Baud1.EnableWindow(TRUE);
		m_Port2.EnableWindow(TRUE);
		m_Baud2.EnableWindow(TRUE);
		m_Port3.EnableWindow(TRUE);
		m_Baud3.EnableWindow(TRUE);
		m_Port4.EnableWindow(TRUE);
		m_Baud4.EnableWindow(TRUE);
		m_Port5.EnableWindow(TRUE);
		m_Baud5.EnableWindow(TRUE);
		m_Port6.EnableWindow(TRUE);
		m_Baud6.EnableWindow(TRUE);

		m_Port7.EnableWindow(TRUE);
		m_Port8.EnableWindow(TRUE);
		m_Port9.EnableWindow(TRUE);
		m_Port10.EnableWindow(TRUE);
		m_Port11.EnableWindow(TRUE);
		m_Port12.EnableWindow(TRUE);
		m_Port13.EnableWindow(TRUE);
		m_Port14.EnableWindow(TRUE);
		m_Port15.EnableWindow(TRUE);
		m_Port16.EnableWindow(TRUE);
		m_Baud7.EnableWindow(TRUE);
		m_Baud8.EnableWindow(TRUE);
		m_Baud9.EnableWindow(TRUE);
		m_Baud10.EnableWindow(TRUE);
		m_Baud11.EnableWindow(TRUE);
		m_Baud12.EnableWindow(TRUE);
		m_Baud13.EnableWindow(TRUE);
		m_Baud14.EnableWindow(TRUE);
		m_Baud15.EnableWindow(TRUE);
		m_Baud16.EnableWindow(TRUE);

		CPUControlPort.EnableWindow(TRUE);
		CPUControlBaud.EnableWindow(TRUE);

		ScanGunPort1.EnableWindow(TRUE);
		ScanGunPort2.EnableWindow(TRUE);
		ScanGunPort3.EnableWindow(TRUE);
		ScanGunPort4.EnableWindow(TRUE);
		ScanGunPort5.EnableWindow(TRUE);
		ScanGunPort6.EnableWindow(TRUE);
		ScanGunBaud1.EnableWindow(TRUE);
		ScanGunBaud2.EnableWindow(TRUE);
		ScanGunBaud3.EnableWindow(TRUE);
		ScanGunBaud4.EnableWindow(TRUE);
		ScanGunBaud5.EnableWindow(TRUE);
		ScanGunBaud6.EnableWindow(TRUE);

		ScanGunTpye.EnableWindow(TRUE);
		PortType.EnableWindow(TRUE);
		OrderNumbersControl.EnableWindow(TRUE);
		GPS_SNRLimit.EnableWindow(TRUE);

		EnableMenu = TRUE;
	}
	else
	{
		m_Port1.EnableWindow(FALSE);
		m_Baud1.EnableWindow(FALSE);
		m_Port2.EnableWindow(FALSE);
		m_Baud2.EnableWindow(FALSE);
		m_Port3.EnableWindow(FALSE);
		m_Baud3.EnableWindow(FALSE);
		m_Port4.EnableWindow(FALSE);
		m_Baud4.EnableWindow(FALSE);
		m_Port5.EnableWindow(FALSE);
		m_Baud5.EnableWindow(FALSE);
		m_Port6.EnableWindow(FALSE);
		m_Baud6.EnableWindow(FALSE);

		m_Port7.EnableWindow(FALSE);
		m_Port8.EnableWindow(FALSE);
		m_Port9.EnableWindow(FALSE);
		m_Port10.EnableWindow(FALSE);
		m_Port11.EnableWindow(FALSE);
		m_Port12.EnableWindow(FALSE);
		m_Port13.EnableWindow(FALSE);
		m_Port14.EnableWindow(FALSE);
		m_Port15.EnableWindow(FALSE);
		m_Port16.EnableWindow(FALSE);
		m_Baud7.EnableWindow(FALSE);
		m_Baud8.EnableWindow(FALSE);
		m_Baud9.EnableWindow(FALSE);
		m_Baud10.EnableWindow(FALSE);
		m_Baud11.EnableWindow(FALSE);
		m_Baud12.EnableWindow(FALSE);
		m_Baud13.EnableWindow(FALSE);
		m_Baud14.EnableWindow(FALSE);
		m_Baud15.EnableWindow(FALSE);
		m_Baud16.EnableWindow(FALSE);

		CPUControlPort.EnableWindow(FALSE);
		CPUControlBaud.EnableWindow(FALSE);

		ScanGunPort1.EnableWindow(FALSE);
		ScanGunPort2.EnableWindow(FALSE);
		ScanGunPort3.EnableWindow(FALSE);
		ScanGunPort4.EnableWindow(FALSE);
		ScanGunPort5.EnableWindow(FALSE);
		ScanGunPort6.EnableWindow(FALSE);
		ScanGunBaud1.EnableWindow(FALSE);
		ScanGunBaud2.EnableWindow(FALSE);
		ScanGunBaud3.EnableWindow(FALSE);
		ScanGunBaud4.EnableWindow(FALSE);
		ScanGunBaud5.EnableWindow(FALSE);
		ScanGunBaud6.EnableWindow(FALSE);

		ScanGunTpye.EnableWindow(FALSE);
		PortType.EnableWindow(FALSE);
		OrderNumbersControl.EnableWindow(FALSE);
		GPS_SNRLimit.EnableWindow(FALSE);

		EnableMenu = FALSE;
	}
}
void IMEIWrite_MulAT::EnableWindow_StartALL(BOOL Choose)
{
	OnBnClickedButton2();
	OnBnClickedButton15();
	OnBnClickedButton17();
	OnBnClickedButton19();
	OnBnClickedButton21();
	OnBnClickedButton23();
	OnBnClickedButtonstop7();
	OnBnClickedButtonstop8();
	OnBnClickedButtonstop9();
	OnBnClickedButtonstop10();
	OnBnClickedButtonstop11();
	OnBnClickedButtonstop12();
	OnBnClickedButtonstop13();
	OnBnClickedButtonstop14();
	OnBnClickedButtonstop15();
	OnBnClickedButtonstop16();
	if (1)
	{
		StartA_Control.EnableWindow(Choose);
		StartB_Control.EnableWindow(Choose);
		StartC_Control.EnableWindow(Choose);
		StartD_Control.EnableWindow(Choose);
		StartE_Control.EnableWindow(Choose);
		StartF_Control.EnableWindow(Choose);
		Start7_Control.EnableWindow(Choose);
		Start8_Control.EnableWindow(Choose);
		Start9_Control.EnableWindow(Choose);
		Start10_Control.EnableWindow(Choose);
		Start11_Control.EnableWindow(Choose);
		Start12_Control.EnableWindow(Choose);
		Start13_Control.EnableWindow(Choose);
		Start14_Control.EnableWindow(Choose);
		Start15_Control.EnableWindow(Choose);
		Start16_Control.EnableWindow(Choose);
		StartALL_Control.EnableWindow(Choose);
		StopALL_Control.EnableWindow(Choose);

		GetDlgItem(IDC_BUTTON2)->EnableWindow(Choose);
		GetDlgItem(IDC_BUTTON15)->EnableWindow(Choose);
		GetDlgItem(IDC_BUTTON17)->EnableWindow(Choose);
		GetDlgItem(IDC_BUTTON19)->EnableWindow(Choose);
		GetDlgItem(IDC_BUTTON21)->EnableWindow(Choose);
		GetDlgItem(IDC_BUTTON23)->EnableWindow(Choose);

		GetDlgItem(IDC_BUTTONSTOP7)->EnableWindow(Choose);
		GetDlgItem(IDC_BUTTONSTOP8)->EnableWindow(Choose);
		GetDlgItem(IDC_BUTTONSTOP9)->EnableWindow(Choose);
		GetDlgItem(IDC_BUTTONSTOP10)->EnableWindow(Choose);
		GetDlgItem(IDC_BUTTONSTOP11)->EnableWindow(Choose);
		GetDlgItem(IDC_BUTTONSTOP12)->EnableWindow(Choose);
		GetDlgItem(IDC_BUTTONSTOP13)->EnableWindow(Choose);
		GetDlgItem(IDC_BUTTONSTOP14)->EnableWindow(Choose);
		GetDlgItem(IDC_BUTTONSTOP15)->EnableWindow(Choose);
		GetDlgItem(IDC_BUTTONSTOP16)->EnableWindow(Choose);
	}

}
void IMEIWrite_MulAT::EnableWindow_Start(int HandleNum)
{
	switch (HandleNum)
	{
	case 0:
		StartA_Control.EnableWindow(TRUE);
		break;
	case 1:
		StartB_Control.EnableWindow(TRUE);
		break;
	case 2:
		StartC_Control.EnableWindow(TRUE);
		break;
	case 3:
		StartD_Control.EnableWindow(TRUE);
		break;
	case 4:
		StartE_Control.EnableWindow(TRUE);
		break;
	case 5:
		StartF_Control.EnableWindow(TRUE);
		break;
	case 6:
		Start7_Control.EnableWindow(TRUE);
		break;
	case 7:
		Start8_Control.EnableWindow(TRUE);
		break;
	case 8:
		if (GetPicChoose != TRUE)
			Start9_Control.EnableWindow(TRUE);
		break;
	case 9:
		if (GetPicChoose != TRUE)
			Start10_Control.EnableWindow(TRUE);
		break;
	case 10:
		if (GetPicChoose != TRUE)
			Start11_Control.EnableWindow(TRUE);
		break;
	case 11:
		if (GetPicChoose != TRUE)
			Start12_Control.EnableWindow(TRUE);
		break;
	case 12:
		if (GetPicChoose != TRUE)
			Start13_Control.EnableWindow(TRUE);
		break;
	case 13:
		if (GetPicChoose != TRUE)
			Start14_Control.EnableWindow(TRUE);
		break;
	case 14:
		if (GetPicChoose != TRUE)
			Start15_Control.EnableWindow(TRUE);
		break;
	case 15:
		if (GetPicChoose != TRUE)
			Start16_Control.EnableWindow(TRUE);
		break;
	default:
		StartA_Control.EnableWindow(TRUE);
		break;
	}
}
void IMEIWrite_MulAT::EnableWindow_Choose(int HandleNum)
{
	switch (HandleNum)
	{
	case 0:
		IMEI_InputA_Control.EnableWindow(TRUE);
		IMEI_InputB_Control.EnableWindow(FALSE);
		IMEI_InputC_Control.EnableWindow(FALSE);
		IMEI_InputD_Control.EnableWindow(FALSE);
		IMEI_InputE_Control.EnableWindow(FALSE);
		IMEI_InputF_Control.EnableWindow(FALSE);
		break;
	case 1:
		IMEI_InputA_Control.EnableWindow(FALSE);
		IMEI_InputB_Control.EnableWindow(TRUE);
		IMEI_InputC_Control.EnableWindow(FALSE);
		IMEI_InputD_Control.EnableWindow(FALSE);
		IMEI_InputE_Control.EnableWindow(FALSE);
		IMEI_InputF_Control.EnableWindow(FALSE);
		break;
	case 2:
		IMEI_InputA_Control.EnableWindow(FALSE);
		IMEI_InputB_Control.EnableWindow(FALSE);
		IMEI_InputC_Control.EnableWindow(TRUE);
		IMEI_InputD_Control.EnableWindow(FALSE);
		IMEI_InputE_Control.EnableWindow(FALSE);
		IMEI_InputF_Control.EnableWindow(FALSE);
		break;
	case 3:
		IMEI_InputA_Control.EnableWindow(FALSE);
		IMEI_InputB_Control.EnableWindow(FALSE);
		IMEI_InputC_Control.EnableWindow(FALSE);
		IMEI_InputD_Control.EnableWindow(TRUE);
		IMEI_InputE_Control.EnableWindow(FALSE);
		IMEI_InputF_Control.EnableWindow(FALSE);
		break;
	case 4:
		IMEI_InputA_Control.EnableWindow(FALSE);
		IMEI_InputB_Control.EnableWindow(FALSE);
		IMEI_InputC_Control.EnableWindow(FALSE);
		IMEI_InputD_Control.EnableWindow(FALSE);
		IMEI_InputE_Control.EnableWindow(TRUE);
		IMEI_InputF_Control.EnableWindow(FALSE);
		break;
	case 5:
		IMEI_InputA_Control.EnableWindow(FALSE);
		IMEI_InputB_Control.EnableWindow(FALSE);
		IMEI_InputC_Control.EnableWindow(FALSE);
		IMEI_InputD_Control.EnableWindow(FALSE);
		IMEI_InputE_Control.EnableWindow(FALSE);
		IMEI_InputF_Control.EnableWindow(TRUE);
		break;
	case 6://回到第一个IMEI
		IMEI_InputA_Control.EnableWindow(TRUE);
		IMEI_InputB_Control.EnableWindow(FALSE);
		IMEI_InputC_Control.EnableWindow(FALSE);
		IMEI_InputD_Control.EnableWindow(FALSE);
		IMEI_InputE_Control.EnableWindow(FALSE);
		IMEI_InputF_Control.EnableWindow(FALSE);
		break;
	default:
		IMEI_InputA_Control.EnableWindow(FALSE);
		IMEI_InputB_Control.EnableWindow(FALSE);
		IMEI_InputC_Control.EnableWindow(FALSE);
		IMEI_InputD_Control.EnableWindow(FALSE);
		IMEI_InputE_Control.EnableWindow(FALSE);
		IMEI_InputF_Control.EnableWindow(FALSE);
		break;
	}
}

void IMEIWrite_MulAT::OnTimer(UINT nIDEvent)
{
	switch (nIDEvent)
	{
	case 0:
		if ((Thread_State[0] == FALSE) || (BGConfirmChoose == TRUE))
		{
			KillTimer(0);
			OnBnClickedButton1();
		}
		break;
	case 1:
		if ((Thread_State[1] == FALSE) || (BGConfirmChoose == TRUE))
		{
			KillTimer(1);
			OnBnClickedButton14();
		}
		break;
	case 2:
		if ((Thread_State[2] == FALSE) || (BGConfirmChoose == TRUE))
		{
			KillTimer(2);
			OnBnClickedButton16();
		}
		break;
	case 3:
		if ((Thread_State[3] == FALSE) || (BGConfirmChoose == TRUE))
		{
			KillTimer(3);
			OnBnClickedButton18();
		}
		break;
	case 4:
		if ((Thread_State[4] == FALSE) || (BGConfirmChoose == TRUE))
		{
			KillTimer(4);
			OnBnClickedButton20();
		}
		break;
	case 5:
		if ((Thread_State[5] == FALSE) || (BGConfirmChoose == TRUE))
		{
			KillTimer(5);
			OnBnClickedButton22();
		}
		break;

	case 6:
		if ((Thread_State[6] == FALSE) || (BGConfirmChoose == TRUE))
		{
			KillTimer(6);
			OnBnClickedButtonstart7();
		}
		break;
	case 7:
		if ((Thread_State[7] == FALSE) || (BGConfirmChoose == TRUE))
		{
			KillTimer(7);
			OnBnClickedButtonstart8();
		}
		break;
	case 8:
		if (Thread_State[8] == FALSE)
		{
			KillTimer(8);
			OnBnClickedButtonstart9();
		}
		break;
	case 9:
		if (Thread_State[9] == FALSE)
		{
			KillTimer(9);
			OnBnClickedButtonstart10();
		}
		break;
	case 10:
		if (Thread_State[10] == FALSE)
		{
			KillTimer(10);
			OnBnClickedButtonstart11();
		}
		break;
	case 11:
		if (Thread_State[11] == FALSE)
		{
			KillTimer(11);
			OnBnClickedButtonstart12();
		}
		break;
	case 12:
		if (Thread_State[12] == FALSE)
		{
			KillTimer(12);
			OnBnClickedButtonstart13();
		}
		break;
	case 13:
		if (Thread_State[13] == FALSE)
		{
			KillTimer(13);
			OnBnClickedButtonstart14();
		}
		break;
	case 14:
		if (Thread_State[14] == FALSE)
		{
			KillTimer(14);
			OnBnClickedButtonstart15();
		}
		break;
	case 15:
		if (Thread_State[15] == FALSE)
		{
			KillTimer(15);
			OnBnClickedButtonstart16();
		}
		break;

	case 30:
		KillTimer(30);
		if (TestCheck == TRUE)
		{
			Get_ScanGun_Data(&m_Result1, &Final_Result_Control1, 0, "\r");				//清除扫描枪串口数据
			SetTimer(30, 300, NULL);
			return;
		}
		else
		{
			//如果不勾选蓝牙，才按照原逻辑，否则不要让它去check扫描枪数据
			if (Get_ScanGun_Data(&m_Result1, &Final_Result_Control1, 0, "\r") == TRUE&&GetBluetoothCheckValue == FALSE)	//扫描枪有数据
			{
				if (Check_ScanGun_Data(&m_Result1, &Final_Result_Control1, 0, NULL, TRUE, 15) == TRUE)			//IMEI号
				{
					//串口扫描枪有数据了，等待串口OK
					TestCheck = TRUE;//请扫描输入IMEI！
				}
				else
				{
					SetTimer(30, 1000, NULL);
				}
			}
			else
			{
				SetTimer(30, 1000, NULL);
			}
		}
		break;
	case 31:
		KillTimer(31);
		LoadConfig_Change();
		//三合一实现自动登陆,窗口隐藏
		if (g_ADCTFlag == 1)
		{
			OnBnClickedButton6();//点击解锁
			MSG_SendSimpleMessage();//发送句柄给主控程序

			//ShowWindow(SW_HIDE);//隐藏窗口
			break;
		}
		else if (g_ADCTFlag != 1)
		{
			ShowWindow(SW_SHOWMAXIMIZED);
		}
		break;
	case 32:
		KillTimer(32);
		OnBnClickedBtnCopy();
		break;
	case 48:
		KillTimer(48);
		if (Picdlg[TimerHandleNum].DoModal() == -1)
		{
			AfxMessageBox("弹出图片失败");
		}
		break;
	case 49:
		KillTimer(49);
		Picdlg[TimerHandleNum].SendMessage(WM_CLOSE);//Picdlg[TimerHandleNum].EndDialog(0);//Picdlg[TimerHandleNum].DestroyWindow();
		break;
	}
	CDialog::OnTimer(nIDEvent);
}
void IMEIWrite_MulAT::SetFocus_Choose(int HandleNum)//自动开始
{
	if (StopAutoStart == FALSE)
		SetTimer(HandleNum, 500, NULL);
}

BOOL IMEIWrite_MulAT::PreTranslateMessage(MSG* pMsg)
{
	// TODO: Add your specialized code here and/or call the base class
	if (WM_KEYDOWN == pMsg->message)
	{
		UINT nKey = (int)pMsg->wParam;
		if (VK_RETURN == nKey || VK_ESCAPE == nKey)
		{
			//pMsg->message = WM_KILLFOCUS;
			if (VK_ESCAPE == nKey)
			{
				//m_bInputValid = FALSE;    //此时的编辑结果无效
				return TRUE;
			}

		}
	}

	return CDialog::PreTranslateMessage(pMsg);
}




BOOL CALLBACK EnumWindowsProc_1(HWND hwnd, LPARAM lparam)//列举窗口
{

	//Process::EnableDebugPrivilege(TRUE);
	char szClassName[256];
	HICON   hIcon;

	if (!IsWindowVisible(hwnd))
		return TRUE;

	//窗口是否可激活
	if (!IsWindowEnabled(hwnd))
		return TRUE;

	//窗口是否 WS_POPUP 与 WS_CAPTION 共存
	//一些可切换的窗体同时具有 WS_POPUP 与 WS_CAPTION，因而有 WS_POPUP 却无 WS_CAPTION 的应被过滤
	//据 Spy++ 观察，符合如 OneNote TrayIcon 等程序可通过此方式过滤
	LONG gwl_style = GetWindowLong(hwnd, GWL_STYLE);
	if ((gwl_style & WS_POPUP) && !(gwl_style & WS_CAPTION))
		return TRUE;

	//窗口是否具有父窗口？
	HWND hParent = (HWND)GetWindowLong(hwnd, GWL_HWNDPARENT);
	//父窗口是否可激活？
	//据 Spy++ 观察，如“运行”对话框等被应列入列表的程序有一个隐藏的，具有 WS_DISABLED 的父窗口
	/*if (IsWindowEnabled(hParent))
	return TRUE;
	//父窗口是否可视？
	if (IsWindowVisible(hParent))
	return TRUE;
	*/
	//任务栏 Shell_TrayWnd 符合上述过滤条件但是无法被过滤。
	//因而在这里单独列出。


	GetClassName(hwnd, szClassName, 255);
	if (!strcmp(szClassName, "Shell_TrayWnd"))
		return TRUE;


	wininfo infoptr;
	::GetWindowText(hwnd, infoptr.winbuf, 99);//获取窗口名称

	if (!strcmp(infoptr.winbuf, "Windows 任务管理器"))
		return TRUE;

	if (!strcmp(infoptr.winbuf, "奋斗的精灵"))
		return TRUE;

	if (infoptr.winbuf[0] == 0)
		return TRUE;

	if (strcmp(infoptr.filename, "C:\\WINDOWS\\Explorer.EXE"))//获取窗口图标句柄
	{
		hIcon = ExtractIcon((HINSTANCE)OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, infoptr.pid),
			infoptr.filename, 0);
	}
	else
	{
		hIcon = (HICON)(::GetClassLong(hwnd, GCL_HICON));//获取窗口图标句柄
	}

	infoptr.hwnd = hwnd;

	infoptr.winicon = hIcon;

	infoptr.sign = 2;

	((IMEIWrite_MulAT *)lparam)->m_wininfoarray.Add(infoptr);
	return TRUE;
}

void IMEIWrite_MulAT::Get_App_Running()//获取正在运行的应用 
{
	EnumWindows((WNDENUMPROC)EnumWindowsProc_1, (long)this);
	int size = m_wininfoarray.GetSize();
	for (int i = 0; i<size; i++)
	{
		m_currentwin = m_wininfoarray.GetAt(i);

		int Result_compare = _stricmp("多线程IMEI写号", m_currentwin.winbuf);
		if (Result_compare == 0)
		{
		}
	}
	for (int i = 0; i<size; i++)
	{
		m_currentwin = m_wininfoarray.GetAt(i);

		CString IDName;
		IDName.Format("%s", m_currentwin.winbuf);
		int Result_compare0 = IDName.Find("Printing");
		if (Result_compare0 != -1)
		{
			Save_Wait.ResetEvent();//等待模拟按键完成按键
			OnWndFocus(TRUE, TRUE, IMEI_Input);//将其前置,在新的窗口输入数据
			WaitForSingleObject(Save_Wait.m_hObject, 6000);
			OnWndFocus(FALSE, FALSE, IMEI_Input);//将其后置，
			break;
		}
	}
	for (int i = 0; i<size; i++)
	{
		m_currentwin = m_wininfoarray.GetAt(i);

		int Result_compare = _stricmp("多线程IMEI写号", m_currentwin.winbuf);
		if (Result_compare == 0)
		{
			OnWndFocus(TRUE, FALSE, IMEI_Input);//将其前置
		}
	}
	m_wininfoarray.RemoveAll();//执行后，将缓存中的清除
}

void IMEIWrite_MulAT::OnWndFocus(BOOL ShowWin, BOOL Print, CString IMEI_Print[THREAD_NUM])
{
	WINDOWPLACEMENT   ws;
	ws.length = sizeof(WINDOWPLACEMENT);
	::GetWindowPlacement(m_currentwin.hwnd, &ws);
	if (ShowWin == FALSE)
	{
		switch (ws.showCmd)
		{
		case   1://正常; 

		case   3://原始窗口为最大化	
			//::ShowWindow(m_currentwin.hwnd,SW_SHOW);			//原始
			::ShowWindow(m_currentwin.hwnd, SW_NORMAL);			//修改
			//::ShowWindow(m_currentwin.hwnd,SW_SHOW);
			//::SetForegroundWindow(m_currentwin.hwnd);
			::SetWindowPos(m_currentwin.hwnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
			/*
			hWnd：要激活的窗口句柄；
			fAltTab：是否使最小化的窗口还原。
			使用说明：
			想要将最小化的窗口还原并使其在屏幕最前，只要向fAltTab参数传入TRUE就可以了。
			*/
			//::SwitchToThisWindow(m_currentwin.hwnd,FALSE);
			break;

		case   2://原始窗口为(最)小化;
			//::ShowWindow(m_currentwin.hwnd,SW_SHOWMAXIMIZED);								//SW_SHOWMINIMIZED
			::SetWindowPos(m_currentwin.hwnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);	//新增
			break;
		default:
			::ShowWindow(m_currentwin.hwnd, SW_NORMAL);										//SW_SHOWMINIMIZED
			::SetWindowPos(m_currentwin.hwnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);	//新增
			break;
		}
	}
	else
	{
		switch (ws.showCmd)
		{
		case   1://正常; 

		case   3://原始窗口为最大化	
			::ShowWindow(m_currentwin.hwnd, SW_NORMAL);			//修改
			::SetWindowPos(m_currentwin.hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
			break;

		case   2://原始窗口为(最)小化;
			//::ShowWindow(m_currentwin.hwnd,SW_SHOWMAXIMIZED);								//SW_SHOWMINIMIZED
			::SwitchToThisWindow(m_currentwin.hwnd, TRUE);
			::SetWindowPos(m_currentwin.hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);	//新增
			break;
		default:
			::ShowWindow(m_currentwin.hwnd, SW_NORMAL);										//SW_SHOWMINIMIZED
			::SetWindowPos(m_currentwin.hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);	//新增
			break;
		}
	}
	if (Print == TRUE)
	{
		byte buf[THREAD_NUM][32] = { '\0' };
		for (int i = 0; i<THREAD_NUM; i++)
		{
			if (IMEI_Print[i] == "")
				continue;
			memcpy(buf[i], IMEI_Print[i].GetBuffer(IMEI_Print[i].GetLength()), IMEI_Print[i].GetLength());  //将cstring放入byte数组
			//模拟输入
			Sleep(500);
			for (int j = 0; j<32; j++)
			{
				if (buf[i][j] == '\0')
					break;
				//keybd_event('A', 0, 0, 0); 
				//keybd_event('A', 0, KEYEVENTF_KEYUP, 0); 
				keybd_event(buf[i][j], 0, 0, 0);
				keybd_event(buf[i][j], 0, KEYEVENTF_KEYUP, 0);
				Sleep(5);


				//keybd_event(VK_CONTROL, 0, KEYEVENTF_KEYUP, 0); 
				//keybd_event(VK_RETURN, 0, 0, 0); 
				//keybd_event(VK_RETURN, 0, KEYEVENTF_KEYUP, 0);  
			}
			keybd_event('\r', 0, 0, 0);
			keybd_event('\r', 0, KEYEVENTF_KEYUP, 0);
			keybd_event('\n', 0, 0, 0);
			keybd_event('\n', 0, KEYEVENTF_KEYUP, 0);
			Sleep(100);
		}
	}
	Save_Wait.SetEvent();
	//模拟ctrl+S保存
}


void IMEIWrite_MulAT::OnEnChangeImeia()
{
	// TODO:  在此添加控件通知处理程序代码
	//if(NETCODE_Choose_M=="true")
	{
		UpdateData(TRUE);
		UpdateData(false);//把变量显示在界面

		int IMEI_length;
		IMEI_Input[0] = "";
		if (((IMEI_InputA.Find("\r") != -1) || (IMEI_InputA.Find("\n") != -1)))//选择了IMEI1  &&IMEI_length==15
		{
			EnableWindow_Choose(-1);
			//check whether digits
			IMEI_InputA.Replace("\r", "");
			IMEI_InputA.Replace("\n", "");
			IMEI_length = IMEI_InputA.GetLength();
			////////////////////入网号判断///////////////////
			//check whether digits
			if ((IMEI_length == 0))
			{
				EnableWindow_Choose(0);
				UpdateData(FALSE);//将输入的回车清掉
				return;
			}
			//AfxMessageBox(IMEI_InputA);
			IMEI_InputA_Control.SetWindowTextA(IMEI_InputA);
			for (int i = 0; i<IMEI_length; i++)
			{
				if (IMEI_length != 15 || (IMEI_InputA.GetAt(i) <'0' || IMEI_InputA.GetAt(i) >'9'))
				{
					EnableWindow_Choose(0);
					LogShow_exchange(&m_Result1, &Final_Result_Control1, 128, "Please check IMEI_InputA. Only 15 digits allowed!\r\n", 0);
					IMEI_InputA_Control.SetFocus();
					IMEI_InputA_Control.SetSel(0, -1);
					return;
				}

			}
			if (CheckIMEI(IMEI_InputA) == FALSE)
			{
				EnableWindow_Choose(0);
				LogShow_exchange(&m_Result1, &Final_Result_Control1, 128, "IMEI_InputA 不合法", 0);
				IMEI_InputA_Control.SetFocus();
				IMEI_InputA_Control.SetSel(0, -1);
				return;
			}
			////////////////////入网号判断///////////////////
			IMEI_Input[0] = IMEI_InputA;
			LogShow_exchange(&m_Result1, &Final_Result_Control1, 0, "扫描输入IMEI:" + IMEI_Input[0] + "成功...\r\n", 0);

			OnBnClickedButton1();
		}
	}
}



UINT static __cdecl WriteIMEIFunction1(LPVOID pParam)
{
	BOOL flag;

	IMEIWrite_MulAT* Mead_Main_Win = (IMEIWrite_MulAT*)pParam;
	flag = Mead_Main_Win->WriteIMEIFunction_Thread(&(Mead_Main_Win->m_Port1), &(Mead_Main_Win->m_Baud1), 0, &(Mead_Main_Win->m_Result1), &(Mead_Main_Win->Final_Result_Control1), &(Mead_Main_Win->IMEI_InputA_Control));

	//向ADCT主控程序发送测试结果
	if (g_ADCTFlag == 1)
	{
		//如果是被强制停止了，那就直接返回，不要给主控程序发消息
		if (Mead_Main_Win->StopSign[0] == TRUE)
		{
			return 0;
		}

		CString str_Port;
		Mead_Main_Win->GetDlgItemText(IDC_COMBO1, str_Port);
		if (flag == TRUE)
		{
			Mead_Main_Win->SendADCTTestResult(str_Port, "1");
		}
		else if (flag == FALSE)
		{
			Mead_Main_Win->SendADCTTestResult(str_Port, "0");
		}
		Mead_Main_Win->OnBnClickedButton2();
	}
	return 0;
}

void IMEIWrite_MulAT::OnBnClickedButton1()
{
	// TODO: 在此添加控件通知处理程序代码
	StopSign[0] = FALSE;
	StartA_Control.EnableWindow(FALSE);
	Thread_Handle[0] = AfxBeginThread(WriteIMEIFunction1, (LPVOID)this, THREAD_PRIORITY_NORMAL, 0, 0, NULL);
	SetTimer(32, 5000, NULL);
}

void IMEIWrite_MulAT::OnBnClickedButton2()
{
	// TODO: 在此添加控件通知处理程序代码
	int HandleNum = 0;
	LeaveCriticalSection(&WIFICOMMUNICATE[HandleNum]);
	KillTimer(32);
	StopSign[0] = TRUE;
	Sleep(100);
	int i = 0;
	while (Thread_State[0] == TRUE)
	{
		Sleep(STOPDELAY);
		i++;
		if (i>1)
			break;
	};
	//COM_State[0]=FALSE;
	LogShow_exchange(&m_Result1, &Final_Result_Control1, 250, "结束测试！！", 0);

	int PortHandle = 0;

	//停止时如果存在蓝牙连接就给蓝牙发送断开指令
	if (GetBluetoothCheckValue == TRUE&&BleGetSettingFlag[PortHandle] == 1)
	{
		BleGetSettingFlag[PortHandle] = 0;
		BluetoothDisconnect(PortHandle);
	}

	if (0)
	{
		BOOL Return = CloseHandle(hPort[PortHandle]);				//测试成功后
		if (Return == TRUE)
		{
		}
		else
		{
			AfxMessageBox("关闭失败！");												//关闭串口失败
			return;
		}
		hPort[PortHandle] = NULL;
	}

	//停止时删除占用队列中对应的端口号
	if (GetBluetoothCheckValue == TRUE || g_ADCTFlag == 1)
	{
		DeleteComoOccupancydDeq(0);
	}
}


//线程2

void IMEIWrite_MulAT::OnEnChangeImeia2()
{
	//if(NETCODE_Choose_M=="true")
	{
		UpdateData(TRUE);
		UpdateData(false);//把变量显示在界面

		int IMEI_length;
		IMEI_Input[1] = "";
		if (((IMEI_InputB.Find("\r") != -1) || (IMEI_InputB.Find("\n") != -1)))//选择了IMEI1  &&IMEI_length==15
		{
			EnableWindow_Choose(-1);
			//check whether digits
			IMEI_InputB.Replace("\r", "");
			IMEI_InputB.Replace("\n", "");
			IMEI_length = IMEI_InputB.GetLength();
			////////////////////入网号判断///////////////////
			//check whether digits
			if ((IMEI_length == 0))
			{
				EnableWindow_Choose(1);
				UpdateData(FALSE);//将输入的回车清掉
				return;
			}
			IMEI_InputB_Control.SetWindowTextA(IMEI_InputB);
			for (int i = 0; i<IMEI_length; i++)
			{
				if (IMEI_length != 15 || (IMEI_InputB.GetAt(i) <'0' || IMEI_InputB.GetAt(i) >'9'))
				{
					EnableWindow_Choose(1);
					LogShow_exchange(&m_Result2, &Final_Result_Control2, 128, "Please check IMEI_InputB. Only 15 digits allowed!\r\n", 1);
					IMEI_InputB_Control.SetFocus();
					IMEI_InputB_Control.SetSel(0, -1);
					return;
				}

			}
			if (CheckIMEI(IMEI_InputB) == FALSE)
			{
				EnableWindow_Choose(1);
				LogShow_exchange(&m_Result2, &Final_Result_Control2, 128, "IMEI_InputA 不合法", 1);
				IMEI_InputB_Control.SetFocus();
				IMEI_InputB_Control.SetSel(0, -1);
				return;
			}
			////////////////////入网号判断///////////////////
			IMEI_Input[1] = IMEI_InputB;
			LogShow_exchange(&m_Result2, &Final_Result_Control2, 0, "扫描输入IMEI:" + IMEI_Input[1] + "成功...\r\n", 1);

			OnBnClickedButton14();
		}
	}
}


UINT static __cdecl WriteIMEIFunction2(LPVOID pParam)
{
	BOOL flag;

	IMEIWrite_MulAT* Mead_Main_Win = (IMEIWrite_MulAT*)pParam;
	flag=Mead_Main_Win->WriteIMEIFunction_Thread(&(Mead_Main_Win->m_Port2), &(Mead_Main_Win->m_Baud2), 1, &(Mead_Main_Win->m_Result2), &(Mead_Main_Win->Final_Result_Control2), &(Mead_Main_Win->IMEI_InputB_Control));


	//向ADCT主控程序发送测试结果
	if (g_ADCTFlag == 1)
	{
		//如果是被强制停止了，那就直接返回，不要给主控程序发消息
		if (Mead_Main_Win->StopSign[1] == TRUE)
		{
			return 0;
		}

		CString str_Port;
		Mead_Main_Win->GetDlgItemText(IDC_COMBO4, str_Port);
		if (flag == TRUE)
		{
			Mead_Main_Win->SendADCTTestResult(str_Port, "1");
		}
		else if (flag == FALSE)
		{
			Mead_Main_Win->SendADCTTestResult(str_Port, "0");
		}
	}
	return 0;
}

void IMEIWrite_MulAT::OnBnClickedButton14()
{
	// TODO: 在此添加控件通知处理程序代码
	StopSign[1] = FALSE;
	StartB_Control.EnableWindow(FALSE);
	Thread_Handle[1] = AfxBeginThread(WriteIMEIFunction2, (LPVOID)this, THREAD_PRIORITY_NORMAL, 0, 0, NULL);
}

void IMEIWrite_MulAT::OnBnClickedButton15()
{
	// TODO: 在此添加控件通知处理程序代码
	int HandleNum = 1;
	LeaveCriticalSection(&WIFICOMMUNICATE[HandleNum]);
	StopSign[1] = TRUE;
	Sleep(100);
	int i = 0;
	while (Thread_State[1] == TRUE)
	{
		Sleep(STOPDELAY);
		i++;
		if (i>1)
			break;
	};
	//COM_State[1]=FALSE;
	LogShow_exchange(&m_Result2, &Final_Result_Control2, 250, "结束测试！！", 1);

	int PortHandle = 1;

	//停止时如果存在蓝牙连接就给蓝牙发送断开指令
	if (GetBluetoothCheckValue == TRUE&&BleGetSettingFlag[PortHandle] == 1)
	{
		BleGetSettingFlag[PortHandle] = 0;
		BluetoothDisconnect(PortHandle);
	}

	if (0)
	{
		BOOL Return = CloseHandle(hPort[PortHandle]);				//测试成功后
		if (Return == TRUE)
		{
		}
		else
		{
			AfxMessageBox("关闭失败！");												//关闭串口失败
			return;
		}
		hPort[PortHandle] = NULL;
	}
	//停止时删除占用队列中对应的端口号
	if (GetBluetoothCheckValue == TRUE || g_ADCTFlag == 1)
	{
		DeleteComoOccupancydDeq(1);
	}
}

void IMEIWrite_MulAT::OnEnChangeImeia3()
{
	//if(NETCODE_Choose_M=="true")
	{
		UpdateData(TRUE);
		UpdateData(false);//把变量显示在界面

		int IMEI_length;
		IMEI_Input[2] = "";
		if (((IMEI_InputC.Find("\r") != -1) || (IMEI_InputC.Find("\n") != -1)))//选择了IMEI1  &&IMEI_length==15
		{
			EnableWindow_Choose(-1);
			//check whether digits
			IMEI_InputC.Replace("\r", "");
			IMEI_InputC.Replace("\n", "");
			IMEI_length = IMEI_InputC.GetLength();
			////////////////////入网号判断///////////////////
			//check whether digits
			if ((IMEI_length == 0))
			{
				EnableWindow_Choose(2);
				UpdateData(FALSE);//将输入的回车清掉
				return;
			}
			IMEI_InputC_Control.SetWindowTextA(IMEI_InputC);
			for (int i = 0; i<IMEI_length; i++)
			{
				if (IMEI_length != 15 || (IMEI_InputC.GetAt(i) <'0' || IMEI_InputC.GetAt(i) >'9'))
				{
					EnableWindow_Choose(2);
					LogShow_exchange(&m_Result3, &Final_Result_Control3, 128, "Please check IMEI_InputC. Only 15 digits allowed!\r\n", 2);
					IMEI_InputC_Control.SetFocus();
					IMEI_InputC_Control.SetSel(0, -1);
					return;
				}

			}
			if (CheckIMEI(IMEI_InputC) == FALSE)
			{
				EnableWindow_Choose(2);
				LogShow_exchange(&m_Result3, &Final_Result_Control3, 128, "IMEI_InputC 不合法", 2);
				IMEI_InputC_Control.SetFocus();
				IMEI_InputC_Control.SetSel(0, -1);
				return;
			}
			////////////////////入网号判断///////////////////
			IMEI_Input[2] = IMEI_InputC;
			LogShow_exchange(&m_Result3, &Final_Result_Control3, 0, "扫描输入IMEI:" + IMEI_Input[2] + "成功...\r\n", 2);

			OnBnClickedButton16();
		}
	}
}

UINT static __cdecl WriteIMEIFunction3(LPVOID pParam)
{
	BOOL flag;

	IMEIWrite_MulAT* Mead_Main_Win = (IMEIWrite_MulAT*)pParam;
	flag = Mead_Main_Win->WriteIMEIFunction_Thread(&(Mead_Main_Win->m_Port3), &(Mead_Main_Win->m_Baud3), 2, &(Mead_Main_Win->m_Result3), &(Mead_Main_Win->Final_Result_Control3), &(Mead_Main_Win->IMEI_InputC_Control));


	//向ADCT主控程序发送测试结果
	if (g_ADCTFlag == 1)
	{
		//如果是被强制停止了，那就直接返回，不要给主控程序发消息
		if (Mead_Main_Win->StopSign[0] == TRUE)
		{
			return 0;
		}

		CString str_Port;
		Mead_Main_Win->GetDlgItemText(IDC_COMBO6, str_Port);
		if (flag == TRUE)
		{
			Mead_Main_Win->SendADCTTestResult(str_Port, "1");
		}
		else if (flag == FALSE)
		{
			Mead_Main_Win->SendADCTTestResult(str_Port, "0");
		}
	}
	return 0;
}
void IMEIWrite_MulAT::OnBnClickedButton16()
{
	// TODO: 在此添加控件通知处理程序代码
	StopSign[2] = FALSE;
	StartC_Control.EnableWindow(FALSE);
	Thread_Handle[2] = AfxBeginThread(WriteIMEIFunction3, (LPVOID)this, THREAD_PRIORITY_NORMAL, 0, 0, NULL);
}

void IMEIWrite_MulAT::OnBnClickedButton17()
{
	// TODO: 在此添加控件通知处理程序代码
	int HandleNum = 2;
	LeaveCriticalSection(&WIFICOMMUNICATE[HandleNum]);
	StopSign[2] = TRUE;
	Sleep(100);
	int i = 0;
	while (Thread_State[2] == TRUE)
	{
		Sleep(STOPDELAY);
		i++;
		if (i>1)
			break;
	};
	//COM_State[2]=FALSE;
	LogShow_exchange(&m_Result3, &Final_Result_Control3, 250, "结束测试！！", 2);

	int PortHandle = 2;

	//停止时如果存在蓝牙连接就给蓝牙发送断开指令
	if (GetBluetoothCheckValue == TRUE&&BleGetSettingFlag[PortHandle] == 1)
	{
		BleGetSettingFlag[PortHandle] = 0;
		BluetoothDisconnect(PortHandle);
	}

	if (0)
	{
		BOOL Return = CloseHandle(hPort[PortHandle]);				//测试成功后
		if (Return == TRUE)
		{
		}
		else
		{
			AfxMessageBox("关闭失败！");												//关闭串口失败
			return;
		}
		hPort[PortHandle] = NULL;
	}

	//停止时删除占用队列中对应的端口号
	if (GetBluetoothCheckValue == TRUE || g_ADCTFlag == 1)
	{
		DeleteComoOccupancydDeq(2);
	}
}

void IMEIWrite_MulAT::OnEnChangeImeia4()
{
	//if(NETCODE_Choose_M=="true")
	{
		UpdateData(TRUE);
		UpdateData(false);//把变量显示在界面

		int IMEI_length;
		IMEI_Input[3] = "";
		if (((IMEI_InputD.Find("\r") != -1) || (IMEI_InputD.Find("\n") != -1)))//选择了IMEI1  &&IMEI_length==15
		{
			EnableWindow_Choose(-1);
			//check whether digits
			IMEI_InputD.Replace("\r", "");
			IMEI_InputD.Replace("\n", "");
			IMEI_length = IMEI_InputD.GetLength();
			////////////////////入网号判断///////////////////
			//check whether digits
			if ((IMEI_length == 0))
			{
				EnableWindow_Choose(3);
				UpdateData(FALSE);//将输入的回车清掉
				return;
			}
			IMEI_InputD_Control.SetWindowTextA(IMEI_InputD);
			for (int i = 0; i<IMEI_length; i++)
			{
				if (IMEI_length != 15 || (IMEI_InputD.GetAt(i) <'0' || IMEI_InputD.GetAt(i) >'9'))
				{
					EnableWindow_Choose(3);
					LogShow_exchange(&m_Result4, &Final_Result_Control4, 128, "Please check IMEI_InputD. Only 15 digits allowed!\r\n", 3);
					IMEI_InputD_Control.SetFocus();
					IMEI_InputD_Control.SetSel(0, -1);
					return;
				}

			}
			if (CheckIMEI(IMEI_InputD) == FALSE)
			{
				EnableWindow_Choose(3);
				LogShow_exchange(&m_Result4, &Final_Result_Control4, 128, "IMEI_InputD 不合法", 3);
				IMEI_InputD_Control.SetFocus();
				IMEI_InputD_Control.SetSel(0, -1);
				return;
			}
			////////////////////入网号判断///////////////////
			IMEI_Input[3] = IMEI_InputD;
			LogShow_exchange(&m_Result4, &Final_Result_Control4, 0, "扫描输入IMEI:" + IMEI_Input[3] + "成功...\r\n", 3);

			OnBnClickedButton18();
		}
	}
}


UINT static __cdecl WriteIMEIFunction4(LPVOID pParam)
{
	BOOL flag;

	IMEIWrite_MulAT* Mead_Main_Win = (IMEIWrite_MulAT*)pParam;
	flag = Mead_Main_Win->WriteIMEIFunction_Thread(&(Mead_Main_Win->m_Port4), &(Mead_Main_Win->m_Baud4), 3, &(Mead_Main_Win->m_Result4), &(Mead_Main_Win->Final_Result_Control4), &(Mead_Main_Win->IMEI_InputD_Control));


	//向ADCT主控程序发送测试结果
	if (g_ADCTFlag == 1)
	{
		//如果是被强制停止了，那就直接返回，不要给主控程序发消息
		if (Mead_Main_Win->StopSign[0] == TRUE)
		{
			return 0;
		}

		CString str_Port;
		Mead_Main_Win->GetDlgItemText(IDC_COMBO8, str_Port);
		if (flag == TRUE)
		{
			Mead_Main_Win->SendADCTTestResult(str_Port, "1");
		}
		else if (flag == FALSE)
		{
			Mead_Main_Win->SendADCTTestResult(str_Port, "0");
		}
	}
	return 0;
}

void IMEIWrite_MulAT::OnBnClickedButton18()
{
	// TODO: 在此添加控件通知处理程序代码
	StopSign[3] = FALSE;
	StartD_Control.EnableWindow(FALSE);
	Thread_Handle[3] = AfxBeginThread(WriteIMEIFunction4, (LPVOID)this, THREAD_PRIORITY_NORMAL, 0, 0, NULL);
}

void IMEIWrite_MulAT::OnBnClickedButton19()
{
	// TODO: 在此添加控件通知处理程序代码
	int HandleNum = 3;
	LeaveCriticalSection(&WIFICOMMUNICATE[HandleNum]);
	StopSign[3] = TRUE;
	Sleep(100);
	int i = 0;
	while (Thread_State[3] == TRUE)
	{
		Sleep(STOPDELAY);
		i++;
		if (i>1)
			break;
	};
	//COM_State[3]=FALSE;
	LogShow_exchange(&m_Result4, &Final_Result_Control4, 250, "结束测试！！", 3);

	int PortHandle = 3;

	//停止时如果存在蓝牙连接就给蓝牙发送断开指令
	if (GetBluetoothCheckValue == TRUE&&BleGetSettingFlag[PortHandle] == 1)
	{
		BleGetSettingFlag[PortHandle] = 0;
		BluetoothDisconnect(PortHandle);
	}

	if (0)
	{
		BOOL Return = CloseHandle(hPort[PortHandle]);				//测试成功后
		if (Return == TRUE)
		{
		}
		else
		{
			AfxMessageBox("关闭失败！");												//关闭串口失败
			return;
		}
		hPort[PortHandle] = NULL;
	}

	//停止时删除占用队列中对应的端口号
	if (GetBluetoothCheckValue == TRUE || g_ADCTFlag == 1)
	{
		DeleteComoOccupancydDeq(3);
	}
}

void IMEIWrite_MulAT::OnEnChangeImeia5()
{
	//if(NETCODE_Choose_M=="true")
	{
		UpdateData(TRUE);
		UpdateData(false);//把变量显示在界面

		int IMEI_length;
		IMEI_Input[4] = "";
		if (((IMEI_InputE.Find("\r") != -1) || (IMEI_InputE.Find("\n") != -1)))//选择了IMEI1  &&IMEI_length==15
		{
			EnableWindow_Choose(-1);
			//check whether digits
			IMEI_InputE.Replace("\r", "");
			IMEI_InputE.Replace("\n", "");
			IMEI_length = IMEI_InputE.GetLength();
			////////////////////入网号判断///////////////////
			//check whether digits
			if ((IMEI_length == 0))
			{
				EnableWindow_Choose(4);
				UpdateData(FALSE);//将输入的回车清掉
				return;
			}
			IMEI_InputE_Control.SetWindowTextA(IMEI_InputE);
			for (int i = 0; i<IMEI_length; i++)
			{
				if (IMEI_length != 15 || (IMEI_InputE.GetAt(i) <'0' || IMEI_InputE.GetAt(i) >'9'))
				{
					EnableWindow_Choose(4);
					LogShow_exchange(&m_Result5, &Final_Result_Control5, 128, "Please check IMEI_InputE. Only 15 digits allowed!\r\n", 4);
					IMEI_InputE_Control.SetFocus();
					IMEI_InputE_Control.SetSel(0, -1);
					return;
				}

			}
			if (CheckIMEI(IMEI_InputD) == FALSE)
			{
				EnableWindow_Choose(4);
				LogShow_exchange(&m_Result5, &Final_Result_Control5, 128, "IMEI_InputE 不合法", 4);
				IMEI_InputE_Control.SetFocus();
				IMEI_InputE_Control.SetSel(0, -1);
				return;
			}
			////////////////////入网号判断///////////////////
			IMEI_Input[4] = IMEI_InputE;
			LogShow_exchange(&m_Result5, &Final_Result_Control5, 0, "扫描输入IMEI:" + IMEI_Input[4] + "成功...\r\n", 4);

			OnBnClickedButton20();
		}
	}
}


UINT static __cdecl WriteIMEIFunction5(LPVOID pParam)
{
	IMEIWrite_MulAT* Mead_Main_Win = (IMEIWrite_MulAT*)pParam;
	Mead_Main_Win->WriteIMEIFunction_Thread(&(Mead_Main_Win->m_Port5), &(Mead_Main_Win->m_Baud5), 4, &(Mead_Main_Win->m_Result5), &(Mead_Main_Win->Final_Result_Control5), &(Mead_Main_Win->IMEI_InputE_Control));
	return 0;
}

void IMEIWrite_MulAT::OnBnClickedButton20()
{
	// TODO: 在此添加控件通知处理程序代码
	StopSign[4] = FALSE;
	StartE_Control.EnableWindow(FALSE);
	Thread_Handle[4] = AfxBeginThread(WriteIMEIFunction5, (LPVOID)this, THREAD_PRIORITY_NORMAL, 0, 0, NULL);
}

void IMEIWrite_MulAT::OnBnClickedButton21()
{
	// TODO: 在此添加控件通知处理程序代码
	int HandleNum = 4;
	LeaveCriticalSection(&WIFICOMMUNICATE[HandleNum]);
	StopSign[4] = TRUE;
	Sleep(100);
	int i = 0;
	while (Thread_State[4] == TRUE)
	{
		Sleep(STOPDELAY);
		i++;
		if (i>1)
			break;
	};
	//COM_State[4]=FALSE;
	LogShow_exchange(&m_Result5, &Final_Result_Control5, 250, "结束测试！！", 4);

	int PortHandle = 4;

	//停止时如果存在蓝牙连接就给蓝牙发送断开指令
	if (GetBluetoothCheckValue == TRUE&&BleGetSettingFlag[PortHandle] == 1)
	{
		BleGetSettingFlag[PortHandle] = 0;
		BluetoothDisconnect(PortHandle);
	}

	if (0)
	{
		BOOL Return = CloseHandle(hPort[PortHandle]);				//测试成功后
		if (Return == TRUE)
		{
		}
		else
		{
			AfxMessageBox("关闭失败！");												//关闭串口失败
			return;
		}
		hPort[PortHandle] = NULL;
	}

	//停止时删除占用队列中对应的端口号
	if (GetBluetoothCheckValue == TRUE || g_ADCTFlag == 1)
	{
		DeleteComoOccupancydDeq(4);
	}
}

void IMEIWrite_MulAT::OnEnChangeImeia6()
{
	//if(NETCODE_Choose_M=="true")
	{
		UpdateData(TRUE);
		UpdateData(false);//把变量显示在界面

		int IMEI_length;
		IMEI_Input[5] = "";
		if (((IMEI_InputF.Find("\r") != -1) || (IMEI_InputF.Find("\n") != -1)))//选择了IMEI1  &&IMEI_length==15
		{
			EnableWindow_Choose(-1);
			//check whether digits
			IMEI_InputF.Replace("\r", "");
			IMEI_InputF.Replace("\n", "");
			IMEI_length = IMEI_InputF.GetLength();
			////////////////////入网号判断///////////////////
			//check whether digits
			if ((IMEI_length == 0))
			{
				EnableWindow_Choose(5);
				UpdateData(FALSE);//将输入的回车清掉
				return;
			}
			IMEI_InputF_Control.SetWindowTextA(IMEI_InputF);
			for (int i = 0; i<IMEI_length; i++)
			{
				if (IMEI_length != 15 || (IMEI_InputF.GetAt(i) <'0' || IMEI_InputF.GetAt(i) >'9'))
				{
					EnableWindow_Choose(5);
					LogShow_exchange(&m_Result6, &Final_Result_Control6, 128, "Please check IMEI_InputF. Only 15 digits allowed!\r\n", 5);
					IMEI_InputF_Control.SetFocus();
					IMEI_InputF_Control.SetSel(0, -1);
					return;
				}

			}
			if (CheckIMEI(IMEI_InputD) == FALSE)
			{
				EnableWindow_Choose(5);
				LogShow_exchange(&m_Result6, &Final_Result_Control6, 128, "IMEI_InputF 不合法", 5);
				IMEI_InputF_Control.SetFocus();
				IMEI_InputF_Control.SetSel(0, -1);
				return;
			}
			////////////////////入网号判断///////////////////
			IMEI_Input[5] = IMEI_InputF;
			LogShow_exchange(&m_Result6, &Final_Result_Control6, 0, "扫描输入IMEI:" + IMEI_Input[5] + "成功...\r\n", 5);

			OnBnClickedButton22();
		}
	}
}

UINT static __cdecl WriteIMEIFunction6(LPVOID pParam)
{
	IMEIWrite_MulAT* Mead_Main_Win = (IMEIWrite_MulAT*)pParam;
	Mead_Main_Win->WriteIMEIFunction_Thread(&(Mead_Main_Win->m_Port6), &(Mead_Main_Win->m_Baud6), 5, &(Mead_Main_Win->m_Result6), &(Mead_Main_Win->Final_Result_Control6), &(Mead_Main_Win->IMEI_InputF_Control));
	return 0;
}

void IMEIWrite_MulAT::OnBnClickedButton22()
{
	// TODO: 在此添加控件通知处理程序代码
	StopSign[5] = FALSE;
	StartF_Control.EnableWindow(FALSE);
	Thread_Handle[5] = AfxBeginThread(WriteIMEIFunction6, (LPVOID)this, THREAD_PRIORITY_NORMAL, 0, 0, NULL);
}

void IMEIWrite_MulAT::OnBnClickedButton23()
{
	// TODO: 在此添加控件通知处理程序代码
	int HandleNum = 5;
	LeaveCriticalSection(&WIFICOMMUNICATE[HandleNum]);
	StopSign[5] = TRUE;
	Sleep(100);
	int i = 0;
	while (Thread_State[5] == TRUE)
	{
		Sleep(STOPDELAY);
		i++;
		if (i>1)
			break;
	};
	//COM_State[5]=FALSE;
	LogShow_exchange(&m_Result6, &Final_Result_Control6, 250, "结束测试！！", 5);

	int PortHandle = 5;

	//停止时如果存在蓝牙连接就给蓝牙发送断开指令
	if (GetBluetoothCheckValue == TRUE&&BleGetSettingFlag[PortHandle] == 1)
	{
		BleGetSettingFlag[PortHandle] = 0;
		BluetoothDisconnect(PortHandle);
	}

	if (0)
	{
		BOOL Return = CloseHandle(hPort[PortHandle]);				//测试成功后
		if (Return == TRUE)
		{
		}
		else
		{
			AfxMessageBox("关闭失败！");												//关闭串口失败
			return;
		}
		hPort[PortHandle] = NULL;
	}

	//停止时删除占用队列中对应的端口号
	if (GetBluetoothCheckValue == TRUE || g_ADCTFlag == 1)
	{
		DeleteComoOccupancydDeq(5);
	}
}

void IMEIWrite_MulAT::OnBnClickedButton6()
{
	// TODO: 在此添加控件通知处理程序代码
	CLogin2 dig2;
	dig2.LANDING_OK = FALSE;
	dig2.DoModal();
	if (dig2.LANDING_OK == TRUE)
	{
		EnableWindow_ALL(TRUE);
	}
}

void IMEIWrite_MulAT::OnBnClickedButton7()
{
	//ShowPassWord("0F 88 BE 55 21 D4 DB ED D3 75 13 46 67 28 BF 63",&m_Result1,&Final_Result_Control1,0);
	//ShowPassWord("0F88BE5521D4DBEDD37513466728BF63",&m_Result1,&Final_Result_Control1,0);
	// TODO: 在此添加控件通知处理程序代码
	EnableWindow_ALL(FALSE);
	IMEI_InputA_Control.SetWindowTextA("");
	IMEI_InputB_Control.SetWindowTextA("");
	IMEI_InputC_Control.SetWindowTextA("");
	IMEI_InputD_Control.SetWindowTextA("");
	IMEI_InputE_Control.SetWindowTextA("");
	IMEI_InputF_Control.SetWindowTextA("");

	CString ScanGunTpyeCS;
	ScanGunTpye.GetWindowTextA(ScanGunTpyeCS);
	EnableWindow_Choose(-1);
	SaveConfig_Change();
}



//蓝牙整机测试开始连接的线程函数
UINT static __cdecl TH_BleStartConnect_Control(LPVOID pParam)
{
	CString StrMac;
	IMEIWrite_MulAT* Mead_Main_Win = (IMEIWrite_MulAT*)pParam;

	Mead_Main_Win->GetDlgItemText(IDC_SCANDATA_EDIT, StrMac);
	Mead_Main_Win->BluetoothCoreFunction(StrMac);

	return 0;
}

/*扫描枪模块*/

BOOL IMEIWrite_MulAT::OPen_ScanGun_Port(CComboBox* m_Port, CComboBox* m_Baud, int HandleNum)		//打开扫描枪端口
{
	CString sPort, sBaud;
	int port, baud;
	//get port
	m_Port->GetWindowText(sPort);
	sPort = sPort.Right(sPort.GetLength() - 3);
	port = atoi(sPort);
	sPort.Format(_T("\\\\.\\COM%d"), port);

	//get baud
	m_Baud->GetWindowText(sBaud);
	baud = atoi(sBaud);

	//open com port打开端口
	hScanGun[HandleNum] = CreateFile(sPort, GENERIC_READ | GENERIC_WRITE, 0, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hScanGun[HandleNum] == INVALID_HANDLE_VALUE)
	{
		CString csTemp;
		m_Port->GetWindowText(csTemp);
		//AfxMessageBox("Can't open "+csTemp);
		hScanGun[HandleNum] = NULL;
		return FALSE;
	}

	//config the com port
	DCB dcb;
	dcb.DCBlength = sizeof(DCB);
	GetCommState(hScanGun[HandleNum], &dcb);
	SetupComm(hScanGun[HandleNum], 4096, 4096);
	PurgeComm(hScanGun[HandleNum], PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);
	dcb.Parity = NOPARITY;
	dcb.ByteSize = 8;
	dcb.StopBits = ONESTOPBIT;
	dcb.BaudRate = baud;															//57600(MT6205B), 115200 (MT6218B)			
	dcb.fBinary = TRUE;
	dcb.fParity = FALSE;
	dcb.fOutxCtsFlow = FALSE;
	dcb.fOutxDsrFlow = FALSE;
	dcb.fDtrControl = DTR_CONTROL_DISABLE;
	dcb.fDsrSensitivity = FALSE;
	dcb.fTXContinueOnXoff = FALSE;
	dcb.fOutX = FALSE;
	dcb.fInX = FALSE;
	dcb.fErrorChar = FALSE;
	dcb.fNull = FALSE;
	dcb.fRtsControl = RTS_CONTROL_ENABLE;
	dcb.fAbortOnError = FALSE;
	dcb.XonChar = 0;
	dcb.XoffChar = 0;
	dcb.ErrorChar = 0;
	dcb.EofChar = 0;
	dcb.EvtChar = 0;
	SetCommState(hScanGun[HandleNum], &dcb);

	//set time out struct
	COMMTIMEOUTS timeouts;
	timeouts.ReadIntervalTimeout = 0xFFFFFFFF;
	timeouts.ReadTotalTimeoutMultiplier = 0;
	timeouts.ReadTotalTimeoutConstant = 0;//OPen_ScanGun_Port
	timeouts.WriteTotalTimeoutMultiplier = 0;
	timeouts.WriteTotalTimeoutConstant = 2000;
	SetCommTimeouts(hScanGun[HandleNum], &timeouts);

	//SetCommMask(hScanGun[HandleNum], EV_RXCHAR|EV_TXEMPTY );							//设置事件驱动的类型

	//read and the com port
	BOOL bReadStatus;
	DWORD dwBytesRead;
	char buffer[256];

	Sleep(100);
	bReadStatus = ReadFile(hScanGun[HandleNum], buffer, 256, &dwBytesRead, NULL);		//make sure rx data cleaned

	return TRUE;																		//返回TRUE，打开串口正常
}


BOOL IMEIWrite_MulAT::Get_ScanGun_Data(CEdit* m_Result, CEdit* Final_Result_Control, int HandleNum, char* EndSign)		//获取扫描枪端口数据
{
	BOOL bReadStatus;
	DWORD dwBytesRead;
	char *buffer;
	char *p = NULL;
	char buf[1024];

	//read and the com port
	CString Port_Temp = "";//串口读取数据 的缓存
	for (int r = 0; r<2; r++)
	{
		memset(buf, 0, sizeof(buf));
		buffer = buf;
		bReadStatus = ReadFile(hScanGun[HandleNum], buffer, 512, &dwBytesRead, NULL);
		if (dwBytesRead != 0)
		{
			if (strstr(EndSign, "NULL"))
				p = strstr(buffer, "\r");
			else
				p = strstr(buffer, EndSign);
			if (p)
			{
				CString strCommand_Vaule_Return, MAC;//串口返回值
				strCommand_Vaule_Return = Port_Temp + CString(buffer);

				CString  selPort;
				selPort = strCommand_Vaule_Return;
				selPort.Replace("\r", "");
				selPort.Replace("\n", "");
				//获取参数的个数
				CString string_sn_r;
				int npos = selPort.Find('"');
				if (npos >= 0)
				{
					string_sn_r = selPort.Mid(npos + 1);
					Vaule_ScanGun[HandleNum] = string_sn_r.Left(string_sn_r.Find('"'));
				}
				else
				{
					Vaule_ScanGun[HandleNum] = selPort;
				}


				//GetBluetoothCheckValue指是否勾选蓝牙,MacScanEnbleFlag指是否可扫入MAC地址
				SetDlgItemText(IDC_SCANDATA_EDIT, Vaule_ScanGun[HandleNum].Right(MacIntercept));

				if (GetBluetoothCheckValue == TRUE&&MacScanEnbleFlag == FALSE)
				{
					Vaule_ScanGun[HandleNum] = "";
					return TRUE;
				}
				else if (GetBluetoothCheckValue == TRUE&&MacScanEnbleFlag == TRUE&&BLEGetSettingFlag == 1)
				{
					AfxBeginThread(TH_BleStartConnect_Control, (LPVOID)this, THREAD_PRIORITY_NORMAL, 0, 0, NULL);

					//GetDlgItemText(IDC_SCANDATA_EDIT, MAC);
					//BluetoothCoreFunction(MAC);
				}



				//LogShow_exchange(m_Result,Final_Result_Control,5,"成功获取扫描收\r\n"+Vaule_ScanGun[HandleNum],HandleNum);
				return TRUE;
			}
			else
			{
				Port_Temp += CString(buffer);
				Sleep(200);
				continue;
			}
		}
		else
		{
			Sleep(200);
			continue;
		}
	}
	return FALSE;
}


BOOL IMEIWrite_MulAT::Check_ScanGun_Data(CEdit* m_Result, CEdit* Final_Result_Control, int HandleNum, CEdit* Data_Input_Control, BOOL ImeiChoose, int LengthLimit)
{
	if (ImeiChoose == TRUE)
	{
		int IMEI_length;
		CString ScanGunData;
		ScanGunData = Vaule_ScanGun[HandleNum];
		IMEI_length = ScanGunData.GetLength();
		for (int i = 0; i<IMEI_length; i++)
		{
			if (IMEI_length != 15 || (ScanGunData.GetAt(i) <'0' || ScanGunData.GetAt(i) >'9'))
			{
				return FALSE;
			}

		}
		if (CheckIMEI(ScanGunData) == FALSE)
		{
			return FALSE;
		}
		//Data_Input_Control->SetWindowTextA(ScanGunData);
		//IMEI_Input[HandleNum]=ScanGunData;
		//LogShow_exchange(m_Result,Final_Result_Control,0,"扫描输入IMEI:"+ScanGunData+"正常...\r\n",HandleNum);
		PublicIMEI = ScanGunData;
	}
	else
	{
		int IMEI_length;
		CString ScanGunData;
		ScanGunData = Vaule_ScanGun[HandleNum];
		IMEI_length = ScanGunData.GetLength();
		if (IMEI_length != LengthLimit)
		{
			return FALSE;
		}
		////////////////////入网号判断///////////////////
		//Data_Input_Control->SetWindowTextA(ScanGunData);
		OtherData_Input[HandleNum] = ScanGunData;
		//LogShow_exchange(m_Result,Final_Result_Control,0,"扫描输入数据:"+OtherData_Input[HandleNum]+"位数正常...\r\n",HandleNum);
	}
	return TRUE;
}
void IMEIWrite_MulAT::OnBnClickedButton3()//打开扫描枪1
{
	if (OPen_ScanGun_Port(&ScanGunPort1, &ScanGunBaud1, 0) == FALSE)
	{
		AfxMessageBox("打开扫描枪端口失败！");
		return;
	}
	else
	{
		GetDlgItem(IDC_BUTTON3)->EnableWindow(FALSE);
	}

	SetTimer(30, 1000, NULL);

	//如果没有勾选蓝牙才去判断IMEI
	if (GetBluetoothCheckValue == FALSE)
	{
		CompareIMEI = TRUE;
	}
	else if (GetBluetoothCheckValue == TRUE)
	{
		AfxMessageBox("获取配置前请先扫描MAC地址！");
	}

}

void IMEIWrite_MulAT::OnBnClickedButton25()
{
	KillTimer(30);
	if (hScanGun[0] != NULL)
	{
		BOOL Return = CloseHandle(hScanGun[0]);				//测试成功后
		if (Return == TRUE)
			LogShow_exchange(&m_Result1, &Final_Result_Control1, 250, "关闭该串口成功！！", 0);
		else
		{
			LogShow_exchange(&m_Result1, &Final_Result_Control1, 128, "关闭该串口失败！！", 0);
			//关闭串口失败
		}
		hScanGun[0] = NULL;
	}
	GetDlgItem(IDC_BUTTON3)->EnableWindow(TRUE);
	CompareIMEI = FALSE;
}


//扫描枪2
void IMEIWrite_MulAT::OnBnClickedButton26()
{

}

void IMEIWrite_MulAT::OnBnClickedButton27()
{

}

//扫描枪3
void IMEIWrite_MulAT::OnBnClickedButton28()
{

}

void IMEIWrite_MulAT::OnBnClickedButton29()
{

}

//扫描枪4
void IMEIWrite_MulAT::OnBnClickedButton30()
{

}

void IMEIWrite_MulAT::OnBnClickedButton31()
{

}

//扫描枪5
void IMEIWrite_MulAT::OnBnClickedButton32()
{

}

void IMEIWrite_MulAT::OnBnClickedButton33()
{

}
//扫描枪6
void IMEIWrite_MulAT::OnBnClickedButton34()
{

}

void IMEIWrite_MulAT::OnBnClickedButton35()
{

}

void IMEIWrite_MulAT::OnBnClickedButton10()
{
	// TODO: 在此添加控件通知处理程序代码
	PrintInt.DoModal();
}

void IMEIWrite_MulAT::OnBnClickedButton37()
{
	// TODO: 在此添加控件通知处理程序代码
	PrintInt.PrintFunIni();
	GetDlgItem(IDC_BUTTON37)->EnableWindow(FALSE);
	GetDlgItem(IDC_BUTTON38)->EnableWindow(TRUE);
}

void IMEIWrite_MulAT::OnBnClickedButton38()
{
	// TODO: 在此添加控件通知处理程序代码
	PrintInt.PrintFunEnd();
	GetDlgItem(IDC_BUTTON37)->EnableWindow(TRUE);
	GetDlgItem(IDC_BUTTON38)->EnableWindow(FALSE);
}

void IMEIWrite_MulAT::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	//OnOK();
	GetDlgItem(IDOK)->EnableWindow(FALSE);
	for (int i = 0; i<THREAD_NUM; i++)
	{
		StopSign[i] = FALSE;
	}
	StopAutoStart = FALSE;
	if (Thread_State[0] != TRUE)
		OnBnClickedButton1();
	if (Thread_State[1] != TRUE)
		OnBnClickedButton14();
	if (Thread_State[2] != TRUE)
		OnBnClickedButton16();
	if (Thread_State[3] != TRUE)
		OnBnClickedButton18();
	if (Thread_State[4] != TRUE)
		OnBnClickedButton20();
	if (Thread_State[5] != TRUE)
		OnBnClickedButton22();

	if (Thread_State[6] != TRUE)
		OnBnClickedButtonstart7();
	if (Thread_State[7] != TRUE)
		OnBnClickedButtonstart8();
	if (Thread_State[8] != TRUE)
		OnBnClickedButtonstart9();
	if (Thread_State[9] != TRUE)
		OnBnClickedButtonstart10();
	if (Thread_State[10] != TRUE)
		OnBnClickedButtonstart11();
	if (Thread_State[11] != TRUE)
		OnBnClickedButtonstart12();
	if (Thread_State[12] != TRUE)
		OnBnClickedButtonstart13();
	if (Thread_State[13] != TRUE)
		OnBnClickedButtonstart14();
	if (Thread_State[14] != TRUE)
		OnBnClickedButtonstart15();
	if (Thread_State[15] != TRUE)
		OnBnClickedButtonstart16();
}

void IMEIWrite_MulAT::OnBnClickedCancel()//停止所有
{
	// TODO: 在此添加控件通知处理程序代码
	//OnCancel();
	for (int i = 0; i<THREAD_NUM; i++)
	{
		StopSign[i] = TRUE;
	}
	StopAutoStart = TRUE;

	if (Thread_State[0] == TRUE)OnBnClickedButton2();
	if (Thread_State[1] == TRUE)OnBnClickedButton15();
	if (Thread_State[2] == TRUE)OnBnClickedButton17();
	if (Thread_State[3] == TRUE)OnBnClickedButton19();
	if (Thread_State[4] == TRUE)OnBnClickedButton21();
	if (Thread_State[5] == TRUE)OnBnClickedButton23();

	if (Thread_State[6] == TRUE)OnBnClickedButtonstop7();
	if (Thread_State[7] == TRUE)OnBnClickedButtonstop8();
	if (Thread_State[8] == TRUE)OnBnClickedButtonstop9();
	if (Thread_State[9] == TRUE)OnBnClickedButtonstop10();
	if (Thread_State[10] == TRUE)OnBnClickedButtonstop11();
	if (Thread_State[11] == TRUE)OnBnClickedButtonstop12();
	if (Thread_State[12] == TRUE)OnBnClickedButtonstop13();
	if (Thread_State[13] == TRUE)OnBnClickedButtonstop14();
	if (Thread_State[14] == TRUE)OnBnClickedButtonstop15();
	if (Thread_State[15] == TRUE)OnBnClickedButtonstop16();

	if ((Thread_State[0] == TRUE) || (Thread_State[1] == TRUE) || (Thread_State[2] == TRUE) || (Thread_State[3] == TRUE) || (Thread_State[4] == TRUE) || (Thread_State[5] == TRUE) || \
		(Thread_State[6] == TRUE) || (Thread_State[7] == TRUE) || (Thread_State[8] == TRUE) || (Thread_State[9] == TRUE) || (Thread_State[10] == TRUE) || (Thread_State[11] == TRUE) || \
		(Thread_State[12] == TRUE) || (Thread_State[13] == TRUE) || (Thread_State[14] == TRUE) || (Thread_State[15] == TRUE))
	{
		AfxMessageBox("请先等待线程结束！");
		return;
	}
	GetDlgItem(IDOK)->EnableWindow(TRUE);
}

void IMEIWrite_MulAT::OnBnClickedButton40()//停止自动开始
{
	// TODO: 在此添加控件通知处理程序代码
	StopAutoStart = TRUE;
}

void IMEIWrite_MulAT::OnBnClickedButton42()//启动自动开始
{
	// TODO: 在此添加控件通知处理程序代码

	StopAutoStart = FALSE;
}


void IMEIWrite_MulAT::WriteLogIni(int LogNum, CString ChipRfIDbg)
{
	CString ChipRfIDCurrent;
	if (ChipRfIDbg != "")
	{
		ChipRfIDCurrent = ChipRfIDbg;
	}
	else
	{
		ChipRfIDCurrent = ChipRfID[LogNum];
	}
	CTime cTime;
	char m_Time_Date[128];
	char m_Time_Time[128];
	CString m_strModulePath;
	CString LogFile;

	char cReportPath[256] = { 0 };
	DWORD dwAttr;

	char file_name[_MAX_PATH] = { 0 };
	GetModuleFileName(NULL, file_name, _MAX_PATH);
	CString strFileName = file_name;
	int num = strFileName.ReverseFind('\\');
	CString LogNum_CS;
	LogNum_CS.Format("%d", LogNum);
	//LogNum_CS="0";
	m_strModulePath = strFileName.Left(num + 1) + LogNum_CS + "\\";

	cTime = CTime::GetCurrentTime();                                                         //创建以当日日期的文件夹来保存数据
	sprintf(m_Time_Date, "%s%04d%02d%02d", m_strModulePath, cTime.GetYear(), cTime.GetMonth(), cTime.GetDay());//把日期以6位放入一个Char组
	//创建以日期为名字的文件夹
	dwAttr = GetFileAttributes(m_Time_Date);
	if (dwAttr == 0xFFFFFFFF)                                                                 //文件夹不存在   
	{
		CreateDirectory(m_Time_Date, NULL);
	}

	sprintf(m_Time_Time, "%s-%02d%02d%02d", ChipRfIDCurrent, cTime.GetHour(), cTime.GetMinute(), cTime.GetSecond());
	//sprintf(cReportPath, "%s%04d%02d%02d/%s @@%s.txt",m_strModulePath,cTime.GetYear(), cTime.GetMonth(), cTime.GetDay(),IMEIRecord[LogNum],m_Time_Time);
	//sprintf(m_Time_Time, "%d-Log",LogNum);
	sprintf(cReportPath, "%s%04d%02d%02d/%s.txt", m_strModulePath, cTime.GetYear(), cTime.GetMonth(), cTime.GetDay(), m_Time_Time);
	cReport[LogNum].Open(cReportPath, CFile::modeCreate | CFile::modeReadWrite | CFile::modeNoTruncate);
	//CFile::modeCreate 文件不存在，则新建；存在并且没有CFile::modeNoTruncate则清空原文件，有CFile::modeNoTruncate，则不清空原文件（不把长度删除为0）
	/*if(!cReport[LogNum].Open(cReportPath,CFile::modeReadWrite))
	{
	cReport[LogNum].Open(cReportPath,CFile::modeCreate|CFile::modeReadWrite);//原文件不存在，则新建
	}*/
}
void IMEIWrite_MulAT::WriteLog(CString Text, int	  LogNum)
{
	cReport[LogNum].SeekToEnd();											//cReport[LogNum].Seek(0,CFile::end);
	cReport[LogNum].Write((LPSTR)(LPCSTR)Text, Text.GetLength());			//strlen(Lreport)
	cReport[LogNum].Write("\r\n", 2);
	//char ReadData[256]={0};
	//cReport[LogNum].SeekToBegin();
	//cReport[LogNum].Read(ReadData,256);
}

void IMEIWrite_MulAT::WriteLogEnd(int LogNum)
{
	cReport[LogNum].Close();
}


UINT static __cdecl WriteIMEIFunction7(LPVOID pParam)
{
	IMEIWrite_MulAT* Mead_Main_Win = (IMEIWrite_MulAT*)pParam;
	Mead_Main_Win->WriteIMEIFunction_Thread(&(Mead_Main_Win->m_Port7), &(Mead_Main_Win->m_Baud7), 6, &(Mead_Main_Win->m_Result7), &(Mead_Main_Win->Final_Result_Control7), NULL);
	return 0;
}
UINT static __cdecl WriteIMEIFunction8(LPVOID pParam)
{
	IMEIWrite_MulAT* Mead_Main_Win = (IMEIWrite_MulAT*)pParam;
	Mead_Main_Win->WriteIMEIFunction_Thread(&(Mead_Main_Win->m_Port8), &(Mead_Main_Win->m_Baud8), 7, &(Mead_Main_Win->m_Result8), &(Mead_Main_Win->Final_Result_Control8), NULL);
	return 0;
}
UINT static __cdecl WriteIMEIFunction9(LPVOID pParam)
{
	IMEIWrite_MulAT* Mead_Main_Win = (IMEIWrite_MulAT*)pParam;
	Mead_Main_Win->WriteIMEIFunction_Thread(&(Mead_Main_Win->m_Port9), &(Mead_Main_Win->m_Baud9), 8, &(Mead_Main_Win->m_Result9), &(Mead_Main_Win->Final_Result_Control9), NULL);
	return 0;
}
UINT static __cdecl WriteIMEIFunction10(LPVOID pParam)
{
	IMEIWrite_MulAT* Mead_Main_Win = (IMEIWrite_MulAT*)pParam;
	Mead_Main_Win->WriteIMEIFunction_Thread(&(Mead_Main_Win->m_Port10), &(Mead_Main_Win->m_Baud10), 9, &(Mead_Main_Win->m_Result10), &(Mead_Main_Win->Final_Result_Control10), NULL);
	return 0;
}
UINT static __cdecl WriteIMEIFunction11(LPVOID pParam)
{
	IMEIWrite_MulAT* Mead_Main_Win = (IMEIWrite_MulAT*)pParam;
	Mead_Main_Win->WriteIMEIFunction_Thread(&(Mead_Main_Win->m_Port11), &(Mead_Main_Win->m_Baud11), 10, &(Mead_Main_Win->m_Result11), &(Mead_Main_Win->Final_Result_Control11), NULL);
	return 0;
}
UINT static __cdecl WriteIMEIFunction12(LPVOID pParam)
{
	IMEIWrite_MulAT* Mead_Main_Win = (IMEIWrite_MulAT*)pParam;
	Mead_Main_Win->WriteIMEIFunction_Thread(&(Mead_Main_Win->m_Port12), &(Mead_Main_Win->m_Baud12), 11, &(Mead_Main_Win->m_Result12), &(Mead_Main_Win->Final_Result_Control12), NULL);
	return 0;
}
UINT static __cdecl WriteIMEIFunction13(LPVOID pParam)
{
	IMEIWrite_MulAT* Mead_Main_Win = (IMEIWrite_MulAT*)pParam;
	Mead_Main_Win->WriteIMEIFunction_Thread(&(Mead_Main_Win->m_Port13), &(Mead_Main_Win->m_Baud13), 12, &(Mead_Main_Win->m_Result13), &(Mead_Main_Win->Final_Result_Control13), NULL);
	return 0;
}
UINT static __cdecl WriteIMEIFunction14(LPVOID pParam)
{
	IMEIWrite_MulAT* Mead_Main_Win = (IMEIWrite_MulAT*)pParam;
	Mead_Main_Win->WriteIMEIFunction_Thread(&(Mead_Main_Win->m_Port14), &(Mead_Main_Win->m_Baud14), 13, &(Mead_Main_Win->m_Result14), &(Mead_Main_Win->Final_Result_Control14), NULL);
	return 0;
}
UINT static __cdecl WriteIMEIFunction15(LPVOID pParam)
{
	IMEIWrite_MulAT* Mead_Main_Win = (IMEIWrite_MulAT*)pParam;
	Mead_Main_Win->WriteIMEIFunction_Thread(&(Mead_Main_Win->m_Port15), &(Mead_Main_Win->m_Baud15), 14, &(Mead_Main_Win->m_Result15), &(Mead_Main_Win->Final_Result_Control15), NULL);
	return 0;
}
UINT static __cdecl WriteIMEIFunction16(LPVOID pParam)
{
	IMEIWrite_MulAT* Mead_Main_Win = (IMEIWrite_MulAT*)pParam;
	Mead_Main_Win->WriteIMEIFunction_Thread(&(Mead_Main_Win->m_Port16), &(Mead_Main_Win->m_Baud16), 15, &(Mead_Main_Win->m_Result16), &(Mead_Main_Win->Final_Result_Control16), NULL);
	return 0;
}
void IMEIWrite_MulAT::OnBnClickedButtonstart7()
{
	// TODO: 在此添加控件通知处理程序代码
	StopSign[6] = FALSE;
	Start7_Control.EnableWindow(FALSE);
	Thread_Handle[6] = AfxBeginThread(WriteIMEIFunction7, (LPVOID)this, THREAD_PRIORITY_NORMAL, 0, 0, NULL);
}

void IMEIWrite_MulAT::OnBnClickedButtonstart8()
{
	// TODO: 在此添加控件通知处理程序代码
	StopSign[7] = FALSE;
	Start8_Control.EnableWindow(FALSE);
	Thread_Handle[7] = AfxBeginThread(WriteIMEIFunction8, (LPVOID)this, THREAD_PRIORITY_NORMAL, 0, 0, NULL);
}

void IMEIWrite_MulAT::OnBnClickedButtonstart9()
{
	// TODO: 在此添加控件通知处理程序代码
	int HandleNum = 8;
	StopSign[8] = FALSE;
	Start9_Control.EnableWindow(FALSE);
	if (GetPicChoose == TRUE)
	{
		HandleNum = 0;
		if (OPen_Pic_Port(&m_Port9, &m_Baud9, HandleNum) == FALSE)
		{
			AfxMessageBox("打开图像端口失败！");
			Start9_Control.EnableWindow(TRUE);
		}
	}
	else
	{
		Thread_Handle[8] = AfxBeginThread(WriteIMEIFunction9, (LPVOID)this, THREAD_PRIORITY_NORMAL, 0, 0, NULL);
	}
}

void IMEIWrite_MulAT::OnBnClickedButtonstart10()
{
	// TODO: 在此添加控件通知处理程序代码
	int HandleNum = 9;
	StopSign[9] = FALSE;
	Start10_Control.EnableWindow(FALSE);
	if (GetPicChoose == TRUE)
	{
		HandleNum = 1;
		if (OPen_Pic_Port(&m_Port10, &m_Baud10, HandleNum) == FALSE)
		{
			AfxMessageBox("打开图像端口失败！");
			Start10_Control.EnableWindow(TRUE);
		}
	}
	else
	{
		Thread_Handle[9] = AfxBeginThread(WriteIMEIFunction10, (LPVOID)this, THREAD_PRIORITY_NORMAL, 0, 0, NULL);
	}
}

void IMEIWrite_MulAT::OnBnClickedButtonstart11()
{
	// TODO: 在此添加控件通知处理程序代码
	int HandleNum = 10;
	StopSign[10] = FALSE;
	Start11_Control.EnableWindow(FALSE);
	if (GetPicChoose == TRUE)
	{
		HandleNum = 2;
		if (OPen_Pic_Port(&m_Port11, &m_Baud11, HandleNum) == FALSE)
		{
			AfxMessageBox("打开图像端口失败！");
			Start11_Control.EnableWindow(TRUE);
		}
	}
	else
	{
		Thread_Handle[10] = AfxBeginThread(WriteIMEIFunction11, (LPVOID)this, THREAD_PRIORITY_NORMAL, 0, 0, NULL);
	}
}

void IMEIWrite_MulAT::OnBnClickedButtonstart12()
{
	// TODO: 在此添加控件通知处理程序代码
	int HandleNum = 11;
	StopSign[11] = FALSE;
	Start12_Control.EnableWindow(FALSE);
	if (GetPicChoose == TRUE)
	{
		HandleNum = 3;
		if (OPen_Pic_Port(&m_Port12, &m_Baud12, HandleNum) == FALSE)
		{
			AfxMessageBox("打开图像端口失败！");
			Start12_Control.EnableWindow(TRUE);
		}
	}
	else
	{
		Thread_Handle[11] = AfxBeginThread(WriteIMEIFunction12, (LPVOID)this, THREAD_PRIORITY_NORMAL, 0, 0, NULL);
	}
}

void IMEIWrite_MulAT::OnBnClickedButtonstart13()
{
	// TODO: 在此添加控件通知处理程序代码
	int HandleNum = 12;
	StopSign[12] = FALSE;
	Start13_Control.EnableWindow(FALSE);
	if (GetPicChoose == TRUE)
	{
		HandleNum = 4;
		if (OPen_Pic_Port(&m_Port13, &m_Baud13, HandleNum) == FALSE)
		{
			AfxMessageBox("打开图像端口失败！");
			Start13_Control.EnableWindow(TRUE);
		}
	}
	else
	{
		Thread_Handle[12] = AfxBeginThread(WriteIMEIFunction13, (LPVOID)this, THREAD_PRIORITY_NORMAL, 0, 0, NULL);
	}
}

void IMEIWrite_MulAT::OnBnClickedButtonstart14()
{
	// TODO: 在此添加控件通知处理程序代码
	int HandleNum = 13;
	StopSign[13] = FALSE;
	Start14_Control.EnableWindow(FALSE);
	if (GetPicChoose == TRUE)
	{
		HandleNum = 5;
		if (OPen_Pic_Port(&m_Port14, &m_Baud14, HandleNum) == FALSE)
		{
			AfxMessageBox("打开图像端口失败！");
			Start14_Control.EnableWindow(TRUE);
		}
	}
	else
	{
		Thread_Handle[13] = AfxBeginThread(WriteIMEIFunction14, (LPVOID)this, THREAD_PRIORITY_NORMAL, 0, 0, NULL);
	}
}

void IMEIWrite_MulAT::OnBnClickedButtonstart15()
{
	// TODO: 在此添加控件通知处理程序代码
	int HandleNum = 14;
	StopSign[14] = FALSE;
	Start15_Control.EnableWindow(FALSE);
	if (GetPicChoose == TRUE)
	{
		HandleNum = 6;
		if (OPen_Pic_Port(&m_Port15, &m_Baud15, HandleNum) == FALSE)
		{
			AfxMessageBox("打开图像端口失败！");
			Start15_Control.EnableWindow(TRUE);
		}
	}
	else
	{
		Thread_Handle[14] = AfxBeginThread(WriteIMEIFunction15, (LPVOID)this, THREAD_PRIORITY_NORMAL, 0, 0, NULL);
	}
}

void IMEIWrite_MulAT::OnBnClickedButtonstart16()
{
	// TODO: 在此添加控件通知处理程序代码
	int HandleNum = 15;
	StopSign[15] = FALSE;
	Start16_Control.EnableWindow(FALSE);
	if (GetPicChoose == TRUE)
	{
		HandleNum = 7;
		if (OPen_Pic_Port(&m_Port16, &m_Baud16, HandleNum) == FALSE)
		{
			AfxMessageBox("打开图像端口失败！");
			Start16_Control.EnableWindow(TRUE);
		}
	}
	else
	{
		Thread_Handle[15] = AfxBeginThread(WriteIMEIFunction16, (LPVOID)this, THREAD_PRIORITY_NORMAL, 0, 0, NULL);
	}
}


void IMEIWrite_MulAT::OnBnClickedButtonstop7()
{
	// TODO: 在此添加控件通知处理程序代码
	int HandleNum = 6;
	LeaveCriticalSection(&WIFICOMMUNICATE[HandleNum]);
	StopSign[HandleNum] = TRUE;
	Sleep(100);
	int i = 0;
	while (Thread_State[HandleNum] == TRUE)
	{
		Sleep(STOPDELAY);
		i++;
		if (i>1)
			break;
	};
	//COM_State[HandleNum]=FALSE;
	LogShow_exchange(&m_Result7, &Final_Result_Control7, 250, "结束测试！！", HandleNum);

	int PortHandle = 6;

	//停止时如果存在蓝牙连接就给蓝牙发送断开指令
	if (GetBluetoothCheckValue == TRUE&&BleGetSettingFlag[PortHandle] == 1)
	{
		BleGetSettingFlag[PortHandle] = 0;
		BluetoothDisconnect(PortHandle);
	}

	if (0)
	{
		BOOL Return = CloseHandle(hPort[PortHandle]);				//测试成功后
		if (Return == TRUE)
		{
		}
		else
		{
			AfxMessageBox("关闭失败！");												//关闭串口失败
			return;
		}
		hPort[PortHandle] = NULL;
	}

	//停止时删除占用队列中对应的端口号
	if (GetBluetoothCheckValue == TRUE || g_ADCTFlag == 1)
	{
		DeleteComoOccupancydDeq(6);
	}
}
void IMEIWrite_MulAT::OnBnClickedButtonstop8()
{
	// TODO: 在此添加控件通知处理程序代码
	int HandleNum = 7;
	LeaveCriticalSection(&WIFICOMMUNICATE[HandleNum]);
	StopSign[HandleNum] = TRUE;
	Sleep(100);
	int i = 0;
	while (Thread_State[HandleNum] == TRUE)
	{
		Sleep(STOPDELAY);
		i++;
		if (i>1)
			break;
	};
	//COM_State[HandleNum]=FALSE;
	LogShow_exchange(&m_Result8, &Final_Result_Control8, 250, "结束测试！！", HandleNum);

	int PortHandle = 7;

	//停止时如果存在蓝牙连接就给蓝牙发送断开指令
	if (GetBluetoothCheckValue == TRUE&&BleGetSettingFlag[PortHandle] == 1)
	{
		BleGetSettingFlag[PortHandle] = 0;
		BluetoothDisconnect(PortHandle);
	}

	if (0)
	{
		BOOL Return = CloseHandle(hPort[PortHandle]);				//测试成功后
		if (Return == TRUE)
		{
		}
		else
		{
			AfxMessageBox("关闭失败！");												//关闭串口失败
			return;
		}
		hPort[PortHandle] = NULL;
	}

	//停止时删除占用队列中对应的端口号
	if (GetBluetoothCheckValue == TRUE || g_ADCTFlag == 1)
	{
		DeleteComoOccupancydDeq(7);
	}
}

void IMEIWrite_MulAT::OnBnClickedButtonstop9()
{
	// TODO: 在此添加控件通知处理程序代码
	int HandleNum = 8;
	StopSign[HandleNum] = TRUE;
	Sleep(100);
	int i = 0;
	while (Thread_State[HandleNum] == TRUE)
	{
		Sleep(STOPDELAY);
		i++;
		if (i>1)
			break;
	};
	//COM_State[HandleNum]=FALSE;
	LogShow_exchange(&m_Result9, &Final_Result_Control9, 250, "结束测试！！", HandleNum);

	int PortHandle = 8;

	//停止时如果存在蓝牙连接就给蓝牙发送断开指令
	if (GetBluetoothCheckValue == TRUE&&BleGetSettingFlag[PortHandle] == 1)
	{
		BleGetSettingFlag[PortHandle] = 0;
		BluetoothDisconnect(PortHandle);
	}

	if (GetPicChoose == TRUE)
	{
		Start9_Control.EnableWindow(TRUE);
		PortHandle = 0;
		if (hPicPort[PortHandle] == NULL)
			return;
		BOOL Return = CloseHandle(hPicPort[PortHandle]);				//测试成功后
		if (Return == TRUE)
		{
		}
		else
		{
			AfxMessageBox("关闭失败！");												//关闭串口失败
			return;
		}
		hPicPort[PortHandle] = NULL;
	}
	else if (0)
	{
		BOOL Return = CloseHandle(hPort[PortHandle]);				//测试成功后
		if (Return == TRUE)
		{
		}
		else
		{
			AfxMessageBox("关闭失败！");												//关闭串口失败
			return;
		}
		hPort[PortHandle] = NULL;
	}

	//停止时删除占用队列中对应的端口号
	if (GetBluetoothCheckValue == TRUE || g_ADCTFlag == 1)
	{
		DeleteComoOccupancydDeq(8);
	}
}

void IMEIWrite_MulAT::OnBnClickedButtonstop10()
{
	// TODO: 在此添加控件通知处理程序代码
	int HandleNum = 9;
	StopSign[HandleNum] = TRUE;
	Sleep(100);
	int i = 0;
	while (Thread_State[HandleNum] == TRUE)
	{
		Sleep(STOPDELAY);
		i++;
		if (i>1)
			break;
	};
	//COM_State[HandleNum]=FALSE;
	LogShow_exchange(&m_Result10, &Final_Result_Control10, 250, "结束测试！！", HandleNum);

	int PortHandle = 9;

	//停止时如果存在蓝牙连接就给蓝牙发送断开指令
	if (GetBluetoothCheckValue == TRUE&&BleGetSettingFlag[PortHandle] == 1)
	{
		BleGetSettingFlag[PortHandle] = 0;
		BluetoothDisconnect(PortHandle);
	}

	if (GetPicChoose == TRUE)
	{
		Start10_Control.EnableWindow(TRUE);
		PortHandle = 1;
		if (hPicPort[PortHandle] == NULL)
			return;
		BOOL Return = CloseHandle(hPicPort[PortHandle]);				//测试成功后
		if (Return == TRUE)
		{
		}
		else
		{
			AfxMessageBox("关闭失败！");												//关闭串口失败
			return;
		}
		hPicPort[PortHandle] = NULL;
	}
	else if (0)
	{
		BOOL Return = CloseHandle(hPort[PortHandle]);				//测试成功后
		if (Return == TRUE)
		{
		}
		else
		{
			AfxMessageBox("关闭失败！");												//关闭串口失败
			return;
		}
		hPort[PortHandle] = NULL;
	}

	//停止时删除占用队列中对应的端口号
	if (GetBluetoothCheckValue == TRUE || g_ADCTFlag == 1)
	{
		DeleteComoOccupancydDeq(9);
	}
}

void IMEIWrite_MulAT::OnBnClickedButtonstop11()
{
	// TODO: 在此添加控件通知处理程序代码
	int HandleNum = 10;
	StopSign[HandleNum] = TRUE;
	Sleep(100);
	int i = 0;
	while (Thread_State[HandleNum] == TRUE)
	{
		Sleep(STOPDELAY);
		i++;
		if (i>1)
			break;
	};
	//COM_State[HandleNum]=FALSE;
	LogShow_exchange(&m_Result11, &Final_Result_Control11, 250, "结束测试！！", HandleNum);

	int PortHandle = 10;

	//停止时如果存在蓝牙连接就给蓝牙发送断开指令
	if (GetBluetoothCheckValue == TRUE&&BleGetSettingFlag[PortHandle] == 1)
	{
		BleGetSettingFlag[PortHandle] = 0;
		BluetoothDisconnect(PortHandle);
	}

	if (GetPicChoose == TRUE)
	{
		Start11_Control.EnableWindow(TRUE);
		PortHandle = 2;
		if (hPicPort[PortHandle] == NULL)
			return;
		BOOL Return = CloseHandle(hPicPort[PortHandle]);				//测试成功后
		if (Return == TRUE)
		{
		}
		else
		{
			AfxMessageBox("关闭失败！");												//关闭串口失败
			return;
		}
		hPicPort[PortHandle] = NULL;
	}
	else if (0)
	{
		BOOL Return = CloseHandle(hPort[PortHandle]);				//测试成功后
		if (Return == TRUE)
		{
		}
		else
		{
			AfxMessageBox("关闭失败！");												//关闭串口失败
			return;
		}
		hPort[PortHandle] = NULL;
	}

	//停止时删除占用队列中对应的端口号
	if (GetBluetoothCheckValue == TRUE || g_ADCTFlag == 1)
	{
		DeleteComoOccupancydDeq(10);
	}
}

void IMEIWrite_MulAT::OnBnClickedButtonstop12()
{
	// TODO: 在此添加控件通知处理程序代码
	int HandleNum = 11;
	StopSign[HandleNum] = TRUE;
	Sleep(100);
	int i = 0;
	while (Thread_State[HandleNum] == TRUE)
	{
		Sleep(STOPDELAY);
		i++;
		if (i>1)
			break;
	};
	//COM_State[HandleNum]=FALSE;
	LogShow_exchange(&m_Result12, &Final_Result_Control12, 250, "结束测试！！", HandleNum);

	int PortHandle = 11;

	//停止时如果存在蓝牙连接就给蓝牙发送断开指令
	if (GetBluetoothCheckValue == TRUE&&BleGetSettingFlag[PortHandle] == 1)
	{
		BleGetSettingFlag[PortHandle] = 0;
		BluetoothDisconnect(PortHandle);
	}

	if (GetPicChoose == TRUE)
	{
		Start12_Control.EnableWindow(TRUE);
		PortHandle = 3;
		if (hPicPort[PortHandle] == NULL)
			return;
		BOOL Return = CloseHandle(hPicPort[PortHandle]);				//测试成功后
		if (Return == TRUE)
		{
		}
		else
		{
			AfxMessageBox("关闭失败！");												//关闭串口失败
			return;
		}
		hPicPort[PortHandle] = NULL;
	}
	else if (0)
	{
		BOOL Return = CloseHandle(hPort[PortHandle]);				//测试成功后
		if (Return == TRUE)
		{
		}
		else
		{
			AfxMessageBox("关闭失败！");												//关闭串口失败
			return;
		}
		hPort[PortHandle] = NULL;
	}

	//停止时删除占用队列中对应的端口号
	if (GetBluetoothCheckValue == TRUE || g_ADCTFlag == 1)
	{
		DeleteComoOccupancydDeq(11);
	}
}

void IMEIWrite_MulAT::OnBnClickedButtonstop13()
{
	// TODO: 在此添加控件通知处理程序代码
	int HandleNum = 12;
	StopSign[HandleNum] = TRUE;
	Sleep(100);
	int i = 0;
	while (Thread_State[HandleNum] == TRUE)
	{
		Sleep(STOPDELAY);
		i++;
		if (i>1)
			break;
	};
	//COM_State[HandleNum]=FALSE;
	LogShow_exchange(&m_Result13, &Final_Result_Control13, 250, "结束测试！！", HandleNum);

	int PortHandle = 12;

	//停止时如果存在蓝牙连接就给蓝牙发送断开指令
	if (GetBluetoothCheckValue == TRUE&&BleGetSettingFlag[PortHandle] == 1)
	{
		BleGetSettingFlag[PortHandle] = 0;
		BluetoothDisconnect(PortHandle);
	}

	if (GetPicChoose == TRUE)
	{
		Start13_Control.EnableWindow(TRUE);
		PortHandle = 4;
		if (hPicPort[PortHandle] == NULL)
			return;
		BOOL Return = CloseHandle(hPicPort[PortHandle]);				//测试成功后
		if (Return == TRUE)
		{
		}
		else
		{
			AfxMessageBox("关闭失败！");												//关闭串口失败
			return;
		}
		hPicPort[PortHandle] = NULL;
	}
	else if (0)
	{
		BOOL Return = CloseHandle(hPort[PortHandle]);				//测试成功后
		if (Return == TRUE)
		{
		}
		else
		{
			AfxMessageBox("关闭失败！");												//关闭串口失败
			return;
		}
		hPort[PortHandle] = NULL;
	}

	//停止时删除占用队列中对应的端口号
	if (GetBluetoothCheckValue == TRUE || g_ADCTFlag == 1)
	{
		DeleteComoOccupancydDeq(12);
	}
}

void IMEIWrite_MulAT::OnBnClickedButtonstop14()
{
	// TODO: 在此添加控件通知处理程序代码
	int HandleNum = 13;
	StopSign[HandleNum] = TRUE;
	Sleep(100);
	int i = 0;
	while (Thread_State[HandleNum] == TRUE)
	{
		Sleep(STOPDELAY);
		i++;
		if (i>1)
			break;
	};
	//COM_State[HandleNum]=FALSE;
	LogShow_exchange(&m_Result14, &Final_Result_Control14, 250, "结束测试！！", HandleNum);

	int PortHandle = 13;

	//停止时如果存在蓝牙连接就给蓝牙发送断开指令
	if (GetBluetoothCheckValue == TRUE&&BleGetSettingFlag[PortHandle] == 1)
	{
		BleGetSettingFlag[PortHandle] = 0;
		BluetoothDisconnect(PortHandle);
	}

	if (GetPicChoose == TRUE)
	{
		Start14_Control.EnableWindow(TRUE);
		PortHandle = 5;
		if (hPicPort[PortHandle] == NULL)
			return;
		BOOL Return = CloseHandle(hPicPort[PortHandle]);				//测试成功后
		if (Return == TRUE)
		{
		}
		else
		{
			AfxMessageBox("关闭失败！");												//关闭串口失败
			return;
		}
		hPicPort[PortHandle] = NULL;
	}
	else if (0)
	{
		BOOL Return = CloseHandle(hPort[PortHandle]);				//测试成功后
		if (Return == TRUE)
		{
		}
		else
		{
			AfxMessageBox("关闭失败！");												//关闭串口失败
			return;
		}
		hPort[PortHandle] = NULL;
	}

	//停止时删除占用队列中对应的端口号
	if (GetBluetoothCheckValue == TRUE || g_ADCTFlag == 1)
	{
		DeleteComoOccupancydDeq(13);
	}
}

void IMEIWrite_MulAT::OnBnClickedButtonstop15()
{
	// TODO: 在此添加控件通知处理程序代码
	int HandleNum = 14;
	StopSign[HandleNum] = TRUE;
	Sleep(100);
	int i = 0;
	while (Thread_State[HandleNum] == TRUE)
	{
		Sleep(STOPDELAY);
		i++;
		if (i>1)
			break;
	};
	//COM_State[HandleNum]=FALSE;
	LogShow_exchange(&m_Result15, &Final_Result_Control15, 250, "结束测试！！", HandleNum);

	int PortHandle = 14;

	//停止时如果存在蓝牙连接就给蓝牙发送断开指令
	if (GetBluetoothCheckValue == TRUE&&BleGetSettingFlag[PortHandle] == 1)
	{
		BleGetSettingFlag[PortHandle] = 0;
		BluetoothDisconnect(PortHandle);
	}

	if (GetPicChoose == TRUE)
	{
		Start15_Control.EnableWindow(TRUE);
		PortHandle = 6;
		if (hPicPort[PortHandle] == NULL)
			return;
		BOOL Return = CloseHandle(hPicPort[PortHandle]);				//测试成功后
		if (Return == TRUE)
		{
		}
		else
		{
			AfxMessageBox("关闭失败！");												//关闭串口失败
			return;
		}
		hPicPort[PortHandle] = NULL;
	}
	else if (0)
	{
		BOOL Return = CloseHandle(hPort[PortHandle]);				//测试成功后
		if (Return == TRUE)
		{
		}
		else
		{
			AfxMessageBox("关闭失败！");												//关闭串口失败
			return;
		}
		hPort[PortHandle] = NULL;
	}

	//停止时删除占用队列中对应的端口号
	if (GetBluetoothCheckValue == TRUE || g_ADCTFlag == 1)
	{
		DeleteComoOccupancydDeq(14);
	}
}

void IMEIWrite_MulAT::OnBnClickedButtonstop16()
{
	// TODO: 在此添加控件通知处理程序代码
	int HandleNum = 15;
	StopSign[HandleNum] = TRUE;
	Sleep(100);
	int i = 0;
	while (Thread_State[HandleNum] == TRUE)
	{
		Sleep(STOPDELAY);
		i++;
		if (i>1)
			break;
	};
	//COM_State[HandleNum]=FALSE;
	LogShow_exchange(&m_Result16, &Final_Result_Control16, 250, "结束测试！！", HandleNum);

	int PortHandle = 15;

	//停止时如果存在蓝牙连接就给蓝牙发送断开指令
	if (GetBluetoothCheckValue == TRUE&&BleGetSettingFlag[PortHandle] == 1)
	{
		BleGetSettingFlag[PortHandle] = 0;
		BluetoothDisconnect(PortHandle);
	}

	if (GetPicChoose == TRUE)
	{
		Start16_Control.EnableWindow(TRUE);
		PortHandle = 7;
		if (hPicPort[PortHandle] == NULL)
			return;
		BOOL Return = CloseHandle(hPicPort[PortHandle]);				//测试成功后
		if (Return == TRUE)
		{
		}
		else
		{
			AfxMessageBox("关闭失败！");												//关闭串口失败
			return;
		}
		hPicPort[PortHandle] = NULL;
	}
	else if (0)
	{
		BOOL Return = CloseHandle(hPort[PortHandle]);				//测试成功后
		if (Return == TRUE)
		{
		}
		else
		{
			AfxMessageBox("关闭失败！");												//关闭串口失败
			return;
		}
		hPort[PortHandle] = NULL;
	}

	//停止时删除占用队列中对应的端口号
	if (GetBluetoothCheckValue == TRUE || g_ADCTFlag == 1)
	{
		DeleteComoOccupancydDeq(15);
	}

}

void IMEIWrite_MulAT::OnBnClickedButtonconncpu()
{
	// TODO: 在此添加控件通知处理程序代码
	if (OPen_Serial_PortReadConstant(&CPUControlPort, &CPUControlBaud, THREAD_NUM, TRUE) == TRUE)
	{
		//打开串口成功
		COM_State[THREAD_NUM] = TRUE;
		GetDlgItem(IDC_BUTTONCONNCPU)->EnableWindow(FALSE);
	}
	else
	{
		//打开串口失败
		COM_State[THREAD_NUM] = FALSE;
		AfxMessageBox("打开端口失败！");
	}
}

void IMEIWrite_MulAT::OnBnClickedButtondisconncpu()
{
	// TODO: 在此添加控件通知处理程序代码
	if (hPort[THREAD_NUM] != NULL)
	{
		BOOL Return = CloseHandle(hPort[THREAD_NUM]);				//测试成功后
		if (Return == TRUE)
		{
		}
		else
		{
			AfxMessageBox("关闭失败！");												//关闭串口失败
			return;
		}
		hPort[THREAD_NUM] = NULL;
	}
	GetDlgItem(IDC_BUTTONCONNCPU)->EnableWindow(TRUE);
	COM_State[THREAD_NUM] = FALSE;
}

BOOL IMEIWrite_MulAT::CPUCommunication(CString CpuOrderName, int HandleNum, CString EndSign, CString StartSign, BOOL Enable, CEdit* m_Result, CEdit* Final_Result_Control)
{
	LogShow_exchange(m_Result, Final_Result_Control, 6, "开始与单片机通信0-'" + CpuOrderName + "'命令", HandleNum);
	EnterCriticalSection(&CPUCOMMUNICATE);
	if (Enable == FALSE)
	{
		LeaveCriticalSection(&CPUCOMMUNICATE);
		return TRUE;
	}
	LogShow_exchange(m_Result, Final_Result_Control, 6, "开始与单片机通信1-'" + CpuOrderName + "'命令", HandleNum);
	int CpuOrderNum = -1;
	if (CpuOrderName == "成功")
		CpuOrderNum = 0;
	else if (CpuOrderName == "失败")
		CpuOrderNum = 1;
	else if (CpuOrderName.Find("开蜂鸣器") != -1)
		CpuOrderNum = 2;
	else if (CpuOrderName.Find("关蜂鸣器") != -1)
		CpuOrderNum = 3;
	else if (CpuOrderName.Find("SOS按键") != -1)
		CpuOrderNum = 4;
	else if (CpuOrderName.Find("SOS关按键") != -1)
		CpuOrderNum = 5;
	else if (CpuOrderName.Find("按键D") != -1)
		CpuOrderNum = 6;
	else if ((CpuOrderName.Find("按键E") != -1) || (CpuOrderName.Find("震动") != -1))
		CpuOrderNum = 7;
	else if (CpuOrderName.Find("控制板状态") != -1)
		CpuOrderNum = 14;
	else if (CpuOrderName.Find("控制板") != -1)
		CpuOrderNum = 8;
	else if (CpuOrderName.Find("模拟电流") != -1)
		CpuOrderNum = 9;
	else if (CpuOrderName.Find("打开通路") != -1)
		CpuOrderNum = 10;
	else if (CpuOrderName.Find("关闭通路") != -1)
		CpuOrderNum = 11;
	else if (CpuOrderName.Find("开照明灯") != -1)
	{
		CpuOrderNum = 12;
		DayTimeState[HandleNum] = TRUE;
	}
	else if (CpuOrderName.Find("关照明灯") != -1)
	{
		CpuOrderNum = 13;
		DayTimeState[HandleNum] = FALSE;
	}
	else if (CpuOrderName.Find("确认失败") != -1)
		CpuOrderNum = 15;
	else if (CpuOrderName.Find("确认成功") != -1)
		CpuOrderNum = 16;
	else if (CpuOrderName.Find("外电1开") != -1)
		CpuOrderNum = 17;
	else if (CpuOrderName.Find("外电2开") != -1)
		CpuOrderNum = 18;
	else if (CpuOrderName.Find("外电3开") != -1)
		CpuOrderNum = 19;
	else if (CpuOrderName.Find("外电1关") != -1)
	{
		CpuOrderNum = 20;
		RecoverVoltage = TRUE;
	}
	else if (CpuOrderName.Find("外电2关") != -1)
		CpuOrderNum = 21;
	else if (CpuOrderName.Find("外电3关") != -1)
		CpuOrderNum = 22;
	else
	{
		LeaveCriticalSection(&CPUCOMMUNICATE);
		return FALSE;
	}

	/*
	开1#电压1：AT+01a+#+Voltage_1_ON+校验  例如：AT01a#Voltage_1_ONAC
	开1#电压2：AT+01a+#+Voltage_2_ON+校验
	开1#电压3：AT+01a+#+Voltage_3_ON+校验
	开2#电压1：
	关1#电压1：AT+01a+#+Voltage_1_OFF+校验 例如：AT01a#Voltage_1_OFFFE
	关1#电压2：AT+01a+#+Voltage_2_OFF+校验
	关1#电压3：AT+01a+#+Voltage_3_OFF+校验
	*/
	//--开绿灯--开红灯--开蜂鸣器--关蜂鸣器--SOS按键--SOS关按键--按键1--按键2--获取到终端--外电模拟电流--打开通路--关闭通路--开照明灯--关照明灯--门状态--按钮状态
	CString CpuOrder[23] = { "CTNGL", "CTNRL", "TNB", "TFB", "TNK", "TFK", "TNDSV", "TNCSV", "GetCom", "Crt", "TNPW", "TFPW", "TNL", "TFL", "CheckDoor", "CheckButtonF", "CheckButtonP", \
		"Voltage_1_ON", "Voltage_2_ON", "Voltage_3_ON", "Voltage_1_OFF", "Voltage_2_OFF", "Voltage_3_OFF" };
	CString Command_Position[16] = { "AT01a#", "AT02b#", "AT03c#", "AT04d#", "AT05e#", "AT06f#", "AT07g#", "AT08h#",
		"AT09i#", "AT10j#", "AT11k#", "AT12l#", "AT13m#", "AT14n#", "AT15o#", "AT16p#" };

	CString TestType;
	GetDlgItem(IDC_COMBOTESTYYPE)->GetWindowTextA(TestType);
	if (TestType == "无尾线")
	{
		CpuOrder[0] = "TNGL";
		CpuOrder[1] = "TNRL";
	}

	CString TestType2;
	GetDlgItem(IDC_COMBOTESTYYPE2)->GetWindowTextA(TestType2);//气动
	CString TestType3;
	GetDlgItem(IDC_COMBOTESTYYPE3)->GetWindowTextA(TestType3);//电流
	CString TestType4;
	GetDlgItem(IDC_COMBOTESTYYPE4)->GetWindowTextA(TestType4);//开关

	if (TestType2 == "")//无气动
	{
		if (CpuOrderNum <= 8)
		{
			LeaveCriticalSection(&CPUCOMMUNICATE);
			return TRUE;
		}
	}
	if (TestType3 == "无电流")//无模拟电流
	{
		if (CpuOrderNum == 9)
		{
			LeaveCriticalSection(&CPUCOMMUNICATE);
			return TRUE;
		}
	}
	if (TestType4 == "")//无开关
	{
		if ((CpuOrderNum == 10) || (CpuOrderNum == 11))
		{
			LeaveCriticalSection(&CPUCOMMUNICATE);
			return TRUE;
		}
	}

	CString CpuOrderComplete;
	CString CpuOrderTemp;
	CString Serial_Order_Return;//参数

	//CpuOrderTemp=CpuOrder[CpuOrderNum]+Command_Position[HandleNum];//工位号
	CpuOrderTemp = Command_Position[HandleNum] + CpuOrder[CpuOrderNum];//工位号
	CpuOrderTemp = CpuOrderTemp.Mid(2);

	MD5 mb;
	mb.update(CpuOrderTemp, strlen(CpuOrderTemp));
	CString CpuOrderMD5 = (mb.toString()).c_str();

	CpuOrderComplete = "AT" + CpuOrderTemp + CpuOrderMD5.Left(2);
	if ((m_Result != NULL) && (Final_Result_Control != NULL))
		LogShow_exchange(m_Result, Final_Result_Control, 6, "控制通信发送值:\r\n" + CpuOrderComplete, HandleNum);
	BOOL Result = FALSE;
	for (int i = 0; i<2; i++)
	{
		if (CpuOrderName.Find("模拟电流") != -1)
			Send_Serial_Order(&Serial_Order_Return, CpuOrderComplete, THREAD_NUM, "NULL", (LPSTR)(LPCTSTR)StartSign);
		else
		{
			Send_Serial_Order(&Serial_Order_Return, CpuOrderComplete, THREAD_NUM, "NULL", (LPSTR)(LPCTSTR)StartSign);//Send_Serial_Order(&Serial_Order_Return,CpuOrderComplete,THREAD_NUM,(LPSTR)(LPCTSTR)CpuOrder[CpuOrderNum],(LPSTR)(LPCTSTR)StartSign);
		}
		if ((m_Result != NULL) && (Final_Result_Control != NULL))
		{
			LogShow_exchange(m_Result, Final_Result_Control, 6, "\r\n控制通信返回值:\r\n" + Vaule_Return_Count_CS[THREAD_NUM], HandleNum);
			LogShow_exchange(m_Result, Final_Result_Control, 6, "\r\n期望返回值:\r\n" + CpuOrder[CpuOrderNum], HandleNum);
		}
		if (Serial_Order_Return == "Analysis_SUCCESS")
		{
			if ((m_Result != NULL) && (Final_Result_Control != NULL))
				LogShow_exchange(m_Result, Final_Result_Control, 6, "返回值Analysis_Succ", HandleNum);
			if (CpuOrderName.Find("模拟电流") != -1)
			{
				if (Vaule_Return_Count_CS[THREAD_NUM].Find("A") != -1)
				{
					Result = TRUE;
					SimulationCurrent[HandleNum] = Vaule_Return_Count_CS[THREAD_NUM];
					break;
				}
			}
			else if (Vaule_Return_Count_CS[THREAD_NUM].Find("R" + CpuOrderComplete) != -1)
			{
				if (CpuOrderName.Find("控制板状态") != -1)
				{
					if (Vaule_Return_Count_CS[THREAD_NUM].Find("Open") != -1)
					{
						Result = TRUE;
						break;
					}
					else
					{
						Result = FALSE;
						break;
					}
				}
				else if (CpuOrderName.Find("确认") != -1)
				{
					if (Vaule_Return_Count_CS[THREAD_NUM].Find("Pressed") != -1)
					{
						Result = TRUE;
						break;
					}
					else
					{
						Result = FALSE;
						break;
					}
				}
				else
				{
					Result = TRUE;
					break;
				}
			}
		}
		else
		{
			if ((m_Result != NULL) && (Final_Result_Control != NULL))
				LogShow_exchange(m_Result, Final_Result_Control, 6, "返回值Analysis_Fail", HandleNum);
		}
	}
	LeaveCriticalSection(&CPUCOMMUNICATE);
	Vaule_Return_Count_CS[THREAD_NUM] = "";
	return Result;
}

BOOL IMEIWrite_MulAT::GetExistReturnCode(CAdoInterface& myado, int DataUpNum, CEdit* m_Result, CEdit* Final_Result_Control, CString *ExistMessage, CString ChipRfIDbg)
{
	CString ChipRfIDCurrent;
	if (ChipRfIDbg != "")
	{
		ChipRfIDCurrent = ChipRfIDbg;
	}
	else
	{
		ChipRfIDCurrent = ChipRfID[DataUpNum];
	}
	*ExistMessage = "";
	_variant_t var;
	CString FieldName, FieldValue;
	myado.OpenSheet("select * from dbo.Gps_TestResult WHERE SN ='" + ChipRfIDCurrent + "' AND Version='" + Software_Version[DataUpNum] + "'AND SoftModel='" + MachineType_CS + "'");
	FieldName = "OtherTestSign";
	var = myado.m_pRec->GetCollect((LPCTSTR)FieldName);
	if (var.vt != VT_NULL)
	{
		FieldValue.Format("%s", (LPCSTR)(_bstr_t)var);
		LogShow_exchange(m_Result, Final_Result_Control, 5, "服务器中存在字段:OtherTestSign，成功获取:" + FieldValue, DataUpNum);
		*ExistMessage = FieldValue;

		myado.CloseSheet();
		return TRUE;
	}
	else
	{
		LogShow_exchange(m_Result, Final_Result_Control, 5, "服务器中不存在返工信息", DataUpNum);

	}
	myado.CloseSheet();
	return FALSE;
}
//以下是数据库Data_UpdatePara
BOOL IMEIWrite_MulAT::Data_UpdatePara(CAdoInterface& myado, int DataUpNum, CEdit* m_Result, CEdit* Final_Result_Control, BOOL ErrorUpEnable, CString ChipRfIDbg)//---------0
{
	CString ChipRfIDCurrent;
	if (ChipRfIDbg != "")
	{
		ChipRfIDCurrent = ChipRfIDbg;
	}
	else
	{
		ChipRfIDCurrent = ChipRfID[DataUpNum];
	}
	_variant_t var;
	CString DB_FAIL;
	if (IMEI_Setdlg.Initial_Connect_DB == TRUE)
	{
		CString Conn = "";
		Conn.Format("driver={SQL Server};Server=%s;DATABASE=%s;UID=%s;PWD=%s", IMEI_Setdlg.m_server, IMEI_Setdlg.m_db, IMEI_Setdlg.m_user, IMEI_Setdlg.m_pwd);

		if (myado.m_pCon == NULL)
		{
			CoInitialize(NULL);
			DB_FAIL = myado.ConnecDataLibrary(Conn, "", "", adModeUnknown);
		}
		if (myado.m_pCon->State == 0)  //1表示已经打开,0表示关闭，数据库意外断开，重连
		{
			LogShow_exchange(m_Result, Final_Result_Control, 5, "重新连接数据库.......", DataUpNum);
			DB_FAIL = myado.ConnecDataLibrary(Conn, "", "", adModeUnknown);//数据库意外断开，重连
		}
		else
		{
			LogShow_exchange(m_Result, Final_Result_Control, 0, "数据库连接正常中...", DataUpNum);
		}
		if (DB_FAIL == "FAIL")
		{
			LogShow_exchange(m_Result, Final_Result_Control, 128, "连接数据库失败，无法上传数据！！！请检查网络.......", DataUpNum, "功能测试:3-DB", ChipRfIDCurrent);
			return FALSE;
		}

		OrderNumbersControl.GetWindowTextA(ZhiDanCS);
		if ((ChipRfIDCurrent == "") || (Software_Version[DataUpNum] == ""))
		{
			LogShow_exchange(m_Result, Final_Result_Control, 128, "SN或者软件版本为空", DataUpNum);
			return FALSE;
		}



		//////////////////////////////////////////////////
		//返工操作
		ReturnCodeControl.GetWindowTextA(ReturnCodeCS);
		if (ReturnCodeCS != "")
		{
			BOOL UP_Barcode = TRUE;
			BOOL Barcode_Check_UP;
			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			LogShow_exchange(m_Result, Final_Result_Control, 0, "查看数据总表(返工)...", DataUpNum);
			BOOL ReduceBarcode_Check = FALSE;
			////////////////////////////////检查RFID、机型、软件版本是否已经存在//////////////////////////////
			myado.OpenSheet("select SN,SoftModel,Version,OtherTestSign from dbo.Gps_TestResult WHERE SN ='" + ChipRfIDCurrent + "' AND Version='" + Software_Version[DataUpNum] + "'AND SoftModel='" + MachineType_CS + "'");
			BOOL Barcode_Check = myado.Find("SN='" + ChipRfIDCurrent + "'");
			Barcode_Check &= myado.Find("SoftModel='" + MachineType_CS + "'");
			Barcode_Check &= myado.Find("Version='" + Software_Version[DataUpNum] + "'");
			ReduceBarcode_Check = Barcode_Check;

			myado.CloseSheet();

			CString ExistMessage = "";
			GetExistReturnCode(myado, DataUpNum, m_Result, Final_Result_Control, &ExistMessage, ChipRfIDCurrent);
			Barcode_Check &= (ExistMessage.Find(ReturnCodeCS) != -1);

			if (Barcode_Check == TRUE)
			{
				LogShow_exchange(m_Result, Final_Result_Control, 0, "数据库总表已经有，此机已经测试过(返工)！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！...", DataUpNum);
				return TRUE;//有2
			}
			else
			{
				/*
				myado.OpenSheet("select SN,SoftModel,Version,AutoTestResult from dbo.Gps_TestResult WHERE SN ='"+ChipRfIDCurrent+"' AND Version='"+Software_Version[DataUpNum]+"'AND SoftModel='"+MachineType_CS+"'");
				Barcode_Check=myado.Find("SN='"+ChipRfIDCurrent+"'");
				Barcode_Check&=myado.Find("SoftModel='"+MachineType_CS+"'");
				Barcode_Check&=myado.Find("Version='"+Software_Version[DataUpNum]+"'");
				myado.CloseSheet();
				*/
				Barcode_Check = ReduceBarcode_Check;
				if (Barcode_Check == TRUE)
				{
					LogShow_exchange(m_Result, Final_Result_Control, 0, "数据库已经有功能测试记录，更新(返工)...", DataUpNum);

				}
				else
				{
					LogShow_exchange(m_Result, Final_Result_Control, 0, "数据库不存在，直接上传(返工)...", DataUpNum);
				}
			}
			////////////////////////////////检查RFID是否已经存在//////////////////////////////
			CTime ReturnTime;
			CString ReturnTimeCS;
			ReturnTime = CTime::GetCurrentTime();
			ReturnTimeCS.Format("%04d-%02d-%02d->", ReturnTime.GetYear(), ReturnTime.GetMonth(), ReturnTime.GetDay());


			UP_Barcode = TRUE;
			if (Barcode_Check == TRUE)
			{
				//更新
				CString strSQL_Write_Barcode;
				//myado.OpenSheet("select * from dbo.Gps_TestResult WHERE SN ='"+ChipRfIDCurrent+"' AND Version='"+Software_Version[DataUpNum]+"'AND SoftModel='"+MachineType_CS+"'");

				if (1)
				{
					strSQL_Write_Barcode = "UPDATE dbo.Gps_TestResult SET OtherTestSign = '" + ReturnTimeCS + ReturnCodeCS + ExistMessage + "',ZhiDan = '" + ZhiDanCS + "' WHERE SN ='" + ChipRfIDCurrent + "' AND Version='" + Software_Version[DataUpNum] + "'AND SoftModel='" + MachineType_CS + "'";
					UP_Barcode = myado.Execute(strSQL_Write_Barcode, &var);
				}
				//myado.CloseSheet();
				LogShow_exchange(m_Result, Final_Result_Control, 0, "返工sql:" + strSQL_Write_Barcode, DataUpNum);
				//更新
			}
			else
			{
				//直接上传
				CString strSQL1 = "Insert into dbo.Gps_TestResult(SN,IMEI,Version,SoftModel,OtherTestSign,ZhiDan)\
								  								 values('" + ChipRfIDCurrent + "','','" + Software_Version[DataUpNum] + "','" + MachineType_CS + "','" + ReturnTimeCS + ReturnCodeCS + "','" + ZhiDanCS + "')";


				//myado.OpenSheet("select * from dbo.Gps_TestResult");
				UP_Barcode = myado.Execute(strSQL1, &var);
				//myado.CloseSheet();
				LogShow_exchange(m_Result, Final_Result_Control, 0, "返工sql:" + strSQL1, DataUpNum);
			}
			if (UP_Barcode == TRUE)
			{
				//再次检查
				/*
				Sleep(500);
				myado.OpenSheet("select SN,SoftModel,Version,AutoTestResult from dbo.Gps_TestResult WHERE SN ='"+ChipRfIDCurrent+"' AND Version='"+Software_Version[DataUpNum]+"'AND SoftModel='"+MachineType_CS+"' ");
				Barcode_Check_UP=myado.Find("SN='"+ChipRfIDCurrent+"'");
				Barcode_Check_UP&=myado.Find("SoftModel='"+MachineType_CS+"'");
				Barcode_Check_UP&=myado.Find("Version='"+Software_Version[DataUpNum]+"'");
				Barcode_Check_UP&=myado.Find("AutoTestResult='1'");
				myado.CloseSheet();
				*/
				if (var.intVal<1)//if(var.intVal!=1)
					Barcode_Check_UP = FALSE;
				else
					Barcode_Check_UP = TRUE;
				//再次检查
			}
			if ((UP_Barcode == TRUE&&Barcode_Check_UP == TRUE))
			{
				LogShow_exchange(m_Result, Final_Result_Control, 0, "SN:" + ChipRfIDCurrent + "数据库上传总表成功(返工)...", DataUpNum);
			}
			else
			{
				if (ErrorUpEnable == TRUE)LogShow_exchange(m_Result, Final_Result_Control, 128, "SN:" + ChipRfIDCurrent + "数据库上传总表失败(返工)", DataUpNum, "功能测试:3-DB", ChipRfIDCurrent);
				return FALSE;
			}
			return TRUE;
		}
		//返工操作
		///////////////////////////////////////////////////////////




		///////////////////////////////////////////////////////////
		//上传功能位

		LogShow_exchange(m_Result, Final_Result_Control, 0, "查看功能表...", DataUpNum);

		myado.OpenSheet("select SN,Result,SoftModel,Version from dbo.Gps_AutoTest_Result WHERE SN ='" + ChipRfIDCurrent + "' AND Version='" + Software_Version[DataUpNum] + "'AND SoftModel='" + MachineType_CS + "'");//变1
		BOOL Barcode_Check = myado.Find("SN='" + ChipRfIDCurrent + "'");
		Barcode_Check &= myado.Find("SoftModel='" + MachineType_CS + "'");
		Barcode_Check &= myado.Find("Version='" + Software_Version[DataUpNum] + "'");
		//Barcode_Check&=myado.Find("Result='1'");
		myado.CloseSheet();
		if (Barcode_Check == TRUE)
		{
			LogShow_exchange(m_Result, Final_Result_Control, 0, "数据库已经有功能测试记录，更新...", DataUpNum);
		}
		else
		{
			LogShow_exchange(m_Result, Final_Result_Control, 0, "数据库不存在，直接上传...", DataUpNum);
		}

		BOOL UP_Barcode = TRUE;
		BOOL Barcode_Check_UP;
		if (Barcode_Check == TRUE)
		{

			//更新--被原始系统测试过
			CString strSQL_Write_Barcode;
			//myado.OpenSheet("select * from dbo.Gps_AutoTest_Result WHERE SN ='"+ChipRfIDCurrent+"' AND Version='"+Software_Version[DataUpNum]+"'AND SoftModel='"+MachineType_CS+"'");//变1

			if (1)
			{
				strSQL_Write_Barcode = "UPDATE dbo.Gps_AutoTest_Result SET Result='1',ZhiDan = '" + ZhiDanCS + "',Computer = '" + Hostname + Ipaddress + "',TestSetting = '" + ATCommandList_CSDBCompare + "' WHERE SN ='" + ChipRfIDCurrent + "' AND Version='" + Software_Version[DataUpNum] + "'AND SoftModel='" + MachineType_CS + "'";//变2
				UP_Barcode = myado.Execute(strSQL_Write_Barcode, &var);
			}
			//myado.CloseSheet();
			//更新
		}
		else
		{
			//直接上传
			CString strSQL1 = "Insert into dbo.Gps_AutoTest_Result(SN,IMEI,Version,SoftModel,Result,Remark,TesterId,ZhiDan,Computer,TestSetting)\
							  							 values('" + ChipRfIDCurrent + "','','" + Software_Version[DataUpNum] + "','" + MachineType_CS + "','1','gps','lbc','" + ZhiDanCS + "','" + Hostname + Ipaddress + "','" + ATCommandList_CSDBCompare + "')";//变3


			//myado.OpenSheet("select * from dbo.Gps_AutoTest_Result");
			UP_Barcode = myado.Execute(strSQL1, &var);
			//myado.CloseSheet();
		}
		if (UP_Barcode == TRUE)
		{
			//再次检查
			/*Sleep(500);
			myado.OpenSheet("select SN,SoftModel,Version,Result from dbo.Gps_AutoTest_Result WHERE SN ='"+ChipRfIDCurrent+"' AND Version='"+Software_Version[DataUpNum]+"'AND SoftModel='"+MachineType_CS+"' AND Result ='1'");//变3
			Barcode_Check_UP=myado.Find("SN='"+ChipRfIDCurrent+"'");
			Barcode_Check_UP&=myado.Find("SoftModel='"+MachineType_CS+"'");
			Barcode_Check_UP&=myado.Find("Version='"+Software_Version[DataUpNum]+"'");
			Barcode_Check_UP&=myado.Find("Result='1'");//变1
			myado.CloseSheet();*/
			if (var.intVal<1)//if(var.intVal!=1)
				Barcode_Check_UP = FALSE;
			else
				Barcode_Check_UP = TRUE;
			//再次检查
		}

		if ((UP_Barcode == TRUE&&Barcode_Check_UP == TRUE))
		{
			LogShow_exchange(m_Result, Final_Result_Control, 0, "SN:" + ChipRfIDCurrent + "数据库上传功能位成功...", DataUpNum);
		}
		else
		{
			if (ErrorUpEnable == TRUE)LogShow_exchange(m_Result, Final_Result_Control, 128, "SN:" + ChipRfIDCurrent + "数据库上传功能位失败", DataUpNum, "功能测试:3-DB", ChipRfIDCurrent);
			return FALSE;
		}

		//上传功能位
		///////////////////////////////////////////////////////////


		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		LogShow_exchange(m_Result, Final_Result_Control, 0, "查看数据总表...", DataUpNum);
		BOOL ReduceBarcode_Check = FALSE;
		////////////////////////////////检查RFID、机型、软件版本是否已经存在//////////////////////////////
		myado.OpenSheet("select SN,SoftModel,Version,AutoTestResult from dbo.Gps_TestResult WHERE SN ='" + ChipRfIDCurrent + "' AND Version='" + Software_Version[DataUpNum] + "'AND SoftModel='" + MachineType_CS + "'");
		Barcode_Check = myado.Find("SN='" + ChipRfIDCurrent + "'");
		Barcode_Check &= myado.Find("SoftModel='" + MachineType_CS + "'");
		Barcode_Check &= myado.Find("Version='" + Software_Version[DataUpNum] + "'");
		ReduceBarcode_Check = Barcode_Check;
		Barcode_Check &= myado.Find("AutoTestResult='1'");
		myado.CloseSheet();
		if (Barcode_Check == TRUE)
		{
			LogShow_exchange(m_Result, Final_Result_Control, 0, "数据库总表已经有，此机已经测试过！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！...", DataUpNum);
			//三合一传递信息
			if (g_ADCTFlag == TRUE)
			{
				SendADCTHint(get_map_key_value(PortStatusMap, DataUpNum), "已测试", 0);
			}
			return TRUE;//有2
		}
		else
		{
			/*
			myado.OpenSheet("select SN,SoftModel,Version,AutoTestResult from dbo.Gps_TestResult WHERE SN ='"+ChipRfIDCurrent+"' AND Version='"+Software_Version[DataUpNum]+"'AND SoftModel='"+MachineType_CS+"'");
			Barcode_Check=myado.Find("SN='"+ChipRfIDCurrent+"'");
			Barcode_Check&=myado.Find("SoftModel='"+MachineType_CS+"'");
			Barcode_Check&=myado.Find("Version='"+Software_Version[DataUpNum]+"'");
			myado.CloseSheet();
			*/
			Barcode_Check = ReduceBarcode_Check;
			if (Barcode_Check == TRUE)
			{
				LogShow_exchange(m_Result, Final_Result_Control, 0, "数据库已经有功能测试记录，更新...", DataUpNum);
			}
			else
			{
				LogShow_exchange(m_Result, Final_Result_Control, 0, "数据库不存在，直接上传...", DataUpNum);
			}
		}
		////////////////////////////////检查RFID是否已经存在//////////////////////////////
		UP_Barcode = TRUE;
		if (Barcode_Check == TRUE)
		{
			//更新
			CString strSQL_Write_Barcode;
			//myado.OpenSheet("select * from dbo.Gps_TestResult WHERE SN ='"+ChipRfIDCurrent+"' AND Version='"+Software_Version[DataUpNum]+"'AND SoftModel='"+MachineType_CS+"'");

			if (1)
			{
				//strSQL_Write_Barcode="UPDATE dbo.Gps_TestResult SET AutoTestResult = '1' WHERE SN ='"+ChipRfIDCurrent+"' AND Version='"+Software_Version[DataUpNum]+"'AND SoftModel='"+MachineType_CS+"'";
				strSQL_Write_Barcode = "UPDATE dbo.Gps_TestResult SET AutoTestResult = '1',ZhiDan = '" + ZhiDanCS + "' WHERE SN ='" + ChipRfIDCurrent + "' AND Version='" + Software_Version[DataUpNum] + "'AND SoftModel='" + MachineType_CS + "'";
				UP_Barcode = myado.Execute(strSQL_Write_Barcode, &var);
			}
			//myado.CloseSheet();
			LogShow_exchange(m_Result, Final_Result_Control, 0, "sql:" + strSQL_Write_Barcode, DataUpNum);
			//更新
		}
		else
		{
			//直接上传
			CString strSQL1 = "Insert into dbo.Gps_TestResult(SN,IMEI,Version,SoftModel,AutoTestResult,ZhiDan)\
							  							 values('" + ChipRfIDCurrent + "','','" + Software_Version[DataUpNum] + "','" + MachineType_CS + "','1','" + ZhiDanCS + "')";


			//myado.OpenSheet("select * from dbo.Gps_TestResult");
			UP_Barcode = myado.Execute(strSQL1, &var);
			//myado.CloseSheet();
			LogShow_exchange(m_Result, Final_Result_Control, 0, "sql:" + strSQL1, DataUpNum);
		}
		if (UP_Barcode == TRUE)
		{
			//再次检查
			/*
			Sleep(500);
			myado.OpenSheet("select SN,SoftModel,Version,AutoTestResult from dbo.Gps_TestResult WHERE SN ='"+ChipRfIDCurrent+"' AND Version='"+Software_Version[DataUpNum]+"'AND SoftModel='"+MachineType_CS+"' ");
			Barcode_Check_UP=myado.Find("SN='"+ChipRfIDCurrent+"'");
			Barcode_Check_UP&=myado.Find("SoftModel='"+MachineType_CS+"'");
			Barcode_Check_UP&=myado.Find("Version='"+Software_Version[DataUpNum]+"'");
			Barcode_Check_UP&=myado.Find("AutoTestResult='1'");
			myado.CloseSheet();
			*/
			if (var.intVal<1)//if(var.intVal!=1)
				Barcode_Check_UP = FALSE;
			else
				Barcode_Check_UP = TRUE;
			//再次检查
		}


		if ((UP_Barcode == TRUE&&Barcode_Check_UP == TRUE))
		{
			LogShow_exchange(m_Result, Final_Result_Control, 0, "SN:" + ChipRfIDCurrent + "数据库上传总表成功...", DataUpNum);
		}
		else
		{
			if (ErrorUpEnable == TRUE)LogShow_exchange(m_Result, Final_Result_Control, 128, "SN:" + ChipRfIDCurrent + "数据库上传总表失败", DataUpNum, "功能测试:3-DB", ChipRfIDCurrent);
			return FALSE;
		}

	}
	else
	{
		LogShow_exchange(m_Result, Final_Result_Control, 0, "SN:" + ChipRfIDCurrent + "没有勾选数据库不上传...", DataUpNum);
		return TRUE;
	}
	myado.CloseDataLibrary();
	return TRUE;//LBCCheckIMEI
}
//以上是数据库Data_UpdatePara

//以下是数据库Data_UpdatePara2
BOOL IMEIWrite_MulAT::Data_UpdatePara2(CAdoInterface& myado, int DataUpNum, CEdit* m_Result, CEdit* Final_Result_Control, BOOL ErrorUpEnable, CString ChipRfIDbg)//---------0
{
	CString ChipRfIDCurrent;
	if (ChipRfIDbg != "")
	{
		ChipRfIDCurrent = ChipRfIDbg;
	}
	else
	{
		ChipRfIDCurrent = ChipRfID[DataUpNum];
	}
	_variant_t var;
	CString DB_FAIL;
	if (IMEI_Setdlg.Initial_Connect_DB == TRUE)
	{
		CString Conn = "";
		Conn.Format("driver={SQL Server};Server=%s;DATABASE=%s;UID=%s;PWD=%s", IMEI_Setdlg.m_server, IMEI_Setdlg.m_db, IMEI_Setdlg.m_user, IMEI_Setdlg.m_pwd);

		if (myado.m_pCon == NULL)
		{
			CoInitialize(NULL);
			DB_FAIL = myado.ConnecDataLibrary(Conn, "", "", adModeUnknown);
		}
		if (myado.m_pCon->State == 0)  //1表示已经打开,0表示关闭，数据库意外断开，重连
		{
			LogShow_exchange(m_Result, Final_Result_Control, 5, "重新连接数据库.......", DataUpNum);
			DB_FAIL = myado.ConnecDataLibrary(Conn, "", "", adModeUnknown);//数据库意外断开，重连
		}
		else
		{
			LogShow_exchange(m_Result, Final_Result_Control, 0, "数据库连接正常中...", DataUpNum);
		}
		if (DB_FAIL == "FAIL")
		{
			LogShow_exchange(m_Result, Final_Result_Control, 128, "连接数据库失败，无法上传数据！！！请检查网络.......", DataUpNum, "功能测试:3-DB", ChipRfIDCurrent);
			return FALSE;
		}


		if ((ChipRfIDCurrent == "") || (Software_Version[DataUpNum] == ""))
		{
			LogShow_exchange(m_Result, Final_Result_Control, 128, "SN或者软件版本为空", DataUpNum);
			return FALSE;
		}

		///////////////////////////////////////////////////////////
		//上传功能位

		LogShow_exchange(m_Result, Final_Result_Control, 0, "查看功能表...", DataUpNum);

		myado.OpenSheet("select SN,Result,SoftModel,Version from dbo.Gps_AutoTest_Result2 WHERE SN ='" + ChipRfIDCurrent + "' AND Version='" + Software_Version[DataUpNum] + "'AND SoftModel='" + MachineType_CS + "'");//变1
		BOOL Barcode_Check = myado.Find("SN='" + ChipRfIDCurrent + "'");
		Barcode_Check &= myado.Find("SoftModel='" + MachineType_CS + "'");
		Barcode_Check &= myado.Find("Version='" + Software_Version[DataUpNum] + "'");
		//Barcode_Check&=myado.Find("Result='1'");
		myado.CloseSheet();
		if (Barcode_Check == TRUE)
		{
			LogShow_exchange(m_Result, Final_Result_Control, 0, "数据库已经有功能测试记录，更新...", DataUpNum);
		}
		else
		{
			LogShow_exchange(m_Result, Final_Result_Control, 0, "数据库不存在，直接上传...", DataUpNum);
		}

		BOOL UP_Barcode = TRUE;
		BOOL Barcode_Check_UP;
		if (Barcode_Check == TRUE)
		{

			//更新--被原始系统测试过
			CString strSQL_Write_Barcode;
			//myado.OpenSheet("select * from dbo.Gps_AutoTest_Result2 WHERE SN ='"+ChipRfIDCurrent+"' AND Version='"+Software_Version[DataUpNum]+"'AND SoftModel='"+MachineType_CS+"'");//变1

			if (1)
			{
				strSQL_Write_Barcode = "UPDATE dbo.Gps_AutoTest_Result2 SET Result='1',ZhiDan = '" + ZhiDanCS + "',Computer = '" + Hostname + Ipaddress + "',TestSetting = '" + ATCommandList_CSDBCompare + "' WHERE SN ='" + ChipRfIDCurrent + "' AND Version='" + Software_Version[DataUpNum] + "'AND SoftModel='" + MachineType_CS + "'";//变2
				UP_Barcode = myado.Execute(strSQL_Write_Barcode, &var);
			}
			//myado.CloseSheet();
			//更新
		}
		else
		{
			//直接上传
			CString strSQL1 = "Insert into dbo.Gps_AutoTest_Result2(SN,IMEI,Version,SoftModel,Result,Remark,TesterId,ZhiDan,Computer,TestSetting)\
							  							 values('" + ChipRfIDCurrent + "','','" + Software_Version[DataUpNum] + "','" + MachineType_CS + "','1','gps','lbc','" + ZhiDanCS + "','" + Hostname + Ipaddress + "','" + ATCommandList_CSDBCompare + "')";//变3	

			//myado.OpenSheet("select * from dbo.Gps_AutoTest_Result2");
			UP_Barcode = myado.Execute(strSQL1, &var);
			//myado.CloseSheet();
		}
		if (UP_Barcode == TRUE)
		{
			//再次检查
			/*Sleep(500);
			myado.OpenSheet("select SN,SoftModel,Version,Result from dbo.Gps_AutoTest_Result2 WHERE SN ='"+ChipRfIDCurrent+"' AND Version='"+Software_Version[DataUpNum]+"'AND SoftModel='"+MachineType_CS+"' AND Result ='1'");//变3
			Barcode_Check_UP=myado.Find("SN='"+ChipRfIDCurrent+"'");
			Barcode_Check_UP&=myado.Find("SoftModel='"+MachineType_CS+"'");
			Barcode_Check_UP&=myado.Find("Version='"+Software_Version[DataUpNum]+"'");
			Barcode_Check_UP&=myado.Find("Result='1'");//变1
			myado.CloseSheet();*/
			if (var.intVal<1)//if(var.intVal!=1)
				Barcode_Check_UP = FALSE;
			else
				Barcode_Check_UP = TRUE;
			//再次检查
		}

		if ((UP_Barcode == TRUE&&Barcode_Check_UP == TRUE))
		{
			LogShow_exchange(m_Result, Final_Result_Control, 0, "SN:" + ChipRfIDCurrent + "数据库上传功能位成功...", DataUpNum);
		}
		else
		{
			if (ErrorUpEnable == TRUE)LogShow_exchange(m_Result, Final_Result_Control, 128, "SN:" + ChipRfIDCurrent + "数据库上传功能位失败", DataUpNum, "功能测试:3-DB", ChipRfIDCurrent);
			return FALSE;
		}

		//上传功能位
		///////////////////////////////////////////////////////////


		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		LogShow_exchange(m_Result, Final_Result_Control, 0, "查看数据总表...", DataUpNum);
		////////////////////////////////检查RFID、机型、软件版本是否已经存在//////////////////////////////
		myado.OpenSheet("select SN,SoftModel,Version,FunctionResult from dbo.Gps_TestResult WHERE SN ='" + ChipRfIDCurrent + "' AND Version='" + Software_Version[DataUpNum] + "'AND SoftModel='" + MachineType_CS + "'");
		Barcode_Check = myado.Find("SN='" + ChipRfIDCurrent + "'");
		Barcode_Check &= myado.Find("SoftModel='" + MachineType_CS + "'");
		Barcode_Check &= myado.Find("Version='" + Software_Version[DataUpNum] + "'");
		Barcode_Check &= myado.Find("FunctionResult='1'");
		myado.CloseSheet();
		if (Barcode_Check == TRUE)
		{
			LogShow_exchange(m_Result, Final_Result_Control, 0, "数据库总表已经有，此机已经测试过！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！...", DataUpNum);
			return TRUE;//有2
		}
		else
		{
			myado.OpenSheet("select SN,SoftModel,Version,FunctionResult from dbo.Gps_TestResult WHERE SN ='" + ChipRfIDCurrent + "' AND Version='" + Software_Version[DataUpNum] + "'AND SoftModel='" + MachineType_CS + "'");
			Barcode_Check = myado.Find("SN='" + ChipRfIDCurrent + "'");
			Barcode_Check &= myado.Find("SoftModel='" + MachineType_CS + "'");
			Barcode_Check &= myado.Find("Version='" + Software_Version[DataUpNum] + "'");
			myado.CloseSheet();
			if (Barcode_Check == TRUE)
			{
				LogShow_exchange(m_Result, Final_Result_Control, 0, "数据库已经有功能测试记录，更新...", DataUpNum);
			}
			else
			{
				LogShow_exchange(m_Result, Final_Result_Control, 0, "数据库不存在，直接上传...", DataUpNum);
			}
		}
		////////////////////////////////检查RFID是否已经存在//////////////////////////////
		UP_Barcode = TRUE;
		if (Barcode_Check == TRUE)
		{
			//更新
			CString strSQL_Write_Barcode;
			//myado.OpenSheet("select * from dbo.Gps_TestResult WHERE SN ='"+ChipRfIDCurrent+"' AND Version='"+Software_Version[DataUpNum]+"'AND SoftModel='"+MachineType_CS+"'");

			if (1)
			{
				strSQL_Write_Barcode = "UPDATE dbo.Gps_TestResult SET FunctionResult = '1' WHERE SN ='" + ChipRfIDCurrent + "' AND Version='" + Software_Version[DataUpNum] + "'AND SoftModel='" + MachineType_CS + "'";
				UP_Barcode = myado.Execute(strSQL_Write_Barcode, &var);
			}
			//myado.CloseSheet();
			//更新
		}
		else
		{
			//直接上传
			CString strSQL1 = "Insert into dbo.Gps_TestResult(SN,IMEI,Version,SoftModel,FunctionResult)\
							  							 values('" + ChipRfIDCurrent + "','','" + Software_Version[DataUpNum] + "','" + MachineType_CS + "','1')";


			//myado.OpenSheet("select * from dbo.Gps_TestResult");
			UP_Barcode = myado.Execute(strSQL1, &var);
			//myado.CloseSheet();
		}
		if (UP_Barcode == TRUE)
		{
			//再次检查
			if (var.intVal<1)//if(var.intVal!=1)
				Barcode_Check_UP = FALSE;
			else
				Barcode_Check_UP = TRUE;
			//再次检查
		}


		if ((UP_Barcode == TRUE&&Barcode_Check_UP == TRUE))
		{
			LogShow_exchange(m_Result, Final_Result_Control, 0, "SN:" + ChipRfIDCurrent + "数据库上传总表成功...", DataUpNum);
		}
		else
		{
			if (ErrorUpEnable == TRUE)LogShow_exchange(m_Result, Final_Result_Control, 128, "SN:" + ChipRfIDCurrent + "数据库上传总表失败", DataUpNum, "功能测试:3-DB", ChipRfIDCurrent);
			return FALSE;
		}

	}
	else
	{
		LogShow_exchange(m_Result, Final_Result_Control, 0, "SN:" + ChipRfIDCurrent + "没有勾选数据库不上传...", DataUpNum);
		return TRUE;
	}
	myado.CloseDataLibrary();
	return TRUE;//LBCCheckIMEI
}
//以上是数据库Data_UpdatePara2

//以下是数据库Data_UpdateParaPre
BOOL IMEIWrite_MulAT::Data_UpdateParaPre(CAdoInterface& myado, int DataUpNum, CEdit* m_Result, CEdit* Final_Result_Control, BOOL* smt_Check)//---------0
{
	CString ChipRfIDCurrent;
	ChipRfIDCurrent = ChipRfID[DataUpNum];
	_variant_t var;
	CString DB_FAIL;
	if (IMEI_Setdlg.Initial_Connect_DB == TRUE)
	{
		CString Conn = "";

		Conn.Format("driver={SQL Server};Server=%s;DATABASE=%s;UID=%s;PWD=%s", IMEI_Setdlg.m_server, IMEI_Setdlg.m_db, IMEI_Setdlg.m_user, IMEI_Setdlg.m_pwd);

		if (myado.m_pCon == NULL)
		{
			CoInitialize(NULL);
			DB_FAIL = myado.ConnecDataLibrary(Conn, "", "", adModeUnknown);
		}
		if (myado.m_pCon->State == 0)  //1表示已经打开,0表示关闭，数据库意外断开，重连
		{
			LogShow_exchange(m_Result, Final_Result_Control, 5, "重新连接数据库.......", DataUpNum);
			DB_FAIL = myado.ConnecDataLibrary(Conn, "", "", adModeUnknown);//数据库意外断开，重连
		}
		else
		{
			LogShow_exchange(m_Result, Final_Result_Control, 0, "数据库连接正常中...", DataUpNum);
		}
		if (DB_FAIL == "FAIL")
		{
			LogShow_exchange(m_Result, Final_Result_Control, 128, "连接数据库失败，无法上传数据！！！请检查网络.......", DataUpNum, "功能测试:3-DB");
			return FALSE;
		}

		if ((ChipRfIDCurrent == "") || (Software_Version[DataUpNum] == ""))
		{
			LogShow_exchange(m_Result, Final_Result_Control, 5, "SN或者软件版本为空", DataUpNum);
			return FALSE;
		}



		//////////////////////////////////////////////////
		//卡SMT测试
		//WorkStationControl.GetWindowTextA(WorkStationCS);
		//if(WorkStationCS=="")WorkStationControl.GetWindowTextA(WorkStationCS);
		if ((CheckSMTChoose == TRUE) && ((WorkStationCS != "SMT测试") && (WorkStationCS != "研发测试")))
		{
			//myado.OpenSheet("select SN,SoftModel,Version,AutoTestResult from dbo.Gps_TestResult WHERE SN ='"+ChipRfIDCurrent+"' AND Version='"+Software_Version[DataUpNum]+"'AND SoftModel='"+MachineType_CS+"'");
			myado.OpenSheet("select SN,SoftModel,Version,FunctionResult from dbo.Gps_TestResult WHERE SN ='" + ChipRfIDCurrent + "' AND Version='" + Software_Version[DataUpNum] + "'");
			BOOL SMT_Check = myado.Find("SN='" + ChipRfIDCurrent + "'");
			//SMT_Check&=myado.Find("SoftModel='"+MachineType_CS+"'");
			SMT_Check &= myado.Find("Version='" + Software_Version[DataUpNum] + "'");
			SMT_Check &= myado.Find("FunctionResult='1'");
			myado.CloseSheet();

			*smt_Check = SMT_Check;
			if (SMT_Check == TRUE)
			{
				LogShow_exchange(m_Result, Final_Result_Control, 0, "SMT功能测试通过...", DataUpNum);
			}
			else
			{
				myado.OpenSheet("select SN,SoftModel,Version,FunctionResult from dbo.Gps_TestResult WHERE SN ='" + ChipRfIDCurrent + "'");
				BOOL SMT_Check = myado.Find("SN='" + ChipRfIDCurrent + "'");
				//SMT_Check&=myado.Find("SoftModel='"+MachineType_CS+"'");
				//SMT_Check&=myado.Find("Version='"+Software_Version[DataUpNum]+"'");
				SMT_Check &= myado.Find("FunctionResult='1'");
				myado.CloseSheet();

				*smt_Check = SMT_Check;
				if (SMT_Check == TRUE)
				{
					LogShow_exchange(m_Result, Final_Result_Control, 0, "SMT功能测试通过...", DataUpNum);
				}
				else
				{
					LogShow_exchange(m_Result, Final_Result_Control, 128, "SN:" + ChipRfIDCurrent + "SMT功能漏测！！！", DataUpNum, "功能测试:3-DB");
					return TRUE;
				}
			}
		}
		else
			*smt_Check = TRUE;
		//卡SMT测试
		/////////////////////////////////////////////////



		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		LogShow_exchange(m_Result, Final_Result_Control, 0, "查看数据总表...", DataUpNum);//Data_UpdateParaPre
		////////////////////////////////检查RFID、机型、软件版本是否已经存在//////////////////////////////
		myado.OpenSheet("select SN,SoftModel,Version,AutoTestResult,FunctionResult from dbo.Gps_TestResult WHERE SN ='" + ChipRfIDCurrent + "' AND Version='" + Software_Version[DataUpNum] + "'AND SoftModel='" + MachineType_CS + "'");
		BOOL Barcode_Check = myado.Find("SN='" + ChipRfIDCurrent + "'");
		Barcode_Check &= myado.Find("SoftModel='" + MachineType_CS + "'");
		Barcode_Check &= myado.Find("Version='" + Software_Version[DataUpNum] + "'");
		CString m_dbNew;
		//WorkStationControl.GetWindowTextA(WorkStationCS);
		//if(WorkStationCS=="")WorkStationControl.GetWindowTextA(WorkStationCS);
		if (WorkStationCS == "SMT测试")
			Barcode_Check &= myado.Find("FunctionResult='1'");
		else if (WorkStationCS == "组装测试")
			Barcode_Check &= myado.Find("AutoTestResult='1'");
		else if (WorkStationCS == "研发测试")
			Barcode_Check = FALSE;
		else
		{
			LogShow_exchange(m_Result, Final_Result_Control, 128, "请先选择站位！！！", DataUpNum, "功能测试:2-TEST");
			return FALSE;
		}
		///////////////////////////////////
		///////////////////////////////////
		myado.CloseSheet();

		ReturnCodeControl.GetWindowTextA(ReturnCodeCS);
		if (ReturnCodeCS != "")
			/////////////////////////////////////
		{
			LogShow_exchange(m_Result, Final_Result_Control, 0, "查看返工编码...", DataUpNum);
			CString ExistMessage = "";
			GetExistReturnCode(myado, DataUpNum, m_Result, Final_Result_Control, &ExistMessage, ChipRfIDCurrent);
			Barcode_Check &= (ExistMessage.Find(ReturnCodeCS) != -1);
		}
		/////////////////////////////////////
		if (Barcode_Check == TRUE)
		{
			LogShow_exchange(m_Result, Final_Result_Control, 0, "数据库总表已经有，此机已经测试过！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！...", DataUpNum);
			//三合一传递信息
			if (g_ADCTFlag == TRUE)
			{
				SendADCTHint(get_map_key_value(PortStatusMap, DataUpNum), "已测试", 0);
			}
			return TRUE;//已经测试过
		}

	}
	else
	{
		LogShow_exchange(m_Result, Final_Result_Control, 0, "SN:" + ChipRfIDCurrent + "没有勾选数据库不上传...", DataUpNum);
	}
	return FALSE;
}
//以上是数据库Data_UpdateParaPre
BOOL IMEIWrite_MulAT::Data_UpdateError(CAdoInterface& myado, int DataUpNum, CEdit* m_Result, CEdit* Final_Result_Control, CString Message, CString Category, CString ChipRfIDbg)//LogShow_exchange(m_Result,Final_Result_Control,5
{
	CString ChipRfIDCurrent;
	if (ChipRfIDbg != "")
	{
		ChipRfIDCurrent = ChipRfIDbg;
	}
	else
	{
		ChipRfIDCurrent = ChipRfID[DataUpNum];
	}
	_variant_t var;
	CString DB_FAIL;
	if (IMEI_Setdlg.Initial_Connect_DB == TRUE)
	{
		CString Conn = "";
		Conn.Format("driver={SQL Server};Server=%s;DATABASE=%s;UID=%s;PWD=%s", IMEI_Setdlg.m_server, IMEI_Setdlg.m_db, IMEI_Setdlg.m_user, IMEI_Setdlg.m_pwd);

		if (myado.m_pCon == NULL)
		{
			CoInitialize(NULL);
			DB_FAIL = myado.ConnecDataLibrary(Conn, "", "", adModeUnknown);
		}
		if (myado.m_pCon->State == 0)  //1表示已经打开,0表示关闭，数据库意外断开，重连
		{
			LogShow_exchange(m_Result, Final_Result_Control, 5, "重新连接数据库.......", DataUpNum);
			DB_FAIL = myado.ConnecDataLibrary(Conn, "", "", adModeUnknown);//数据库意外断开，重连
		}
		else
		{
			//LogShow_exchange(m_Result,Final_Result_Control,5,"数据库连接正常中...",DataUpNum);
		}
		if (DB_FAIL == "FAIL")
		{
			LogShow_exchange(m_Result, Final_Result_Control, 5, "连接数据库失败，无法上传数据！！！请检查网络.......", DataUpNum);
			return FALSE;
		}

		OrderNumbersControl.GetWindowTextA(ZhiDanCS);
		if ((ChipRfIDCurrent == "") || (Software_Version[DataUpNum] == ""))
		{
			LogShow_exchange(m_Result, Final_Result_Control, 5, "SN或者软件版本为空", DataUpNum);
			return FALSE;
		}

		if (SuccessCount<80)
		{
			LogShow_exchange(m_Result, Final_Result_Control, 5, "刚开始成功记录不到80，处于调试状态，暂时不记错误", DataUpNum);
			return TRUE;
		}
		///////////////////////////////////////////////////////////
		//上传错误信息

		//LogShow_exchange(m_Result,Final_Result_Control,5,"查看log信息表...",DataUpNum);

		myado.OpenSheet("select SN,SoftModel,Version from dbo.LTestLogMessage WHERE SN ='" + ChipRfIDCurrent + "' AND Version='" + Software_Version[DataUpNum] + "'AND SoftModel='" + MachineType_CS + "'");//变1
		BOOL Barcode_Check = myado.Find("SN='" + ChipRfIDCurrent + "'");
		Barcode_Check &= myado.Find("SoftModel='" + MachineType_CS + "'");
		Barcode_Check &= myado.Find("Version='" + Software_Version[DataUpNum] + "'");
		myado.CloseSheet();
		CString ExistMessage = "";
		if (Barcode_Check == TRUE)
		{
			GetExistError(myado, DataUpNum, m_Result, Final_Result_Control, &ExistMessage, ChipRfIDCurrent);//GetExistError(myado,DataUpNum,m_Result,Final_Result_Control,&ExistMessage,ChipRfIDbg);
			//LogShow_exchange(m_Result,Final_Result_Control,5,"数据库已经有功能测试记录，更新...",DataUpNum);
		}
		else
		{
			//LogShow_exchange(m_Result,Final_Result_Control,5,"数据库不存在，直接上传...",DataUpNum);
		}


		CTime logtime;
		logtime = CTime::GetCurrentTime();

		CString DataUpNumCS;
		DataUpNumCS.Format("->%02d:%02d:%02d线程%02d", logtime.GetHour(), logtime.GetMinute(), logtime.GetSecond(), DataUpNum);

		Message += DataUpNumCS;
		int epos = ExistMessage.Find(Message);
		if (epos == -1)								//新的错误
			ExistMessage += ("\r\n+++" + Message);
		else										//已经存在此错误
		{
			ExistMessage = ExistMessage.Left(epos) + Message + "|" + ExistMessage.Mid(epos + Message.GetLength());
		}

		BOOL UP_Barcode = TRUE;
		BOOL Barcode_Check_UP;

		//WorkStationControl.GetWindowTextA(WorkStationCS);
		//if(WorkStationCS=="")WorkStationControl.GetWindowTextA(WorkStationCS);
		if (WorkStationCS == "研发测试")
			return TRUE;

		if (Barcode_Check == TRUE)
		{
			CString strSQL_Write_Barcode;
			//myado.OpenSheet("select * from dbo.LTestLogMessage WHERE SN ='"+ChipRfIDCurrent+"' AND Version='"+Software_Version[DataUpNum]+"'AND SoftModel='"+MachineType_CS+"'");//变1

			if (1)
			{
				if (WorkStationCS == "SMT测试")
					strSQL_Write_Barcode = "UPDATE dbo.LTestLogMessage SET ErrorMessage3='" + ExistMessage + "' ,ZhiDan='" + ZhiDanCS + "' ,CategoryNum3='" + Category + "',Computer3 = '" + Hostname + Ipaddress + "' WHERE SN ='" + ChipRfIDCurrent + "' AND Version='" + Software_Version[DataUpNum] + "'AND SoftModel='" + MachineType_CS + "'";//变2
				else if (WorkStationCS == "组装测试")
					strSQL_Write_Barcode = "UPDATE dbo.LTestLogMessage SET ErrorMessage1='" + ExistMessage + "' ,ZhiDan='" + ZhiDanCS + "' ,CategoryNum1='" + Category + "',Computer = '" + Hostname + Ipaddress + "' WHERE SN ='" + ChipRfIDCurrent + "' AND Version='" + Software_Version[DataUpNum] + "'AND SoftModel='" + MachineType_CS + "'";

				UP_Barcode = myado.Execute(strSQL_Write_Barcode, &var);
			}
			//myado.CloseSheet();
			//更新
		}
		else
		{
			//直接上传
			CString strSQL1;
			if (WorkStationCS == "SMT测试")
				strSQL1 = "Insert into dbo.LTestLogMessage(SN,SoftModel,Version,ZhiDan,ErrorMessage3,CategoryNum3,Computer3)\
						  							 values('" + ChipRfIDCurrent + "','" + MachineType_CS + "','" + Software_Version[DataUpNum] + "','" + ZhiDanCS + "','" + ExistMessage + "','" + Category + "','" + Hostname + Ipaddress + "')";//变3
			else if (WorkStationCS == "组装测试")
				strSQL1 = "Insert into dbo.LTestLogMessage(SN,SoftModel,Version,ZhiDan,ErrorMessage1,CategoryNum1,Computer)\
						  							 values('" + ChipRfIDCurrent + "','" + MachineType_CS + "','" + Software_Version[DataUpNum] + "','" + ZhiDanCS + "','" + ExistMessage + "','" + Category + "','" + Hostname + Ipaddress + "')";


			//myado.OpenSheet("select * from dbo.LTestLogMessage");
			UP_Barcode = myado.Execute(strSQL1, &var);
			//myado.CloseSheet();
		}
		if (UP_Barcode == TRUE)
		{
			//再次检查
			/*Sleep(500);
			myado.OpenSheet("select SN,SoftModel,Version from dbo.LTestLogMessage WHERE SN ='"+ChipRfIDCurrent+"' AND Version='"+Software_Version[DataUpNum]+"'AND SoftModel='"+MachineType_CS+"' ");//变3
			Barcode_Check_UP=myado.Find("SN='"+ChipRfIDCurrent+"'");
			Barcode_Check_UP&=myado.Find("SoftModel='"+MachineType_CS+"'");
			Barcode_Check_UP&=myado.Find("Version='"+Software_Version[DataUpNum]+"'");
			myado.CloseSheet();
			*/
			if (var.intVal<1)//if(var.intVal!=1)
				Barcode_Check_UP = FALSE;
			else
				Barcode_Check_UP = TRUE;
			//再次检查
		}

		if ((UP_Barcode == TRUE&&Barcode_Check_UP == TRUE))
		{
			LogShow_exchange(m_Result, Final_Result_Control, 5, "SN:" + ChipRfIDCurrent + "上传错误信息OK...", DataUpNum);
		}
		else
		{
			LogShow_exchange(m_Result, Final_Result_Control, 5, "SN:" + ChipRfIDCurrent + "上传错误信息FAIL", DataUpNum);//LogShow_exchange(m_Result,Final_Result_Control,5
			return FALSE;
		}

		//上传错误信息
		///////////////////////////////////////////////////////////
	}
	return TRUE;
}

BOOL IMEIWrite_MulAT::GetExistError(CAdoInterface& myado, int DataUpNum, CEdit* m_Result, CEdit* Final_Result_Control, CString *ExistMessage, CString ChipRfIDbg)//---------2
{
	CString ChipRfIDCurrent;
	if (ChipRfIDbg != "")
	{
		ChipRfIDCurrent = ChipRfIDbg;
	}
	else
	{
		ChipRfIDCurrent = ChipRfID[DataUpNum];
	}
	_variant_t var;
	CString FieldName, FieldValue;
	myado.OpenSheet("select * from dbo.LTestLogMessage WHERE SN ='" + ChipRfIDCurrent + "' AND Version='" + Software_Version[DataUpNum] + "'AND SoftModel='" + MachineType_CS + "'");
	//var= myado.m_pRec->GetCollect(_variant_t((long)5));
	FieldName.Format("ErrorMessage%d", 1);
	var = myado.m_pRec->GetCollect((LPCTSTR)FieldName);
	if (var.vt != VT_NULL)
	{
		FieldValue.Format("%s", (LPCSTR)(_bstr_t)var);
		//LogShow_exchange(m_Result,Final_Result_Control,5,"服务器中存在字段:ErrorMessage1，成功获取:"+FieldValue,DataUpNum);
		*ExistMessage = FieldValue;
	}
	else
	{
		//LogShow_exchange(m_Result,Final_Result_Control,5,"服务器中不存在错误信息",DataUpNum);

	}
	myado.CloseSheet();//---------2
	return TRUE;
}

void IMEIWrite_MulAT::Voice_Ini()
{
	pVoice = NULL;
	if (FAILED(::CoInitialize(NULL)))
		return;
	hr = CoCreateInstance(CLSID_SpVoice, NULL, CLSCTX_ALL, IID_ISpVoice, (void **)&pVoice);
	pVoice->SetVolume(100);	//0~100
	pVoice->SetRate(8);		//-10~+10
}
void IMEIWrite_MulAT::Voice_Speak(CString Text)
{
	EnterCriticalSection(&SPEAKVOICE);
	if (SUCCEEDED(hr))
	{
		hr = pVoice->Speak(Text.AllocSysString(), SPF_ASYNC, NULL);
	}
	LeaveCriticalSection(&SPEAKVOICE);
	//::CoUninitialize();
}




/*菜单栏*/

//获取配置，网络获取
void IMEIWrite_MulAT::OnGetWebSetting()
{
	CString Serial_Order_Return, Serial_Order_ReturnTemp;//Serial_Order_Return是正常使用的参数，Serial_Order_ReturnTemp是给蓝牙指令用的参数
	CString OrderNumbersControlCS;
	CString StrMac;
	GetDlgItemText(IDC_SCANDATA_EDIT, StrMac);

	UpdateData(TRUE);
	if (GetBluetoothCheckValue == TRUE)
	{
		if (hScanGun[0] == NULL)
		{
			AfxMessageBox("请先打开串口扫描枪！");
			goto WEBGETFAILE;
		}
		if (StopSign[0] == FALSE)
		{
			AfxMessageBox("请先关闭串口线程！");
			goto WEBGETFAILE;
		}
		if (Thread_State[0] == TRUE)
		{
			AfxMessageBox("线程1串口已经打开");
			goto WEBGETFAILE;
		}
		if (StrMac == "")
		{
			AfxMessageBox("请先扫入MAC地址");
			goto WEBGETFAILE;
		}
	}


	if (GetIp_Name(Hostname, Ipaddress) == FALSE)
	{
		AfxMessageBoxHint("获取本机信息失败！！");
		goto WEBGETFAILE;
	}
	else
	{
		AfxMessageBoxHint(Hostname + Ipaddress);
	}

	OrderNumbersControl.GetWindowTextA(OrderNumbersControlCS);
	ReturnCodeControl.GetWindowTextA(ReturnCodeCS);
	if (OrderNumbersControlCS == "")
	{
		AfxMessageBoxHint("请先输入订单号");
		goto WEBGETFAILE;
	}
	OrderNumbersControlCS += ReturnCodeCS;

	//蓝牙整机测试的连接会在这里进行判断
	if (GetBluetoothCheckValue == TRUE)
	{
		//AfxMessageBox("在显示可扫后请扫入MAC地址！");

		BLEGetSettingFlag = 0;
		//BluetoothHint(0,"可扫");
		//while (BLEGetSettingFlag == 0)
		//{
		//	Get_ScanGun_Data(&m_Result1, &Final_Result_Control1, 0, "\r");
		//	Sleep(500);
		//}
		BluetoothCoreFunction(StrMac);

		if (BLEGetSettingFlag == 2 || BLEGetSettingFlag == -1)
		{
			if (BLEGetSettingFlag == 2)
			{
				AfxMessageBox("蓝牙连接失败，请检查透传模块与蓝牙设备");
			}

			goto WEBGETFAILE;
		}
		BluetoothHint(0, "连接");
	}


	//如果是三合一的时候这段不执行，直接使用ADCT主控程序传来的版本号
	if (g_ADCTFlag != 1)
	{
		if (Thread_State[0] == TRUE)
		{
			AfxMessageBox("线程1串口已经打开");
			goto WEBGETFAILE;
		}
		if (OPen_Serial_Port(&m_Port1, &m_Baud1, 0) == TRUE)//OnGetWebSetting
		{
		}
		else
		{
			AfxMessageBox("打开串口失败");
			goto WEBGETFAILE;
		}
		Send_Serial_Order(&Serial_Order_Return, "AT^GT_CM=VERSION", 0, "NULL", "", 500);


		/*
		int pos=Vaule_Return_Count_CS[0].Find(':');
		if(pos>=0)
		{
		Vaule_Return_Count_CS[0]=Vaule_Return_Count_CS[0].Mid(pos+1);
		pos=Vaule_Return_Count_CS[0].Find(' ');
		if(pos>=0)
		Software_Version[0]=Vaule_Return_Count_CS[0].Left(pos);
		else
		Software_Version[0]=Vaule_Return_Count_CS[0].Left(22);
		}
		else
		*/
		//////////////////////////////////////////////////
		{
			int pos = Vaule_Return_Count_CS[0].Find(']');
			if (pos >= 0)
				Vaule_Return_Count_CS[0] = Vaule_Return_Count_CS[0].Mid(pos + 1);
			pos = Vaule_Return_Count_CS[0].Find('[');
			if (pos >= 0)
				Software_Version[0] = Vaule_Return_Count_CS[0].Left(pos);
			else
				Software_Version[0] = Vaule_Return_Count_CS[0];
		}


		Software_Version[0].Replace("\r", "");
		Software_Version[0].Replace("\n", "");

		//如果是蓝牙整机测试，到这里获取版本号后，就可以发送蓝牙断开指令，来断开透传模块和蓝牙的连接了
		if (GetBluetoothCheckValue == TRUE)
		{
			BluetoothDisconnect(0);
		}

		BOOL Return;
		if (hPort[0] != NULL)
			Return = CloseHandle(hPort[0]);				//测试成功后
		hPort[0] = NULL;
		if (Return == TRUE)
		{
		}
		else
		{
			AfxMessageBoxHint("关闭串口失败，重新插入USB可解决问题");
		}
	}
	//以上到这里过都不执行，三合一的话直接获取版本
	else if (g_ADCTFlag == 1)
	{
		Software_Version[0] = m_SoftVersion;
	}

	if (Serial_Order_Return != "FAIL")//成功
	{
		if (Software_Version[0] != "")
		{
			CString m_dbNew;
			WorkStationControl.GetWindowTextA(WorkStationCS);
			if (WorkStationCS == "")WorkStationControl.GetWindowTextA(WorkStationCS);
			if (WorkStationCS == "SMT测试")
				m_dbNew = "NetMarkIMEI";
			else if (WorkStationCS == "组装测试")
				m_dbNew = "NetMarkIMEI2";
			else if (WorkStationCS == "研发测试")
				m_dbNew = "NetMarkIMEI3";
			else
			{
				AfxMessageBoxHint("请先选择站位！");
				goto WEBGETFAILE;
			}
			if (GetSettingFromDB(IMEI_Setdlg.myado, IMEI_Setdlg.m_server, m_dbNew, IMEI_Setdlg.m_user, IMEI_Setdlg.m_pwd, Software_Version[0] + OrderNumbersControlCS, &m_Result1, &Final_Result_Control1) == TRUE)
			{
				Software_VersionFull = Software_Version[0];

				AfxMessageBoxHint("获取成功，可以开始测试了");
				//else if (g_ADCTFlag == 1)
				//{
				//	SendADCTHint("")
				//}
				EnableWindow_StartALL(TRUE);
				m_bVar = 1;//只有进入到这里才算是真正获取成功

				//只有蓝牙整机测试才会执行这块
				if (GetBluetoothCheckValue == TRUE)
				{
					BLEGetSettingEndFlag = 1;
					AfxMessageBox("注意！只有当端口显示可扫时才可以扫入MAC地址！！");
					MacScanEnbleFlag = TRUE;
					BluetoothHint(0, "可扫");
				}
				return;
			}
		}
	}
	else
	{
		AfxMessageBoxHint("读取软件版本失败！！");
		goto WEBGETFAILE;
	}

	//因为要添加很多相同的东西，所以失败的时候这里改成了用goto的方法
WEBGETFAILE:
	if (GetBluetoothCheckValue == TRUE)
	{
		BLEGetSettingEndFlag = 0;
		BLEGetSettingFlag = -1;
		MacScanEnbleFlag = TRUE;
		BluetoothHint(0, "W");
	}

	m_bVar = 0;
	return;
}

BOOL IMEIWrite_MulAT::GetSettingFromDB(CAdoInterface& myado, CString m_server, CString m_db, CString m_user, CString m_pwd, CString SoftwareVer, CEdit* m_Result, CEdit* Final_Result_Control)//---------3
{
	if (IMEI_Setdlg.Initial_Connect_DB == TRUE)
	{
		CString Conn = "";
		CString DB_FAIL = "";
		Conn.Format("driver={SQL Server};Server=%s;DATABASE=%s;UID=%s;PWD=%s", m_server, m_db, m_user, m_pwd);
		if (myado.m_pCon == NULL)
		{
			CoInitialize(NULL);
			DB_FAIL = myado.ConnecDataLibrary(Conn, "", "", adModeUnknown);
		}
		if (myado.m_pCon->State == 0)  //1表示已经打开,0表示关闭，数据库意外断开，重连
		{
			LogShow_exchange(m_Result, Final_Result_Control, 5, "重新连接数据库.......", 0);
			DB_FAIL = myado.ConnecDataLibrary(Conn, "", "", adModeUnknown);//数据库意外断开，重连
		}
		else
		{
			LogShow_exchange(m_Result, Final_Result_Control, 5, "数据库连接正常中...", 0);
		}

		if (DB_FAIL == "FAIL")
		{
			LogShow_exchange(m_Result, Final_Result_Control, 250, "连接数据库失败，无法上传数据！！！请检查网络.......", 0);
			return FALSE;//GetSettingFromDB
		}

		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		if (SoftwareVer == "")
		{
			LogShow_exchange(m_Result, Final_Result_Control, 250, "SN或者软件版本为空", 0);
			return FALSE;
		}
		////////////////////////////////检查RFID、机型、软件版本是否已经存在//////////////////////////////
		CString DBLog;

		myado.OpenSheet("select * from TestSystemSettingFunc WHERE SoftWare='" + SoftwareVer + "'");
		BOOL Barcode_Check = myado.Find("SoftWare='" + SoftwareVer + "'");
		if (Barcode_Check == TRUE)
		{
			LogShow_exchange(m_Result, Final_Result_Control, 5, "服务器中存在此软件版本:" + SoftwareVer + "，开始获取配置...", 0);
		}
		else
		{
			LogShow_exchange(m_Result, Final_Result_Control, 250, "服务器中不存在此软件版本:" + SoftwareVer, 0);
			myado.CloseSheet();
			return FALSE;
		}
		////////////////////////////////检查RFID是否已经存在//////////////////////////////
		BOOL UP_Barcode = TRUE;
		//BOOL Barcode_Check_UP;
		if (Barcode_Check == TRUE)
		{
			_variant_t var;
			CString FieldName, FieldValue;
			myado.OpenSheet("select * from TestSystemSettingFunc WHERE SoftWare='" + SoftwareVer + "'");
			var = myado.m_pRec->GetCollect(_variant_t((long)1));
			if (var.vt != VT_NULL)
			{
				FieldValue.Format("%s", (LPCSTR)(_bstr_t)var);
				LogShow_exchange(m_Result, Final_Result_Control, 5, "服务器中存在字段:MachineName，成功获取:" + FieldValue, 0);

				MachineType_CS = FieldValue;
			}
			else
			{
				LogShow_exchange(m_Result, Final_Result_Control, 250, "服务器中不存在字段:MachineName", 0);
				myado.CloseSheet();
				return FALSE;
			}
			var = myado.m_pRec->GetCollect(_variant_t((long)2));
			if (var.vt != VT_NULL)
			{
				FieldValue.Format("%s", (LPCSTR)(_bstr_t)var);
				LogShow_exchange(m_Result, Final_Result_Control, 5, "服务器中存在字段:Station，成功获取:" + FieldValue, 0);
				CString MachineNameC;
				int pos = FieldValue.Find("}}");
				if (pos >= 0)
				{
					MachineNameC = FieldValue.Left(pos);
					FieldValue = FieldValue.Mid(pos + 2);
				}
				else
				{
					MachineNameC = "NULL";
				}
				if (MachineType_CS != MachineNameC)
				{
					LogShow_exchange(m_Result, Final_Result_Control, 250, "服务器中机型获取失败", 0);
					myado.CloseSheet();
					return FALSE;
				}
				else
				{
					LogShow_exchange(m_Result, Final_Result_Control, 5, "服务器中机型获取成功:" + MachineType_CS, 0);
				}
			}
			else
			{
				LogShow_exchange(m_Result, Final_Result_Control, 250, "服务器中不存在此对应机型", 0);
				myado.CloseSheet();
				return FALSE;
			}
			int OrderNUM = atoi(FieldValue.Left(2));
			int OrderNUMTemp = 0;
			//CString ATCommandList_CSDBCompare;
			ATCommandList_CSDBCompare = FieldValue.Mid(2);
			CString ATCommandList_CSDB = "";

			int i = 0;
			for (i = 0; i<SETTING_ITEM_MAX; i++)//for(i=0;i<32;i++)
			{

				FieldName.Format("Setting%d", i);
				var = myado.m_pRec->GetCollect((LPCTSTR)FieldName);
				if (var.vt != VT_NULL)
				{
					FieldValue.Format("%s", (LPCSTR)(_bstr_t)var);
					LogShow_exchange(m_Result, Final_Result_Control, 5, "服务器中存在字段:" + FieldName + "，成功获取:" + FieldValue, 0);

					if (FieldValue != "")
						ATCommandList_CSDB += (FieldValue);
					else
						break;
				}
				else
					break;

			}
			if (1)
			{
				if (g_ADCTFlag != 1 && GetBluetoothCheckValue == FALSE)
				{
					AfxMessageBox(ATCommandList_CSDB + "\r\n==\r\n" + ATCommandList_CSDBCompare);
					CString NUMtem, NUMtem2;
					NUMtem.Format("%d", i);
					NUMtem2.Format("%d", OrderNUM);
					AfxMessageBox(NUMtem + "==" + NUMtem2);
				}
				if (OrderNUM != i)
				{
					myado.CloseSheet();
					return FALSE;
				}
				else if (ATCommandList_CSDB != ATCommandList_CSDBCompare)
				{
					myado.CloseSheet();
					LogShow_exchange(m_Result, Final_Result_Control, 250, "服务器中命令获取失败" + ATCommandList_CSDB + "==" + ATCommandList_CSDBCompare, 0);
					return FALSE;
				}
				else
				{
					myado.CloseSheet();
					GetATSetting(ATCommandList_CSDB);
					LogShow_exchange(m_Result, Final_Result_Control, 256, "...", 0);
					LogShow_exchange(m_Result, Final_Result_Control, 5, "请注意！！！\r\n请注意！！！\r\n请注意！！！\r\n当前配置为：" + paraArray[2].Low_Limit_Value, 0);
					if (g_ADCTFlag != 1 && GetBluetoothCheckValue == FALSE)
					{
						AfxMessageBox(paraArray[2].Low_Limit_Value);
					}
					return TRUE;
				}
			}
			myado.CloseSheet();
		}
	}
	else
	{
		LogShow_exchange(m_Result, Final_Result_Control, 250, "没有设置服务器", 0);
		return FALSE;
	}
	return FALSE;//myado---------3
}


BOOL IMEIWrite_MulAT::GetATSetting(CString SettingChar)
{
	CString Check_item = _T("");//是否选中
	CString strValue_low = _T("");//测试下限值
	CString strValue_high = _T("");//测试上限值
	CString strValue_OtherITEM = _T("");//AT指令

	CString strValue;
	//SettingChar.Replace(" ","");
	SettingChar.Replace("\r", "");
	SettingChar.Replace("\n", "");
	strValue = SettingChar;
	for (int i = 0; i<ITEMMAX; i++)
	{
		int npos = strValue.Find("@@");
		if (npos >= 0)
		{
			Check_item = strValue.Left(npos);
			Check_item = Check_item.Mid(8);
			strValue = strValue.Mid(npos + 2);
		}
		else
		{
			Check_item = "";
		}
		npos = strValue.Find("@@");
		if (npos >= 0)
		{
			strValue_low = strValue.Left(npos);
			strValue = strValue.Mid(npos + 2);
		}
		else
		{
			strValue_low = "";
		}
		npos = strValue.Find("@@");
		if (npos >= 0)
		{
			strValue_high = strValue.Left(npos);
			strValue = strValue.Mid(npos + 2);
		}
		else
		{
			strValue_high = "";
		}
		npos = strValue.Find("}}");
		if (npos >= 0)
		{
			strValue_OtherITEM = strValue.Left(npos);
			strValue = strValue.Mid(npos + 2);
		}
		else
		{
			strValue_OtherITEM = "";
		}

		npos = Check_item.Find("=");
		if (npos>0)
		{
			paraArray[i].showName = Check_item.Left(npos);
		}
		else
		{
			paraArray[i].showName = Check_item;
		}
		paraArray[i].Low_Limit_Value = strValue_low;	//期望上下限值
		paraArray[i].High_Limit_Value = strValue_high;	//期望上下限值
		paraArray[i].Other_ITEM = strValue_OtherITEM;	//其他自定义
		if ((Check_item != "") && (Check_item.Find("false") == -1))
			paraArray[i].ifCheck = true;					//选中       
		else
			paraArray[i].ifCheck = false;					//没选中
	}
	return TRUE;
}


void  IMEIWrite_MulAT::SaveConfig_Change()
{
	char szName[MAX_PATH] = _T("");
	CString  szPath = _T("");

	GetModuleFileName(NULL, szName, MAX_PATH);

	szPath = szName;
	szPath = szPath.Mid(0, szPath.ReverseFind('\\') + 1);


	CString strLoadFile = szPath + STRING_PATH_CONFIG;

	GPS_SNRLimit.GetWindowTextA(GPS_SNRLimitCS);
	WIFI_RSSILimit.GetWindowTextA(WIFI_RSSILimitCS);
	// 写出INI文件
	CIniFile m_fIniFile(strLoadFile);
	m_fIniFile.WriteString(STRING_SECTION_CONFIG, "GPS_SNRLimit", GPS_SNRLimitCS);
	m_fIniFile.WriteString(STRING_SECTION_CONFIG, "WIFI_RSSILimit", WIFI_RSSILimitCS);
}

void IMEIWrite_MulAT::LoadConfig_Change()
{

	int nCommandNum = 0;


	CString strCommand = _T("");
	CString strValue = _T("");

	CString strName = _T("");
	CStringArray strCommandAry;



	char szName[MAX_PATH] = _T("");
	CString  szPath = _T("");
	GetModuleFileName(NULL, szName, MAX_PATH);
	szPath = szName;
	szPath = szPath.Mid(0, szPath.ReverseFind('\\') + 1);
	CString strLoadFile = szPath + STRING_PATH_CONFIG;
	CIniFile m_fIniFile(strLoadFile);

	m_fIniFile.ReadSection(STRING_SECTION_CONFIG, strCommandAry);
	nCommandNum = strCommandAry.GetSize();
	for (int i = 0; i<nCommandNum; i++)
	{
		strCommand = strCommandAry[i];//编号
		if (strCommand == "GPS_SNRLimit")
		{
			GPS_SNRLimitCS = m_fIniFile.ReadString(STRING_SECTION_CONFIG, strCommand, "");
			GPS_SNRLimit.SetWindowTextA(GPS_SNRLimitCS);
		}
		else if (strCommand == "WIFI_RSSILimit")
		{
			WIFI_RSSILimitCS = m_fIniFile.ReadString(STRING_SECTION_CONFIG, strCommand, "");
			WIFI_RSSILimit.SetWindowTextA(WIFI_RSSILimitCS);
		}
	}
	UpdateData(false);

}

bool IMEIWrite_MulAT::LoadDll()
{
	password_Handle = LoadLibrary("NEWTTL_encrypt.dll");
	if (!password_Handle)
	{
		AfxMessageBox("Load NEWTTL_encrypt.dll fail!");
		return false;
	}
	GetNEWTTL_encrypt_16 = (unsigned short int(__stdcall *)(const unsigned char* pData, int nLength))GetProcAddress(password_Handle, "GetNEWTTL_encrypt_16");
	return true;
}

char  IMEIWrite_MulAT::HexToASCII(unsigned char  data_hex)
{
	char  ASCII_Data;
	ASCII_Data = data_hex & 0x0F;
	if (ASCII_Data<10)
		ASCII_Data = ASCII_Data + 0x30;			//‘0--9’
	else
		ASCII_Data = ASCII_Data + 0x37;			//‘A--F’
	return ASCII_Data;
}

void IMEIWrite_MulAT::HexGroupToString(char *OutStrBuffer, unsigned char *InHexBuffer, unsigned int HexLength)
{
	unsigned int i, k = 0;
	for (i = 0; i<HexLength; i++)
	{
		OutStrBuffer[k++] = HexToASCII((InHexBuffer[i] >> 4) & 0x0F);
		OutStrBuffer[k++] = HexToASCII(InHexBuffer[i] & 0x0F);
	}
	OutStrBuffer[k] = '\0';
}

BOOL IMEIWrite_MulAT::StringToHexGroup(unsigned char *OutHexBuffer, char *InStrBuffer, unsigned int strLength)
{
	unsigned int i, k = 0;
	unsigned char HByte, LByte;
	if (strLength % 2 != 0)
		return FALSE;

	for (i = 0; i<strLength; i = i + 2)
	{
		if (InStrBuffer[i] >= '0' && InStrBuffer[i] <= '9')
		{
			HByte = InStrBuffer[i] - '0';
		}
		else if (InStrBuffer[i] >= 'A' && InStrBuffer[i] <= 'F')
		{
			HByte = InStrBuffer[i] - 'A' + 10;
		}
		else
		{
			HByte = InStrBuffer[i];
			return FALSE;
		}
		HByte = HByte << 4;
		HByte = HByte & 0xF0;
		if (InStrBuffer[i + 1] >= '0' && InStrBuffer[i + 1] <= '9')
		{
			LByte = InStrBuffer[i + 1] - '0';
		}
		else if (InStrBuffer[i + 1] >= 'A' && InStrBuffer[i + 1] <= 'F')
		{
			LByte = InStrBuffer[i + 1] - 'A' + 10;
		}
		else
		{
			LByte = InStrBuffer[i];
			return FALSE;
		}
		OutHexBuffer[k++] = HByte | LByte;
	}
	return TRUE;
}

/*
Function Name:unsigned int   SeperateToHex(const char *src, unsigned char *des)
Tasks: ASCII to Hex converter

ASCII Sheet Number
A-Z:65-90
a-z:97-122
0-9:48-57
space:32
enter:
*/
unsigned int IMEIWrite_MulAT::SeperateToHex(const char *src, unsigned char *des)
{
	unsigned int i;
	unsigned int Src_Length;
	unsigned int Des_Length;
	unsigned char tmp_h;
	unsigned char tmp_l;
	tmp_h = 0x00u;
	tmp_l = 0x00u;
	unsigned int flag = 0;
	Des_Length = 0;
	//Src_Length=sizeof(src)/sizeof(char) ;//中间不能有空格
	//Src_Length=sizeof(src);
	Src_Length = strlen(src);
	for (i = 0; i<Src_Length; i++)
	{
		if (*src == 32)///space
		{
			if (2 == flag)///
			{
				*des++ = (tmp_h << 4) + (tmp_l & 0x0F);
				Des_Length++;
			}
			else if (1 == flag)
			{
				*des++ = tmp_h;
				Des_Length++;
			}
			else///in case of more than 2 spaces
			{
				;
			}
			tmp_h = 0x00u;
			tmp_l = 0x00u;
			src++;
			flag = 0;
		}
		else if (*src>47 && *src<58)///0-9
		{
			if (0 == flag)
			{
				tmp_h = 0x00 + *src - 48;
				flag = 1;
			}
			else
			{
				tmp_l = 0x00 + *src - 48;
				flag = 2;
			}

			if (i == Src_Length - 1)//最后没有空格
			{
				if (2 == flag)///
				{
					*des++ = (tmp_h << 4) + (tmp_l & 0x0F);
					Des_Length++;
				}
				else if (1 == flag)
				{
					*des++ = tmp_h;
					Des_Length++;
				}
				else///in case of more than 2 spaces
				{
					;
				}
				tmp_h = 0x00u;
				tmp_l = 0x00u;
				src++;
				flag = 0;
			}
			src++;
		}

		else if (*src>64 && *src<71)///A-F,
		{
			if (0 == flag)
			{
				tmp_h = 0x0A + *src - 65;
				flag = 1;
			}
			else
			{
				tmp_l = 0x0A + *src - 65;
				flag = 2;
			}
			if (i == Src_Length - 1)//最后没有空格
			{
				if (2 == flag)///
				{
					*des++ = (tmp_h << 4) + (tmp_l & 0x0F);
					Des_Length++;
				}
				else if (1 == flag)
				{
					*des++ = tmp_h;
					Des_Length++;
				}
				else///in case of more than 2 spaces
				{
					;
				}
				tmp_h = 0x00u;
				tmp_l = 0x00u;
				src++;
				flag = 0;
			}
			src++;
		}
		else if (*src>96 && *src<103)///a-f
		{
			if (0 == flag)
			{
				tmp_h = 0x0a + *src - 97;
				flag = 1;
			}
			else
			{
				tmp_l = 0x0a + *src - 97;
				flag = 2;
			}
			if (i == Src_Length - 1)//最后没有空格
			{
				if (2 == flag)///
				{
					*des++ = (tmp_h << 4) + (tmp_l & 0x0F);
					Des_Length++;
				}
				else if (1 == flag)
				{
					*des++ = tmp_h;
					Des_Length++;
				}
				else///in case of more than 2 spaces
				{
					;
				}
				tmp_h = 0x00u;
				tmp_l = 0x00u;
				src++;
				flag = 0;
			}
			src++;
		}
		else
		{
			;
		}
	}
	return Des_Length;///return HEX Data length
}


CString IMEIWrite_MulAT::ShowPassWord(CString InputID, CEdit* m_Result, CEdit* Final_Result_Control, int HandleNum)
{
	unsigned short PassWord = 0;
	CString PassWordCS;
	//unsigned char* in=(unsigned char*)(LPCTSTR)InputID;
	//unsigned char in[]={0x0F,0x88,0xBE,0x55,0x21,0xD4,0xDB,0xED,0xD3,0x75,0x13,0x46,0x67,0x28,0xBF,0x63};
	unsigned char OutHexBuffer[256] = { 0 };
	//int Length=SeperateToHex((LPSTR)(LPCTSTR)InputID, OutHexBuffer);
	int Length = InputID.GetLength() / 2;
	StringToHexGroup(OutHexBuffer, (LPSTR)(LPCTSTR)InputID, InputID.GetLength());

	PassWord = GetNEWTTL_encrypt_16(OutHexBuffer, Length);
	PassWordCS.Format("%d", PassWord);
	LogShow_exchange(m_Result, Final_Result_Control, 0, "PASSWORD= " + PassWordCS, HandleNum);
	return PassWordCS;
}

//把ascii形式的字串转换成16进制数
//例如:"ab01234" -> "61 62 30 31 32 33 34 "
char* IMEIWrite_MulAT::AscToHexString(char* pInData, char* pOutData)
{
	//char* pIn = pInData;
	//char* pOut = pOutData;
	char pIn[256] = { 0 };
	char pOut[256] = { 0 };
	strcpy(pIn, pInData);

	int hexValue = 0;
	for (int i = 0; i<strlen(pIn); i++)
	{
		hexValue = pIn[i];
		sprintf(pOut + 3 * i, "%02X ", hexValue);
	}
	return pOut;
}
void IMEIWrite_MulAT::OnBnClickedButtonconncpu2()
{
	// TODO: 在此添加控件通知处理程序代码
}

void IMEIWrite_MulAT::OnBnClickedButtondisconncpu2()
{
	// TODO: 在此添加控件通知处理程序代码
}


BOOL IMEIWrite_MulAT::GetIp_Name(CString& m_hostname, CString& m_ipaddress)
{
	char 	szHostname[256];
	BOOL	bGetIpInfo_Success = FALSE;

	m_hostname = "";
	m_ipaddress = "";

	//获取IP信息：
	if (gethostname(szHostname, 256) == 0)
	{
		// get host adresses
		struct hostent * phost;
		int i;

		phost = gethostbyname(szHostname);
		m_hostname = szHostname;

		for (i = 0; phost != NULL && phost->h_addr_list[i] != NULL; i++)
		{
			int j;
			int h_length = 4;

			CString CStr_Tmp;
			CStr_Tmp.Format("%u", i + 1);
			m_ipaddress += "IP";
			m_ipaddress += CStr_Tmp;
			m_ipaddress += ":";

			for (j = 0; j<h_length; j++)
			{
				CString addr;

				if (j > 0)
					m_ipaddress += ".";

				addr.Format("%u", (unsigned int)((unsigned char*)phost->h_addr_list[i])[j]);
				m_ipaddress += addr;
			}
			break;//获取一个IP即可
		}
		bGetIpInfo_Success = TRUE;
	}

	return	bGetIpInfo_Success;
}

void IMEIWrite_MulAT::OnBnClickedCheck39()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);
}

void IMEIWrite_MulAT::OnBnClickedCheck40()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);
}

void IMEIWrite_MulAT::OnCbnSelchangeCombo53()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);
}


UINT static __cdecl ThreadCopy(LPVOID pParam)
{
	IMEIWrite_MulAT* HostControl = (IMEIWrite_MulAT*)pParam;
	HostControl->FileCopy();
	return 0;
}
void IMEIWrite_MulAT::OnBnClickedBtnCopy()
{
	// TODO: Add your control notification handler code here
	if ((IMEI_Setdlg.DestFile != "") && (PathFileExists(IMEI_Setdlg.DestFile)))
		AfxBeginThread(ThreadCopy, (LPVOID)this, THREAD_PRIORITY_NORMAL, 0, 0, NULL);

}
void IMEIWrite_MulAT::FileCopy()
{
	CString strSrc, strDest, strExt;

	char szName[MAX_PATH] = _T("");
	CString  szPath = _T("");
	GetModuleFileName(NULL, szName, MAX_PATH);
	szPath = szName;
	strSrc = szPath.Mid(0, szPath.ReverseFind('\\') + 1);

	strDest = IMEI_Setdlg.DestFile;
	strExt = "*";
	//TCHAR pszMsg[MAX_PATH];
	if (!strSrc.IsEmpty() && !strDest.IsEmpty() && !strExt.IsEmpty())
	{
		//CTime cTime = CTime::GetCurrentTime();
		//CString strTime = cTime.Format(_T("%Y-%m-%d %H:%M:%S"));
		//CString Data = cTime.Format(_T("%Y-%m-%d %H-%M-%S\\"));
		//CString Data = cTime.Format(_T("%Y-%m\\"));
		CopyFileExLBC(strSrc, strDest, strExt);
	}
	SetTimer(32, 60000, NULL);
}

DWORD IMEIWrite_MulAT::CopyFileExLBC(CString strSrcPath, CString strDestPath, CString strExt)
{
	if (!PathFileExists(strDestPath))
	{
		if (!CreateDirectory(strDestPath, NULL))
			return GetLastError();
	}

	CFileFind ff;
	CString szDir = strSrcPath;
	CString strSize = _T("");
	if (szDir.Right(1) != _T("\\"))
		szDir += _T("\\");
	szDir += _T("*.*");

	BOOL res = ff.FindFile(szDir);
	while (res)
	{
		res = ff.FindNextFile();
		if (ff.IsDirectory() && !ff.IsDots()) // Folder 
		{
			CString strFolderPath = ff.GetFilePath();
			CString strFolderName = ff.GetFileName();
			CString strDestSubFolder = strDestPath + _T("\\") + strFolderName;
			if (!PathFileExists(strDestSubFolder))
			{
				if (!CreateDirectory(strDestSubFolder, NULL))
					return GetLastError();
			}
			CopyFileExLBC(strFolderPath, strDestSubFolder, strExt);
		}
		else if (!ff.IsDirectory() && !ff.IsDots()) // File
		{
			// Judge extend name
			CString strFileName = ff.GetFileName();
			CString strFileExt = (strrchr(strFileName, _T('.')) + 1);
			if (strExt == _T("*"))
				strFileExt = strExt;
			if (strFileExt == strExt)
			{
				// Copy files
				CString strFilePath;
				strFilePath = ff.GetFilePath();
				CString strNewFilePath;
				strNewFilePath = strDestPath + _T("\\") + strFileName;
				/*if (!::CopyFile(strFilePath, strNewFilePath, FALSE))
				{
				DWORD dwError = GetLastError();
				if (dwError == ERROR_SHARING_VIOLATION)
				{ continue; }
				return dwError;
				}
				*/
				if (!::CopyFile(strFilePath, strNewFilePath, TRUE))//bFailIfExists Long，如果设为TRUE（非零），那么一旦目标文件已经存在，则函数调用会失败。否则目标文件被改写
				{
					DWORD dwError = GetLastError();
					if (dwError == ERROR_SHARING_VIOLATION)
					{
						continue;
					}
					continue;//return dwError;
				}
			}
		}
	}
	ff.Close();

	return ERROR_SUCCESS;
}

//获取图片数据
BOOL IMEIWrite_MulAT::GetPicFunction_Thread(int HandleNum, CEdit* m_Result, CEdit* Final_Result_Control, CThumbnailBoxDlg* picdlg, CString ChipRfIDbg, BOOL DayTimeStatep)//WriteIMEIFunction_Thread
{
	EnterCriticalSection(&WIFICOMMUNICATE[HandleNum]);
	BOOL Result = Get_Pic_Data(m_Result, Final_Result_Control, picdlg, HandleNum, "FFD8", "FFD9", ChipRfIDbg, DayTimeStatep);
	LeaveCriticalSection(&WIFICOMMUNICATE[HandleNum]);
	return Result;
}

PARA_Handle PicHandle;
UINT static __cdecl GetPicFunction(LPVOID pParam)
{
	PARA_Handle* Mead_Main_Win = (PARA_Handle*)pParam;
	IMEIWrite_MulAT* MulAT_Win = (IMEIWrite_MulAT*)Mead_Main_Win->WinHandle;
	MulAT_Win->GetPicFunction_Thread(Mead_Main_Win->HandleNum, Mead_Main_Win->m_Result, Mead_Main_Win->Final_Result_Control, Mead_Main_Win->picdlg, Mead_Main_Win->ChipRfIDBG, Mead_Main_Win->DayTimeStatep);
	return 0;
}
void IMEIWrite_MulAT::StartGetPic(int HandleNum, CEdit* m_Result, CEdit* Final_Result_Control)
{
	LogShow_exchange(m_Result, Final_Result_Control, 0, "开始创建图片线程\r\n", HandleNum, "-1", ChipRfID[HandleNum]);
	PicHandle.WinHandle = this;
	PicHandle.HandleNum = HandleNum;
	PicHandle.m_Result = m_Result;
	PicHandle.Final_Result_Control = Final_Result_Control;
	PicHandle.picdlg = &Picdlg[HandleNum];
	PicHandle.ChipRfIDBG = ChipRfID[HandleNum];
	PicHandle.DayTimeStatep = DayTimeState[HandleNum];
	AfxBeginThread(GetPicFunction, (LPVOID)&PicHandle, THREAD_PRIORITY_BELOW_NORMAL, 0, 0, NULL);
	LogShow_exchange(m_Result, Final_Result_Control, 0, "创建图片线程完成\r\n", HandleNum, "-1", ChipRfID[HandleNum]);
}

//显示图片数据
BOOL IMEIWrite_MulAT::ShowPicFunction_Thread(CString RecString, int HandleNum, CEdit* m_Result, CEdit* Final_Result_Control, CThumbnailBoxDlg* picdlg, CString ChipRfIDbg, BOOL DayTimeStatep)//WriteIMEIFunction_Thread
{
	EnterCriticalSection(&WIFICOMMUNICATE[HandleNum]);
	BOOL Result = Show_Pic_Data(RecString, m_Result, Final_Result_Control, picdlg, HandleNum, "FFD8", "FFD9", ChipRfIDbg, DayTimeStatep);
	LeaveCriticalSection(&WIFICOMMUNICATE[HandleNum]);
	return Result;
}
UINT static __cdecl ShowPicFunction(LPVOID pParam)
{
	PARA_Handle* Mead_Main_Win = (PARA_Handle*)pParam;
	IMEIWrite_MulAT* MulAT_Win = (IMEIWrite_MulAT*)Mead_Main_Win->WinHandle;//待处理
	MulAT_Win->ShowPicFunction_Thread(Mead_Main_Win->RecString, Mead_Main_Win->HandleNum, Mead_Main_Win->m_Result, Mead_Main_Win->Final_Result_Control, Mead_Main_Win->picdlg, Mead_Main_Win->ChipRfIDBG, Mead_Main_Win->DayTimeStatep);
	return 0;
}
void IMEIWrite_MulAT::StartShowPic(int HandleNum, CEdit* m_Result, CEdit* Final_Result_Control, CString RecString)
{
	LogShow_exchange(m_Result, Final_Result_Control, 0, "开始创建图片线程\r\n", HandleNum, "-1", ChipRfID[HandleNum]);
	PicHandle.WinHandle = this;
	PicHandle.HandleNum = HandleNum;
	PicHandle.m_Result = m_Result;
	PicHandle.Final_Result_Control = Final_Result_Control;
	PicHandle.picdlg = &Picdlg[HandleNum];
	PicHandle.ChipRfIDBG = ChipRfID[HandleNum];
	PicHandle.DayTimeStatep = DayTimeState[HandleNum];
	PicHandle.RecString = RecString;
	AfxBeginThread(ShowPicFunction, (LPVOID)&PicHandle, THREAD_PRIORITY_BELOW_NORMAL, 0, 0, NULL);
	LogShow_exchange(m_Result, Final_Result_Control, 0, "创建图片线程完成\r\n", HandleNum, "-1", ChipRfID[HandleNum]);
}

BOOL IMEIWrite_MulAT::Show_Pic_Data(CString RecString, CEdit* m_Result, CEdit* Final_Result_Control, CThumbnailBoxDlg* picdlg, int HandleNum, CString StartSign, CString EndSign, CString ChipRfIDbg, BOOL DayTimeStatep)		//获取图像端口数据
{
	LogShow_exchange(m_Result, Final_Result_Control, 0, "开始新一轮0\r\n", HandleNum, "-1", ChipRfIDbg);
	//char OutStringBuffer[PICDATAMAX]={0};

	CString NightDayValue;
	if (DayTimeStatep == TRUE)
		NightDayValue = "白天";
	else
		NightDayValue = "夜晚";
	CTime cTime;
	CString TimeShow;

	PicUnionOK[HandleNum] = FALSE;
	UnionPicPath[HandleNum] = "";				//要清理上一次路径---------防止本次合成失败
	PicPath[DayTimeStatep][HandleNum] = "";	//要清理白天和黑夜的路径---防止本次获取失败

	cTime = CTime::GetCurrentTime();
	TimeShow.Format("-%02d%02d%02d", cTime.GetHour(), cTime.GetMinute(), cTime.GetSecond());
	LogShow_exchange(m_Result, Final_Result_Control, 0, TimeShow + "开始新一轮1\r\n", HandleNum, "-1", ChipRfIDbg);

	if (1)
	{
		cTime = CTime::GetCurrentTime();
		TimeShow.Format("-%02d%02d%02d", cTime.GetHour(), cTime.GetMinute(), cTime.GetSecond());

		//HexGroupToString(OutStringBuffer,(unsigned char *)buffer,dwBytesRead);
		//RecString.Format("%s",OutStringBuffer);//--------------------------------------------------------------注意看是否需要清理空格
		LogShow_exchange(m_Result, Final_Result_Control, 5, NightDayValue + TimeShow + "开始接收图片数据\r\n", HandleNum, "-1", ChipRfIDbg);

		RecString.Replace(" ", "");

		int npos = RecString.Right(4).Find(EndSign);//FF D9             Right(4)-----数据中间也有FFD9
		if (npos >= 0)
		{
			RecString = RecString.Left(npos + EndSign.GetLength());
			CString  selPort;
			selPort = RecString;

			npos = selPort.Find(StartSign);//FF D8 
			if (npos >= 0)
			{
				Vaule_Pic[HandleNum] = selPort.Mid(npos);
				LogShow_exchange(m_Result, Final_Result_Control, 5, NightDayValue + TimeShow + "成功获取图片数据\r\n", HandleNum, "-1", ChipRfIDbg);
			}
			else
			{
				LogShow_exchange(m_Result, Final_Result_Control, 128, NightDayValue + TimeShow + "图片数据异常\r\n", HandleNum, "功能测试:2-TEST", ChipRfIDbg);
				goto ERRORPRO;
			}


			HEX2JPG3(Vaule_Pic[HandleNum], PicPath[DayTimeStatep][HandleNum], NightDayValue, HandleNum, ChipRfIDbg);
			LogShow_exchange(m_Result, Final_Result_Control, 5, TimeShow + "将16进制数据转换成JPG图片\r\n", HandleNum, "-1", ChipRfIDbg);
			if (DayTimeStatep == TRUE)//先夜拍，完成白天拍照后将两张图片拼接
			{
				if (PathFileExists(PicPath[0][HandleNum]) != TRUE)
				{
					LogShow_exchange(m_Result, Final_Result_Control, 5, TimeShow + "夜间模式图片不存在\r\n", HandleNum, "功能测试:2-TEST", ChipRfIDbg);
					//goto ERRORPRO;
				}
				if (PathFileExists(PicPath[1][HandleNum]) != TRUE)
				{
					LogShow_exchange(m_Result, Final_Result_Control, 128, TimeShow + "白天模式图片不存在\r\n", HandleNum, "功能测试:2-TEST", ChipRfIDbg);
					goto ERRORPRO;
				}

				if (1)//if(Jpgunionjpg(PicPath[0][HandleNum],PicPath[1][HandleNum],ChipRfIDbg,ChipRfIDbg,UnionPicPath[HandleNum])==TRUE)
				{
					PicUnionOK[HandleNum] = TRUE;
					LogShow_exchange(m_Result, Final_Result_Control, 5, TimeShow + "成功合成图片\r\n", HandleNum, "-1", ChipRfIDbg);

					if (BGConfirmChoose == TRUE)//后台开始确认，1、等待“控制板状态”，2、弹出确认界面，3、按钮后上传数据（线程传递芯片ID和图片显示时间参数）,4、失败按钮
					{
						/////////////////////////////////////////////////////////////
						//等待门打开，才开始显示图片
						//WaitForSingleObject(DoorWait[HandleNum].m_hObject,60*1000);//等待60秒INFINITE
						WaitForSingleObject(DoorWait[HandleNum].m_hObject, INFINITE);
						DoorWait[HandleNum].ResetEvent();
						int t = 0;
						while (CPUCommunication("控制板状态", HandleNum, "", "", COM_State[THREAD_NUM], m_Result, Final_Result_Control) == FALSE)
						{

							if (StopSign[HandleNum + 8] == TRUE)
								goto ERRORPRO;
							Sleep(2000);
							t++;
							if ((t>3) && (TestResult[HandleNum] == FALSE))
								break;
						}

						//等待门打开
						/////////////////////////////////////////////////////////////
						if (TestResult[HandleNum] == FALSE)
						{
							LogShow_exchange(m_Result, Final_Result_Control, 128, TimeShow + "测试线程失败！！！\r\n", HandleNum, "功能测试:2-TEST", ChipRfIDbg);
							goto ERRORPRO;
						}
						else
						{
							TestResult[HandleNum] = FALSE;//下一轮测试结果重新赋值
						}


						LogShow_exchange(m_Result, Final_Result_Control, 5, TimeShow + "单线程图片开始显示\r\n", HandleNum, "-1", ChipRfIDbg);
						EnterCriticalSection(&SHOWONEPIC);
						int i = 0;

						////////////////////////////////////////////////////
						//采用定时器显示图片
						if (PicStaticChoose == TRUE)
						{
							Picdlg[HandleNum].ShowPicTime = atoi(BGShowPicTime[HandleNum]);
							if ((PicPath[0][HandleNum] == "") || PicPath[1][HandleNum] == "")
							{
								LogShow_exchange(m_Result, Final_Result_Control, 5, "不存在图片", HandleNum, "功能测试:2-TEST", ChipRfIDbg);
								//LeaveCriticalSection(&SHOWONEPIC);
								//goto ERRORPRO;
							}
							Picdlg[HandleNum].PicPath0 = PicPath[0][HandleNum];
							Picdlg[HandleNum].PicPath1 = PicPath[1][HandleNum];
							Picdlg[HandleNum].ShowHandleHun = HandleNum;
							TimerHandleNum = HandleNum;
							SetTimer(48, 1, NULL);
						}
						//采用定时器显示图片
						/////////////////////////////////////////////////////
						CPUCommunication("确认失败", HandleNum, "", "", COM_State[THREAD_NUM], m_Result, Final_Result_Control);//先清理前面的
						CPUCommunication("确认成功", HandleNum, "", "", COM_State[THREAD_NUM], m_Result, Final_Result_Control);
						while (i<BGShowPicCount[HandleNum])
						{
							////////////////////////////////////////////////
							//失败按钮
							BOOL Result = CPUCommunication("确认失败", HandleNum, "", "", COM_State[THREAD_NUM], m_Result, Final_Result_Control);
							if (Result == TRUE)
							{
								LogShow_exchange(m_Result, Final_Result_Control, 128, "确认失败", HandleNum, "功能测试:2-TEST", ChipRfIDbg);
								Picdlg[TimerHandleNum].SendMessage(WM_CLOSE);//Picdlg[TimerHandleNum].EndDialog(0);
								LeaveCriticalSection(&SHOWONEPIC);
								goto ERRORPRO;
							}
							//失败按钮
							////////////////////////////////////////////////


							////////////////////////////////////////////////
							//循环等待WIFI按钮
							Final_Result_Control->SetFont(fontsp);
							gColor[HandleNum + 8] = RGB(205, 0, 205);
							Final_Result_Control->SetWindowTextA("请确认");
							Final_Result_Control->UpdateWindow();

							if (Get_Wifi_Data(m_Result, Final_Result_Control, HandleNum, "", "RESET", TRUE) != TRUE)
							{
								if (PicStaticChoose == FALSE)
								{
									Picdlg[HandleNum].ShowPicTime = atoi(BGShowPicTime[HandleNum]);
									if ((PicPath[0][HandleNum] == "") || PicPath[1][HandleNum] == "")
									{
										LogShow_exchange(m_Result, Final_Result_Control, 5, "不存在图片", HandleNum, "功能测试:2-TEST", ChipRfIDbg);
										//LeaveCriticalSection(&SHOWONEPIC);
										//goto ERRORPRO;
									}
									Picdlg[HandleNum].PicPath0 = PicPath[0][HandleNum];
									Picdlg[HandleNum].PicPath1 = PicPath[1][HandleNum];
									if (Picdlg[HandleNum].DoModal() == -1)
									{
										LogShow_exchange(m_Result, Final_Result_Control, 128, "弹出窗口失败", HandleNum, "功能测试:2-TEST", ChipRfIDbg);
										LeaveCriticalSection(&SHOWONEPIC);
										goto ERRORPRO;
									}
								}
								else
								{
									Sleep(1500);
								}
							}
							else//上传数据
							{
								BOOL DBResult = FALSE;
								EnterCriticalSection(&UPDATEDB[HandleNum % (THREAD_NUM / 8)]);
								if (WorkStationCS == "SMT测试")
								{
									DBResult = Data_UpdatePara2(IMEI_Setdlg.Mulmyado[HandleNum], HandleNum, m_Result, Final_Result_Control, FALSE, ChipRfIDbg);
									if (DBResult == FALSE)
									{
										Sleep(3000);
										DBResult = Data_UpdatePara2(IMEI_Setdlg.Mulmyado[HandleNum], HandleNum, m_Result, Final_Result_Control, TRUE, ChipRfIDbg);
									}
								}
								else if (WorkStationCS == "组装测试")
								{
									DBResult = Data_UpdatePara(IMEI_Setdlg.Mulmyado[HandleNum], HandleNum, m_Result, Final_Result_Control, FALSE, ChipRfIDbg);
									if (DBResult == FALSE)
									{
										Sleep(3000);
										DBResult = Data_UpdatePara(IMEI_Setdlg.Mulmyado[HandleNum], HandleNum, m_Result, Final_Result_Control, TRUE, ChipRfIDbg);
									}
								}
								else if (WorkStationCS == "研发测试")
								{
									DBResult = TRUE;
								}
								else
								{
									LeaveCriticalSection(&UPDATEDB[HandleNum % (THREAD_NUM / 8)]);
									LogShow_exchange(m_Result, Final_Result_Control, 128, "请先选择站位！！！", HandleNum, "功能测试:2-TEST", ChipRfIDbg);
									Picdlg[TimerHandleNum].SendMessage(WM_CLOSE);//Picdlg[TimerHandleNum].EndDialog(0);
									LeaveCriticalSection(&SHOWONEPIC);
									goto ERRORPRO;
								}
								LeaveCriticalSection(&UPDATEDB[HandleNum % (THREAD_NUM / 8)]);

								if (DBResult == FALSE)
								{
									LogShow_exchange(m_Result, Final_Result_Control, 128, "上传数据失败", HandleNum, "功能测试:3-DB", ChipRfIDbg);
									Picdlg[TimerHandleNum].SendMessage(WM_CLOSE);//Picdlg[TimerHandleNum].EndDialog(0);
									LeaveCriticalSection(&SHOWONEPIC);
									goto ERRORPRO;
								}
								else
								{
									LogShow_exchange(m_Result, Final_Result_Control, 255, "成功", HandleNum, "-1", ChipRfIDbg);
									//Sleep(2000);
									//LogShow_exchange(m_Result,Final_Result_Control,0,TimeShow+"等待下一台\r\n",HandleNum,"-1",ChipRfIDbg);
									Picdlg[TimerHandleNum].SendMessage(WM_CLOSE);//
									Sleep(1000);
									LeaveCriticalSection(&SHOWONEPIC);
									return TRUE;
								}
							}
							//循环等待WIFI按钮
							////////////////////////////////////////////////
							i++;
							if (i == BGShowPicCount[HandleNum])//最后一次了，要失败
							{
								Picdlg[TimerHandleNum].SendMessage(WM_CLOSE);//Picdlg[TimerHandleNum].EndDialog(0);
								LeaveCriticalSection(&SHOWONEPIC);
								LogShow_exchange(m_Result, Final_Result_Control, 128, "确认超时", HandleNum, "功能测试:2-TEST", ChipRfIDbg);
								goto ERRORPRO;
							}
						}
					}
					else
					{
						return TRUE;
					}
				}
				else
				{
					LogShow_exchange(m_Result, Final_Result_Control, 128, TimeShow + "合成图片失败\r\n", HandleNum, "功能测试:2-TEST", ChipRfIDbg);
					return FALSE;
				}
			}
			else
			{
				//ReadFile( hPicPort[HandleNum], buffer, PICDATAMAX, &dwBytesRead, NULL);
				//return TRUE;//夜拍模式成功
			}
		}
	}
ERRORPRO:
	//if(BGConfirmChoose==TRUE)Sleep(2000);
	LogShow_exchange(m_Result, Final_Result_Control, 128, "获取图片超时", HandleNum, "功能测试:2-TEST", ChipRfIDbg);
	return FALSE;
}



BOOL IMEIWrite_MulAT::OPen_Pic_Port(CComboBox* m_Port, CComboBox* m_Baud, int HandleNum)		//打开图像端口OPen_ScanGun_Port
{
	CString sPort, sBaud;
	int port, baud;
	//get port
	m_Port->GetWindowText(sPort);
	sPort = sPort.Right(sPort.GetLength() - 3);
	port = atoi(sPort);
	sPort.Format(_T("\\\\.\\COM%d"), port);

	//get baud
	m_Baud->GetWindowText(sBaud);
	baud = atoi(sBaud);

	//open com port打开端口
	hPicPort[HandleNum] = CreateFile(sPort, GENERIC_READ | GENERIC_WRITE, 0, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hPicPort[HandleNum] == INVALID_HANDLE_VALUE)
	{
		CString csTemp;
		m_Port->GetWindowText(csTemp);
		//AfxMessageBox("Can't open "+csTemp);
		hPicPort[HandleNum] = NULL;
		return FALSE;
	}

	//config the com port
	DCB dcb;
	dcb.DCBlength = sizeof(DCB);
	GetCommState(hPicPort[HandleNum], &dcb);
	SetupComm(hPicPort[HandleNum], PICDATAMAX, 4096);
	PurgeComm(hPicPort[HandleNum], PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);
	dcb.Parity = NOPARITY;
	dcb.ByteSize = 8;
	dcb.StopBits = ONESTOPBIT;
	dcb.BaudRate = baud;															//57600(MT6205B), 115200 (MT6218B)			
	dcb.fBinary = TRUE;
	dcb.fParity = FALSE;
	dcb.fOutxCtsFlow = FALSE;
	dcb.fOutxDsrFlow = FALSE;
	dcb.fDtrControl = DTR_CONTROL_DISABLE;
	dcb.fDsrSensitivity = FALSE;
	dcb.fTXContinueOnXoff = FALSE;
	dcb.fOutX = FALSE;
	dcb.fInX = FALSE;
	dcb.fErrorChar = FALSE;
	dcb.fNull = FALSE;
	dcb.fRtsControl = RTS_CONTROL_ENABLE;
	dcb.fAbortOnError = FALSE;
	dcb.XonChar = 0;
	dcb.XoffChar = 0;
	dcb.ErrorChar = 0;
	dcb.EofChar = 0;
	dcb.EvtChar = 0;
	SetCommState(hPicPort[HandleNum], &dcb);

	//set time out struct
	COMMTIMEOUTS timeouts;
	timeouts.ReadIntervalTimeout = 0xFFFFFFFF;
	timeouts.ReadTotalTimeoutMultiplier = 0;
	timeouts.ReadTotalTimeoutConstant = 0;//OPen_Pic_Port
	timeouts.WriteTotalTimeoutMultiplier = 0;
	timeouts.WriteTotalTimeoutConstant = 2000;
	SetCommTimeouts(hPicPort[HandleNum], &timeouts);

	//SetCommMask(hPicPort[HandleNum], EV_RXCHAR|EV_TXEMPTY );							//设置事件驱动的类型

	//read and the com port
	BOOL bReadStatus;
	DWORD dwBytesRead;
	char buffer[256];

	Sleep(100);
	int i = 0;
	do
	{
		bReadStatus = ReadFile(hPicPort[HandleNum], buffer, 10000, &dwBytesRead, NULL);	//make sure rx data cleaned
		i++;
	} while ((dwBytesRead != 0) && i<10);

	return TRUE;																		//返回TRUE，打开串口正常
}

BOOL IMEIWrite_MulAT::Get_Wifi_Data(CEdit* m_Result, CEdit* Final_Result_Control, int HandleNum, CString StartSign, CString EndSign, BOOL InThread)	//获取按钮WIFI数据---WIFI确认标识
{
	if (CPUCommunication("确认成功", HandleNum, "", "", COM_State[THREAD_NUM], m_Result, Final_Result_Control) == TRUE)
		return TRUE;
	if (InThread == FALSE)EnterCriticalSection(&WIFICOMMUNICATE[HandleNum]);
	BOOL bReadStatus;
	DWORD dwBytesRead;
	char *buffer;
	char *p = NULL;
	char buf[1024];

	ResetButtonWIFI[HandleNum] = FALSE;
	//read and the com port
	CString Port_Temp = "";//串口读取数据 的缓存
	for (int r = 0; r<1; r++)
	{
		memset(buf, 0, sizeof(buf));
		buffer = buf;
		bReadStatus = ReadFile(hPicPort[HandleNum], buffer, 512, &dwBytesRead, NULL);
		if (dwBytesRead != 0)
		{
			if (strstr(EndSign, "NULL"))
				p = strstr(buffer, "\r");
			else
				p = strstr(buffer, EndSign);//RESET
			if (p)
			{
				ResetButtonWIFI[HandleNum] = TRUE;
				CString WIFILog;
				WIFILog.Format("%s\r\n", buffer);
				LogShow_exchange(m_Result, Final_Result_Control, 5, "收到WIFI数据:" + WIFILog, HandleNum);
				if (InThread == FALSE)LeaveCriticalSection(&WIFICOMMUNICATE[HandleNum]);
				return TRUE;
			}
			else
			{
				Port_Temp += CString(buffer);
				Sleep(200 * r);
				continue;
			}
		}
		else
		{
			Sleep(200 * r);
			continue;
		}
	}
	if (InThread == FALSE)LeaveCriticalSection(&WIFICOMMUNICATE[HandleNum]);
	return FALSE;
}
BOOL IMEIWrite_MulAT::Get_Pic_Data(CEdit* m_Result, CEdit* Final_Result_Control, CThumbnailBoxDlg* picdlg, int HandleNum, CString StartSign, CString EndSign, CString ChipRfIDbg, BOOL DayTimeStatep)		//获取图像端口数据
{
	LogShow_exchange(m_Result, Final_Result_Control, 0, "开始新一轮0\r\n", HandleNum, "-1", ChipRfIDbg);
	BOOL bReadStatus;
	DWORD dwBytesRead;
	char *buffer;
	char *p = NULL;
	char buf[PICDATAMAX];
	char OutStringBuffer[PICDATAMAX] = { 0 };
	CString RecString = "";

	CString NightDayValue;
	if (DayTimeStatep == TRUE)
		NightDayValue = "白天";
	else
		NightDayValue = "夜晚";
	CTime cTime;
	CString TimeShow;
	//read and the com port
	CString Port_Temp = "";//串口读取数据 的缓存

	PicUnionOK[HandleNum] = FALSE;
	UnionPicPath[HandleNum] = "";				//要清理上一次路径---------防止本次合成失败
	PicPath[DayTimeStatep][HandleNum] = "";	//要清理白天和黑夜的路径---防止本次获取失败

	cTime = CTime::GetCurrentTime();
	TimeShow.Format("-%02d%02d%02d", cTime.GetHour(), cTime.GetMinute(), cTime.GetSecond());
	LogShow_exchange(m_Result, Final_Result_Control, 0, TimeShow + "开始新一轮1\r\n", HandleNum, "-1", ChipRfIDbg);

	int r = 0;
	for (r = 0; r<11; r++)
	{
		memset(buf, 0, sizeof(buf));
		buffer = buf;
		bReadStatus = ReadFile(hPicPort[HandleNum], buffer, PICDATAMAX, &dwBytesRead, NULL);
		if (dwBytesRead != 0)
		{
			cTime = CTime::GetCurrentTime();
			TimeShow.Format("-%02d%02d%02d", cTime.GetHour(), cTime.GetMinute(), cTime.GetSecond());

			HexGroupToString(OutStringBuffer, (unsigned char *)buffer, dwBytesRead);
			RecString.Format("%s", OutStringBuffer);//--------------------------------------------------------------注意看是否需要清理空格
			LogShow_exchange(m_Result, Final_Result_Control, 5, NightDayValue + TimeShow + "开始接收图片数据\r\n", HandleNum, "-1", ChipRfIDbg);

			RecString.Replace(" ", "");
			WriteLogIni(HandleNum);
			WriteLog(RecString, HandleNum);
			WriteLogEnd(HandleNum);
			//if(strstr(EndSign,"NULL"))
			//p=strstr(OutStringBuffer,"\r");
			//else
			//p=strstr(OutStringBuffer,EndSign);
			//if(p)
			//int npos=RecString.Right(4).Find(EndSign);//FF D9             Right(4)-----数据中间也有FFD9
			//int npos=RecString.Right(32).Find(EndSign);
			int npos = RecString.Right(36).Find(EndSign);
			if (npos >= 0)
			{
				SerialPicOver[HandleNum] = TRUE; SerialPic[HandleNum].SetEvent();
				//RecString=RecString.Left(npos+EndSign.GetLength());                //数据中间也有FFD9
				CString strCommand_Vaule_Return;//串口返回值
				strCommand_Vaule_Return = Port_Temp + RecString;

				CString  selPort;
				selPort = strCommand_Vaule_Return;
				//selPort.Replace("\r","");
				//selPort.Replace("\n","");

				npos = selPort.Find(StartSign);//FF D8 
				if (npos >= 0)
				{
					Vaule_Pic[HandleNum] = selPort.Mid(npos);
					LogShow_exchange(m_Result, Final_Result_Control, 5, NightDayValue + TimeShow + "成功获取图片数据\r\n", HandleNum, "-1", ChipRfIDbg);
				}
				else
				{
					LogShow_exchange(m_Result, Final_Result_Control, 128, NightDayValue + TimeShow + "图片数据异常\r\n", HandleNum, "功能测试:2-TEST", ChipRfIDbg);
					goto ERRORPRO;
				}


				HEX2JPG3(Vaule_Pic[HandleNum], PicPath[DayTimeStatep][HandleNum], NightDayValue, HandleNum, ChipRfIDbg);
				LogShow_exchange(m_Result, Final_Result_Control, 5, TimeShow + "将16进制数据转换成JPG图片\r\n", HandleNum, "-1", ChipRfIDbg);
				if (DayTimeStatep == TRUE)//先夜拍，完成白天拍照后将两张图片拼接
				{
					if (PathFileExists(PicPath[0][HandleNum]) != TRUE)
					{
						LogShow_exchange(m_Result, Final_Result_Control, 5, TimeShow + "夜间模式图片不存在\r\n", HandleNum, "功能测试:2-TEST", ChipRfIDbg);
						//goto ERRORPRO;
					}
					if (PathFileExists(PicPath[1][HandleNum]) != TRUE)
					{
						LogShow_exchange(m_Result, Final_Result_Control, 128, TimeShow + "白天模式图片不存在\r\n", HandleNum, "功能测试:2-TEST", ChipRfIDbg);
						goto ERRORPRO;
					}

					if (1)//if(Jpgunionjpg(PicPath[0][HandleNum],PicPath[1][HandleNum],ChipRfIDbg,ChipRfIDbg,UnionPicPath[HandleNum])==TRUE)
					{
						PicUnionOK[HandleNum] = TRUE;
						LogShow_exchange(m_Result, Final_Result_Control, 5, TimeShow + "成功合成图片\r\n", HandleNum, "-1", ChipRfIDbg);

						if (BGConfirmChoose == TRUE)//后台开始确认，1、等待“控制板状态”，2、弹出确认界面，3、按钮后上传数据（线程传递芯片ID和图片显示时间参数）,4、失败按钮
						{
							/*
							CEdit* m_ResultPic;
							CEdit* Final_Result_ControlPic;

							if(HandleNum==0)
							{
							m_ResultPic=&m_Result9;
							Final_Result_ControlPic=&Final_Result_Control9;
							}
							else if(HandleNum==1)
							{
							m_ResultPic=&m_Result10;
							Final_Result_ControlPic=&Final_Result_Control10;
							}
							else if(HandleNum==2)
							{
							m_ResultPic=&m_Result11;
							Final_Result_ControlPic=&Final_Result_Control11;
							}
							else if(HandleNum==3)
							{
							m_ResultPic=&m_Result12;
							Final_Result_ControlPic=&Final_Result_Control12;
							}
							else if(HandleNum==4)
							{
							m_ResultPic=&m_Result13;
							Final_Result_ControlPic=&Final_Result_Control13;
							}
							else if(HandleNum==5)
							{
							m_ResultPic=&m_Result14;
							Final_Result_ControlPic=&Final_Result_Control14;
							}
							else if(HandleNum==6)
							{
							m_ResultPic=&m_Result15;
							Final_Result_ControlPic=&Final_Result_Control15;
							}
							else if(HandleNum==7)
							{
							m_ResultPic=&m_Result16;
							Final_Result_ControlPic=&Final_Result_Control16;
							}
							*/
							/////////////////////////////////////////////////////////////
							//等待门打开，才开始显示图片
							//WaitForSingleObject(DoorWait[HandleNum].m_hObject,60*1000);//等待60秒INFINITE
							WaitForSingleObject(DoorWait[HandleNum].m_hObject, INFINITE);
							DoorWait[HandleNum].ResetEvent();
							int t = 0;
							while (CPUCommunication("控制板状态", HandleNum, "", "", COM_State[THREAD_NUM], m_Result, Final_Result_Control) == FALSE)
							{

								if ((StopSign[HandleNum + 8] == TRUE) && (t>3))
									goto ERRORPRO;
								Sleep(2000);
								t++;
								if ((t>3) && (TestResult[HandleNum] == FALSE))
									break;
							}

							//等待门打开
							/////////////////////////////////////////////////////////////
							if (TestResult[HandleNum] == FALSE)
							{
								LogShow_exchange(m_Result, Final_Result_Control, 128, TimeShow + "测试线程失败！！！\r\n", HandleNum, "功能测试:2-TEST", ChipRfIDbg);
								goto ERRORPRO;
							}
							else
							{
								TestResult[HandleNum] = FALSE;//下一轮测试结果重新赋值
							}


							LogShow_exchange(m_Result, Final_Result_Control, 5, TimeShow + "单线程图片开始显示\r\n", HandleNum, "-1", ChipRfIDbg);
							EnterCriticalSection(&SHOWONEPIC);
							int i = 0;

							////////////////////////////////////////////////////
							//采用定时器显示图片
							if (PicStaticChoose == TRUE)
							{
								Picdlg[HandleNum].ShowPicTime = atoi(BGShowPicTime[HandleNum]);
								if ((PicPath[0][HandleNum] == "") || PicPath[1][HandleNum] == "")
								{
									LogShow_exchange(m_Result, Final_Result_Control, 5, "不存在图片", HandleNum, "功能测试:2-TEST", ChipRfIDbg);
									//LeaveCriticalSection(&SHOWONEPIC);
									//goto ERRORPRO;
								}
								Picdlg[HandleNum].PicPath0 = PicPath[0][HandleNum];
								Picdlg[HandleNum].PicPath1 = PicPath[1][HandleNum];
								Picdlg[HandleNum].ShowHandleHun = HandleNum;
								TimerHandleNum = HandleNum;
								SetTimer(48, 1, NULL);
							}
							//采用定时器显示图片
							/////////////////////////////////////////////////////
							CPUCommunication("确认失败", HandleNum, "", "", COM_State[THREAD_NUM], m_Result, Final_Result_Control);//先清理前面的
							CPUCommunication("确认成功", HandleNum, "", "", COM_State[THREAD_NUM], m_Result, Final_Result_Control);
							while (i<BGShowPicCount[HandleNum])
							{
								////////////////////////////////////////////////
								//失败按钮
								BOOL Result = CPUCommunication("确认失败", HandleNum, "", "", COM_State[THREAD_NUM], m_Result, Final_Result_Control);
								if (Result == TRUE)
								{
									LogShow_exchange(m_Result, Final_Result_Control, 128, "确认失败", HandleNum, "功能测试:2-TEST", ChipRfIDbg);
									Picdlg[TimerHandleNum].SendMessage(WM_CLOSE);//Picdlg[TimerHandleNum].EndDialog(0);
									LeaveCriticalSection(&SHOWONEPIC);
									goto ERRORPRO;
								}
								//失败按钮
								////////////////////////////////////////////////


								////////////////////////////////////////////////
								//循环等待WIFI按钮
								Final_Result_Control->SetFont(fontsp);
								gColor[HandleNum + 8] = RGB(205, 0, 205);
								Final_Result_Control->SetWindowTextA("请确认");
								Final_Result_Control->UpdateWindow();

								if (Get_Wifi_Data(m_Result, Final_Result_Control, HandleNum, "", "RESET", TRUE) != TRUE)
								{
									if (PicStaticChoose == FALSE)
									{
										Picdlg[HandleNum].ShowPicTime = atoi(BGShowPicTime[HandleNum]);
										if ((PicPath[0][HandleNum] == "") || PicPath[1][HandleNum] == "")
										{
											LogShow_exchange(m_Result, Final_Result_Control, 5, "不存在图片", HandleNum, "功能测试:2-TEST", ChipRfIDbg);
											//LeaveCriticalSection(&SHOWONEPIC);
											//goto ERRORPRO;
										}
										Picdlg[HandleNum].PicPath0 = PicPath[0][HandleNum];
										Picdlg[HandleNum].PicPath1 = PicPath[1][HandleNum];
										if (Picdlg[HandleNum].DoModal() == -1)
										{
											LogShow_exchange(m_Result, Final_Result_Control, 128, "弹出窗口失败", HandleNum, "功能测试:2-TEST", ChipRfIDbg);
											LeaveCriticalSection(&SHOWONEPIC);
											goto ERRORPRO;
										}
									}
									else
									{
										Sleep(1500);
									}
								}
								else//上传数据
								{
									BOOL DBResult = FALSE;
									EnterCriticalSection(&UPDATEDB[HandleNum % (THREAD_NUM / 8)]);
									if (WorkStationCS == "SMT测试")
									{
										DBResult = Data_UpdatePara2(IMEI_Setdlg.Mulmyado[HandleNum], HandleNum, m_Result, Final_Result_Control, FALSE, ChipRfIDbg);
										if (DBResult == FALSE)
										{
											Sleep(3000);
											DBResult = Data_UpdatePara2(IMEI_Setdlg.Mulmyado[HandleNum], HandleNum, m_Result, Final_Result_Control, TRUE, ChipRfIDbg);
										}
									}
									else if (WorkStationCS == "组装测试")
									{
										DBResult = Data_UpdatePara(IMEI_Setdlg.Mulmyado[HandleNum], HandleNum, m_Result, Final_Result_Control, FALSE, ChipRfIDbg);
										if (DBResult == FALSE)
										{
											Sleep(3000);
											DBResult = Data_UpdatePara(IMEI_Setdlg.Mulmyado[HandleNum], HandleNum, m_Result, Final_Result_Control, TRUE, ChipRfIDbg);
										}
									}
									else if (WorkStationCS == "研发测试")
									{
										DBResult = TRUE;
									}
									else
									{
										LeaveCriticalSection(&UPDATEDB[HandleNum % (THREAD_NUM / 8)]);
										LogShow_exchange(m_Result, Final_Result_Control, 128, "请先选择站位！！！", HandleNum, "功能测试:2-TEST", ChipRfIDbg);
										Picdlg[TimerHandleNum].SendMessage(WM_CLOSE);//Picdlg[TimerHandleNum].EndDialog(0);
										LeaveCriticalSection(&SHOWONEPIC);
										goto ERRORPRO;
									}
									LeaveCriticalSection(&UPDATEDB[HandleNum % (THREAD_NUM / 8)]);

									if (DBResult == FALSE)
									{
										LogShow_exchange(m_Result, Final_Result_Control, 128, "上传数据失败", HandleNum, "功能测试:3-DB", ChipRfIDbg);
										Picdlg[TimerHandleNum].SendMessage(WM_CLOSE);//Picdlg[TimerHandleNum].EndDialog(0);
										LeaveCriticalSection(&SHOWONEPIC);
										goto ERRORPRO;
									}
									else
									{
										LogShow_exchange(m_Result, Final_Result_Control, 255, "成功", HandleNum, "-1", ChipRfIDbg);
										//Sleep(2000);
										//LogShow_exchange(m_Result,Final_Result_Control,0,TimeShow+"等待下一台\r\n",HandleNum,"-1",ChipRfIDbg);
										Picdlg[TimerHandleNum].SendMessage(WM_CLOSE);//
										Sleep(1000);
										LeaveCriticalSection(&SHOWONEPIC);
										return TRUE;
									}
								}
								//循环等待WIFI按钮
								////////////////////////////////////////////////
								i++;
								if (i == BGShowPicCount[HandleNum])//最后一次了，要失败
								{
									Picdlg[TimerHandleNum].SendMessage(WM_CLOSE);//Picdlg[TimerHandleNum].EndDialog(0);
									LeaveCriticalSection(&SHOWONEPIC);
									LogShow_exchange(m_Result, Final_Result_Control, 128, "确认超时", HandleNum, "功能测试:2-TEST", ChipRfIDbg);
									goto ERRORPRO;
								}
							}
						}
						else
						{
							return TRUE;
						}
					}
					else
					{
						LogShow_exchange(m_Result, Final_Result_Control, 128, TimeShow + "合成图片失败\r\n", HandleNum, "功能测试:2-TEST", ChipRfIDbg);
						return FALSE;
					}
				}
				else
				{
					ReadFile(hPicPort[HandleNum], buffer, PICDATAMAX, &dwBytesRead, NULL);
					return TRUE;//夜拍模式成功
				}
			}
			else
			{
				Port_Temp += RecString;//CString(buffer);
				Sleep(200 * r);
				continue;
			}
		}
		else
		{
			Sleep(200 * r);
			continue;
		}
	}
ERRORPRO:
	//if(BGConfirmChoose==TRUE)Sleep(2000);
	SerialPicOver[HandleNum] = TRUE; SerialPic[HandleNum].SetEvent();
	if (r >= 11)
	{
		LogShow_exchange(m_Result, Final_Result_Control, 128, "获取图片超时", HandleNum, "功能测试:2-TEST", ChipRfIDbg);
	}
	else
	{
		//LogShow_exchange(m_Result,Final_Result_Control,0,TimeShow+"等待下一台\r\n",HandleNum,"-1",ChipRfIDbg);
	}
	return FALSE;
}

/***********************************************************************************
fp:  源图片，jpg->bin数据
out: 输出图片的Ascii码
dstFile:最终生成的图片 bin数据->jpg

r 打开只读文件，该文件必须存在。
r+ 打开可读写的文件，该文件必须存在。
rb+ 读写打开一个二进制文件，只允许读写数据。
rt+ 读写打开一个文本文件，允许读和写。
w 打开只写文件，若文件存在则文件长度清为0，即该文件内容会消失。若文件不存在则建立该文件。
w+ 打开可读写文件，若文件存在则文件长度清为零，即该文件内容会消失。若文件不存在则建立该文件。
a 以附加的方式打开只写文件。若文件不存在，则会建立该文件，如果文件存在，写入的数据会被加到文件尾，即文件原先的内容会被保留。
a+ 以附加方式打开可读写的文件。若文件不存在，则会建立该文件，如果文件存在，写入的数据会被加到文件尾后，即文件原先的内容会被保留。
wb 只写打开或新建一个二进制文件；只允许写数据。
wb+ 读写打开或建立一个二进制文件，允许读和写。
wt+ 读写打开或着建立一个文本文件；允许读写。
at+ 读写打开一个文本文件，允许读或在文本末追加数据。
ab+ 读写打开一个二进制文件，允许读或在文件末追加数据。
***********************************************************************************/

BOOL IMEIWrite_MulAT::HEX2JPG()
{
	FILE *fp, *out, *dstFile;
	long int size;
	unsigned char ch1, ch2;
	long int i;

	if ((out = fopen("JpgToBin.txt", "wb+")) == NULL)
	{
		printf("can't open the txt file\n");
		return FALSE;
	}
	if ((dstFile = fopen("Recv.jpg", "wb+")) == NULL)
	{
		printf("can't open the dst file\n");
		return FALSE;
	}
	rewind(fp);
	bool FormatRight = false;
	for (i = 0; i<size; i++)
	{
		ch1 = fgetc(fp);
		if (ch1 == 0xff)
		{
			ch2 = fgetc(fp);
			if (ch2 == 0xd8)
			{
				fputc(ch1, dstFile);															//fp为文件指针，它的值是执行fopen()打开文件时获得的。
				fputc(ch2, dstFile);
				FormatRight = true;
				break;
			}
		}
	}
	if (FormatRight)
	{
		for (; i<size; i++)
		{
			ch1 = fgetc(fp);
			fputc(ch1, dstFile);
			if (ch1 == 0xff)
			{
				ch2 = fgetc(fp);
				if (ch2 == 0xd9)
					break;
				else
					fputc(ch2, dstFile);
			}
		}
	}
	fclose(out);
	fclose(dstFile);

	return TRUE;
}



//Socket接收到的视频数据包，都存储到source.txt文件中，然后读这个文件，找FFD8和FFD9之间的数据，写入dest.jpg文件中；
//从文本文件中读FFD8和FFD9之间的数据，保存到一张图片中
BOOL IMEIWrite_MulAT::HEX2JPG2()
{
	unsigned char chBuf[PICDATAMAX] = { 0 };
	unsigned char chBegin[2] = { 0xFF, 0xD8 };
	unsigned char chEnd[2] = { 0xFF, 0xD9 };
	int iLen = 0;
	int iBegin = 0;
	int iEnd = 0;

	FILE* fp = fopen("source.txt", "rb+"); //读写方式打开二进制文件
	if (fp != NULL)
	{
		fseek(fp, 0, SEEK_END); //文件指针定向到文件结尾
		iLen = ftell(fp);       //获取文件长度  
		fseek(fp, 0, SEEK_SET); //定位到文件开始位置  
		fread(chBuf, 1, iLen, fp);
		int ioffset = 0;        //偏移量,一次移动两个字节

		while (1)
		{
			//查找 FF D8
			if (!memcmp(chBuf + ioffset, chBegin, 2))
			{
				iBegin = ioffset;
			}

			//查找 FF D9
			if (!memcmp(chBuf + ioffset, chEnd, 2))
			{
				iEnd = ioffset;
				break;
			}

			ioffset += 2;
		}
	}
	fclose(fp);

	fp = fopen("dest.jpg", "wb");
	if (NULL != fp)
	{
		fwrite(chBuf + iBegin, iEnd - iBegin + 2, 1, fp);
	}
	fclose(fp);
	return TRUE;
}



// 打开图片文件以16进制显示
void IMEIWrite_MulAT::JPG2HEX()
{
	// TODO: Add your control notification handler code here
	CString m_sPath;
	CFileDialog dlg(TRUE, "jpg", "*.jpg",
		OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		"JPEG文件(*.jpg)|*.jpg|JPEG文件(*.JPG)|*.JPG||", NULL);
	if (dlg.DoModal() == IDOK)
	{
		m_sPath = dlg.GetPathName();
		Invalidate();
	}
	if (!m_sPath.IsEmpty())
	{
		CString tmp, str;
		CFile file;
		file.Open(m_sPath, CFile::typeBinary | CFile::modeRead | CFile::shareDenyNone);
		DWORD dwSize = (DWORD)file.GetLength();
		BYTE buf[PICDATAMAX];
		int count = file.Read(buf, PICDATAMAX);
		file.Close();
		for (int i = 0; i<count; i++)
		{
			tmp.Format("%02X", buf[i]);
			if (strlen(tmp) > 2)
				tmp = tmp.Mid(strlen(tmp) - 2);
			str += tmp;
		}
	}
}

// 将16进制数据转换成JPG图片
BOOL IMEIWrite_MulAT::HEX2JPG3(CString str, CString& m_sPath, CString NightDay, int HandleNum, CString ChipRfIDbg)
{
	// TODO: Add your control notification handler code here
	CString ChipRfIDCurrent;
	if (ChipRfIDbg != "")
	{
		ChipRfIDCurrent = ChipRfIDbg;
	}
	else
	{
		ChipRfIDCurrent = ChipRfID[HandleNum];
	}
	CTime cTime;
	char m_Time_Date[128];
	char m_Time_Time[128];
	CString m_strModulePath;
	CString LogFile;

	char cReportPath[256] = { 0 };
	DWORD dwAttr;

	char file_name[_MAX_PATH] = { 0 };
	GetModuleFileName(NULL, file_name, _MAX_PATH);
	CString strFileName = file_name;
	int num = strFileName.ReverseFind('\\');

	CString LogNum_CS;
	LogNum_CS.Format("%d", HandleNum);

	m_strModulePath = strFileName.Left(num + 1) + LogNum_CS + "\\";

	cTime = CTime::GetCurrentTime();                                                         //创建以当日日期的文件夹来保存数据
	sprintf(m_Time_Date, "%s%04d%02d%02d", m_strModulePath, cTime.GetYear(), cTime.GetMonth(), cTime.GetDay());//把日期以6位放入一个Char组
	//创建以日期为名字的文件夹
	dwAttr = GetFileAttributes(m_Time_Date);
	if (dwAttr == 0xFFFFFFFF)                                                                 //文件夹不存在   
	{
		CreateDirectory(m_Time_Date, NULL);
	}

	sprintf(m_Time_Time, "%s-%s-%02d%02d%02d", ChipRfIDCurrent, NightDay, cTime.GetHour(), cTime.GetMinute(), cTime.GetSecond());

	m_sPath.Format("%s\\%s.jpg", m_Time_Date, m_Time_Time);


	if (!m_sPath.IsEmpty())
	{
		CFile file;
		file.Open(m_sPath, CFile::typeBinary | CFile::modeCreate | CFile::modeWrite | CFile::modeNoTruncate, NULL);

		int dwSize = str.GetLength() / 2;
		BYTE *buf = StrDecToCHex(dwSize, str);
		file.Write(buf, dwSize);
		file.Close();
		//AfxMessageBox("转换成功！");
	}
	return TRUE;
}


// 16进制特殊字符转换
BYTE IMEIWrite_MulAT::ConvertHexChar(BYTE ch)
{
	if ((ch >= '0') && (ch <= '9'))
		return ch - 0x30;
	else if ((ch >= 'A') && (ch <= 'F'))
		return ch - 'A' + 10;
	else if ((ch >= 'a') && (ch <= 'f'))
		return ch - 'a' + 10;
	else
		return -1;
}

// CString转BYTE*
BYTE * IMEIWrite_MulAT::StrDecToCHex(int &length, CString str)
{
	BYTE	tmpByte = 0x00;
	int		slen = str.GetLength();

	length = 0;

	//BYTE * h_data = new BYTE[102400];
	BYTE * h_data = new BYTE[slen];

	::memset(h_data, '\0', sizeof(h_data));

	//for (int i=0, j=0; i<102400,j<slen; i++,j++)
	for (int i = 0, j = 0; i<slen, j<slen; i++, j++)
	{
		if (str[j] == ' ')
			++j;
		tmpByte = str[j];
		h_data[i] = ConvertHexChar(tmpByte) << 4;				//左移4位
		if (str[++j] == ' ')
			++j;
		tmpByte = str[j];
		h_data[i] = h_data[i] + (ConvertHexChar(tmpByte) & 0xF);	//取低4位然后相加。
		length = i + 1;
	}

	return h_data;
}



BOOL IMEIWrite_MulAT::Jpgunionjpg(CString szPathName1, CString szPathName2, CString strInfo, CString PicName, CString& strDestJpgName)
{
	CBasicUse m_bUse;
	CString m_dllNameBmpToJpg;		//指定动态库的名称
	//先取到应用程序目录，否则在对话框窗口被打开后，应用程序目录会被改
	CString strCurPath;//CString strCurPath= m_bUse.GetApp_Path();
	char file_name[_MAX_PATH] = { 0 };
	GetModuleFileName(NULL, file_name, _MAX_PATH);
	CString strFileName = file_name;
	int num = strFileName.ReverseFind('\\');

	strCurPath = strFileName.Left(num + 1);


	m_dllNameBmpToJpg = strCurPath + "\\PictureProcessDll.dll";
	CPictureProcess picProcess;


	int nPicCounts = 2;
	CArray<CString, CString> arrPic;
	arrPic.Add(szPathName1);
	arrPic.Add(szPathName2);

	//SetCurrentDirectory(strCurPath);	//重新设置回EXE程序所在目录，以便动态库里用
	//得到目录
	CString strPicPath = arrPic.GetAt(0);
	//int nFindPos = strPicPath.ReverseFind('\\');
	int nFindPos = strPicPath.ReverseFind('.');
	strPicPath = strPicPath.Mid(0, nFindPos);

	//JPG TO BMP 并放到应用程序目录里的临时目录tempPic下
	CString * sFilePath;

	int nLen = arrPic.GetSize();	//图片个数
	sFilePath = new CString[nLen];
	for (int i = 0; i<nLen; i++)
	{
		sFilePath[i] = arrPic.GetAt(i);
		//先转成BMP，之后返回的文件目录在应用程序目录的临时目录下，作为临时文件BMP了
		//sFilePath[i] = picProcess.ChangeJPGToBMP(sFilePath[i],m_dllNameBmpToJpg);
		sFilePath[i] = picProcess.ChangeJPGToBMP2(sFilePath[i]);
	}


	//
	//CString strDestBmpName= strPicPath+"\\"+PicName+".bmp";		//合成bmp图名称
	//strDestJpgName= strPicPath+"\\"+PicName+".jpg";				//jpg图名称
	CString strDestBmpName = strPicPath + "ALL.bmp";		//合成bmp图名称
	strDestJpgName = strPicPath + "ALL.jpg";				//jpg图名称

	/////////合成BMP图片////////////////////////////////
	CCombineImages CombineImages;
	CombineImages.SetpWnd(this);
	CombineImages.SetAddTextFlag(TRUE);
	CombineImages.SetTextInfo(strInfo);

	/////合成
	//CombineImages.CombineImagesEntry(sFilePath,nLen,strDestBmpName,1);	//按横向合成
	if (CombineImages.CombineImagesEntry(sFilePath, nLen, strDestBmpName, 0) == FALSE)
	{
		strDestJpgName = "";
		return FALSE;
	}
	Sleep(10);
	//delete tempPic目录下的所有bmp
	m_bUse.DeleteFilesInFolder("tempPic");
	//bmp to jpg
	//picProcess.ChangeBMPToJPG(strDestBmpName,m_dllNameBmpToJpg);
	picProcess.ChangeBMPToJPG2(strDestBmpName);
	Sleep(10);

	//显示合成的BMP到控件上
	::DeleteFile(strDestBmpName);
	//HWND hwndStill = NULL;
	//m_Pic.MoveWindow(20,40,1000,200,TRUE);
	//GetDlgItem(IDC_PIC, &hwndStill);
	//picProcess.ShowPicture(strDestBmpName,hwndStill);
	return TRUE;
}
void IMEIWrite_MulAT::OnBnClickedCheck41()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);
}

void IMEIWrite_MulAT::OnBnClickedCheck42()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);
}

void IMEIWrite_MulAT::OnBnClickedCheck43()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);
}

void IMEIWrite_MulAT::OnCbnSelchangeCombo56()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);
}


/*三合一新增功能*/

/*进程通信模块*/
//自定义系统消息处理函数（截止摄像头功能添加，目前还没用到）
afx_msg LRESULT IMEIWrite_MulAT::MSG_GetSimpleMessage(WPARAM wParam, LPARAM lParam)
{
	switch (wParam)
	{
	case 3:
		break;
	default:
		break;
	}
	return 0;
}

//自定义系统消息发送函数，用来发送句柄给主控程序的
void IMEIWrite_MulAT::MSG_SendSimpleMessage()
{
	HWND ADCTHwnd = NULL;
	ADCTHwnd = ::FindWindow("ADCT", "AutoDownloadATETest");
	::SendMessage(ADCTHwnd, WM_SimpleMessage, 3, (LPARAM)this->m_hWnd);
}

//通过COPYDATA发送结构体
void IMEIWrite_MulAT::MSG_SendCopyDataMessage(CopyDataMSG *MessageStruct, int replyInt)
{
	COPYDATASTRUCT copyData;
	copyData.dwData = replyInt;//0是发送，1是回复
	copyData.cbData = sizeof(CopyDataMSG);
	copyData.lpData = (PVOID)MessageStruct;
	::SendMessageTimeout(::FindWindow("ADCT", "AutoDownloadATETest"), WM_COPYDATA, (WPARAM)GetSafeHwnd(), (LPARAM)&copyData, SMTO_ABORTIFHUNG, 3000, 0);
}

//通过COPYDATA接收结构体
BOOL IMEIWrite_MulAT::OnCopyData(CWnd* pWnd, COPYDATASTRUCT* pCopyDataStruct)
{
	// TODO:  在此添加消息处理程序代码和/或调用默认值
	if (pCopyDataStruct->cbData > 0)
	{
		CopyDataMSG *copydataMsg = new CopyDataMSG;
		copydataMsg = (CopyDataMSG *)pCopyDataStruct->lpData;

		//0代表是ADCT主控进行查询，1代表是ADCT主控现在要进行设置
		switch (pCopyDataStruct->dwData)
		{
		case 0:
			ADCTInquire(copydataMsg->Port, copydataMsg->MessageNum);
			break;
		case 1:
			ADCTSetup(copydataMsg->Port, copydataMsg->MessageNum, copydataMsg->MessageChar);
			break;
		default:
			break;
		}
	}

	return CResizableDialog::OnCopyData(pWnd, pCopyDataStruct);
}


//ADCT进行查询时
void IMEIWrite_MulAT::ADCTInquire(CString PortNo, INT CommandNo)
{
	CopyDataMSG *copydataMsg = new CopyDataMSG;
	strcpy(copydataMsg->Port, (LPCTSTR)PortNo);
	copydataMsg->MessageNum = CommandNo;

	//0代表检测串口状态，1表示查询当前窗口还在不在
	switch (CommandNo)
	{
	case 0:
		copydataMsg->MessageNum = 5;
		if (StopSign[PortStatusMap[PortNo]])//StopSign为FLASE时表示串口被占用，为TRUE时表示该串口为停止状态
		{
			strcpy(copydataMsg->MessageChar, (LPCTSTR)"1");
		}
		else
		{

			strcpy(copydataMsg->MessageChar, (LPCTSTR)"0");
		}
		break;
	case 1:
		copydataMsg->MessageNum = 4;
		strcpy(copydataMsg->MessageChar, (LPCTSTR)"OK");
		break;
	default:
		break;
	}


	MSG_SendCopyDataMessage(copydataMsg, 1);

	if (copydataMsg != NULL)
	{
		delete copydataMsg;
		copydataMsg = NULL;
	}
}

//ADCT进行设置时
void IMEIWrite_MulAT::ADCTSetup(CString PortNo, INT CommandNo, CString message)
{
	int ResultFlag;//结果返回值
	CString Str_ResultFlag;
	CopyDataMSG *copydataMsg = new CopyDataMSG;
	strcpy(copydataMsg->Port, (LPCTSTR)PortNo);
	copydataMsg->MessageNum = CommandNo;

	//CommandNo和Status变量，0是初始化，1是串口开始，2是串口关闭，3是设置重新测试，4是泰斗，5是终端稳定或跳动串口
	switch (CommandNo)
	{
	case 0:
		ResultFlag = InitSetting(message.Left(message.Find("@@")), message.Right(message.GetLength() - message.Find("@@") - 2));//截取订单号和版本号
		Str_ResultFlag.Format("%d", ResultFlag);
		strcpy(copydataMsg->MessageChar, (LPCTSTR)Str_ResultFlag);//根据初始化结果决定是0还是1
		break;
	case 1:

		//利用队列来判断哪个端口没被占用，一般来说ComFreeDeq的队首肯定是空闲的，然后用map去存放当前COM号处在哪个端口中，后续可利用StopSign[PortStatusMap[PortNo]]判断此端口状态
		if (StopSign[ComFreeDeq[0]] == TRUE)
		{
			PortStatusMap[PortNo] = ComFreeDeq[0];
			ComoOccupancydDeq.push_front(ComFreeDeq[0]);
			ComFreeDeq.pop_front();
			ResultFlag = StartPortTest(PortNo);//实现串口开始

			//如果串口开始失败
			if (ResultFlag == 0)
			{
				if (!ComoOccupancydDeq.empty())
				{
					//先从占用队列中删掉对应的端口
					deque<int>::iterator pos;
					pos = find(ComoOccupancydDeq.begin(), ComoOccupancydDeq.end(), PortStatusMap[PortNo]);
					ComoOccupancydDeq.erase(pos);
					//然后再将其重新放到空闲队列中并排序
					ComFreeDeq.push_front(PortStatusMap[PortNo]);
					std::sort(ComFreeDeq.begin(), ComFreeDeq.end());
				}
			}

		}
		else
		{
			ResultFlag = 0;//如果16个串口都被占用了，直接回复0
		}

		Str_ResultFlag.Format("%d", ResultFlag);
		strcpy(copydataMsg->MessageChar, (LPCTSTR)Str_ResultFlag);//根据串口是否能够开始决定是0还是1		
		break;
	case 2:
		if (PortNo == "ALL")
		{
			OnBnClickedCancel();
			strcpy(copydataMsg->MessageChar, (LPCTSTR)"1");//根据串口是否能够关闭决定是0还是1
			break;
		}
		ResultFlag = StopPortTest(PortNo);//实现串口强制关闭
		Str_ResultFlag.Format("%d", ResultFlag);
		strcpy(copydataMsg->MessageChar, (LPCTSTR)Str_ResultFlag);//根据串口是否能够关闭决定是0还是1
		break;
	case 3://重新测试
		if (message == "0")
		{
			RTestChoose = FALSE;
		}
		else if (message == "1")
		{
			RTestChoose = TRUE;
		}
		//if (RTestChoose == FALSE)
		//{
		//	RTestChoose = TRUE;
		//}
		//else if (RTestChoose == TRUE)
		//{
		//	RTestChoose = FALSE;
		//}
		strcpy(copydataMsg->MessageChar, (LPCTSTR)"ALL");
		UpdateData(FALSE);
		break;
	case 4://泰斗
		if (message == "0")
		{
			GPSDataTypeChoose = FALSE;
		}
		else if (message == "1")
		{
			GPSDataTypeChoose = TRUE;
		}
		strcpy(copydataMsg->MessageChar, (LPCTSTR)"ALL");
		UpdateData(FALSE);
		break;
	case 5://终端稳定端口和跳动端口
		if (message == "0")
		{
			SetDlgItemText(IDC_COMBO28,"终端---稳定端口");
		}
		else if (message == "1")
		{
			SetDlgItemText(IDC_COMBO28, "终端---跳动端口");
		}
		strcpy(copydataMsg->MessageChar, (LPCTSTR)"ALL");
		UpdateData(FALSE);
		break;
	default:
		break;
	}

	MSG_SendCopyDataMessage(copydataMsg, 1);//1代表回复给主控程序

	if (copydataMsg != NULL)
	{
		delete copydataMsg;
		copydataMsg = NULL;
	}
}

//给ADCT主控程序发送测试结果
void IMEIWrite_MulAT::SendADCTTestResult(CString PortNo, CString message)
{
	CopyDataMSG *copydataMsg = new CopyDataMSG;
	strcpy(copydataMsg->Port, (LPCTSTR)PortNo);
	copydataMsg->MessageNum = 3;
	strcpy(copydataMsg->MessageChar, (LPCTSTR)message);

	MSG_SendCopyDataMessage(copydataMsg, 1);

	if (copydataMsg != NULL)
	{
		delete copydataMsg;
		copydataMsg = NULL;
	}
}

//给ADCT主控程序发送提示信息
void IMEIWrite_MulAT::SendADCTHint(CString PortNo, CString message, int MessageType)
{
	CopyDataMSG *copydataMsg = new CopyDataMSG;
	strcpy(copydataMsg->Port, (LPCTSTR)PortNo);
	copydataMsg->MessageNum = MessageType;
	strcpy(copydataMsg->MessageChar, (LPCTSTR)message);

	MSG_SendCopyDataMessage(copydataMsg, 0);

	if (copydataMsg != NULL)
	{
		delete copydataMsg;
		copydataMsg = NULL;
	}
}

//程序异常退出，微软自带的API，当程序触发异常时，优先调用此程序
LONG CallBackCrashHandler(EXCEPTION_POINTERS *pException)
//LONG WINAPI IMEIWrite_MulAT::FreeEIM_UnhandledExceptionFilter(LPEXCEPTION_POINTERS ExceptionInfo)
{
	if (g_ADCTFlag == 1)
	{
		IMEIWrite_MulAT imeidlg;
		//AfxMessageBox("Exception");	//调用回调函数成功

		CopyDataMSG *copydataMsg = new CopyDataMSG;

		strcpy(copydataMsg->Port, (LPCTSTR)"ALL");
		copydataMsg->MessageNum = 5;
		strcpy(copydataMsg->MessageChar, (LPCTSTR)"98");

		imeidlg.MSG_SendCopyDataMessage(copydataMsg, 0);

		if (copydataMsg != NULL)
		{
			delete copydataMsg;
			copydataMsg = NULL;
		}
	}

	return EXCEPTION_CONTINUE_SEARCH;//返回本回调函数的处理结果
	//return EXCEPTION_EXECUTE_HANDLER;
}

//程序正常退出
void IMEIWrite_MulAT::OnClose()
{
	// TODO:  在此添加消息处理程序代码和/或调用默认值
	if (g_ADCTFlag == 1)
	{
		CopyDataMSG *copydataMsg = new CopyDataMSG;

		strcpy(copydataMsg->Port, (LPCTSTR)"ALL");
		copydataMsg->MessageNum = 5;
		strcpy(copydataMsg->MessageChar, (LPCTSTR)"99");

		MSG_SendCopyDataMessage(copydataMsg, 0);

		if (copydataMsg != NULL)
		{
			delete copydataMsg;
			copydataMsg = NULL;
		}
	}

	CResizableDialog::OnClose();
}

//实现自动获取配置
int IMEIWrite_MulAT::InitSetting(CString Zhidan, CString Version)
{
	SetDlgItemText(IDC_COMBO51, Zhidan);
	SetDlgItemText(IDC_COMBO56, "SMT测试");
	m_SoftVersion = Version;
	OnGetWebSetting();

	return m_bVar;
}

//实现串口开始
int IMEIWrite_MulAT::StartPortTest(CString PortNo)
{
	int flag;

	//if (StopSign[0] == TRUE)
	//{
	//	PortStatusMap[PortNo] = 0;
	//}
	//else if (StopSign[1] == TRUE)
	//{
	//	PortStatusMap[PortNo] = 1;
	//}
	//else if (StopSign[2] == TRUE)
	//{
	//	PortStatusMap[PortNo] = 2;
	//}
	//else if (StopSign[3] == TRUE)
	//{
	//	PortStatusMap[PortNo] = 3;

	//}
	//else
	//{
	//	return 0;//如果以上四个串口都被占用了，直接回复0
	//}

	//然后直接开始当前能用的端口
	switch (PortStatusMap[PortNo])
	{
	case 0:
		SetDlgItemText(IDC_COMBO1, PortNo);
		OnBnClickedButton1();
		break;
	case 1:
		SetDlgItemText(IDC_COMBO4, PortNo);
		OnBnClickedButton14();
		break;
	case 2:
		SetDlgItemText(IDC_COMBO6, PortNo);
		OnBnClickedButton16();
		break;
	case 3:
		SetDlgItemText(IDC_COMBO8, PortNo);
		OnBnClickedButton18();
		break;
	case 4:
		SetDlgItemText(IDC_COMBO10, PortNo);
		OnBnClickedButton20();
		break;
	case 5:
		SetDlgItemText(IDC_COMBO12, PortNo);
		OnBnClickedButton22();
		break;
	case 6:
		SetDlgItemText(IDC_COMBO29, PortNo);
		OnBnClickedButtonstart7();
		break;
	case 7:
		SetDlgItemText(IDC_COMBO31, PortNo);
		OnBnClickedButtonstart8();
		break;
	case 8:
		SetDlgItemText(IDC_COMBO33, PortNo);
		OnBnClickedButtonstart9();
		break;
	case 9:
		SetDlgItemText(IDC_COMBO35, PortNo);
		OnBnClickedButtonstart10();
		break;
	case 10:
		SetDlgItemText(IDC_COMBO37, PortNo);
		OnBnClickedButtonstart11();
		break;
	case 11:
		SetDlgItemText(IDC_COMBO39, PortNo);
		OnBnClickedButtonstart12();
		break;
	case 12:
		SetDlgItemText(IDC_COMBO41, PortNo);
		OnBnClickedButtonstart13();
		break;
	case 13:
		SetDlgItemText(IDC_COMBO43, PortNo);
		OnBnClickedButtonstart14();
		break;
	case 14:
		SetDlgItemText(IDC_COMBO45, PortNo);
		OnBnClickedButtonstart15();
		break;
	case 15:
		SetDlgItemText(IDC_COMBO47, PortNo);
		OnBnClickedButtonstart16();
		break;
	default:
		break;
	}

	//如果该端口成功被开启，就返回1，否则返回0
	if (StopSign[PortStatusMap[PortNo]] == FALSE)
	{
		flag = 1;
	}
	else if (StopSign[PortStatusMap[PortNo]] == TRUE)
	{
		flag = 0;
	}
	return flag;
}

//实现串口关闭
int IMEIWrite_MulAT::StopPortTest(CString PortNo)
{
	int flag;

	if (PortStatusMap.count(PortNo) == 0)
	{
		return 1;
	}

	//直接关闭当前端口
	switch (PortStatusMap[PortNo])
	{
	case 0:
		if (Thread_State[PortStatusMap[PortNo]] == TRUE)
		OnBnClickedButton2();
		break;
	case 1:
		if (Thread_State[PortStatusMap[PortNo]] == TRUE)
		OnBnClickedButton15();
		break;
	case 2:
		if (Thread_State[PortStatusMap[PortNo]] == TRUE)
		OnBnClickedButton17();
		break;
	case 3:
		if (Thread_State[PortStatusMap[PortNo]] == TRUE)
		OnBnClickedButton19();
		break;
	case 4:
		if (Thread_State[PortStatusMap[PortNo]] == TRUE)
		OnBnClickedButton21();
		break;
	case 5:
		if (Thread_State[PortStatusMap[PortNo]] == TRUE)
		OnBnClickedButton23();
		break;
	case 6:
		if (Thread_State[PortStatusMap[PortNo]] == TRUE)
		OnBnClickedButtonstop7();
		break;
	case 7:
		if (Thread_State[PortStatusMap[PortNo]] == TRUE)
		OnBnClickedButtonstop8();
		break;
	case 8:
		if (Thread_State[PortStatusMap[PortNo]] == TRUE)
		OnBnClickedButtonstop9();
		break;
	case 9:
		if (Thread_State[PortStatusMap[PortNo]] == TRUE)
		OnBnClickedButtonstop10();
		break;
	case 10:
		if (Thread_State[PortStatusMap[PortNo]] == TRUE)
		OnBnClickedButtonstop11();
		break;
	case 11:
		if (Thread_State[PortStatusMap[PortNo]]] == TRUE)
		OnBnClickedButtonstop12();
		break;
	case 12:
		if (Thread_State[PortStatusMap[PortNo]]] == TRUE)
		OnBnClickedButtonstop13();
		break;
	case 13:
		if (Thread_State[PortStatusMap[PortNo]]] == TRUE)
		OnBnClickedButtonstop14();
		break;
	case 14:
		if (Thread_State[PortStatusMap[PortNo]]] == TRUE)
		OnBnClickedButtonstop15();
		break;
	case 15:
		if (Thread_State[PortStatusMap[PortNo]]] == TRUE)
		OnBnClickedButtonstop16();
		break;
	default:
		return 1;//如果没找到串口就代表这个串口从来没开始过，直接返回就行。
		break;
	}

	//如果该端口成功被关闭，就返回1，否则返回0
	if (StopSign[PortStatusMap[PortNo]] == TRUE)
	{
		flag = 1;
	}
	else if (StopSign[PortStatusMap[PortNo]] == FALSE)
	{
		flag = 0;
	}
	return flag;
}

//因为三合一的关系，所以要用它来代替程序原本的弹出框
void IMEIWrite_MulAT::AfxMessageBoxHint(CString str)
{
	if (g_ADCTFlag == TRUE)
	{
		SendADCTHint("ALL", str, 3);
	}
	else if (g_ADCTFlag == FALSE)
	{
		AfxMessageBox(str);
	}
}


/*整机测试新增功能*/

//点击蓝牙复选框的时候
void IMEIWrite_MulAT::OnBnClickedBluetoothCheck()
{
	// TODO:  在此添加控件通知处理程序代码
	UpdateData(TRUE);
}

//获取蓝牙配置文件里的指令和初始化一些变量
void IMEIWrite_MulAT::GetBleOrder()
{
	CFileFind finder;

	BOOL ifFind = finder.FindFile(_T(".\\BluetoothSetting.ini"));
	if (ifFind)
	{
		//将ini配置文件信息的东西读出来
		int IntValue;
		IntValue = GetPrivateProfileInt(_T("BluetoothOrder"), _T("OrderNumber"), 3, _T(".\\BluetoothSetting.ini"));
		OrderNumber = IntValue;//指令条数
		IntValue = GetPrivateProfileInt(_T("BluetoothInfo"), _T("ExitOrderNumber"), 2, _T(".\\BluetoothSetting.ini"));
		ExitOrderNumber = IntValue;//指令条数
		IntValue = GetPrivateProfileInt(_T("BluetoothInfo"), _T("MacIntercept"), 12, _T(".\\BluetoothSetting.ini"));
		MacIntercept = IntValue;//MAC应截取后几位
		IntValue = GetPrivateProfileInt(_T("BluetoothInfo"), _T("OrderCount"), 2, _T(".\\BluetoothSetting.ini"));
		OrderCount = IntValue;//蓝牙指令发送次数

		CString StrValue, stt;
		CString StrSetting = "Setting";
		//获取蓝牙连接指令
		for (int i = 0; i < OrderNumber; i++)
		{
			StrSetting.Format("Setting%d", i);
			GetPrivateProfileString(_T("BluetoothOrder"), StrSetting, _T(""), StrValue.GetBuffer(50), 50, _T(".\\BluetoothSetting.ini"));
			StrValue.ReleaseBuffer();//不先releasebuffer的话，会导致left等函数无法使用。
			BleOrder[i][0] = StrValue.Left(StrValue.Find("@@"));
			StrValue = StrValue.Right(StrValue.GetLength() - StrValue.Find("@@") - 2);
			BleOrder[i][1] = StrValue.Left(StrValue.Find("##"));
			BleOrderCount[i] = _ttoi(StrValue.Right(StrValue.GetLength() - StrValue.Find("##") - 2));
		}

		//获取蓝牙断开指令
		for (int i = 0; i < ExitOrderNumber; i++)
		{
			StrSetting.Format("ExitSetting%d", i);
			GetPrivateProfileString(_T("BluetoothInfo"), StrSetting, _T(""), StrValue.GetBuffer(50), 50, _T(".\\BluetoothSetting.ini"));
			StrValue.ReleaseBuffer();//不先releasebuffer的话，会导致left等函数无法使用。
			ExitOrder[i][0] = StrValue.Left(StrValue.Find("@@"));
			ExitOrder[i][1] = StrValue.Right(StrValue.GetLength() - StrValue.Find("@@") - 2);
		}
	}


	for (int i = 0; i < 16; i++)
	{
		StopSign[i] = 1;
	}

	ComFreeDeq = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
	MacScanEnbleFlag = TRUE;//使当前扫描枪能够接收MAC地址
	//std::sort(ComFreeDeq.begin(), ComFreeDeq.end());//这个是用来排序用的

}

//根据空闲队列ComFreeDeq的首位来开始某个端口
void IMEIWrite_MulAT::BleStartPortTest(CString MACStr)
{
	//先将空闲队列的第一个端口号给占用队列，然后再将这个端口号踢出队列
	ComoOccupancydDeq.push_front(ComFreeDeq[0]);
	ComFreeDeq.pop_front();
	BluetoothHint(ComFreeDeq[0], "不可");

	switch (ComoOccupancydDeq[0])
	{
	case 0:
		if (BluetoothConnect(0, MACStr, &m_Port1, &m_Baud1) == 1)
		{
			OnBnClickedButton1();
			BluetoothHint(1, "可扫");
		}
		else
		{
			BluetoothHint(1, "不可");
			BluetoothHint(0, "可扫");
			OnBnClickedButton2();
		}
		break;
	case 1:
		if (BluetoothConnect(1, MACStr, &m_Port2, &m_Baud2) == 1)
		{
			OnBnClickedButton14();
			BluetoothHint(2, "可扫");
		}
		else
		{
			BluetoothHint(2, "不可");
			BluetoothHint(1, "可扫");
			OnBnClickedButton15();
		}
		break;
	case 2:
		if (BluetoothConnect(1, MACStr, &m_Port2, &m_Baud2) == 1)
		{
			OnBnClickedButton16();
			BluetoothHint(3, "可扫");
		}
		else
		{
			BluetoothHint(3, "不可");
			BluetoothHint(2, "可扫");
			OnBnClickedButton17();
		}
		break;
	case 3:
		OnBnClickedButton18();
		break;
	case 4:
		OnBnClickedButton20();
		break;
	case 5:
		OnBnClickedButton22();
		break;
	case 6:
		OnBnClickedButtonstart7();
		break;
	case 7:
		OnBnClickedButtonstart8();
		break;
	case 8:
		OnBnClickedButtonstart9();
		break;
	case 9:
		OnBnClickedButtonstart10();
		break;
	case 10:
		OnBnClickedButtonstart11();
		break;
	case 11:
		OnBnClickedButtonstart12();
		break;
	case 12:
		OnBnClickedButtonstart13();
		break;
	case 13:
		OnBnClickedButtonstart14();
		break;
	case 14:
		OnBnClickedButtonstart15();
		break;
	case 15:
		OnBnClickedButtonstart16();
		break;
	default:
		break;
	}
}

//从队列中删除已经测试完的端口号
void IMEIWrite_MulAT::DeleteComoOccupancydDeq(int PortNo)
{
	if (!ComoOccupancydDeq.empty())
	{
		//先从占用队列中删掉对应的端口
		deque<int>::iterator pos;
		pos = find(ComoOccupancydDeq.begin(), ComoOccupancydDeq.end(), PortNo);
		ComoOccupancydDeq.erase(pos);
		BluetoothHint(ComFreeDeq[0], "W");
		//然后再将其重新放到空闲队列中并排序
		ComFreeDeq.push_front(PortNo);
		std::sort(ComFreeDeq.begin(), ComFreeDeq.end());
		if (g_ADCTFlag != 1)
		{
			BluetoothHint(ComFreeDeq[0], "可扫");

		}
		else if (g_ADCTFlag == 1)
		{
			if (PortStatusMap.count(get_map_key_value(PortStatusMap, PortNo)) > 0)
			{
				PortStatusMap.erase(get_map_key_value(PortStatusMap, PortNo));
			}
		}
	}
}

//蓝牙串口核心函数，里面包括获取配置时的蓝牙连接和开始测试时的蓝牙连接
void IMEIWrite_MulAT::BluetoothCoreFunction(CString MACStr)
{
	MacScanEnbleFlag = FALSE;//进来后先将扫描枪的数据获取失效

	UpdateData(TRUE);
	//先判断获取配置了没,BLEGetSettingFlag参数，-1代表还没获取配置，0代表正在获取配置需要扫入MAC地址去连接蓝牙，1代表蓝牙连接成功过现在去获取配置了，2代表蓝牙连接失败然后不要去获取配置了
	if (BLEGetSettingFlag != 1)
	{
		if (BLEGetSettingFlag == -1)
		{
			AfxMessageBox("请先获取配置！！");
			MacScanEnbleFlag = TRUE;
		}
		else if (BLEGetSettingFlag == 0)
		{
			BLEGetSettingFlag = BluetoothConnect(0, MACStr, &m_Port1, &m_Baud1);
		}
	}
	//如果已经获取配置，那接下来扫描MAC地址的时候就应该直接开始测试了，此时要先判断MacScanEnbleFlag的值，当它为FALSE的时候表示此时正有蓝牙正在连接，不允许再接收MAC地址的扫入了
	else if (GetBluetoothCheckValue == TRUE&&BLEGetSettingFlag == 1)
	{
		//BluetoothHint(ComFreeDeq[0], "可扫");
		BleStartPortTest(MACStr);
	}

	if (BLEGetSettingEndFlag == 1)
	{
		MacScanEnbleFlag = TRUE;//只有配置获取结束后，正常的自动测试的蓝牙连接才会在这里将扫描枪的数据获取置为有效（其余时刻此标志位在配置获取那里置为1或者-1）
	}

	SetDlgItemText(IDC_SCANDATA_EDIT, "");
}

//蓝牙连接函数
int IMEIWrite_MulAT::BluetoothConnect(int PortNo, CString MACStr, CComboBox* m_Port, CComboBox* m_Baud)
{
	BleGetSettingFlag[PortNo] = 1;

	BluetoothHint(PortNo, "等待");

	int ReturnFlag;
	if (OPen_Serial_Port(m_Port, m_Baud, PortNo) == TRUE)//OnGetWebSetting
	{
	}
	else
	{
		AfxMessageBox("打开串口失败");
		ReturnFlag = -1;
		return ReturnFlag;
	}

	CString Serial_Order_BleReturn, StrBleOrderSend, StrBleOrderReturn;//为了防止原有指令受到污染，这里必须利用临时变量来装载指令

	//此时根据蓝牙配置文件里的连接指令去发送蓝牙连接指令,当指令全部发送完且都成功后，就将BLEGetSettingFlag置为1表示连接成功了，否则置为2
	for (int i = 0, j = 0; i < OrderNumber; i++)
	{
		StrBleOrderSend = BleOrder[i][0];
		StrBleOrderReturn = BleOrder[i][1];

		if (StrBleOrderSend.Find("MACValue"))
			StrBleOrderSend.Replace("MACValue", MACStr);

		//如果检测到当前线程为TRUE，那就代表此时端口已经被关掉了
		if (BleGetSettingFlag[PortNo] == 0 && BLEGetSettingFlag == 1)
		{
			return -1;
		}

		Send_Serial_Order(&Serial_Order_BleReturn, StrBleOrderSend, PortNo, "NULL", "", BleOrderCount[i]);

		//这里的逻辑是，假设串口读出来的数据不对，且还没超过指定的发送次数，就利用i-1来重新发送该指令，如果读串口成功了，就将j置为0
		if (Vaule_Return_Count_CS[PortNo].Find(StrBleOrderReturn) != -1)
		{
			if (i == (OrderNumber - 1))
			{
				ReturnFlag = 1;
			}
			j = 0;
		}
		else if (Vaule_Return_Count_CS[PortNo] != StrBleOrderReturn)
		{
			if (j < OrderCount){
				i = i - 1;
				j = j + 1;
				continue;
			}
			ReturnFlag = 2;
			BluetoothHint(0, "连失败");
			LogShow_exchange(&m_Result1, &Final_Result_Control1, 6, "蓝牙连接失败，请检查终端和透传模块", 0);
			Sleep(2000);
			break;
		}

	}

	//要记得关闭串口
	BOOL Return;
	if (hPort[PortNo] != NULL)
		Return = CloseHandle(hPort[PortNo]);
	hPort[PortNo] = NULL;
	if (Return == TRUE)
	{
	}
	else
	{
		AfxMessageBox("关闭串口失败，重新插入USB可解决问题");
	}

	//BleGetSettingFlag[PortNo] = 0;

	return ReturnFlag;
}

//蓝牙断开函数
void IMEIWrite_MulAT::BluetoothDisconnect(int PortNo)
{
	CString Serial_Order_BleReturn;
	//此时根据蓝牙配置文件里的连接指令去发送蓝牙连接指令,当指令全部发送完且都成功后，就将BLEGetSettingFlag置为1表示连接成功了，否则置为2
	for (int i = 0, j = 0; i < ExitOrderNumber; i++)
	{
		Send_Serial_Order(&Serial_Order_BleReturn, ExitOrder[i][0], PortNo, "NULL", "", 500);
	}
}

//因为是通过扫描枪扫描，然后分配MAC给串口，所以无论在测试成功还是失败之后都要停止该串口
void IMEIWrite_MulAT::BleStopPortTest(int PortNo)
{
	switch (PortNo)
	{
	case 0:
		OnBnClickedButton2();
		break;
	case 1:
		OnBnClickedButton15();
		break;
	case 2:
		OnBnClickedButton17();
		break;
	case 3:
		OnBnClickedButton19();
		break;
	case 4:
		OnBnClickedButton21();
		break;
	case 5:
		OnBnClickedButton23();
		break;
	case 6:
		OnBnClickedButtonstop7();
		break;
	case 7:
		OnBnClickedButtonstop8();
		break;
	case 8:
		OnBnClickedButtonstop9();
		break;
	case 9:
		OnBnClickedButtonstop10();
		break;
	case 10:
		OnBnClickedButtonstop11();
		break;
	case 11:
		OnBnClickedButtonstop12();
		break;
	case 12:
		OnBnClickedButtonstop13();
		break;
	case 13:
		OnBnClickedButtonstop14();
		break;
	case 14:
		OnBnClickedButtonstop15();
		break;
	case 15:
		OnBnClickedButtonstop16();
		break;
	default:
		break;
	}
}

//整机测试串口提示用的函数
void IMEIWrite_MulAT::BluetoothHint(int PortNo, CString str)
{
	if (str.Find("失败") != -1)
	{
		gColor[PortNo] = RGB(255, 0, 0);
	}
	else
	{
		gColor[PortNo] = RGB(122, 197, 205);
	}
	switch (PortNo)
	{
	case 0:
		Final_Result_Control1.SetWindowTextA(str);
		break;
	case 1:
		Final_Result_Control2.SetWindowTextA(str);
		break;
	case 2:
		Final_Result_Control3.SetWindowTextA(str);
		break;
	case 3:
		Final_Result_Control4.SetWindowTextA(str);
		break;
	case 4:
		Final_Result_Control5.SetWindowTextA(str);
		break;
	case 5:
		Final_Result_Control6.SetWindowTextA(str);
		break;
	case 6:
		Final_Result_Control7.SetWindowTextA(str);
		break;
	case 7:
		Final_Result_Control8.SetWindowTextA(str);
		break;
	case 8:
		Final_Result_Control9.SetWindowTextA(str);
		break;
	case 9:
		Final_Result_Control10.SetWindowTextA(str);
		break;
	case 10:
		Final_Result_Control11.SetWindowTextA(str);
		break;
	case 11:
		Final_Result_Control12.SetWindowTextA(str);
		break;
	case 12:
		Final_Result_Control13.SetWindowTextA(str);
		break;
	case 13:
		Final_Result_Control14.SetWindowTextA(str);
		break;
	case 14:
		Final_Result_Control15.SetWindowTextA(str);
		break;
	case 15:
		Final_Result_Control16.SetWindowTextA(str);
		break;
	default:
		break;
	}
	UpdateWindow();
}


/*摄像头新增功能，包含Socket服务器功能和图像处理功能*/




