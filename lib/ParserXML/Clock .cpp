#include "Clock.h"
#include "SharedParserFunctions.hpp"

extern char xml_buffer [MAX_XML_BUFF_LENGTH];
extern char crcbuff[10];
extern char calculated_crcbuff[10];

extern uint32_t uart_bufferPointer;
extern uint32_t xml_totalLength;
extern uint32_t new_xml_totalLength;
extern uint32_t xml_bufferPointer;

extern uint32_t gps_new_crc;
static uint16_t crc 					 = 0;
static uint16_t calculated_crc = 0;

extern bool uart_gpsdata_messageReady;

extern uint32_t tim7_user_period;
// extern TIM_HandleTypeDef htim13;
// extern TIM_HandleTypeDef htim7;
// extern TIM_HandleTypeDef htim6;
// extern TIM_HandleTypeDef htim1;

// extern UART_HandleTypeDef huart3;

//extern ProtocolParser s_parser;

// extern Temperature temperature_data;

std::array <uint16_t, 12> dawn_time = {
	8*60 + 20,
	7*60 + 29,
	6*60 + 18,
	5*60 + 8,
	3*60 + 55,
	3*60 + 20,
	3*60 + 40,
	4*60 + 33,
	5*60 + 32,
	6*60 + 30,
	7*60 + 40,
	8*60 + 24
}; //data in min 0 - jan, 11 - dec
std::array <uint16_t, 12> sunset_time = {
	16*60 + 48,
	17*60 + 49,
	18*60 + 45,
	19*60 + 52,
	21*60 + 2,
	21*60 + 45,
	21*60 + 35,
	20*60 + 24,
	19*60 + 1,
	17*60 + 47,
	16*60 + 42,
	16*60 + 20
}; //data in min 0 - jan, 11 - dec

std::string wdays1[7] = {
	"Воскресенье",
  "Понедельник",
  "Вторник",
  "Среда",
  "Четверг",
  "Пятница",
  "Суббота"
};

std::string wdays2[7] = {
	"Вс",
  "Пн",
  "Вт",
  "Ср",
  "Чт",
  "Пт",
  "Сб"
};

std::string wdays3[7] = {
	"Sunday",
  "Monday",
  "Tuesday",
  "Wednesday",
  "Thursday",
  "Friday",
  "Saturday"
};

std::string wdays4[7] = {
	"Sun",
	"Mon",
  "Tue",
  "Wed",
  "Thu",
  "Fri",
  "Sat"
};

std::string months1[12] = {
	"Янв",
	"Фев",
	"Мар",
	"Апр",
	"Мая",
	"Июн",
	"Июл",
	"Авг",
	"Сен",
	"Окт",
	"Ноя",
	"Дек"
};

std::string months2[12] = {
	"Января",
	"Февраля",
	"Марта",
	"Апреля",
	"Мая",
	"Июня",
	"Июля",
	"Августа",
	"Сентября",
	"Октября",
	"Ноября",
	"Декабря"
};

std::string months3[12] = {
	"Jan",
	"Feb",
	"Mar",
	"Apr",
	"May",
	"Jun",
	"Jul",
	"Aug",
	"Sep",
	"Oct",
	"Nov",
	"Dec"
};

std::string months4[12] = {
	"January",
	"February",
	"March",
	"April",
	"May",
	"June",
	"July",
	"August",
	"September",
	"October",
	"November",
	"December"
};

