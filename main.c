/*
 *     COPYRIGHT NOTICE
 *     Copyright (c) 2013,山外科技
 *     All rights reserved.
 *     技术讨论：山外初学论坛 http://www.vcan123.com
 *
 *     除注明出处外，以下所有内容版权均属山外科技所有，未经允许，不得用于商业用途，
 *     修改内容时必须保留山外科技的版权声明�?
 *
 * @file       main.c
 * @brief      山外K60 平台主程�?
 * @author     山外科技
 * @version    v5.0
 * @date       2013-08-28
 */


#include "common.h"
#include "include.h"
#include "MK60_it.h"


#define SD5_FTM   FTM1
#define SD5_CH    FTM_CH0
#define SD5_HZ    300


/*********图像定义*************/
#define image_Center    80	//图像赛道中心位置120行的中点
#define image_Left    3		//图像最左边，在find_left�?
#define image_Right  CAMERA_W-3		//图像最右边	160
#define white 0xff		
#define black 0x00
#define max_Distance 10			//最大前�?
#define start_line    CAMERA_H		//开始行

 #define steer_Center    5100//3910	  //舵机中�?
#define steer_right_max    4100//  4690	   //舵机左极�?
#define steer_left_max   6000//   3240       //舵机右极�?


#define Motor_max 9000            //电机最大速度
#define Motor_min 1000            //电机最小速度



/*******************舵机参数******************/
float S_P=0; 	//P
float S_D=0;	//D
uint8 Fixation1=100; 	//舵机偏差控制�?      90
uint8 Fixation2=70;	    //舵机偏差控制�?      60 
int steer_PWM=0;	//舵机pwm

int err=0;
int err_old=0;

/***电机相关***/
uint16 set_speed;
extern int get_speed;
int e0_m;
int e1_m;
int e2_m;
float32_t kp_m=5;
float32_t ki_m=0.5;
float32_t kd_m=1;

int motor_pwm_var;
int motor_pwm;

/*********************图像*********************/

uint8 imgbuff[CAMERA_SIZE]={0};     //摄像头采集来的八位一组的数组
uint8 img[CAMERA_W*CAMERA_H]={0};			//解压后得到的
uint8 imageBuf[CAMERA_H+1][CAMERA_W]={0};	//图像展开后得到的二位数组
Site_t lcdimg={0,0};
uint8 image_L[start_line+1]={0};	//赛道左端
uint8 image_R[start_line+1]={0};	//赛道右端
uint8 image_C[start_line+1]={0};	//赛道中�?
uint8 left_lose_flag[start_line+1]={0};		//左边丢线标志
uint8 right_lose_flag[start_line+1]={0};	//右边丢线标志
uint8 Dis_H=0;		//前瞻
uint8 stop_F_line=0;	
int16 speed_slope=0;
uint8 imgbuff1[CAMERA_SIZE/4]={0};
uint8 shizi_flag=0;
uint8 shizi_ing=0;


uint8 BLACKLINE[120];


uint8 yuanhuan=0;				//圆环识别标志位
uint8 yuanhuaning=0;			//进圆环打角标志位

uint8 firstline = 0;			//史迎辉十字相关
uint8 secondline = 0;			//史迎辉十字相关
/*
 *前四个函数不用看
 */

void duojizhaozhongwei(void)
{
  
  for(uint16 i=3000;i>0;i=i+30)
  {
    ftm_pwm_duty(SD5_FTM, SD5_CH,i);
    DELAY_MS(100);
  }
  
}


void ring ()
{
	uint8 m;
	for(uint8 i=CAMERA_H;i>0;i--)
	{
		if((left_lose_flag[i]==1)&&(left_lose_flag[i-1]==1)&&(left_lose_flag[i-2]==1)&&(right_lose_flag[i]==0)&&(right_lose_flag[i-1]==0)&&(right_lose_flag[i-2]==0))
		{
			m=i;
			for(i=m;i>0;i--)
			{
				if((left_lose_flag[i]==0)&&(image_L[i]>image_L[i-1])&&(left_lose_flag[i-1]==0)&&(image_L[i]>image_L[m]))
				{
  					yuanhuan=1;
					break;
				}
			}
		}
	}
}
	

