#include<reg51.h>
#include<intrins.h>
#include<stdio.h>

void i2c_start(void);
void i2c_stop(void);
void i2c_ACK(void);
void i2c_write(unsigned char);
void lcd_send_cmd(unsigned char);
void lcd_send_data(unsigned char);
void lcd_send_str(unsigned char *);
void lcd_slave(unsigned char);
void delay_ms(unsigned int);
void display_time1(unsigned int, unsigned int, unsigned int, unsigned int);
void display_time2(unsigned int, unsigned int, unsigned int);
void display_countdown(int, int, int);

unsigned char slave1=0x4E;
unsigned char slave_add;

sbit scl=P0^6;
sbit sda=P0^7;
sbit start_btn=P3^3;
sbit stop_btn=P3^2;
sbit reset_btn=P3^1;
sbit mode_btn=P3^0;
sbit toggle_btn=P3^4;
sbit buzzer=P3^5;

volatile unsigned int ms_counter = 0;
volatile unsigned int sec_counter = 0;
volatile unsigned int min_counter = 0;
volatile unsigned int hour_counter = 0;
volatile bit timer_flag = 0;
volatile bit stopwatch_mode = 1;// 1 for stopwatch, 0 for timer
volatile bit btn_pressed = 0;

int seconds = 0;
int minutes = 0;
int hours = 0;
int settingMode = 1; 

void i2c_start(void)
{
    sda=1; _nop_(); _nop_();
    scl=1; _nop_(); _nop_();
    sda=0; _nop_(); _nop_();
}

void i2c_stop(void)
{
    scl=0;
    sda=0;
    scl=1;
    sda=1;
}

void lcd_slave(unsigned char slave)
{
    slave_add=slave;
}

void i2c_ACK(void)
{
    scl=0;
    sda=1;
    scl=1;
    while(sda);
}

void i2c_write(unsigned char dat)
{
    unsigned char i;
    for(i=0;i<8;i++)
    {
        scl=0;
        sda=(dat&(0x80>>i))?1:0;
        scl=1;
    }
}

void lcd_send_cmd(unsigned char cmd)
{
    unsigned char cmd_l,cmd_u;
    cmd_l=(cmd<<4)&0xf0;
    cmd_u=(cmd &0xf0);
    i2c_start();
    i2c_write(slave_add);
    i2c_ACK();
    i2c_write(cmd_u|0x0C);
    i2c_ACK();
    delay_ms(1);
    i2c_write(cmd_u|0x08);
    i2c_ACK();
    delay_ms(10);
    i2c_write(cmd_l|0x0C);
    i2c_ACK();
    delay_ms(1);
    i2c_write(cmd_l|0x08);
    i2c_ACK();
    delay_ms(10);
    i2c_stop();
}

void lcd_send_data(unsigned char dataw)
{
    unsigned char dataw_l,dataw_u;
    dataw_l=(dataw<<4)&0xf0;
    dataw_u=(dataw &0xf0);
    i2c_start();
    i2c_write(slave_add);
    i2c_ACK();
    i2c_write(dataw_u|0x0D);
    i2c_ACK();
    delay_ms(1);
    i2c_write(dataw_u|0x09);
    i2c_ACK();
    delay_ms(10);
    i2c_write(dataw_l|0x0D);
    i2c_ACK();
    delay_ms(1);
    i2c_write(dataw_l|0x09);
    i2c_ACK();
    delay_ms(10);
    i2c_stop();
}

void lcd_send_str(unsigned char *p)
{
    while(*p != '\0')
        lcd_send_data(*p++);
}

void delay_ms(unsigned int n)
{
    unsigned int m;
    for(n;n>0;n--)
    {
        for(m=121;m>0;m--);
        _nop_();
        _nop_();
        _nop_();
        _nop_();
        _nop_();
        _nop_();
    }
}

void lcd_init()
{
    lcd_send_cmd(0x02);
    lcd_send_cmd(0x28);
    lcd_send_cmd(0x0C);
    lcd_send_cmd(0x06);
    lcd_send_cmd(0x01);
}