bool Clock::parseMessageIfReady()
{
	
	if(!uart_gpsdata_messageReady)
		return false;
	
	/*
	if(!bs_board_ready_to_parse) // && timeout < timeoutmax
		return false;
	*/
	
	// calculated_crc = calcCRC(xml_buffer, new_xml_totalLength);
	sprintf(calculated_crcbuff, "%04X", calculated_crc); //debug
	
	//sprintf(output, "B under crc = %d, total B = %d, B after crc = %d", crc_data, new_xml_totalLength, new_xml_totalLength - crc_data); //debug
	
	if(calculated_crc == gps_new_crc) {
		
		xml_bufferPointer  = 0; //reset size
		uart_bufferPointer = 0;
		
		if((new_xml_totalLength == xml_totalLength) && (gps_new_crc == crc)) {
			uart_gpsdata_messageReady = false;
			
			// #ifdef LOGGING
			// log_rs_gps_new_message(1, new_xml_totalLength, calculated_crc);
			// #endif
			
			// huart3.RxISR = uart3RxInterruptHandler;
			// SET_BIT((huart3.Instance)->CR1, USART_CR1_RXNEIE_RXFNEIE);

			return false;
		}
		else {
			crc = gps_new_crc;
			xml_totalLength = new_xml_totalLength;
		}
	}
	else {
		uart_gpsdata_messageReady = false;
		
		#ifdef LOGGING
		log_rs_gps_new_message(0, new_xml_totalLength, calculated_crc);
		#endif
		
		// huart3.RxISR = uart3RxInterruptHandler;
		// SET_BIT((huart3.Instance)->CR1, USART_CR1_RXNEIE_RXFNEIE);
		
		return false;
	}
	
	#ifdef LOGGING
	log_rs_gps_new_message(2, new_xml_totalLength, calculated_crc);
	#endif
	
	bool result = parseBuffer(xml_buffer, xml_totalLength);
	
	#ifdef LOGGING
	log_rs_gps_parsed(result);
	#endif

	xml_bufferPointer  = 0; //reset size
	uart_bufferPointer = 0;
	memset(xml_buffer, 0, MAX_XML_BUFF_LENGTH);
	
	//matrix.clear();
	
	// __HAL_UART_CLEAR_FLAG(&huart3, UART_CLEAR_OREF);

	uart_gpsdata_messageReady = false;
	
	// huart3.RxISR = uart3RxInterruptHandler;
	// SET_BIT((huart3.Instance)->CR1, USART_CR1_RXNEIE_RXFNEIE);
	
	return result;
}

bool Clock::parseBuffer(char * buffer, uint32_t length)
{
	Buf_t        xmlBuf;
	bool         result;
	bool         addressMatched = false;
	bool         addressTagsAdded = false;

	
	int tmp = 0;
	double temp_speed = 0;
	int	 timezone = 0;
	
	unsigned char  sec	 = 0; 	//0..59
	unsigned char  min	 = 0; 	//0..59
	unsigned char  hour  = 0; 	//0..23
	unsigned char  day	 = 0; 	//1..31
	unsigned char  month = 0; 	//1..12
	unsigned short year  = 0; 	//1..2999

	// Init parameters for checksum calculation
    
	auto bufferEnd = buffer + length;
	
	while (reinterpret_cast<uint32_t>(buffer) < reinterpret_cast<uint32_t>(bufferEnd))
	{
		// Get line from buffer
		if (!extractLineAndParseXml(&buffer, bufferEnd, xmlBuf))
			return false;

		if (xmlBuf.tag == "gps_data")
		{
			// general tag
			continue;
		}

		// Stop processing on the end tag
		if (xmlBuf.end_tag == "gps_data") {
			
			if(2020 <= year && year <= 2060) {
				// clock.SetFullTimeGPS(hour, min, sec, timezone, day, month, year);
				return true;
			}
			else
				return false;
			
		}
		
		// Here starts specific device parameters parsing
		if (xmlBuf.tag == "gmt") {
			timezone = static_cast<int>(strtol(xmlBuf.value.c_str(), nullptr, 10));
		}
		else if (xmlBuf.tag == "time") {
			tmp = static_cast<int>(strtoul(xmlBuf.value.c_str(), nullptr, 10));
			sec  = (tmp % 100);
			min  = (tmp / 100) % 100;
			hour = (tmp / 10000);
		}
		else if (xmlBuf.tag == "date") {
			tmp = static_cast<int>(strtoul(xmlBuf.value.c_str(), nullptr, 10));
			day  = tmp / 10000;
			month  = (tmp / 100) % 100;
			year = 2000 + (tmp % 100); //2000 is added cause yy
			
			
		}
		else if (xmlBuf.tag == "lat") {
			//skip
		}
		else if (xmlBuf.tag == "lon") {
			//skip
		}
		else if (xmlBuf.tag == "speed") {
			temp_speed = strtod(xmlBuf.value.c_str(), NULL);
			
			c_speed = (int)(temp_speed*1.852);
		}
		else if (xmlBuf.tag == "temp1") {
			
			if(xmlBuf.value[0] != 'N') {

				// temperature_data.temp1 = static_cast<int>(strtoul(xmlBuf.value.c_str(), nullptr, 10));
			}
		}
		else if (xmlBuf.tag == "temp2") {
			if(xmlBuf.value[0] != 'N') {
				
				// temperature_data.temp2 = static_cast<int>(strtoul(xmlBuf.value.c_str(), nullptr, 10));
			}
		}
		else if (xmlBuf.tag == "auxtext1") {
			auxtext1 = xmlBuf.value;
		}
			
	}
	// If processor ever reach this place - processing got error
	c_gps_sync = false;
	return false;
}