void threepoint()
{
	uint8 g[3];
	uint8 z;
	uint8 x;
	uint8 v;
	for(uint8 i = CAMERA_H;i > 0;i--)
	{
		if((left_lose_flag[i] == 0) && (left_lose_flag[i-1] == 1) && (left_lose_flag[i-2] == 1))
		{
			z = i;		//z行没有丢线，z行上边两行丢线
			break;
		}
	}
    for(uint8 i = z;i > 0;i--)
	{
		if((left_lose_flag[i] == 0) && (image_L[i] > image_L[i-1]) && (left_lose_flag[i-1] == 0) && (image_L[i] > image_L[z])) 
		{
			x = i;		//x行的左边界在z行以及x行上边一行的左边界的邮编
			break;
		}
	}
	for(uint8 i = x ;i > 0;i--)
	{
		if(image_L[i] > image_L[x])
		{
			v = i;		//v行的左边界在x行的左边界的右边
			break;
		}
	}
	g[0] = image_L[z];
	g[1] = image_L[x];
	g[2] = image_L[v];
}


void closeroad()
{
     uint8 g;
    uint8 h;	 

     for(uint8 i=CAMERA_H;i>0;i--)
     	{
     	if((left_lose_flag[i]==1)&&(right_lose_flag[i]==1)&&(left_lose_flag[i+1]==1)&&(right_lose_flag[i+1]==1)&&(left_lose_flag[i+2]==1)&&(right_lose_flag[i+2]==1))
     		{
                   shizi_flag=1;
		    g=i;
			break;
		}
     	}
     	/*for(uint8 i;i>0;i--)
     		{
		if((left_lose_flag[i])==0&&(right_lose_flag[i]==0)&&(left_lose_flag[i+1])==0&&(right_lose_flag[i+1]==0)&&(left_lose_flag[i+2])==0&&(right_lose_flag[i+2]==0))
			{
	           h=i;
			}
     		}*/
     	}



/*
 *		三种方法测量前瞻
 *		左、上为0
 *		line为上一行的中点
 */


uint8 future_Distance1()               //图像中心列连续出现三个中点即为赛道前�?
{	
	uint8 x=CAMERA_H;					//左、上为0
	uint8 b=0;
	for(uint8 i=120;i>0;i--)
	{
		
		
		BLACKLINE[b]=0;
		b++;
		
		
	}
       b=0;
	for(uint8 i=CAMERA_H;i>0;i--)
	{
		if((imageBuf[i][80]==black&&imageBuf[i-1][80]==white)||(imageBuf[i][80]==black&&imageBuf[i+1][80]==white))		//???
		{
			BLACKLINE[b]=i;
			b++;
		}
		
	}
}


uint8 future_Distance(uint8 line)               //图像中心列连续出现三个中点即为赛道前�?
{												//line为上一行的中点
	uint8 x=CAMERA_H;
	for(uint8 i=CAMERA_H;i>0;i--)
	{
		if(imageBuf[i][line]==BLACK && imageBuf[i-1][line]==BLACK && imageBuf[i-2][line]==BLACK)
		{
			x=i;
			break;
		}
		if(i<=max_Distance)
		{
			x=max_Distance;
			break;
		}
	}
	return x;
}


uint8 future_Distance2(uint8 line)               //图像中心列出现一个中点即为赛道前�?
{	
	uint8 x=CAMERA_H;
	for(uint8 i=CAMERA_H;i>0;i--)
	{
		if(imageBuf[i][line]==BLACK)
		{
			x=i;break;
		}
		if(i<=max_Distance)
		{
			x=max_Distance;break;
		}
	}
	return x;
}



uint8 find_Left(uint8 line,uint8 center)
{
	uint8  Lx = image_Left;
	for(uint8 i = center;i > image_Left;i--)		//从中间往左边找
	{
		if(imageBuf[line][i] == black && imageBuf[line][i-1] == black && imageBuf[line][i-2] == black)
		{
			left_lose_flag[line] = 0;				//左边找到线，所以丢线标志为零
			Lx = i;
			break;
		}
		else if(i == image_Left+1)					//一直到左边都没有找到黑点，丢线
		{
			Lx = image_Left;
			left_lose_flag[line] = 1;
			break;
		}
	}
	return Lx;
}

uint8 find_Right(uint8 line,uint8 center)
{
	uint8  Rx=image_Right;
	for(uint8 i=center;i<image_Right;i++)
	{
		if(imageBuf[line][i]==black&&imageBuf[line][i+1]==black&&imageBuf[line][i+2]==black)
		{
			right_lose_flag[line]=0;
			Rx=i;break;
		}
		else if(i==image_Right-1)
		{
			Rx=image_Right;
			right_lose_flag[line]=1;
			break;
		}
	}
	return Rx;
}

