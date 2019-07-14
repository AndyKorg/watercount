/*
* watercounter.c
*/
#include <avr/io.h>
#include <stdlib.h>
#include <string.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/interrupt.h>
#include <time.h>
#include "epd1in54.h"
#include "epdpaint.h"

#include "spi_bitbang.h"

#define NULL_DATE					0				//Неустановленное время
#define NULL_DATE_STR				"неизвестно"	//Выводится вместо 11.11.1111

#define ADC_MUX_MASK				((1<<MUX4) | (1<<MUX3) |(1<<MUX2) | (1<<MUX1) | (1<<MUX0))
#define ADC_NOT_USE_OFF()			do {DIDR0 = (1<<ADC0D) | (1<<ADC1D) | (1<<ADC2D) | (1<<ADC3D) | (1<<ADC4D) | (1<<ADC5D)| (1<<ADC6D) | (0<<ADC7D); DIDR1 = (1<<AIN1D) | (1<<AIN0D); } while (0)
	
#define MEASURE_COUNT				6				//Количество измерений в пакете (при частоте 8МГц частота измерерний 700 Гц), не меньше 2! см. sensorValue
#define POWER_PERIOD_MEASURE		16				//Период измерерния напряжения батареи в количестве измерений сенсора
volatile uint8_t countMeasure = 0;	//Счетчик измерений в пакете, если не 0, то выключать ADC на время сна нельзя

// датчик водосчетчика
#define NAMUR_PORT_STATE			PINA
#define NAMUR_PIN_STATE				PINA6			//Чем мерить - namur или простой выключатель
#define SENSOR_DIGITAL				PINA7			//Нога датчика для режима простого выключателя
#define SENSOR_ANALOG				ADC7D			//Цифровая нога датчика, выключается если используется namur
#define SENSOR_ANALOG_ON()			do {DIDR0 &= ~(1<<SENSOR_ANALOG);} while (0)
#define SENSOR_ANALOG_OFF()			do {DIDR0 |= (1<<SENSOR_ANALOG);} while (0)
#define SENSOR_MEASURE()			do {ADMUX = (1<<REFS1) | (1<<REFS0) | (0<<ADLAR) | (0<<MUX4) | (0<<MUX3) |(1<<MUX2) | (1<<MUX1) | (1<<MUX0); ADC_NOT_USE_OFF(); SENSOR_ANALOG_ON();} while (0) //REF=2,56V, канал 7, выравнивание вправо
#define SENSOR_MIN					100				//Минимальное значение нижнего порога исправного сенсора
#define SENSOR_MAX					600				//Максимальное значение верхнего порога исправного сенсора
#define SENSOR_LEVEL_DIFF			200				//Минимальная разность порогов исправного сенсора
#define SENSOR_ALARM_NO				0				//Все нормально с датчиком
#define SENSOR_ALARM_SHOT_CIR		1				//Короткое замыкание
#define SENSOR_ALARM_BREAK			2				//Обрыв датчика
#define SENSOR_ALARM_LEVEL			3				//не удалось определить состояние датчика из-за неправильных уровней
static	uint16_t preSensorValue = 0;				//Предыдущее значение сенсора
static	uint8_t alarmSensor	= 0;					//Состояние сенсора
#define	COUNT_SHOW_MAX				99999			//Максимальное отображаемое число на дисплее
static	uint16_t countWater = 0;					//Собственно сам счетчик воды

// Питание
#define SENSOR_POWER				PORTB1			//Подача питания на датчик
#define NAMUR_IS_OFF				(0)// (NAMUR_PORT_STATE & (1<<NAMUR_PIN_STATE))

#define WI_FI_POWER					PORTB3			//Подача питания на esp

//Контроль питания - значение меньше большее напряжение
#define REF_1V1_ACCURATE			1078			//Точное значение опорного напряжения источника 1.1V умноженное на 1000 (1,078*1000)
#define POWER_LOW					0x1b8			//2,49V при REF = 1,072V
#define SELF_SOURCE					((1<<MUX4) | (1<<MUX3) |(1<<MUX2) | (1<<MUX1) | (0<<MUX0))
#define POWER_MEASURE()				do {ADMUX = (0<<REFS1) | (1<<REFS0) | (0<<ADLAR) | SELF_SOURCE; ADC_NOT_USE_OFF(); SENSOR_ANALOG_OFF();} while (0) //REF=VCC, канал AREF
#define IS_POWER_MEASURE			((ADMUX & ADC_MUX_MASK) == SELF_SOURCE)
time_t	dtCheckPower = NULL_DATE;					//Время последней проверки батареи
volatile uint16_t batVoltage;						//Напряжение на батарее в еденицах ADC
#define POWER_IS_LOW()				(batVoltage >= POWER_LOW)	//Батарея разряжена