unsigned char weekday(unsigned short year, unsigned char month, unsigned char day) {
	if (month < 3u) {
			--year;
			month += 10u;
		} else
			month -= 2u;
		return (unsigned char)((day + 31u * month / 12u + year + year / 4u - year / 100u + year / 400u) % 7u);
}

Clock::Clock()
{
	c_gps_sync = false;
	
	c_speed = 0;
	
	c_sec 	= 0;
	c_min 	= 0;
	c_hour 	= 0;
	c_day 	= 1;
	c_month = 1;
	c_year 	= 2020;
	c_wday  = weekday(c_year, c_month ,c_day);
}

Clock::Clock(unsigned char sec, unsigned char min,	unsigned char hour, 
						 unsigned char day, unsigned char month, unsigned short year, int timezone) 
{
	c_gps_sync = false;
	
	c_sec 	= sec;
	c_min 	= min;
	c_hour 	= hour;
	c_day 	= day;
	c_month = month;
	c_year 	= year;
	c_wday  = weekday(year, month ,day);
	c_timezone = timezone;
}

Clock::Clock(const Clock &cl)
{
	c_sec 	= cl.c_sec;
	c_min 	= cl.c_min;
	c_hour 	= cl.c_hour;
	c_day 	= cl.c_day;
	c_month = cl.c_month;
	c_year 	= cl.c_year;
	c_wday  = cl.c_wday;
	c_timezone = cl.c_timezone;
}

//will be called in 1 sec timer to maintain time counting
void Clock::IncrementSec()
{
	c_sec++;
	if(c_sec >= 60) {
		c_sec = 0;
		IncrementMin();
	}
}
void Clock::IncrementMin()
{
	c_min++;
	if(c_min >= 60) {
		c_min = 0;
		IncrementHour();
	}
}
void Clock::IncrementHour()
{
	c_hour++;
	if(c_hour >= 24) {
		c_hour = 0;
		IncrementDay();
		c_wday = weekday(c_year, c_month ,c_day);
	}
}
void Clock::IncrementDay()
{
	unsigned char day_limit;
	
	switch(c_month)
	{
		case 1:
				// January
				day_limit = 31;
		break;
		case 2:
				// February
				if( (c_year % 100 == 0) && (c_year % 400 == 0) )
				{
						day_limit = 29;
				}
				else if(c_year % 4 == 0)
				{
						day_limit = 29;
				}
				else
				{
						day_limit = 28;
				}
		break;
		case 3:
				// March
				day_limit = 31;
		break;
		case 4:
				// April
				day_limit = 30;
		case 5:
				// May
				day_limit = 31;
		break;
		case 6:
				// June
				day_limit = 30;
		break;
		case 7:
				// July
				day_limit = 31;
		break;
		case 8:
				// August
				day_limit = 31;
		break;
		case 9:
				// September
				day_limit = 30;
		break;
		case 10:
				// October
				day_limit = 31;
		break;
		case 11:
				// November
				day_limit = 30;
		break;
		case 12:
				// December
				day_limit = 31;
		break;
		default:
		break;
	}
	
	c_day++;
	if(c_day > day_limit)
	{
		c_day = 1;
		IncrementMonth();
	}
}
void Clock::IncrementMonth()
{
	c_month++;
	if(c_month > 12) {
		c_month = 1;
		c_year++;
	}
}