void find_Side(void)					//只找 start_line 的左右边界及中点
{	
	for(uint8 i = image_Center;i > image_Left;i--)
	{
		if(imageBuf[start_line][i] == black && imageBuf[start_line][i-1] == black && imageBuf[start_line][i-2] == black)
		{
			left_lose_flag[start_line] = 0;
			image_L[start_line] = i;
			break;
		}
		if(i==image_Left+1)
		{
			image_L[start_line] = image_Left;
			left_lose_flag[start_line] = 1;
			break;
		}
	}
	for(uint8 i = image_Center;i < image_Right;i++)
	{
		if(imageBuf[start_line][i] == black && imageBuf[start_line][i+1] == black && imageBuf[start_line][i+2] == black)
		{
			right_lose_flag[start_line] = 0;
			image_R[start_line] = i;
			break;
		}
		else if(i == image_Right-1)
		{
			image_R[start_line] = image_Right;
			right_lose_flag[start_line] = 1;
			break;
		}
	}

	image_C[start_line] = (image_L[start_line] + image_R[start_line]) / 2;
	
	for(uint8 i = start_line-1;i >= 10;i--)
	{
		image_L[i] = find_Left(i,image_C[i+1]);
		image_R[i] = find_Right(i,image_C[i+1]);
		image_C[i] = (image_L[i]+image_R[i]) / 2;
		if(image_L[i] == image_R[i])
		{
			stop_F_line = i;
			break;
		}		
	}

	for(uint8 j = stop_F_line;j > 0;j--)
	{
		image_C[j] = image_C[j+1];
	}


}



void steer_control(void)			//驾驶控制
{
	find_Side();
	//crossRoad();	
    Dis_H = future_Distance(image_Center);		//电机转速控制
	//closeroad();
    if(Dis_H >= 20)
	{
		set_speed = 1000;
	}
	else 
	{
		set_speed = 3000;
	}


	
    /*if(Dis_H<=50){
        //默认为直�?
        S_P=5;
        S_D=5;
        set_speed=230;
    }else if(Dis_H<=70&Dis_H>50){
        //待转�?
        S_P=15;
        S_D=5;
        set_speed=150;
    }else if(Dis_H<=80&Dis_H>70){
        //急转�?
        S_P=30;
        S_D=5;
        set_speed=120;
    }
	else if(Dis_H>80){
        
        S_P=60;
        S_D=5;
        set_speed=0;
	}*/



	
	//err_old=err;
	//err=(image_C[Fixation1]+image_C[Fixation2])-2*image_Center;

	//steer_PWM=steer_Center+S_P*err+(err-err_old)*S_D;

	//steer_PWM=steer_PWM<steer_right_max?steer_right_max:steer_PWM;
	//steer_PWM=steer_PWM>steer_left_max?steer_left_max:steer_PWM;		
	//ftm_pwm_duty(SD5_FTM, SD5_CH,steer_PWM);
	err_old = err;
	err = (image_R[70] + image_L[70]) / 2 + (image_R[100] + image_L[100]) / 2 - image_Center * 2;		//实际赛道中心相对于坐标中心的偏移
	steer_PWM = err * 9 + steer_Center;
	ftm_pwm_duty(SD5_FTM, SD5_CH,steer_PWM);



}






void motor_s_control()
{
	ftm_pwm_duty(FTM0, FTM_CH0, 0);
	ftm_pwm_duty(FTM0, FTM_CH1,1000);	
}





void sendimg(void *imgaddr, uint32 imgsize)					//发送到小液晶？
{
    uint8 cmd[4] = {0, 255, 1, 0 };    //yy_鎽勫儚澶翠覆鍙ｈ皟璇?浣跨敤鐨勫懡�?

    uart_putbuff(VCAN_PORT, (uint8_t *)cmd, sizeof(cmd));    //鍏堝彂閫佸懡�?		这一句的语法？获取一个数组的地址？（unit8_t *）

    uart_putbuff(VCAN_PORT, imgaddr, imgsize); //鍐嶅彂閫佸浘�?
}