//Выключение ненужных блоков на время сна 0 ТАЙМЕР ПОКА ВКЛЮЧИЛ! для spi
#define PWR_TIMER_ONLY()			do{						\
										PRR0  = (1<<PRTWI)	\
										| (0<<PRTIM2)	\
										| (1<<PRTIM1)	\
										| (0<<PRTIM0)	\
										| (1<<PRUSART1)	\
										| (1<<PRUSART0)	\
										| (1<<PRSPI)	\
										| (1<<PRADC);	\
									} while(0)
									
#define PWR_ON						0
#define PWR_OFF						1

#define PWR_ADC(x)					do{ if(x==PWR_ON) {PRR0 &= ~(1<<PRADC);} else {PRR0 |= (1<<PRADC);} } while(0)
#define PWR_USART1(x)				do{ if(x==PWR_ON) {PRR0 &= ~(1<<PRUSART1);} else {PRR0 |= (1<<PRUSART1);} } while(0)

//wifi
time_t	dtSend = NULL_DATE;								//Время последней передачи
#define WIFI_READY_PORT				PORTC
#define WIFI_READY_PORT_IN			PINC
#define WIFI_READY					PINC5
#define WIFI_IS_READY()				((WIFI_READY_PORT_IN & (1<<WIFI_READY)) == 0)
#define WIFI_ISR					PCINT2_vect			//Может пересекатся с прерыванием от SPI!
#define WIFI_READY_INTERUPT()		do {PCMSK2 |= (1<<PCINT21); PCICR |= (1<<PCIE2);} while (0)
#define WIFI_INIT()					do{\
										WIFI_READY_PORT |= (1<<WIFI_READY);\
										WIFI_READY_INTERUPT();\
									}while(0)

//Экран
#define SCREEN_POWER_DDR			DDRD
#define SCREEN_POWER_PORT			PORTD
#define SCREEN_POWER_PIN			PORTD2			//Подача питания на экран
#define SCREEN_POWER_ON()			do {SCREEN_POWER_DDR |= (1<<SCREEN_POWER_PIN); SCREEN_POWER_PORT |= (1<<SCREEN_POWER_PIN); } while (0)
#define SCREEN_POWER_OFF()			do {SCREEN_POWER_DDR |= (1<<SCREEN_POWER_PIN); SCREEN_POWER_PORT &= ~(1<<SCREEN_POWER_PIN); } while (0)

//Позиции курсора
uint16_t countPos,					//Цифры счетчика
		powerPosX, powerPosY,		//Заряд батареи по X и по Y
		sCheckBatPos,				//Надпись "проверено" для батареи
		dtCheckPos,					//Дата проверки батареи
		sSendPos,					//Надпись "передано" для wifi
		dtSendPos					//Дата передачи данных на сервер
		;
#define STRING_MARGIN_HIGHT			5	//Отсутп 
#define STRING_MARGIN_LOW			3
#define	FONT_BIG
#define FONT_LOW

ISR(WIFI_ISR){
	static uint8_t pin_hystory = 0xff;
	uint8_t changepin = WIFI_READY_PORT_IN ^ pin_hystory;
	pin_hystory = WIFI_READY_PORT_IN;
	if (changepin & (1<<WIFI_READY)){
		if (!(pin_hystory & (1<<WIFI_READY))){
			
		}
	}
}

ISR(ADC_vect){
	static uint32_t tmpSum = 0;
	static uint8_t countS = 0;
	uint16_t adc = ADCW;			//Обязательно надо прочитать
	
	if (countMeasure < (MEASURE_COUNT-1)){
			tmpSum += adc;
	}

	if (countMeasure){				//Надо еще раз измерить
		ADCSRA |= (1<<ADSC);
		countMeasure--;
	}
	else{							//Есть результат пакета измерерний
		if (IS_POWER_MEASURE){
			batVoltage = tmpSum/(MEASURE_COUNT-1);
			dtCheckPower = time(NULL);
		}
		else{
			tmpSum = tmpSum/(MEASURE_COUNT-1);
			if (tmpSum < SENSOR_MIN){  //К.З. датчика
				alarmSensor = SENSOR_ALARM_SHOT_CIR;
			}
			else if (tmpSum > SENSOR_MAX){	//Обрыв датчика
				alarmSensor = SENSOR_ALARM_BREAK;
			}
			else {
				if (preSensorValue >= tmpSum){	//Что бы не возится с преобразованием в знаковую переменную
					if ((preSensorValue - tmpSum) >= SENSOR_LEVEL_DIFF){ //Из размкнут (off) в замкнут (on)
						countWater++;
					}
				}
				alarmSensor = SENSOR_ALARM_NO;
			}
			preSensorValue = tmpSum;
		}
		//Подготовка к следующему измерению
		tmpSum = 0;
		if (countS--){
			SENSOR_MEASURE();
		}
		else{
			POWER_MEASURE();
			countS = POWER_PERIOD_MEASURE;
		}
	}
}