void Clock::DecrementHour()
{
	if(c_hour == 0)
	{
		c_hour = 23;
		DecrementDay();
		c_wday = weekday(c_year, c_month ,c_day);
	}
	else --c_hour;
}
void Clock::DecrementDay()
{
	unsigned char day_limit;
	
	if(c_day == 1)
	{
		DecrementMonth();
		switch(c_month)
		{
			case 1:
					// January
					day_limit = 31;
			break;
			case 2:
					// February
					if( (c_year % 100 == 0) && (c_year % 400 == 0) )
					{
							day_limit = 29;
					}
					else if(c_year % 4 == 0)
					{
							day_limit = 29;
					}
					else
					{
							day_limit = 28;
					}
			break;
			case 3:
					// March
					day_limit = 31;
			break;
			case 4:
					// April
					day_limit = 30;
			case 5:
					// May
					day_limit = 31;
			break;
			case 6:
					// June
					day_limit = 30;
			break;
			case 7:
					// July
					day_limit = 31;
			break;
			case 8:
					// August
					day_limit = 31;
			break;
			case 9:
					// September
					day_limit = 30;
			break;
			case 10:
					// October
					day_limit = 31;
			break;
			case 11:
					// November
					day_limit = 30;
			break;
			case 12:
					// December
					day_limit = 31;
			break;
			default:
			break;
		}
		c_day = day_limit;
	}
	else --c_day;
}
void Clock::DecrementMonth()
{
	if(c_month == 1)
	{
		c_month = 12;
		--c_year;
	}
	else --c_month;
}


void Clock::SetTime(unsigned char hour, unsigned char min) //seconds are set to 0
{
	if(hour < 24)
		c_hour = hour;
	if(min < 60)
		c_min = min;
	
	c_sec = 0;
}

void Clock::SetTime(unsigned char hour, unsigned char min, unsigned char sec) //wday will be calculated on it's own
{
	
	if(hour < 24)
		c_hour = hour;
	if(min < 60)
		c_min = min;
	if(sec < 60)
		c_sec = sec;
}

void Clock::SetTime(unsigned char hour, unsigned char min, unsigned char sec, int timezone)
{
	if(hour < 24)
		c_hour = hour;
	if(min < 60)
		c_min = min;
	if(sec < 60)
		c_sec = sec;
	
	if(timezone == 0) return;
	if((timezone > 12) || (timezone < -12)) return;
	if(timezone > 0) {
		for(int i = 0; i < timezone; i++)
			IncrementHour();
	}
	else {
		for(int i = 0; i < timezone * (-1); i++)
			DecrementHour();
	}
	
	c_timezone = timezone;
}

void Clock::SetDate(unsigned char day, unsigned char month, unsigned short year)
{
	unsigned char day_limit;
	
	if(year < 1900) 
		c_year = 1900;
	else 
	if(year > 2100)
		c_year = 1900;
	else
		c_year = year;
	
	if(1 <= month && month <= 12)
		c_month = month;
	else
		c_month = 1;
		
	switch(c_month)
	{
		case 1:
				// January
				day_limit = 31;
		break;
		case 2:
				// February
				if( (c_year % 100 == 0) && (c_year % 400 == 0) )
				{
						day_limit = 29;
				}
				else if(c_year % 4 == 0)
				{
						day_limit = 29;
				}
				else
				{
						day_limit = 28;
				}
		break;
		case 3:
				// March
				day_limit = 31;
		break;
		case 4:
				// April
				day_limit = 30;
		case 5:
				// May
				day_limit = 31;
		break;
		case 6:
				// June
				day_limit = 30;
		break;
		case 7:
				// July
				day_limit = 31;
		break;
		case 8:
				// August
				day_limit = 31;
		break;
		case 9:
				// September
				day_limit = 30;
		break;
		case 10:
				// October
				day_limit = 31;
		break;
		case 11:
				// November
				day_limit = 30;
		break;
		case 12:
				// December
				day_limit = 31;
		break;
		default:
		break;
	}

	if(1 <= day && day <= day_limit)
		c_day = day;
	else
		c_day = 1;
	
	c_wday = weekday(c_year, c_month ,c_day);
}