void send( uint8 i )
{
	uint8 a;
	a = i;
	uint8 c[120];
	// uint8 c;
	uint8 b;
	if(a == 1)
	{
		uart_putbuff (VCAN_PORT,"1234567", 3);
		sendimg(imgbuff, CAMERA_SIZE);
		vcan_sendimg(imgbuff,CAMERA_SIZE);	
	}
	else
	{          
		/*for(uint8 b=120,i=0;b>10;b--,i++)
		{
    		c=image_R[b]-image_L[b];
    		uart_putbuff(UART4,&c, 3);
		}*/
    	for(uint8 b = 120,i = 0;b > 0;b--,i++)
    	{
        	c[0] = image_R[b] - image_L[b];
	    	uart_putbuff(UART4,c, 1);
    	}	
	}
}


/*
 *史迎辉十字
 */
void crossroad1()
{
	uint8 a;
	if(shizi_flag == 0)
	{
		for(uint8 i = CAMERA_H;i > 10;i--)
		{
			if(((image_R[i-1] - image_R[i]) >= 1) && (left_lose_flag[i] == 1))
			{
				shizi_flag = 1;
				break;
			}
		}
	}

	if((shizi_flag == 1) && (shizi_ing == 0))
	{
		for(uint8 i = 110;i > 10;i--)
		{
			if((left_lose_flag[i] == 1) && (right_lose_flag[i] == 1) && (left_lose_flag[i-1] == 1) && (right_lose_flag[i-1] == 1))
			{
				shizi_ing = 1;
				break;
			}
		}
	}

	if(shizi_ing == 1)
	{
		for(uint8 i = CAMERA_H;i > 10;i--)
		{
			if(((image_R[i-1] - image_R[i]) >= 1) && (left_lose_flag[i] == 1))
			{
				firstline = i;
				break;
			}
		}
		for(uint8 i = firstline;i > 10;i--)
		{
			if((left_lose_flag[i+1] == 1) && (left_lose_flag[i] == 0))
			{
				secondline = i;
				break;
			}
		}
		//c = (image_L[b] - image_R[a]) / (b - a);
		for(uint8 i = firstline;i > secondline;i--)
		{
			a = image_L[secondline];
			imageBuf[i + 1][a] = black;
			imageBuf[i + 1][a + 1] = black;
			imageBuf[i + 1][a + 2] = black;
		}
	}
}



/*
 *史迎辉十字
 */
void crossroad()
{
	uint16  a,b,g,d,e;
	if(shizi_flag==0)
	{
		for(uint8 i=CAMERA_H;i>10;i--)
		{
			if(((image_R[i-1]-image_R[i])>=1)&&(left_lose_flag[i]==1))
			{
				shizi_flag=1;
				break;
			}
		}
	}
	
	if((shizi_flag==1)&&(shizi_ing==0))
	{
		for(uint8 i=110;i>10;i--)
		{
			if ((left_lose_flag[i]==1)&&(right_lose_flag[i]==1)&&(right_lose_flag[i-1]==1)&&(left_lose_flag[i-1]==1))
			{
				shizi_ing=1;
				break;
			}	
		}
	}
	
	if (shizi_ing==1)
	{
		for(uint8 i=CAMERA_H;i>10;i--)
		{
			if(((image_R[i-1]-image_R[i])>=1)&&(left_lose_flag[i]==1))
			{	
				firstline=i;
				break;
			}
		}
		for(uint8 i=firstline;i>10;i--)
		{
			if((left_lose_flag[i+1]==1)&&(left_lose_flag[i]==0))
			{
				secondline=i;
				break;
			}
		}
		
		if(image_R[firstline]<image_R[secondline])
		{
			g=((image_R[firstline]-image_L[secondline])<<6)/(firstline-secondline);
			b=image_R[firstline]<<6;
			
			for (uint8 i=firstline,a=0;i>secondline;i--,a++)
			{
				image_R[i]=((b-a*g)>>6);
			}
		}
		
		/*b=image_L[secondline];
		for (uint8 i=firstline;i>secondline;i--)
		{
			image_R[i]=b;
		}*/
		
		for(uint8 i=CAMERA_H;i>10;i--)
		{
			image_C[i]=(image_L[i]+image_R[i])/2;
		}
	}
}