void timer0_init()
{
    TMOD |= 0x01;
    TH0 = 0xFC;
    TL0 = 0x66;
    ET0 = 1;
    TR0 = 1;
    EA = 1;
}

void display_time1(unsigned int hour, unsigned int min, unsigned int sec, unsigned int ms)
{
    unsigned char str[15];
    str[14] = '\0';
    
    str[0] = (hour / 10) + '0';
    str[1] = (hour % 10) + '0';
    str[2] = ':';
    str[3] = (min / 10) + '0';
    str[4] = (min % 10) + '0';
    str[5] = ':';
    str[6] = (sec / 10) + '0';
    str[7] = (sec % 10) + '0';
    str[8] = ':';
    str[9] = (ms / 100) + '0';
    str[10] = ((ms % 100) / 10) + '0';
    str[11] = (ms % 10) + '0';
    
    lcd_send_cmd(0xC0);
    lcd_send_str(str);
}

void display_time2(unsigned int hour, unsigned int min, unsigned int sec)
{
    unsigned char str[9];
    str[8] = '\0';
    
    str[0] = (hour / 10) + '0';
    str[1] = (hour % 10) + '0';
    str[2] = ':';
    str[3] = (min / 10) + '0';
    str[4] = (min % 10) + '0';
    str[5] = ':';
    str[6] = (sec / 10) + '0';
    str[7] = (sec % 10) + '0';
    
    lcd_send_cmd(0xC0);
    lcd_send_str(str);
}

void timer0_isr() interrupt 1
{
    if(timer_flag)
    {
        ms_counter++;
        if(ms_counter >= 1000)
        {
            ms_counter = 0;
            sec_counter++;
            if(sec_counter >= 60)
            {
                sec_counter = 0;
                min_counter++;
                if(min_counter >= 60)
                {
                    min_counter = 0;
                    hour_counter++;
                }
            }
        }
    }
    TH0 = 0xFC;
    TL0 = 0x66;
}

void display_countdown(int sec, int min, int hr)
{
    char buffer[11];
    int i;
    for(i = hr*3600 + min*60 + sec; i > 0; i--)
    {
        hr = i / 3600;
        min = (i % 3600) / 60;
        sec = (i % 3600) % 60;
        lcd_send_cmd(0x80 | 0x40); // set DDRAM address to second line
        sprintf(buffer, "%02d:%02d:%02d", hr, min, sec);
        lcd_send_str(buffer);
        delay_ms(750); // delay for 1 second
    }
}