void Clock::SetSpeed(int speed)
{
	c_speed = speed;
}

void Clock::SetFullTime(unsigned char hour, unsigned char min, unsigned char sec, int timezone, 
									 unsigned char day, unsigned char month, unsigned short year)
{
	SetDate(day, month, year);
	SetTime(hour, min, sec, timezone);
}

void Clock::SetFullTimeGPS(unsigned char hour, unsigned char min, unsigned char sec, int timezone, 
									 unsigned char day, unsigned char month, unsigned short year)
{
	SetDate(day, month, year);
	SetTime(hour, min, sec, timezone);
	
	c_gps_sync = true;
}

//return a pointer to multibyte string (utf-16) of the from hh:mm or whatever depends on arg	
void Clock::GetTime(char* str)
{
	sprintf(str, "%02d:%02d", c_hour, c_min);
}

//return a pointer to multibyte string (utf-16) of the from dd.mm.yy or whatever depends on arg
void Clock::GetDate(DateFormat format, char* str)
{
	switch(format) 
	{
		case DateFormat::Ru :
		{
			sprintf(str, "%02d.%02d.%02d", c_day, c_month, c_year % 100);
			break;
		}
		case DateFormat::UK :
		{
			sprintf(str, "%02d/%02d/%02d", c_day, c_month, c_year % 100);
			break;
		}
		case DateFormat::EUMain :
		{
			sprintf(str, "%02d-%02d-%02d", c_day, c_month, c_year % 100);
			break;
		}
		case DateFormat::USA :
		{
			sprintf(str, "%02d/%02d/%02d", c_month ,c_day, c_year % 100);
			break;
		}
		case DateFormat::EUWeird :
		{
			sprintf(str, "%02d.%02d.%02d", c_year % 100, c_month, c_day);
			break;
		}
	}
}

void Clock::GetWday(WdayFormat format, char* str) //return a pointer to multibyte string (utf-8) of the form dd.mm.yy or whatever depends on arg
{
	switch(format) 
	{
		case WdayFormat::RuFull :
		{
			strcpy(str, wdays1[c_wday].c_str());
			break;
		}
		case WdayFormat::RuShort :
		{
			strcpy(str, wdays2[c_wday].c_str());
			break;
		}
		case WdayFormat::EnFull :
		{
			strcpy(str, wdays3[c_wday].c_str());
			break;
		}
		case WdayFormat::EnShort :
		{
			strcpy(str, wdays4[c_wday].c_str());
			break;
		}
	}
}	

void Clock::GetDay(DayFormat format, char* str) //return a pointer to multibyte string (utf-8) of the form dd or whatever depends on arg
{
	switch(format) 
	{
		case DayFormat::TwoDigit :
		{
			sprintf(str, "%02d", c_day);
			break;
		}
		case DayFormat::OneDigit :
		{
			sprintf(str, "%d", c_day);
			break;
		}
	}
}

void Clock::GetMonth(MonthFormat format, char* str) //return a pointer to multibyte string (utf-8) of the form mm or whatever depends on arg
{
	switch(format) 
	{
		case MonthFormat::Numeric :
		{
			sprintf(str, "%02d", c_month);
			break;
		}
		case MonthFormat::RuShort :
		{
			strcpy(str, months1[c_month - 1].c_str());
			break;
		}
		case MonthFormat::RuFull :
		{
			strcpy(str, months2[c_month - 1].c_str());
			break;
		}
		case MonthFormat::EnFull :
		{
			strcpy(str, months3[c_month - 1].c_str());
			break;
		}
		case MonthFormat::EnShort :
		{
			strcpy(str, months4[c_month - 1].c_str());
			break;
		}
	}
}