void cycle1()
{
	uint8 t;
	if (yuanhuan==0)												//确定圆环（圆环识别）
	{
		for(uint8 i=CAMERA_H;i>10;i--)								//(1)
		{
			if((left_lose_flag[119]==0)&&(left_lose_flag[118]==0)&&(image_L[i]==image_L[i-1])&&((image_L[i]-image_L[i+1])>=1))
			{
				t=i;
				break;
			}
		}
		for(uint8 i=t;i>10;i--)										//(2)
		{
			if((image_L[i]-image_L[i-1])>=1)
			{
				t=i;
				break;
			}
		}
		for(uint8 i=t;i>10;i--)										//(3)
		{
			if((left_lose_flag[i]==1)&&(left_lose_flag[i-1]==1))
			{
				yuanhuan=1;
				break;
			}
		}
	}
	if((yuanhuan==1)&&(yuanhuaning==0))
	{
		if((left_lose_flag[119]==1)&&(left_lose_flag[118]==1)&&(left_lose_flag[117]==1))
		{
			ftm_pwm_duty(SD5_FTM, SD5_CH,4500);
			DELAY_MS(600);
			yuanhuaning=1;
		}
	}
}



void  main(void)
{
    /*************************LCD***************************/
	uint8 f[120];
	uart_init(UART4, 115200);
	gpio_init(PTC3,GPO,HIGH);
	Site_t siteImg = {0,0};
	Site_t site1 = {0,0};            //第一�?
	Site_t site2 = {0,0};            //第二�?
	Site_t site3 = {0,0};
	Size_t imgsize = {CAMERA_W,CAMERA_H};
	Size_t sizeImg;
	sizeImg.H = 60;                  //显示图像区域
	sizeImg.W = 80;
	LCD_init();
	site1.y = 65;
	site2.y = 80;
	site3.y = 95;
	Size_t imgsize2={80,60};
	uint8 b;
	uint8 c;

	ftm_pwm_init(SD5_FTM,SD5_CH,SD5_HZ,3910); //初始化舵�?
	ftm_pwm_init(FTM0,FTM_CH0,20000,0); 	   //初始化电机通道
	ftm_pwm_init(FTM0,FTM_CH1,20000,1000); 	   //初始化电机通道
	
	camera_init(imgbuff);                //摄像头初始化
	
	set_vector_handler(PORTA_VECTORn, PORTA_IRQHandler);         //设置LPTMR的中断复位函数为 PORTA_IRQHandler
	set_vector_handler(DMA0_VECTORn, DMA0_IRQHandler);           //设置DMA0的中断复位函数为 DMA0_IRQHandler



	/*************************PIT***************************/
	pit_init_ms(PIT0,100);			//pit0电机控制时间定时
	set_vector_handler(PIT0_VECTORn,PIT0_IRQHandler);
	enable_irq(PIT0_IRQn);
	
	ftm_quad_init(FTM2);                          //测�?
	uart_init(VCAN_PORT, VCAN_BAUD);



	while(1)
	{
		//duojizhaozhongwei();
		camera_get_img();                 							//采集图像
		steer_control();
		motor_s_control();
		img_extract(img, (uint8 *)imgbuff, CAMERA_SIZE);     		//解压
		uint32 n=0;													//这里可以直接解压到二维数组
	 	for(uint8 i=1;i<=CAMERA_H;i++)    							//得到图像展开为二维数�?
		{
			for(uint8 j=0;j<CAMERA_W;j++)
			{
				imageBuf[i][j]=img[n];
				n++;
			}
	 	}
		//shizi_flag=(image_R[70]+image_L[70])/2;
		  
		//shizi_ing=(image_R[100]+image_L[100])/2;
		
		LCD_num_BC(site2,shizi_flag ,3, RED, GREEN); 				//显示十字标志
		LCD_num_BC(site3,shizi_ing ,3, RED, GREEN);  				//显示十字中标�?
		
		dhd_img_condense(imgbuff1, img, CAMERA_SIZE/4);            	//压缩
		LCD_Img_Binary_Z(siteImg, sizeImg, imgbuff1, imgsize2);    	//lcd显示
		LCD_num_BC(site1,Dis_H, 3,RED, GREEN);       				//第七十三行前瞻显�?
	
		for(uint8 i=60,j=120;i>10;i--,j-=2)                         //画出赛道中点
		{
			lcdimg.y=i;
			lcdimg.x=image_C[j]/2;
		    LCD_point(lcdimg, RED);
		}
	
		
		     

		//uart_putbuff (VCAN_PORT,"1234567", 3);
		sendimg(imgbuff, CAMERA_SIZE);
		vcan_sendimg(imgbuff,CAMERA_SIZE);	
                //threepoint();
                

	
		//future_Distance1();
               //send(1);
		/*for(uint8 b=120,i=0;b>35;b--,i++)    //�������ұߵļ��
               {
               f[i]=image_R[b]-image_L[b];
	       }*/
	}
}
