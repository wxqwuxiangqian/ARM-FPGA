/*
 * "Hello World" example.
 *
 * This example prints 'Hello from Nios II' to the STDOUT stream. It runs on
 * the Nios II 'standard', 'full_featured', 'fast', and 'low_cost' example
 * designs. It runs with or without the MicroC/OS-II RTOS and requires a STDOUT
 * device in your system's hardware.
 * The memory footprint of this hosted application is ~69 kbytes by default
 * using the standard reference design.
 *
 * For a reduced footprint version of this template, and an explanation of how
 * to reduce the memory footprint for a given application, see the
 * "small_hello_world" template.
 *
 */

#include <stdio.h>
#include "LCD1602_Qsys.h"
#include "system.h"
#include <string.h>
#include "sys/alt_irq.h"
#include <time.h>
unsigned int *pUser_IR=IR1498_0_BASE;
unsigned int IR_Data;
unsigned int *pUser_SEG7 = SEG7_DEMO_0_BASE;
unsigned char Data_Ready;

/*----------------------音乐播放器-----------------------------*/
//LCD显示
//界面
unsigned char LCD_Welcome[16] = {"YY Music"};
//播放状态
unsigned char LCD_Playing[] = {"YMusic Playing"};
unsigned char LCD_Pause[] = {"YMusic Pause"};
//Song
unsigned char Song_0[] = {"  Baby "};
unsigned char Song_1[] = {"  Closer "};
unsigned char Song_2[] = {"  Faded "};
unsigned char Song_3[] = {"  After the Afterparty   "};
unsigned char Song_4[] = {"  Keeping Your Head Up   "};
unsigned char Song_5[] = {"  Kill 'Em With Kindness   "};
unsigned char Song_6[] = {"  Stitches "};
unsigned char Song_7[] = {"  Sensual "};
unsigned char Song_8[] = {"  Still Falling For You   "};
unsigned char Song_9[] = {"  Cake By The Ocean   "};
unsigned char Song_10[] = {"  Blank Space   "};
unsigned char Song_11[] = {"  Don't Wanna Know   "};
unsigned char Song_12[] = {"  Glow   "};

unsigned char space[500] = "                                                         ";
unsigned char str[500] = "                ";

int str_len = 16;

//模式选择
int mode = 0; //0为音乐播放器，1为简单计算器

//状态变量
int Power = 0;
int Adjust = 0; //0表示当前曲目，1表示下一首，-1表示上一首
int mute = 0; //1为静音
int Play = 0; //1为播放
//音量
int volume = 40;
int volume_mute;
/*----------------------简单计算器------------------------*/
//两位数和两位数的加减乘
int add1[2] = {0};
int add2[2] = {0};
int answer[4] = {0};
//状态
int state_input = 0; //输入状态，0为输入第一个数，1为输入第二个数，2为符号
int state_add1 = 0; //输入第一个加数的状态
int state_add2 = 0; //输入第二个加数的状态
int state_sign = -1 ; //0为加号，1为减号，2为乘号
//字符串移位函数
void RightLoopMove(unsigned char *pStr, int steps)
{
    unsigned char arr[100] = { 0 };
    int n = strlen(pStr) - steps;
    strcpy(arr, pStr + n);
    strcpy(arr + steps, pStr);
    *(arr + strlen(pStr)) = '\0';
    strcpy(pStr, arr);
}

void LeftLoopMove(unsigned char *pStr , int steps)
{
	unsigned char arr[100] = {0};
	pStr[str_len] = '\0';
	strcpy(arr,pStr+steps);
	strcpy(arr + str_len - steps , pStr);
	arr[str_len] = '\0 ';
	strcpy(pStr , arr);
}
void IR_Irq_Process()
{
	Data_Ready=1;
	IR_Data=*(pUser_IR);
}

void sleep( clock_t wait )
{
    clock_t goal;
    goal = wait + clock();
    while( goal > clock() ) ;
}