void Clock::GetYear(YearFormat format, char* str) //return a pointer to multibyte string (utf-8) of the form mm or whatever depends on arg
{
	switch(format) 
	{
		case YearFormat::TwoDigit :
		{
			sprintf(str, "%02d", c_year % 100);
			break;
		}
		case YearFormat::Full :
		{
			sprintf(str, "%d", c_year);
			break;
		}
	}
}

void Clock::GetSpeed(char* str)
{
	sprintf(str, "%d", c_speed);
}

bool Clock::GetGpsSyncStatus()
{
	return c_gps_sync;
}

bool Clock::operator != (const Clock& clock)
{
	bool res = false;
	
	if(this->c_sec != clock.c_sec || this->c_min != clock.c_min || this->c_hour != clock.c_hour ||
		 this->c_day != clock.c_day || this->c_month != clock.c_month || this->c_year != clock.c_year ||
		 this->c_wday != clock.c_wday || this->c_timezone != clock.c_timezone)
		res = true;

	return res;
}

DynamicColorMode Clock::GetColorMode()
{
	uint16_t current_time_min = c_hour * 60 + c_min;
	DynamicColorMode mode;
	if(c_gps_sync) {
		if(dawn_time[c_month - 1] <= current_time_min && current_time_min <= sunset_time[c_month - 1])
			mode = DynamicColorMode::day;
		else 
			mode = DynamicColorMode::night;
	}
	else
		mode = DynamicColorMode::not_stated;
	
	return mode;
	
}

void Clock::GetAuxText(char* str)
{
	strcpy(str, auxtext1.c_str());
}

void Clock::SetGpsSyncStatus(bool status) //if we get time from other protocol
{
	c_gps_sync = status;
}