ISR(TIMER2_OVF_vect){
	static uint8_t tic = 16;

	TCNT2 = 0;//Взвести TCN2UB для контроля готовности таймера
	while(ASSR & (1<<TCN2UB));// Ждем окончания предыдущей операции - сброса TCN2UB

	tic--;
	
	if (!(NAMUR_IS_OFF)) {
		PWR_ADC(PWR_ON);
		if ((!(ADCSRA & (1<<ADSC))) && (countMeasure == 0)){			//Предыдущее измерение закончено
			countMeasure = MEASURE_COUNT;
			ADCSRA |= (1<<ADSC);
		}
	}
	
	if (!tic){															//Прошла одна секунда
		system_tick();
		tic = 16;
#ifdef DEBUG
		dtSend = time(NULL);
#endif		
	}
	
}

inline void twoDigit(int value, char* str, size_t size){
	char cnv[10];
	if (value<10){
		strlcat(str, "0", size);
	}
	strlcat(str, itoa(value, cnv, 10), size);
}
// Выводит дату, возвращает позицию х на которй был закончен вывод
int dtShow(time_t* value, int x){
	char s[2+1+2+1+4 +1 +2+1+2+1+2 +1]; //dd.mm.yyyy hh:mm:ss(NULL)
	char cnv[10];
	
	memset(s, 0, sizeof(s));
	if (*value == NULL_DATE){
		strlcpy(s, NULL_DATE_STR, sizeof(s));
	} 
	else {
		struct tm* t = localtime(value);
		twoDigit(t->tm_mday, s, sizeof(s));
		strlcat(s, ".", sizeof(s));
		twoDigit(t->tm_mon+1, s, sizeof(s));
		strlcat(s, ".", sizeof(s));
		strlcat(s, itoa((t->tm_year+1900), cnv, 10), sizeof(s));
		strlcat(s, " ", sizeof(s));
		twoDigit(t->tm_hour, s, sizeof(s));
		strlcat(s, ":", sizeof(s));
		twoDigit(t->tm_min, s, sizeof(s));
		strlcat(s, ":", sizeof(s));
		twoDigit(t->tm_sec, s, sizeof(s));
	}
	epdClear(UNCOLORED);//Очистить paint для рисования
	return epdDrawStringAt(x, 0, s, &FontStreched72, COLORED);
}

//Выводит напряжение на батарее до двух знаков после запятой, возвращает позицию х на которй был закончен вывод
int batShow(int x){
	char s[10], cnv[10];
	memset(s, 0, sizeof(s));
	if (batVoltage){
		uint32_t mV = ((uint32_t)REF_1V1_ACCURATE*1023)/batVoltage;
		uint8_t intV = mV/1000;
		uint8_t fracV = (mV-(intV*1000))/10;
		strlcat(s, " ", sizeof(s));
		strlcat(s, itoa(intV, cnv, 10), sizeof(s));
		strlcat(s, ",", sizeof(s));
		strlcat(s, itoa(fracV, cnv, 10), sizeof(s));
		strlcat(s, " V", sizeof(s));
	}
	else{
		strlcat(s, " NA", sizeof(s));
	}
	return epdDrawStringAt(x, 0, s, &FontStreched72, COLORED);
}

