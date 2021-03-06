#include "STC15F2K60S2.H" //头文件
#include "intrins.H"	  //头文件
#include "ctype.h"
//宏定义
#define uchar unsigned char
#define uint unsigned int
#define ulint unsigned long int

//DS1302寄存器的定义
#define DS1302_second_write 0X80
#define DS1302_minutes_write 0X82
#define DS1302_hour_write 0X84
#define DS1302_date_write 0X86
#define DS1302_week_write 0X8A
#define DS1302_month_write 0X88
#define DS1302_year_write 0X8C

#define DS1302_second_read 0X81
#define DS1302_minutes_read 0X83
#define DS1302_hour_read 0X85
#define DS1302_date_read 0X87
#define DS1302_week_read 0X8B
#define DS1302_month_read 0X89
#define DS1302_year_read 0X8D

//位定义
sbit SEL0 = P2 ^ 0;
sbit SEL1 = P2 ^ 1;
sbit SEL2 = P2 ^ 2;
sbit RTC_sclk = P1 ^ 5; //时钟控制引脚，控制数据的输入输出
sbit RTC_rst = P1 ^ 6;	//CE引脚，读写时必须置高电平
sbit RTC_io = P5 ^ 4;	//数据引脚
sbit sbtKey1 = P3 ^ 2;	// 按键K1
sbit sbtKey2 = P3 ^ 3;	// 按键K2
sbit sbtVib = P2 ^ 4;	//振动传感器
//显示的位定义
sbit led_sel = P2 ^ 3;
uchar wei[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};													 //数码管位选
uchar duan[] = {0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, 0x7f, 0x6f, 0x77, 0x7c, 0x39, 0x5e, 0x79, 0x71}; //显示0-f

uint show_flag = 1; // 是否显示数码管
uint light_dig = 1;
uint sbtKey1_state = 1;										  // K1消抖
uint sbtKey2_state = 1;										  // K2消抖
uint Led_Value = 8;											  // 数码管显示位数 8 ~ 92 4位一档 共9档
uint Light_Level[] = {20, 40, 60, 80, 100, 120, 160, 180, 200, 220, 240, 260, 280, 300}; // 不同光照强度阙值
// 光照测试部分变量
uint l = 0;		//执行光的次数
uint time_ = 0; //延时
uint time__ = 0;
ulint suml = 0; //光AD值得总和
uint light = 0; //光
uint light_choice = 0;
uint light_bai = 0;
uint light_shi = 0;
uint light_ge = 0;
// 双击亮屏
uint tiptap = 0;
uint Vib_flap = 0;
uint ret = 0;
//定义时间结构体
typedef struct _systemtime_
{
	uchar second;
	uchar minute;
	uchar hour;
	uchar day;
	uchar week;
	uchar month;
	uchar year;
} systemtime;

systemtime t;
uchar i;
uchar temp;
int sec = 0;
char flag_100mS = 0;

//DS1302写一个字节的数据
void DS1302WriteByte(uchar dat)
{
	uchar i;
	RTC_sclk = 0; //初始时钟线置0
	_nop_();
	_nop_();
	for (i = 0; i < 8; i++) //开始传输8位数据
	{
		RTC_io = dat & 0x01; //取最低位
		_nop_();
		_nop_();
		RTC_sclk = 1; //时钟线拉高，制造上升沿，数据被传输
		_nop_();
		_nop_();
		RTC_sclk = 0; //时钟线拉低，为下一个上升沿做准备
		dat >>= 1;	  //数据右移一位，准备传输下一位数据
	}
}

////DS1302读一个字节的数据
uchar DS1302ReadByte()
{
	uchar i, dat;
	_nop_();
	_nop_();
	for (i = 0; i < 8; i++)
	{
		dat >>= 1;		 //要返回的数据右移一位
		if (RTC_io == 1) //当数据线为高时，证明该位数据为1
			dat |= 0x80;
		RTC_sclk = 1;
		_nop_();
		_nop_();
		RTC_sclk = 0; //制造下降沿
		_nop_();
		_nop_();
	}
	return dat; //返回读取出的数据
}

//读相应地址中写一个字节的数据
uchar DS1302Read(uchar cmd)
{
	uchar dat;
	RTC_rst = 0;			//初始CE置0
	RTC_sclk = 0;			//初始时钟置0
	RTC_rst = 1;			//初始CE置1，传输开始
	DS1302WriteByte(cmd);	//传输命令字，
	dat = DS1302ReadByte(); //读取得到的时间
	RTC_sclk = 1;			//时钟线拉高
	RTC_rst = 0;			//读取结束，CE置0，结束数据传输
	return dat;				//返回得到的时间日期
}