int IR_Irq_Init()
{
    int States;
    States=alt_ic_isr_register(
    		IR1498_0_IRQ_INTERRUPT_CONTROLLER_ID,   // 中断控制器标号，从system.h复制
    		IR1498_0_IRQ,                           // 硬件中断号，从system.h复制
    		IR_Irq_Process,                          // 中断服务子函数
			NULL,                                    // 指向与设备驱动实例相关的数据结构体
			0
						);
    return States;
}
void delay()
{
    int i= 10000;
    while(i--);
}
void Display(unsigned char *Song)
{
	if(Power == 1)
	{
	strcpy(str,space);
	strcpy(str,Song);
	str_len = strlen(Song);
	LCD_Disp(2,0,"                ",16);
	delay();
	LCD_Disp(2,0,Song,strlen(Song));
	}
}

//音量十进制转字符串
void trans_numtochar(int volume,unsigned char *str)
{
	if(volume == 100)
	{
		str[0] = '1';
		str[1] = '0';
		str[2] = '0';
		str[3] = '\0';
	}
	if(volume > 9)
	{
		str[0] = volume/10 + 48;
		str[1] = volume%10 + 48;
		str[2] = '\0';
	}
	if(volume <=9)
	{
		str[0] = volume + 48;
		str[1] = '\0';
	}
}
int main()
{
  int States;
  unsigned char Key_Code;
  unsigned char Last_Key_Code = Key_Code;
  int Key_ID;
  int Current_Song_ID;
  States=IR_Irq_Init();


  //Song定义
  unsigned char **Song[50];
  Song[0] = Song_0;
  Song[1] = Song_1;
  Song[2] = Song_2;
  Song[3] = Song_3;
  Song[4] = Song_4;
  Song[5] = Song_5;
  Song[6] = Song_6;
  Song[7] = Song_7;
  Song[8] = Song_8;
  Song[9] = Song_9;
  Song[10] = Song_10;
  Song[11] = Song_11;
  Song[12] = Song_12;

  //播放状态定义
  unsigned char **Playing_State[2];
  Playing_State[0] = LCD_Playing;
  Playing_State[1] = LCD_Pause;
  int Song_ID = 1; //标注当前播放曲目的标号
  Current_Song_ID = Song_ID;
  int cnt = 0;
  int Song_cnt = 0;
  //七段数码管
  *pUser_SEG7 = 0xffffffff;
  //音乐播放器相关
  unsigned long song_order = 0x01000000,seg_volume = 0x00400000,song_progress = 0x00000000;
  //简单计算器相关
  unsigned long add_seg1 = 0x00000000, add_seg2 = 0x00000000, answer_seg = 0x00000000;
  //歌曲进程
  //加法器get到的数字
  int Num_get = 0;
  int Num_cnt = 0;
  while(1)
  {
  //模式0，音乐播放器
	  if(mode == 0)
  {
	  delay();
	if(Power == 1)
	{
		if(mute == 1)
		{
		   *pUser_SEG7 = song_order|0x00000000|song_progress;
		}
		else
		{
		   *pUser_SEG7 = song_order|seg_volume|song_progress;
		}
    Song_cnt = (Song_cnt + 1)%150;
    if(!Song_cnt && Play == -1)
    {
    	if(song_progress != 0x00000350)
    	{
    		song_progress += 0x00000001;
    		song_progress +=(((song_progress& 0x0000000f)== 0x0000000a)?(0x00000006):(0x00000000));
    		song_progress +=(((song_progress& 0x000000f0)==0x00000060)?(0x000000a0):(0x00000000));
    		song_progress +=(((song_progress& 0x00000f00)==0x0000a000)?(0x00000006):(0x00000000));
    	}
    	else
    	{
			  Song_ID ++;
			  song_progress = 0x00000000;
			  song_order += 0x01000000;
			  song_order +=(((song_order&0x0f000000)==0x0a000000)?(0x06000000):(0x00000000));
			  Display(Song[Song_ID]);
    	}
    }
	  if(!cnt)
	{
	  if(str_len > 16)
	  {
		  LeftLoopMove(str,1);
		  LCD_Disp(2,0,"                ",16);
		  delay();
		  LCD_Disp(2,0,str,str_len);
		  delay();
		 /* LCD_Disp(1,0,"                ",16);
		  delay();
		  LCD_Disp(1,0,LCD_Welcome,16);*/
	  }
	  else
	  {
		  LCD_Disp(2,0,"                ",16);
		  delay();
		  LCD_Disp(2,0,str,str_len);
		  delay();
		 /* LCD_Disp(1,0,"                ",16);
		  delay();
		  LCD_Disp(1,0,LCD_Welcome,16);*/
	  }
	}
	  cnt = (cnt + 1)%35;
	}

	//红外读取
	  if(Data_Ready == 1)
	  {
		  Data_Ready = 0;
		  Key_Code = (IR_Data >> 16) & 0xff;
		  switch(Key_Code)
		  {
		  case 0x01:
		  {
			  song_order = 0x01000000;
			  Song_ID = 1;
			  Key_ID = 1;
			  if(Current_Song_ID == Song_ID)
			  {
				  break;
			  }
			  Display(Song[1]);
			  song_progress = 0x00000000;
			  break;
		  }
		  case 0x02:
		  {
			  song_order = 0x02000000;
			  Song_ID = 2;
			  Key_ID = 2;
			  if(Current_Song_ID == Song_ID)
			  {
				  break;
			  }
			  Display(Song[2]);
			  song_progress = 0x00000000;
			  break;
		  }
		  case 0x03:
		  {
			  song_order = 0x03000000;
			  Song_ID = 3;
			  Key_ID = 3;
			  if(Current_Song_ID == Song_ID)
			  {
				  break;
			  }
			  Display(Song[3]);
			  song_progress = 0x00000000;
			  break;
		  }
		  case 0x04:
		  {
			  song_order = 0x04000000;
			  Song_ID = 4;
			  Key_ID = 4;
			  if(Current_Song_ID == Song_ID)
			  {
				  break;
			  }
			  Display(Song[4]);
			  song_progress = 0x00000000;
			  break;
		  }
		  case 0x05:
		  {
			  song_order = 0x05000000;
			  Song_ID = 5;
			  Key_ID = 5;
			  if(Current_Song_ID == Song_ID)
			  {
				  break;
			  }
			  Display(Song[5]);
			  song_progress = 0x00000000;
			  break;
		  }
		  case 0x06:
		  {
			  song_order = 0x06000000;
			  Song_ID = 6;
			  Key_ID = 6;
			  if(Current_Song_ID == Song_ID)
			  {
				  break;
			  }
			  Display(Song[6]);
			  song_progress = 0x00000000;
			  break;
		  }
		  case 0x07:
		  {
			  song_order = 0x07000000;
			  Song_ID = 7;
			  Key_ID = 7;
			  if(Current_Song_ID == Song_ID)
			  {
				  break;
			  }
			  Display(Song[7]);
			  song_progress = 0x00000000;
			  break;
		  }
		  case 0x08:
		  {
			  song_order = 0x08000000;
			  Song_ID = 8;
			  Key_ID = 8;
			  if(Current_Song_ID == Song_ID)
			  {
				  break;
			  }
			  Display(Song[8]);
			  song_progress = 0x00000000;
			  break;
		  }
		  case 0x09:
		  {
			  song_order = 0x09000000;
			  Song_ID = 9;
			  Key_ID = 9;
			  if(Current_Song_ID == Song_ID)
			  {
				  break;
			  }
			  Display(Song[9]);
			  song_progress = 0x00000000;
			  break;
		  }
		  case 0x00:
		  {
			  song_order = 0x00000000;
			  Song_ID = 0;
			  Key_ID = 0;
			  if(Current_Song_ID == Song_ID)
			  {
				  break;
			  }
			  Display(Song[0]);
			  song_progress = 0x00000000;
			  break;
		  }
		  case 0x12:
		  {
			  Key_ID = 12;
			  if(Power == 1)
			  {
				  LCD_Disp(1,0,"                ",16);
				  delay();
				  LCD_Disp(2,0,"                ",16);
				  Play = 0;
				  *pUser_SEG7 = 0xffffffff;
			  }
			  else
			  {
				  LCD_Disp(1,0,Playing_State[Play+1],strlen(Playing_State[Play+1]));
				  delay();
			  }
			  Power = !Power;

			  break;
		  }
		  case 0x1E:
		  {
			  Key_ID = 14;

			  if(Song_ID > 0)
			  {
				  Song_ID--;
				  Display(Song[Song_ID]);
				  song_progress = 0x00000000;
				  song_order -= 0x01000000;
				  song_order -=(((song_order&0x0f000000)==0x0f000000)?(0x06000000):(0x00000000));
			  }
			  break;
		  }
		  case 0x1A:
		  {
			  Key_ID = 18;
			  if(Song_ID < 12)
			  {
				  Song_ID++;
				  Display(Song[Song_ID]);
				  song_progress = 0x00000000;
				  song_order += 0x01000000;
				  song_order +=(((song_order&0x0f000000)==0x0a000000)?(0x06000000):(0x00000000));
			  }
			  break;
		  }
		  case 0x1B:
		  {
			  if(volume < 100)
			  {
				 mute = 0;
				 volume++;
				 seg_volume += 0x00010000;
				 seg_volume+=(((seg_volume&0x000f0000)==0x000a0000)?(0x00060000):(0x00000000));

			  }
			/*  unsigned char str_tmp[4] = {""};
			  trans_numtochar(volume,str_tmp);
			  LCD_Disp(1,14,str_tmp,strlen(str_tmp));
			  delay();*/
			  break;
		  }
		  case 0x1F:
		  {
			  if(volume > 0)
			  {
				  mute = 0;
				  volume--;
				  seg_volume -= 0x00010000;
				  seg_volume -=(((seg_volume&0x000f0000) == 0x000f0000)?(0x00060000):(0x00000000));

			  }

			  break;
		  }
		  case 0x0C:
		  {
			  if(mute == 0)
			  {
				  volume_mute = volume;
			  }
			  else
			  {
				  volume = volume_mute;
			  }
			  mute = !mute;
			  break;
		  }
		  case 0x16:
		  {
			  if(Play == 0)
			  {
				  Display(Song[Song_ID]);
			  }
			  Play = ~Play;

			  break;
		  }
		  case 0x14:
		  {
			  int tmp = 0;
			  for(;tmp < 14; tmp++)
			  if(song_progress != 0x00000000)
			  {
				 song_progress -= 0x00000001;
				 song_progress -=(((song_progress& 0x0000000f)== 0x0000000f)?(0x00000006):(0x00000000));
				 song_progress -=(((song_progress& 0x000000f0)==0x000000f0)?(0x000000a0):(0x00000000));
				 song_progress -=(((song_progress& 0x00000f00)==0x00000f00)?(0x00000006):(0x00000000));
			  }
			  break;
		  }
		  case 0x18:
		  {
			  int tmp = 0;
			  for(; tmp < 16 ; tmp++)
			  {
				  if(song_progress != 0x00000350)
				  {
				    song_progress += 0x00000001;
				    song_progress +=(((song_progress& 0x0000000f)== 0x0000000a)?(0x00000006):(0x00000000));
				    song_progress +=(((song_progress& 0x000000f0)==0x00000060)?(0x000000a0):(0x00000000));
				    song_progress +=(((song_progress& 0x00000f00)==0x0000a000)?(0x00000006):(0x00000000));
				  }
				  else
				  {
					  Song_ID ++;
					  song_progress = 0x00000000;
					  song_order += 0x01000000;
					  song_order +=(((song_order&0x0f000000)==0x0a000000)?(0x06000000):(0x00000000));
					  Display(Song[Song_ID]);
				  }
			  }
			  break;
		  }
		  case 0x0F:
		  {
			  mode = 0;
			  break;
		  }
		  case 0x13:
		  {
			  mode = 1;
			  LCD_Disp(1,0,"                ",16);
			  delay();
			  LCD_Disp(2,0,"                ",16);
			  break;
		  }
		  default:
		  {
			  LCD_Disp(2,0,"                ",16);
			  LCD_Disp(2,0,"LUV ZN",6);
			  break;
		  }
		  }

	}
	  Current_Song_ID = Song_ID;
	  if(Power == 1)
	  {
	  if(!cnt)
	  {
		 LCD_Disp(1,0,"                ",16);
		 delay();
		 LCD_Disp(1,0,Playing_State[Play+1],strlen(Playing_State[Play+1]));
		 delay();

	  }
	  delay();
	  }

  }
  else if(mode == 1)
  {
	  if(Power == 1)
	 {
		  *pUser_SEG7 = add_seg1|add_seg2|answer_seg;
	 }

	  if(Data_Ready == 1)
	 	  {
	 		  Data_Ready = 0;
	 		  Key_Code = (IR_Data >> 16) & 0xff;
	 		  switch(Key_Code)
	 		  {
	 		  case 0x01:
	 		  {
	 			 Num_get = 1;
	 			 Num_cnt ++;
	 			  break;
	 		  }
	 		  case 0x02:
	 		  {
	 			  Num_get = 2;
	 			 Num_cnt ++;
	 			  break;
	 		  }
	 		  case 0x03:
	 		  {
	 			  Num_get = 3;
	 			 Num_cnt ++;
	 			  break;
	 		  }
	 		  case 0x04:
	 		  {
	 			  Num_get = 4;
	 			 Num_cnt ++;
	 			  break;
	 		  }
	 		  case 0x05:
	 		  {
	 			  Num_get = 5;
	 			 Num_cnt ++;
	 			  break;
	 		  }
	 		  case 0x06:
	 		  {
	 			  Num_get = 6;
	 			 Num_cnt ++;
	 			  break;
	 		  }
	 		  case 0x07:
	 		  {
	 			  Num_get = 7;
	 			 Num_cnt ++;
	 			  break;
	 		  }
	 		  case 0x08:
	 		  {
	 			  Num_get = 8;
	 			 Num_cnt ++;
	 			  break;
	 		  }
	 		  case 0x09:
	 		  {
	 			  Num_get = 9;
	 			 Num_cnt ++;
	 			  break;
	 		  }
	 		  case 0x00:
	 		  {
	 			  Num_get = 0;
	 			 Num_cnt ++;
	 			  break;
	 		  }
	 		  case 0x12:
	 		  {
	 			  if(Power == 1)
	 			  {
	 				Power = 0;
	 				*pUser_SEG7 = 0xffffffff;
	 			  }
	 			  else if(Power == 0)
	 			  {
	 				  Power = 1;
	 				 *pUser_SEG7 = add_seg1|add_seg2|answer_seg;
	 			  }
	 			  break;
	 		  }
	 		  case 0x16:			//加
	 		  {
	 			  state_sign = 0;
	 			  Num_cnt ++;
	 			  break;
	 		  }
	 		  case 0x14:			//减
	 		  {
	 			  state_sign = 1;
	 			  Num_cnt ++;
	 			  break;
	 		  }
	 		  case 0x18:			//乘
	 		  {
	 			  state_sign = 2;
	 			  Num_cnt ++;
	 			  break;
	 		  }
	 		  case 0x17:			//重新输入
	 		  {
	 			  add_seg1 = 0x00000000;
	 			  add_seg2 = 0x00000000;
	 			  answer_seg = 0x00000000;
	 			  Num_cnt = 0;
	 			  state_sign = -1;
					 add1[0] = 0;
					 add1[1] = 0;
					 add2[0] = 0;
					 add2[1] = 0;
	 			  break;
	 		  }
	 		  case 0x0F:
	 		  {
	 			  mode = 0;
	 			  break;
	 		  }
	 		  default:
	 		  {
	 			  break;
	 		  }
	 		  }
	 	  }
	  //读取加数

	  	  add1[0] += (Num_cnt == 1 ? Num_get : 0);
		  add1[1] += (Num_cnt == 2 ? Num_get : 0);
		  int i;
			 /*-------------------第一个加数---------------------*/
			 for(i = 0 ; i < add1[0] ; i++)
			 {
				 add_seg1 += (( (Num_cnt == 1) && (Num_get != 0))? 0x10000000 : 0x00000000);
			 }
			 for(i = 0 ; i < add1[1] ; i++)
			 {
				 add_seg1 += (( (Num_cnt == 2) && (Num_get != 0)) ? 0x01000000 : 0x00000000);
			 }
		  add2[0] += (Num_cnt  == 4 ? Num_get : 0);
		  add2[1] += (Num_cnt  == 5 ? Num_get : 0 );
			 /*--------------------第二个加数--------------------------*/
			 for(i = 0 ; i < add2[0] ; i++)
			 {
				 add_seg2 += (((Num_cnt == 4) && (Num_get != 0)) ? 0x00100000 : 0x00000000);
			 }
			 for(i = 0 ; i < add2[1] ; i++)
			 {
				 add_seg2 += (((Num_cnt == 5 ) && (Num_get != 0 ))? 0x00010000 : 0x00000000);
			 }
		  if(Num_cnt == 5)
		  {
			  int add11 = 10*add1[0] + add1[1];
			  int add22 = 10*add2[0] + add2[1];
			  int answer_tmp = 0;
			  if(state_sign == 0)
			  {
				 answer_tmp = add11 + add22;
			  }
			  if(state_sign == 1)
			  {
				  answer_tmp = abs(add11 - add22);
			  }
			  if(state_sign == 2)
			  {
				  answer_tmp = add11*add22;
			  }
				 answer[0] = answer_tmp/1000;
				 answer[1] = (answer_tmp % 1000) / 100;
				 answer[2] = (answer_tmp % 100) / 10;
				 answer[3] = answer_tmp % 10;
				 int i;
				 /*-----------------结果--------------------*/
				 for(i = 0 ; i < answer[3] ; i++)
				 {
					 answer_seg += 0x00000001;
					 answer_seg +=(((answer_seg&0x0000000f)==0x0000000a)?(0x00000006):(0x00000000));
				 }
				 for(i = 0 ; i < answer[2] ; i++)
				 {
					 answer_seg += 0x00000010;
					 answer_seg +=(((answer_seg&0x000000f0)==0x000000a0)?(0x00000060):(0x00000000));
				 }
				 for(i = 0; i < answer[1] ; i++)
				 {
					 answer_seg += 0x00000100;
					 answer_seg +=(((answer_seg&0x00000f00)==0x00000a00)?(0x00000600):(0x00000000));
				 }
				 for(i = 0; i < answer[0] ; i++)
				 {
					 answer_seg += 0x00001000;
					 answer_seg +=(((answer_seg&0x0000f000)==0x0000a000)?(0x00006000):(0x00000000));
				 }

				 //初始化
				 add1[0] = 0;
				 add1[1] = 0;
				 add2[0] = 0;
				 add2[1] = 0;
		  }
		  	  Num_get = 0;
  }
  }
  return 0;
}
