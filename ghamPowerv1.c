
#ifdef F_CPU
#undef F_CPU
#endif

#define F_CPU 12000000L

#include <avr/io.h>
#include <util/delay.h>

#define RS PD0
#define RW PD1
#define E  PD2
#define R1  2200
#define R2  330

#define HIGH(PORT,PIN) PORT|=(1<<PIN)
#define LOW(PORT,PIN) PORT&=~(1<<PIN)

void wait_till_busy()
{
 
 uint8_t status;

 DDRD&=0x0F;

 HIGH(PORTD,RW);
 LOW(PORTD,RS);

 do
  {
  	HIGH(PORTD,E);
   	_delay_us(1);

   	status=(PIND&0xF0);

   	LOW(PORTD,E);
   	_delay_us(1);
   	HIGH(PORTD,E);
	
	status|=((PIND&0xF0)>>4);

	LOW(PORTD,E);
	_delay_us(1);
  
  }while(status & 0x80);
	
	DDRD|=0xF0;
}

void Send(uint8_t cmd,uint8_t isdata)
{ 
 
	uint8_t temp;
 
 	wait_till_busy();

 	LOW(PORTD,RW);

 	if(isdata)
  		HIGH(PORTD,RS);
 	else
  		LOW(PORTD,RS);
 
 	HIGH(PORTD,E);
 	_delay_us(1);

 	temp=PORTD&0x0F;
 	temp|=(cmd&0xF0);
 	PORTD=temp;

 	LOW(PORTD,E);
 	_delay_us(1);
 	HIGH(PORTD,E);

 	temp=PORTD&0x0F;
 	temp|=((cmd&0x0F)<<4);
 	PORTD=temp;

 	LOW(PORTD,E);

}

void InitLCD(uint8_t flag)
{
 	_delay_ms(30);				//Wait for stable power supply

	PORTD=0x00;					//Make all output low
	DDRD=0xFF;					//Make all line output
	
	Send(0b00000010,0);			//Notify 4 bit mode
	_delay_us(100);				//wait for sometime, BF cannot determine at this stage

	Send(0b00101000,0);  		//DB4=0 4 bit mode, DB3=1 lines>=2, DB2=0 5x7 display
	_delay_us(100);				//wait here for sometime no need to wait here after

	Send(0b00001100|flag,0);  	//DB4=1 Display Control DB3=1 
								//Display on DB2=1 Cursor on
								//DB0=1 Blink On

}

void LCDGotoXY(uint8_t x,uint8_t y)
{
  	uint8_t temp=0x80;

  	if(y==1)
   		temp=0xC0;
	else if(y==2)
		temp=0x94;
	else if(y==3)
		temp=0xD4;
	
	Send(temp+x,0);
}

void LCDWriteString(char *szText)
{
	while(*szText)
	 Send(*szText++,1); 
}

void LCDWriteInt(int16_t data)
{
 	uint8_t num[5],i=0;
	
	if(!data)
	 LCDWriteString("0000");
	else
	 {
	while(data)
	 {
	  num[i++]=data%10;
	  data/=10;
	 }
	  
	 for(;i>0;i--)
	   Send(num[i-1]+'0',1);
	}
}

void LCDWriteFloat(float data)
{
 	int temp;
	float tempData;

	if(!data)
		LCDWriteString("00");
	else
	{
		LCDWriteInt(data);
		LCDWriteString(".");
		tempData=data*100.0;
		temp=tempData;
		temp=temp%100;
		LCDWriteInt(temp);
	}
}

#define _delay(a) _delay_us(a)

void Write(uint16_t num,uint8_t padding)
{
	uint8_t i=0;
	char buffer[5]={'0'};

	if(num<0)
	  {
	 	num=-num;
		LCDWriteString("-");
	  }
	
	while(num)
	 {
	   buffer[i++]=num%10;
	   num/=10;
	 }

	buffer[i]=0;

	while((padding-i)>0)
		{
			LCDWriteString("0");
			i--;
		}
	LCDWriteString(buffer);
}

void initADC()
{
	ADMUX=(1<<REFS0);                         // For Aref=AVcc;
	ADCSRA=(1<<ADEN)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0); //Rrescalar div factor =128
}

uint16_t readADC(uint8_t ch)
{
   //Select ADC Channel ch must be 0-7
   ch=ch&0b00000111;
   ADMUX|=ch;

   //Start Single conversion
   ADCSRA|=(1<<ADSC);

   //Wait for conversion to complete
   while(!(ADCSRA & (1<<ADIF)));

   //Clear ADIF by writing one to it
   //Note you may be wondering why we have write one to clear it
   //This is standard way of clearing bits in io as said in datasheets.
   //The code writes '1' but it result in setting bit to '0' !!!

   ADCSRA|=(1<<ADIF);


	

   return(ADC);
}

int main()
{	
 	
	uint16_t val, temp=0,adcVal,adcVal1;
	unsigned char toggle=1;
 	float voltageDiv, actualVoltage;
	float sensitivity;
	float sensedVoltage, difference, sensedCurrent;

	InitLCD(0);
	initADC();

	Send(0b00000001,0);		//Clear LCD

	LCDGotoXY(0,0);
	
	while(1)
	{
		if(toggle)
		{
			initADC();
			LCDGotoXY(0,0);
			for(unsigned char i=0;i<10;i++)							//looping 10 times for getting average value 
			{	
				val=readADC(2);			//channel 2
				temp+=val;		
			}
		
			adcVal1 = temp/10;					//getting average value
			val=0;
			temp=0;
		
			voltageDiv= ((adcVal1)/1023.0)*5;	//Reference 5V		
			actualVoltage=((R1+R2)*voltageDiv)/R2;
			LCDWriteString("Voltage:");
			LCDWriteFloat(actualVoltage);
			LCDWriteString("V");
			toggle=0;
		}
		else
		{
			initADC();

			for(unsigned char i=0;i<10;i++)							//looping 10 times for getting average value 
			{	
				val=readADC(1);			//channel 1
				temp+=val;		
			}
		
			adcVal = temp/10;					//getting average value
			val=0;
			temp=0;

			//sensitivity    = 0.083; // mV/A
			sensitivity    = 0.083; // mV/A
			sensedVoltage  = ((adcVal)/1024.0)*5;	//Reference 5V
			difference     = sensedVoltage - 2.5; //Vcc/2
			
			//LCDGotoXY(0,0);			//to test output voltage
			
			//LCDWriteString("Voltage:");
			//LCDWriteFloat(sensedVoltage);

			LCDGotoXY(0,1);
			LCDWriteString("Current:");
			if(difference<0) 					//for negative current
			{
				difference=difference*(-1);
				LCDWriteString("-");
			}
			sensedCurrent=difference/sensitivity;		
			LCDWriteFloat(sensedCurrent*1000);
			LCDWriteString("mA");
			toggle=1;
		}	

		_delay_ms(500);
		Send(0b00000001,0);		//Clear LCD
	}
	
 	return 0;
}




