//using this to store and increase(via timer) time and date in class if gps down for some time
#ifndef CLOCK_H
#define CLOCK_H

#include <Arduino.h>

#define MAX_XML_BUFF_LENGTH  1024

// #include <stdint.h>
// #include <array>
// #include <numeric>
// #include <string>

//defines

//$date - dd.mm.yy
//$date1 - dd/mm/yy
//$date2 - dd-mm-yy
//$date3 - mm/dd/yy
//$date4 - yy-mm-dd

enum class DynamicColorMode {
	not_stated,
	day,
	night
};

enum class DateFormat {
	Ru,
	UK,
	EUMain,
	USA,
	EUWeird
};


//$day1 - "Понедельник / Вторник / Среда / Четверг / Пятница / Суббота / Воскресенье"  -->
//$day2 - "Пн, Вт, Ср, Чт, Пт, Сб, Вс"   -->
//$day3 - "Monday / Tuesday / Wednesday / Thursday / Friday / Saturday / Sunday"   -->
//$day4 - "Mon, Tue, Wed, Thu, Fri, Sat, Sun"     -->

enum class WdayFormat {
	RuFull,
	RuShort,
	EnFull,
	EnShort
};


//$dd - dd день, например 01/02/03/04/21/31 -->
//TwoDigit type e.g. 02, 05, 07, 22
//OneDigit won't put 0 befor 1 digit days e.g. 1, 3, 9, 11, 30
enum class DayFormat {
	TwoDigit,
	OneDigit 
};


//$mm - mm месяц, например 01/02/03/04/12  -->
//$mm1 - рус. 3 буквы  Янв/Фев/Мар/Апр/Мая/Июн/Июл/Авг/Сен/Окт/Ноя/Дек  -->
//$mm2 - рус. полные месяцы Января/Февраля/Марта/Апреля/Мая/Июня/Июля/Августа/Сентября/Октября/Ноября/Декабря   -->
//$mm3 - англ. 3 буквы Jan/Feb/Mar/Apr/May/Jun/Jul/Aug/Sep/Oct/Nov/Dec  -->
//$mm4 - англ. полные месяцы January/February/March/April/May/June/July/August/September/Oct/November/December  -->

enum class MonthFormat {
	Numeric,
	RuShort,
	RuFull,
	EnShort,
	EnFull
};


//$yy - год из 2-х последних цифр -->
//$yyyy - год из 4-х цифр -->

enum class YearFormat {
	TwoDigit,
	Full
};

class Clock {
	
	bool c_gps_sync;
	
	unsigned char  c_sec; 	//0..59
	unsigned char  c_min; 	//0..59
	unsigned char  c_hour; 	//0..23
	unsigned char  c_day; 	//1..31
	unsigned char  c_wday; 	//0..6 sat sun mon etc.
	unsigned char  c_month; //1..12
	unsigned short c_year; 	//1..2999
	int  					 c_timezone; //-12..12
	
	int 					 c_speed; //km/h
	
	std::string  	 auxtext1;

	void IncrementMin();
	void IncrementHour();
	void IncrementDay();
	void IncrementMonth();
	
	//those required to apply timezone (e.g. -6)
	void DecrementHour();
	void DecrementDay();
	void DecrementMonth();
	
	
	
	void parseParameters(char & string);
  bool parseBuffer(char * buffer, uint32_t length);
	
	//Buf_t parseXmlString(const char * str);
	//Buf_t parseXmlLikeString(const char * str);
	//bool extractWString(char ** bufferStart, char * bufferEnd, std::string & string);
	//bool extractLineAndParseXml(char ** bufferStart, char * bufferEnd, Buf_t & xmlBuf);
	//int replaceXmlSpecialCharactersIsWideString(char * inputString, uint32_t inputStringLength);

	public:
	
	Clock();
	
	Clock(unsigned char sec, unsigned char min,	unsigned char hour, 
				unsigned char day, unsigned char month, unsigned short year, int timezone);
	
	Clock(const Clock &c);
	
	bool parseMessageIfReady();

	void GetTime(char* str);//printing string of the form hh:mm to str
	void GetDate(DateFormat format, char* str); //printing multibyte string (utf-8) of the form dd.mm.yy or whatever depends on arg
	void GetWday(WdayFormat format, char* str); //printing multibyte string (utf-8) of the form dd.mm.yy or whatever depends on arg
	void GetDay(DayFormat format, char* str); //printing multibyte string (utf-8) of the form dd or whatever depends on arg
	void GetMonth(MonthFormat format, char* str); //printing multibyte string (utf-8) of the form mm or whatever depends on arg
	void GetYear(YearFormat format, char* str); //printing multibyte string (utf-8) of the form mm or whatever depends on arg
	bool GetGpsSyncStatus();
	void GetSpeed(char* str);
	void GetAuxText(char* str);
	
	DynamicColorMode GetColorMode();
	
	void IncrementSec();	//will be called in 1 sec timer to maintain time counting
	//void 	AddSeconds(int amount); 
	
	bool operator != (const Clock& clock);
	
	void SetTime(unsigned char hour, unsigned char min); //seconds are set to 0
	void SetTime(unsigned char hour, unsigned char min, unsigned char sec); //wday will be calculated on it's own
	void SetTime(unsigned char hour, unsigned char min, unsigned char sec, int timezone); //wday will be calculated on it's own
	void SetDate(unsigned char day, unsigned char month, unsigned short year);
	
	void SetSpeed(int speed);
	
	//hh mm ss dd mm yy zz
	void SetFullTime(unsigned char hour, unsigned char min, unsigned char sec, int timezone, 
									 unsigned char day, unsigned char month, unsigned short year);
	void SetFullTimeGPS(unsigned char hour, unsigned char min, unsigned char sec, int timezone, 
									 unsigned char day, unsigned char month, unsigned short year);
	
	void SetGpsSyncStatus(bool status); //if we get time from other protocol
	
};

#endif //CLOCK_HPP