//在相应地址中写数据
void DS1302Write(uchar cmd, uchar dat)
{
	RTC_rst = 0;		  //初始CE置0
	RTC_sclk = 0;		  //初始时钟置0
	RTC_rst = 1;		  //置1，传输开始
	DS1302WriteByte(cmd); //传输命令字，要写入的时间的地址
	DS1302WriteByte(dat); //写入修改的时间
	RTC_sclk = 1;		  //时钟线拉高
	RTC_rst = 0;		  //读取结束，CE=0，结束数据的传输
}
//DS1302的时间值获取程序
systemtime GetDA1302()
{
	systemtime time;
	uchar realvalue;
	realvalue = DS1302Read(DS1302_second_read);
	time.second = ((realvalue & 0x70) >> 4) * 10 + (realvalue & 0x0f);
	realvalue = DS1302Read(DS1302_minutes_read);
	time.minute = ((realvalue & 0x70) >> 4) * 10 + (realvalue & 0x0f);
	realvalue = DS1302Read(DS1302_hour_read);
	time.hour = ((realvalue & 0x70) >> 4) * 10 + (realvalue & 0x0f);
	realvalue = DS1302Read(DS1302_date_read);
	time.day = ((realvalue & 0x70) >> 4) * 10 + (realvalue & 0x0f);
	realvalue = DS1302Read(DS1302_week_read);
	time.week = ((realvalue & 0x70) >> 4) * 10 + (realvalue & 0x0f);
	realvalue = DS1302Read(DS1302_month_read);
	time.month = ((realvalue & 0x70) >> 4) * 10 + (realvalue & 0x0f);
	realvalue = DS1302Read(DS1302_year_read);
	time.year = ((realvalue & 0x70) >> 4) * 10 + (realvalue & 0x0f);
	return time;
}
//DS1302初始化程序
void Init_DS1302()
{
	unsigned char hour, min, sec;
	unsigned char code DataStr[] = __TIME__; //格式："09:12:04"	  9字符（含结束符）

	hour = ((toint(DataStr[0])) << 4) + toint(DataStr[1]);
	min = ((toint(DataStr[3])) << 4) + toint(DataStr[4]);
	sec = ((toint(DataStr[6])) << 4) + toint(DataStr[7]);

	DS1302Write(0X8E, 0X00); //写保护关
	DS1302Write(DS1302_second_write, sec);
	DS1302Write(DS1302_minutes_write, min);
	DS1302Write(DS1302_hour_write, hour);
	temp = DS1302Read(DS1302_second_read) & 0x7f;
	DS1302Write(DS1302_second_write, temp);
	DS1302Write(0X8E, 0X80); //写保护置1
}

void set_charge_DS1302()
{
	DS1302Write(0X8E, 0X00); //写保护关
	DS1302Write(0x90, 0xa9); //  充电设置：允许充电，2个二极管，2K电
	DS1302Write(0X8E, 0X80); //写保护置1
}
void Delay100us()		//@11.0592MHz
{
	unsigned char i, j;

	_nop_();
	_nop_();
	i = 2;
	j = 15;
	do
	{
		while (--j);
	} while (--i);
}
void Delay10ms() //@11.0592MHz
{
	unsigned char i, j;

	i = 108;
	j = 145;
	do
	{
		while (--j)
			;
	} while (--i);
}