/*

int Clock::replaceXmlSpecialCharactersIsWideString(char * inputString, uint32_t inputStringLength)
{ 
	//use iSL
	int 		ret_val; //return length of the new xml string in case all characters are valid
	bool    valid_xml_character_flag = true;
	size_t  length = inputStringLength; //just for testing purposes
	char 		newStr[length];
	
	memset(newStr, 0, length);

	size_t  newStrPtr = 0;
	for (size_t i = 0; i < length; ++i)
	{
		if (inputString[i] == '&')
		{
			// Special character is found
			if ((i + 2) >= length)
			{
				#ifdef DEBUG_PARSER
					printf("End of string on & char\n");
				#endif // DEBUG_PARSER
				break;
			}

			if ((inputString[i + 1] == 'l') && (inputString[i + 2] == 't') && (inputString[i + 3] == ';'))
			{
				newStr[newStrPtr] = '<';
				i += 3;
			}
			else if ((inputString[i + 1] == 'g') && (inputString[i + 2] == 't') && (inputString[i + 3] == ';'))
			{
				newStr[newStrPtr] = '>';
				i += 3;
			}
			else if (((i + 3) < length) && (inputString[i + 1] == 'a') && (inputString[i + 2] == 'm') && (inputString[i + 3] == 'p') && (inputString[i + 4] == ';'))
			{
				newStr[newStrPtr] = '&';
				i += 4;
			}
			else if (((i + 4) < length) && (inputString[i + 1] == 'a') && (inputString[i + 2] == 'p') && (inputString[i + 3] == 'o') && (inputString[i + 4] == 's') && (inputString[i + 5] == ';'))
			{
				newStr[newStrPtr] = '\'';
				i += 5;
			}
			else if (((i + 4) < length) && (inputString[i + 1] == 'q') && (inputString[i + 2] == 'u') && (inputString[i + 3] == 'o') && (inputString[i + 4] == 't') && (inputString[i + 5] == ';'))
			{
				newStr[newStrPtr] = '\"';
				i += 5;
			}
			else
				valid_xml_character_flag = false; // Wrong special character
		}
		else
		{
			newStr[newStrPtr] = inputString[i];
		}
		
		newStrPtr++;
	}

	newStr[newStrPtr] = '\0';

	strcpy(inputString, newStr);

	if(valid_xml_character_flag)
		ret_val = newStrPtr + 1;
	else
		ret_val = 0;
	
	return ret_val;
}

Buf_t Clock::parseXmlLikeString(const char * inputWideString)
{
		static char  str[3000];
	
    Buf_t        buf;
    char	       smallbuf[2] = {0};
    size_t       input_length = strlen(inputWideString);
		int			 		 length = 0;
    ParsingState state  = ParsingState::pre_start_element;
		
		memset(str, 0, 3000);
		
		for(uint16_t i = 0; i < input_length; ++i)
			str[i] = inputWideString[i];
		
		length = replaceXmlSpecialCharactersIsWideString(str, input_length);
		
		if(!length)
    {
			return buf;
    }

    buf.parsed = false;
		
    for (size_t i = 0; i < length; ++i)
    {
        switch (state)
        {
        case ParsingState::pre_start_element:
            if (str[i] != open_bracket)
            {
                continue;
            }
						
						if (str[i + 1] == xml_slash)
            {
								state = ParsingState::end_element; //i++?
								i++;
            }
						else
								state = ParsingState::start_element;
						
            break;
        case ParsingState::start_element:
            if (str[i] == sign_space)
            {
                state = ParsingState::attribute;
            }
            else if (str[i] == close_bracket)
            {
                state = ParsingState::text_element;
            }
            else
            {
                smallbuf[0] = str[i];
                buf.tag.append(smallbuf);
            }
            break;
        case ParsingState::attribute:
            if (str[i] == sign_equal)
            {
                ++i;
                // If string is broken - return what we've collected, parsed field won't be true at this point
                if (i >= length)
                    return buf;

                if (str[i] == sign_double_quote)
                    state = ParsingState::attribute_value;
                else
                    return buf;
                // We can handle spaces if there is any requirement
            }
            else if (str[i] == close_bracket)
            {
                state = ParsingState::text_element;
            }
            else
            {
                smallbuf[0] = str[i];
                buf.attribute.append(smallbuf);
            }
            break;
        case ParsingState::attribute_value:
            if (str[i] == sign_double_quote)
            {
                ++i;

                if (i >= length)
                    return buf;

                if (str[i] == close_bracket)
                    state = ParsingState::text_element;
                else
                    return buf;

                // We can handle spaces if there is any requirement
            }
            else
            {
                smallbuf[0] = str[i];
                buf.attribute_value.append(smallbuf);
            }
            break;
        case ParsingState::text_element:
            if (str[i] == open_bracket)
            {
                ++i;

                if (i >= length)
                    return buf;

                if (str[i] == xml_slash)
                {
                    state = ParsingState::end_element;
                    continue;
                }

                --i;
            }

            smallbuf[0] = str[i];
            buf.value.append(smallbuf);

            break;
        case ParsingState::end_element:
            if ((str[i] == close_bracket) || (str[i] == 0x000D) || (str[i] == 0x000A))
            {
                break;
            }
            else
            {
                smallbuf[0] = str[i];
                buf.end_tag.append(smallbuf);
            }
            break;
        }
    }

    buf.parsed = true;
    return buf;
}

bool Clock::extractWString(char ** bufferStart, char * bufferEnd, std::string & string)
{
	while (*bufferStart != bufferEnd)
    {            
        auto symbol = **bufferStart;
        
        *bufferStart += 1;
        
        string.push_back(symbol);
        
        if (symbol == static_cast<wchar_t>(0xa))
            return true;
    }

	return false;
}

bool Clock::extractLineAndParseXml(char ** bufferStart, char * bufferEnd, Buf_t & xmlBuf)
{
	std::string string;
	auto result = extractWString(bufferStart, bufferEnd, string);
	
	if (!result)
		return false;
	
	xmlBuf = parseXmlLikeString(string.c_str());
	
	return true;
}
*/