//Инициализация одной памяти
void ePatternMemoryInit(){
	uint32_t i = 0;
	uint16_t cursor = 4;
	countPos = cursor;
	char s[100];

		//Цифры счетчика
		epdClear(UNCOLORED);
		epdDrawStringAt(0, 0, itoa(i++, s, 10), &Fontfont39pixel_h_digit, COLORED);
		epdSetPatternMemory(Paint.image, 0, cursor, Paint.width, Fontfont39pixel_h_digit.Height+5);

		//Линия разделения
		cursor += Fontfont39pixel_h_digit.Height+5;
		epdClear(UNCOLORED);
		epdDrawHorizontalLine(0, 0, 200, COLORED);
		epdSetPatternMemory(Paint.image, 0, cursor, Paint.width, 2);//Рисуем в памаяти epd
		
		//Состояние батареи
		cursor += 2;
		powerPosY = cursor;
		epdClear(UNCOLORED);
		memset(s, 0, sizeof(s));
		strlcpy(s, "батарея", sizeof(s));
		powerPosX = epdDrawStringAt(0, 0, s, &FontStreched72, COLORED);
		batShow(powerPosX);
		epdSetPatternMemory(Paint.image, 0, powerPosY, Paint.width, FontStreched72.Height+3);
		
		//Заголовок даты проверки
		cursor += FontStreched72.Height+3;
		sCheckBatPos = cursor;
		epdClear(UNCOLORED);
		memset(s, 0, sizeof(s));
		strlcpy(s, "проверено", sizeof(s));
		epdDrawStringAt(0, 0, s, &FontStreched72, COLORED);
		epdSetPatternMemory(Paint.image, 0, cursor, Paint.width, FontStreched72.Height+3);
		
		//Дата проверки
		cursor += FontStreched72.Height+3;
		dtCheckPos = cursor;
		dtShow(&dtCheckPower, 0);
		epdSetPatternMemory(Paint.image, 0, cursor, Paint.width, FontStreched72.Height+3);
		
		//Заголовок даты отправки
		cursor += FontStreched72.Height+3;
		sSendPos = cursor;		
		epdClear(UNCOLORED);
		memset(s, 0, sizeof(s));
		strlcpy(s, "передано", sizeof(s));
		epdDrawStringAt(0, 0, s, &FontStreched72, COLORED);
		epdSetPatternMemory(Paint.image, 0, cursor, Paint.width, FontStreched72.Height+3);
		
		//Дата отправки
		cursor += FontStreched72.Height+3;
		dtSendPos = cursor;
		epdClear(UNCOLORED);
		dtShow(&dtSend, 0);
		epdSetPatternMemory(Paint.image, 0, cursor, Paint.width, FontStreched72.Height+3);
}

void eInkInit(void){
	epdInit(lut_full_update);
	/**
	*  there are 2 memory areas embedded in the e-paper display
	*  and once the display is refreshed, the memory area will be auto-toggled,
	*  i.e. the next action of SetPatternMemory will set the other memory area
	*  therefore you have to clear the pattern memory twice.
	*/
	epdClearPatternMemory(0xff);// bit set = white, bit reset = black
	epdDisplayPattern();
	epdClearPatternMemory(0xff); //Очистка второй памяти
	epdDisplayPattern();
	
	Paint.rotate = ROTATE_0;
	Paint.width = 200;			//Холст для рисования для одной строки
	Paint.height = 80;
	
	if (epdImageInit() == EXIT_SUCCESS){
		
		ePatternMemoryInit();	//Первую заполнили
		epdDisplayPattern();	//out display
		ePatternMemoryInit();	//вторую заполнили
		epdDisplayPattern();	//out display
	}

}


inline int digitalShow(uint16_t value){
	//Выводим на место даты передачи на время отдладки
	char s[10], cnv[10];
	memset(s, 0, sizeof(s));
	strlcat(s, itoa(value, cnv, 10), sizeof(s));
	epdClear(UNCOLORED);//Очистить paint для рисования
	return epdDrawStringAt(0, 0, s, &FontStreched72, COLORED);
}