//系统初始化程序
void init()
{
	P3 = 0xEF; //关蜂鸣器
	P2M0 = 0XFF;
	P2M1 = 0X00;
	P0M0 = 0XFF;
	P0M1 = 0X00;

	led_sel = 0; //选通数码管
	TMOD = 0X11; //定时器0，工作方式1
	EA = 1;		 //打开总中断
	EADC = 1;
	TH0 = (65535 - 40000) / 256; //设置定时初值
	TL0 = (65535 - 40000) % 256;
	TR0 = 1; //启动定时器

	TH1 = 0xD4;
	TL1 = 0xCD;
	TR1 = 1;
	ET0 = 1; //开启定时器中断
	ET1 = 1;
}
void InitADC_light() //初始化光ADC
{
	ADC_RES = 0;
	ADC_RESL = 0;
	ADC_CONTR = 0x8c; //CHS=100选择P1^4作为A/D输入使用
	CLK_DIV = 0x20;
}
void date_processlight()
{
	light_bai = light % 1000 / 100;
	light_shi = light % 100 / 10;
	light_ge = light % 10;
}
void time0() interrupt 1
{
	TH0 = (65535 - 40000) / 256; //设置定时初值
	TL0 = (65535 - 40000) % 256;
	EA = 0;
	InitADC_light(); //初始化光
	EA = 1;
}
void Light_Control() {
	int targ;
	if (light > 241) light = 241;
	targ = light / 10;
	targ /= 3;
	targ *= 8;
	if (Led_Value == 72 - targ) Led_Value = Led_Value;
	else if (Led_Value > 72 - targ) Led_Value -= 8;
	else Led_Value += 8;
}
void time1() interrupt 3
{
	TH1 = 0xD4;
	TL1 = 0xCD;
	EA = 0;
	time__ += 1;
	if (time__ > 30) {
		time__ = 0;
		Light_Control();
	}
	EA = 1;
}
// AD中断
void adc_isr() interrupt 5 using 1
{
	time_++;
	EA = 0; //关闭中断

	if (time_ > 2000) //取多次值求平均值减小误差
	{
		light = (suml + l / 2) / l; //四舍五入
		suml = 0;
		l = 0;
		time_ = 0;
		date_processlight();
	}
	//处理光部分的数据
	l++;
	suml += ADC_RES * 256 + ADC_RESL; //求l次AD值的和

	ADC_CONTR &= ~0X10; //转换完成后，ADC_FLAG清零
	ADC_CONTR |= 0X08;	//转换完成后，ADC_START赋1
	EA = 1;				//打开中断
}
void show_shumaguan()
{
	i++;
	if (++sec == 100)
	{
		sec = 0;
		flag_100mS = 1;
	}
	if (i == Led_Value)
		i = 0;
	led_sel = 0;
	P0 = 0X00;
	if (show_flag == 1 && i < 8 && light_dig == 1)
	{
		P2 = wei[i];
		switch (i)
		{
		case 0:
			P0 = duan[t.hour / 10];
			break;
		case 1:
			P0 = duan[t.hour % 10];
			break;
		case 3:
			P0 = duan[t.minute / 10];
			break;
		case 4:
			P0 = duan[t.minute % 10];
			break;
		case 6:
			P0 = duan[t.second / 10];
			break;
		case 7:
			P0 = duan[t.second % 10];
			break;
		default:
			P0 = 0x40;
			break;
		}
	}
	else if (show_flag == 0 && tiptap == 1 && light_dig == 1 && i < 8)
	{
		ret += 1;
		if (ret == 2000)
		{
			ret = 0;
			tiptap = 0;
		}
		P2 = wei[i];
		switch (i)
		{
		case 0:
			P0 = duan[t.hour / 10];
			break;
		case 1:
			P0 = duan[t.hour % 10];
			break;
		case 3:
			P0 = duan[t.minute / 10];
			break;
		case 4:
			P0 = duan[t.minute % 10];
			break;
		case 6:
			P0 = duan[t.second / 10];
			break;
		case 7:
			P0 = duan[t.second % 10];
			break;
		default:
			P0 = 0x40;
			break;
		}
	}
	else if (light_dig == 0 && i < 3)
	{
		show_flag = 1;
		P2 = wei[i];
		switch (i)
		{
		case 0:
			P0 = duan[light_bai];
			break;
		case 1:
			P0 = duan[light_shi];
			break;
		case 2:
			P0 = duan[light_ge];
			break;
		}
	}
	Delay100us();
}

void main()
{
	init();
	if (DS1302Read(DS1302_second_read) & 0X80)
		Init_DS1302();
	set_charge_DS1302();
	while(light == 0);
	if (light > 241) light = 241;
	Led_Value = 72 - light / 10 / 3 * 8;
	while (1)
	{
		show_shumaguan();

		if (flag_100mS == 1)
		{
			t = GetDA1302();
			flag_100mS = 0;
		}

		if (sbtKey1 == 0)
		{
			if (sbtKey1_state == 0)
			{
				Delay10ms();
				if (sbtKey1 == 0)
				{
					show_flag = !show_flag;
					sbtKey1_state = 1;
				}
			}
		}
		else
			sbtKey1_state = 0;

		if (sbtKey2 == 0)
		{
			if (sbtKey2_state == 0)
			{
				Delay10ms();
				if (sbtKey2 == 0)
				{
					light_dig = !light_dig;
					sbtKey2_state = 1;
				}
			}
		}
		else
			sbtKey2_state = 0;

		if (show_flag == 0)
		{
			sbtVib = 1;
			if (sbtVib == 0)
			{
				tiptap = 1;
			}
		}
	}
}