void main(void)
{
		buzzer=1;
    lcd_slave(slave1);
    lcd_init();
    timer0_init();

    lcd_send_cmd(0x80);
    lcd_send_str("Stopwatch");
    lcd_send_cmd(0xC0);
    lcd_send_str("00:00:00:000");

    while(1)
    {
        if(toggle_btn == 0 && timer_flag == 0)
        {
            stopwatch_mode = ~stopwatch_mode;
            lcd_send_cmd(0x01);
            if(stopwatch_mode)
            {
								lcd_send_cmd(0x01);
                lcd_send_cmd(0x80);
                lcd_send_str("Stopwatch");
                lcd_send_cmd(0xC0);
                display_time1(hour_counter, min_counter, sec_counter, ms_counter);
            }
            else
            {
								lcd_send_cmd(0x01);
                lcd_send_cmd(0x80);
                lcd_send_str("Set Seconds:");
                lcd_send_cmd(0xC0);
                display_time2(hours,minutes,seconds);
            }
            delay_ms(200);
        }

        if(stopwatch_mode)
        {
            if(start_btn == 0 && btn_pressed == 0)
            {
								btn_pressed = 1;
                timer_flag = ~timer_flag;
            }
            if(stop_btn == 0 && btn_pressed == 0)
            {
								btn_pressed = 1;
                timer_flag = 0;
            }
            if(reset_btn == 0 && btn_pressed == 0)
            {
								btn_pressed = 0;
                ms_counter = 0;
                sec_counter = 0;
                min_counter = 0;
                hour_counter = 0;
                display_time1(hour_counter, min_counter, sec_counter, ms_counter);
            }
            if(start_btn == 1 && stop_btn == 1 && reset_btn == 1)
            {
                btn_pressed = 0;
            }

            if(timer_flag)
            {
                display_time1(hour_counter, min_counter, sec_counter, ms_counter);
            }
        }
        else
        {
						if(stop_btn == 0)
						{
								while(stop_btn == 0);
								if(settingMode == 1)
								{
										seconds = (seconds + 1) % 60;
										lcd_send_cmd(0xC0);
										lcd_send_str("    "); // clear previous number
										lcd_send_cmd(0xC0);
										lcd_send_data((seconds/10) + '0');
										lcd_send_data((seconds%10) + '0');
								}
								else if(settingMode == 3)
								{
										hours = (hours + 1) % 24;
										lcd_send_cmd(0xC0 + 6);
										lcd_send_str("  "); // clear previous number
										lcd_send_cmd(0xC0 + 6);
										lcd_send_data((hours/10) + '0');
										lcd_send_data((hours%10) + '0');
								}
								else
								{
										minutes = (minutes + 1) % 60;
										lcd_send_cmd(0xC0 + 3);
										lcd_send_str("    "); // clear previous number
										lcd_send_cmd(0xC0 + 3);
										lcd_send_data((minutes/10) + '0');
										lcd_send_data((minutes%10) + '0');
								}
								delay_ms(200);
						}
        
						if(reset_btn == 0)
						{
								while(reset_btn == 0);
								if(settingMode == 1)
								{
										seconds = (seconds - 1 + 60) % 60;
										lcd_send_cmd(0xC0);
										lcd_send_str("    "); // clear previous number
										lcd_send_cmd(0xC0);
										lcd_send_data((seconds/10) + '0');
										lcd_send_data((seconds%10) + '0');
								}
								else if(settingMode == 3)
								{
										hours = (hours - 1 + 24) % 24;
										lcd_send_cmd(0xC0 + 6);
										lcd_send_str("  "); // clear previous number
										lcd_send_cmd(0xC0 + 6);
										lcd_send_data((hours/10) + '0');
										lcd_send_data((hours%10) + '0');
								}
								else
								{
										minutes = (minutes - 1 + 60) % 60;
										lcd_send_cmd(0xC0 + 3);
										lcd_send_str("    "); // clear previous number
										lcd_send_cmd(0xC0 + 3);
										lcd_send_data((minutes/10) + '0');
										lcd_send_data((minutes%10) + '0');
								}
								delay_ms(200);
						}
        
						if(mode_btn == 0)
						{
								while(mode_btn == 0);
								settingMode = (settingMode % 3) + 1;
								if(settingMode == 1)
								{
										lcd_send_cmd(0x80);
										lcd_send_str("Set Seconds:");
								}
								else if(settingMode == 2)
								{
										lcd_send_cmd(0x80);
										lcd_send_str("Set Minutes:");
								}
								else
								{
										lcd_send_cmd(0x80);
										lcd_send_str("Set Hours:  ");
								}
								delay_ms(200);
						}

            if(start_btn == 0)
            {
                lcd_send_cmd(0x01);
                lcd_send_cmd(0x80);
                lcd_send_str("Countdown:");
                display_countdown(seconds, minutes, hours);
                lcd_send_cmd(0x80 | 0x40);
                lcd_send_str("           Done!");

                buzzer = 0;
                delay_ms(1000);
                buzzer = 1;
                lcd_send_cmd(0x01);
                lcd_send_cmd(0x80);
                lcd_send_str("Set Seconds:  ");
                lcd_send_cmd(0x80 | 0x40);
                lcd_send_data((hours/10) + '0');
                lcd_send_data((hours%10) + '0');
                lcd_send_data(':');
                lcd_send_data((minutes/10) + '0');
                lcd_send_data((minutes%10) + '0');
                lcd_send_data(':');
                lcd_send_data((seconds/10) + '0');
                lcd_send_data((seconds%10) + '0');
            }
        }
    }
}