int main(void)
{

	MCUCR = (1<<JTD);//jtag off
	MCUCR = (1<<JTD);//обязательно 2 раза сразу!
	clock_prescale_set(clock_div_1);
/*	
	UCSR1B = (1<<TXEN1);
#define BAUD 19200
#include <util/setbaud.h>
	UBRR1 = UBRR_VALUE;
	#if USE_2X
	UCSR1A |= (1 << U2X1);
	#else
	UCSR1A &= ~(0 << U2X1);
	#endif
DDRD = (1<<PORTD7) | (1<<PORTD6) | (1<<PORTD5) | (1<<PORTD4);
PORTD |= (1<<PORTD7) | (1<<PORTD6) | (1<<PORTD5) | (1<<PORTD4);
*/
	PORTA |= (1<<NAMUR_PIN_STATE);//Подтянуть признак namur к 1
	if NAMUR_IS_OFF {
		//Тут настраиваем цфировой вход на прерывания
	}
	else{
		//SENSOR_ANALOG_CHANAL();
		POWER_MEASURE();
		ADCSRA = (1<<ADEN)		//Enable
		| (0<<ADSC)		//no start
		| (0<<ADATE)	//autotrigger off
		| (0<<ADIF)		//flag intrrupt reset
		| (1<<ADIE)		//Interrupt go
		| (0<<ADPS2) | (0<<ADPS1) | (0<<ADPS0) //prescaller 2
		;
		ADCSRB = (0<<ACME)		//Comparator off
		| (0<<ADTS2)	//Free mode
		| (0<<ADTS1)
		| (0<<ADTS0)
		;
	}
	
	//Отсчет времени
	ASSR = (1<<AS2);
	TCNT2 = 0;
	//TCCR2B = (1<<CS02) | (0<<CS01) | (1<<CS00);//clk/128 prescaller
	//TCCR2B = (1<<CS02) | (0<<CS01) | (0<<CS00);//clk/64 prescaller
	TCCR2B = (0<<CS02) | (1<<CS01) | (0<<CS00);//clk/8 prescaller
	//TCCR2B = (0<<CS02) | (0<<CS01) | (1<<CS00);//no prescaller
	TIMSK2 = (1<<TOIE2);	//interrupt overflow
	countMeasure = 0;
	spi_init(NULL);
	
	sei();
	SCREEN_POWER_ON();
	
	struct tm start_time;	//Начальное время
	start_time.tm_hour = 0;
	start_time.tm_min = 0;
	start_time.tm_sec = 0;
	start_time.tm_mday = 4;
	start_time.tm_mon = 5;
	start_time.tm_year = (2019-1900);
	
	set_system_time(mktime(&start_time));
	
//	eInkInit();
	
	uint32_t tmp;
	char s[30];

//	epdInit(lut_partial_update);
	while(1){
		if FIFO_IS_EMPTY(sendBuf){
			if (WIFI_IS_READY()){
				SPI_CS_SELECT();
				FIFO_PUSH(sendBuf, 0xaa);
				spi_send_start(NULL);
			}
		}
		if (!FIFO_IS_EMPTY(recivBuf)){
			FIFO_POP(recivBuf);
		}
continue;

		if (1){				//Надо вывести состояние
			//Заряд батареи
			epdClear(UNCOLORED);
			batShow(0);//слово "батарея" уже есть на экране, поэтому в буфер выводится от 0
			epdSetPatternMemory(Paint.image, powerPosX, powerPosY, Paint.width, FontStreched72.Height+3);
			
			//Цифры счетчика
#ifdef DEBUG
			tmp = countWater;
#elif
			tmp = countWater/100;
#endif
			if (tmp > COUNT_SHOW_MAX){
				tmp = COUNT_SHOW_MAX;
				epdClear(UNCOLORED);
				memset(s, 0, sizeof(s));
				strlcpy(s, "ПЕРЕПОЛНЕНИЕ", sizeof(s));
				epdDrawStringAt(0, 0, s, &FontStreched72, COLORED);
				epdSetPatternMemory(Paint.image, 0, sCheckBatPos, Paint.width, FontStreched72.Height+3);
			}
			epdClear(UNCOLORED);
			memset(s, 0, sizeof(s));
			epdDrawStringAt(0, 0, itoa(tmp, s, 10), &Fontfont39pixel_h_digit, COLORED);
			epdSetPatternMemory(Paint.image, 0, countPos, Paint.width, Fontfont39pixel_h_digit.Height+5);
			//Дата измерения батареи или авария датчика
			epdClear(UNCOLORED);
			switch (alarmSensor) {
				case SENSOR_ALARM_BREAK:
					epdDrawStringAt(0,0,"ОБРЫВ", &FontStreched72, COLORED);
					break;
				case SENSOR_ALARM_SHOT_CIR:
					epdDrawStringAt(0,0,"ЗАМЫКАНИЕ", &FontStreched72, COLORED);
					break;
				case SENSOR_ALARM_NO:
					//дата измерения батареи
					dtShow(&dtCheckPower, 0);
					break;					
				default:
					break;
			}
			epdSetPatternMemory(Paint.image, 0, dtCheckPos, Paint.width, FontStreched72.Height+3);

			//дата передачи
			epdClear(UNCOLORED);
			//dtShow(&dtSend, 0);
			digitalShow(preSensorValue);
			epdSetPatternMemory(Paint.image, 0, dtSendPos, Paint.width, FontStreched72.Height+3);

			epdDisplayPattern();	//out display
		}
		/*if (!IsRun){
			SCREEN_POWER_OFF();
			PWR_TIMER_ONLY();
			set_sleep_mode(SLEEP_MODE_PWR_SAVE);
			sleep_enable();
			sleep_cpu();
		}*/
	}
}
