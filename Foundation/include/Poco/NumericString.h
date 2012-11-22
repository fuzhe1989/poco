//
// NumericString.h
//
// $Id: //poco/1.4/Foundation/include/Poco/NumericString.h#1 $
//
// Library: Foundation
// Package: Core
// Module:  NumericString
//
// Numeric string utility functions.
//
// Copyright (c) 2004-2006, Applied Informatics Software Engineering GmbH.
// and Contributors.
//
// Permission is hereby granted, free of charge, to any person or organization
// obtaining a copy of the software and accompanying documentation covered by
// this license (the "Software") to use, reproduce, display, distribute,
// execute, and transmit the Software, and to prepare derivative works of the
// Software, and to permit third-parties to whom the Software is furnished to
// do so, all subject to the following:
// 
// The copyright notices in the Software and this entire statement, including
// the above license grant, this restriction and the following disclaimer,
// must be included in all copies of the Software, in whole or in part, and
// all derivative works of the Software, unless such copies or derivative
// works are solely in the form of machine-executable object code generated by
// a source language processor.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
// SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
// FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//


#ifndef Foundation_NumericString_INCLUDED
#define Foundation_NumericString_INCLUDED


#include "Poco/Foundation.h"
#include "Poco/Buffer.h"
#include "Poco/FPEnvironment.h"
#ifdef min
	#undef min
#endif
#ifdef max
	#undef max
#endif
#include <limits>
#include <cmath>
#if !defined(POCO_NO_LOCALE)
	#include <locale>
#endif

#define POCO_MAX_NUM_STRING_LEN 65


namespace Poco {


inline char decimalSeparator()
	/// Returns decimal separator from global locale or
	/// default '.' for platforms where locale is unavailable.
{
#if !defined(POCO_NO_LOCALE)
	return std::use_facet<std::numpunct<char> >(std::locale()).decimal_point();
#else
	return '.';
#endif
}


inline char thousandSeparator()
	/// Returns thousand separator from global locale or
	/// default ',' for platforms where locale is unavailable.
{
#if !defined(POCO_NO_LOCALE)
	return std::use_facet<std::numpunct<char> >(std::locale()).thousands_sep();
#else
	return ',';
#endif
}


//
// String to Number Conversions
//

template <typename I>
bool strToInt(const char* pStr, I& result, short base, char thSep = ',')
	/// Converts zero-terminated character array to integer number;
	/// Thousand separators are recognized for base10 and current locale;
	/// it is silently skipped but not verified for correct positioning.
	/// Function returns true if succesful. If parsing was unsuccesful,
	/// the return value is false with the result value undetermined.
{
	if (!pStr) return false;
	while (isspace(*pStr)) ++pStr;
	if (*pStr == '\0') return false;
	char sign = 1;
	if ((base == 10) && (*pStr == '-'))
	{
		sign = -1;
		++pStr;
	}
	else if (*pStr == '+') ++pStr;

	// parser states:
	const char STATE_SIGNIFICANT_DIGITS = 1;
	char state = 0;
	
	result = 0;
	I limitCheck = std::numeric_limits<I>::max() / base;
	for (; *pStr != '\0'; ++pStr)
	{
		switch (*pStr)
		{
		case 'x': case 'X': 
			if (base != 0x10) return false;

		case '0': 
			if (state < STATE_SIGNIFICANT_DIGITS) break;

		case '1': case '2': case '3': case '4':
		case '5': case '6': case '7':
			if (state < STATE_SIGNIFICANT_DIGITS) state = STATE_SIGNIFICANT_DIGITS;
			if (result > limitCheck) return false;
			result = result * base + (*pStr - '0');

			break;

		case '8': case '9':
			if ((base == 10) || (base == 0x10))
			{
				if (state < STATE_SIGNIFICANT_DIGITS) state = STATE_SIGNIFICANT_DIGITS;
				if (result > limitCheck) return false;
				result = result * base + (*pStr - '0');
			}
			else return false;

			break;

		case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
			if (base != 0x10) return false;
			if (state < STATE_SIGNIFICANT_DIGITS) state = STATE_SIGNIFICANT_DIGITS;
			if (result > limitCheck) return false;
			result = result * base + (10 + *pStr - 'a');

			break;

		case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
			if (base != 0x10) return false;

			if (state < STATE_SIGNIFICANT_DIGITS) state = STATE_SIGNIFICANT_DIGITS;
			if (result > limitCheck) return false;
			result = result * base + (10 + *pStr - 'A');

			break;

		case 'U':
		case 'u':
		case 'L':
		case 'l':
			goto done;

		case '.':
			if ((base == 10) && (thSep == '.')) break;
			else return false;

		case ',':
			if ((base == 10) && (thSep == ',')) break;
			else return false;

		case ' ':
			if ((base == 10) && (thSep == ' ')) break;
		case '\t':
		case '\n':
		case '\v':
		case '\f':
		case '\r':
			goto done;

		default:
			return false;
		}
	}

done:
	if ((sign < 0) && (base == 10)) result *= sign;

	return true;
}


template <typename I>
bool strToInt(const std::string& str, I& result, short base, char thSep = ',')
	/// Converts string to integer number;
	/// This is a wrapper function, for details see see the
	/// bool strToInt(const char*, I&, short, char) implementation.
{
	return strToInt(str.c_str(), result, base, thSep);
}


namespace Impl {

static char DUMMY_EXP_UNDERFLOW = 0; // dummy default val

}

template <typename F>
bool strToFloat (const char* pStr, F& result, char& eu = Impl::DUMMY_EXP_UNDERFLOW, char decSep = '.', char thSep = ',')
	/// Converts zero-terminated array to floating-point number;
	/// Returns true if succesful. Exponent underflow (i.e. loss of precision)
	/// is signalled in eu. Thousand separators are recognized for the locale
	/// and silently skipped but not verified for correct positioning.
	///
	/// If parsing was unsuccesful, the return value is false with
	/// result and eu values undetermined.
{
	poco_assert (decSep != thSep);

	if (pStr == 0 || *pStr == '\0') return false;

	// parser states:
	const char STATE_LEADING_SPACES = 0;
	const char STATE_DIGITS_BEFORE_DEC_POINT = 1;
	const char STATE_DIGITS_AFTER_DEC_POINT = 2;
	const char STATE_EXP_CHAR = 3;
	const char STATE_EXP_DIGITS = 4;
	const char STATE_SUFFIX = 5; // 'f' suffix

	char numSign = 1, expSign = 1;
	char state = STATE_LEADING_SPACES;
	F mantissa = 0.0, exponent = 0.0;
	F pow10 = 1.;
	result = 0.0;
	eu = 0;
	for (; *pStr != '\0'; ++pStr)
	{
		switch (*pStr)
		{
		case '.':
			if (decSep == '.')
			{
				if (state >= STATE_DIGITS_AFTER_DEC_POINT) return false;
				state = STATE_DIGITS_AFTER_DEC_POINT;
				break;
			}
			else if ((thSep == '.') && (state == STATE_DIGITS_BEFORE_DEC_POINT))
				break;
			else
				return false;

		case ',':
			if (decSep == ',')
			{
				if (state >= STATE_DIGITS_AFTER_DEC_POINT) return false;
				state = STATE_DIGITS_AFTER_DEC_POINT;
				break;
			}
			else if ((thSep == ',') && (state == STATE_DIGITS_BEFORE_DEC_POINT))
				break;
			else
				return false;

		case ' ': // space (SPC)
			if ((thSep == ' ') && (state == STATE_DIGITS_BEFORE_DEC_POINT)) break;
		case '\t': // horizontal tab (TAB)
		case '\n': // line feed (LF)
		case '\v': // vertical tab (VT)
		case '\f': // form feed (FF)
		case '\r': // carriage return (CR)
			if ((state >= STATE_DIGITS_AFTER_DEC_POINT) || (state >= STATE_EXP_DIGITS))
				break;
			else if ((state > STATE_LEADING_SPACES) && (state < STATE_DIGITS_AFTER_DEC_POINT))
				return false;
			break;

		case '-':
			if (state == STATE_LEADING_SPACES)
				numSign = -1;
			else if (state == STATE_EXP_CHAR) // exponential char
				expSign = -1;
			else return false;
		case '+':
			break;

		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			if (state >= STATE_SUFFIX) return false; // constant suffix
			if (state <= STATE_DIGITS_BEFORE_DEC_POINT) // integral part digits
			{
				result = result * 10 + (*pStr - '0');
				state = STATE_DIGITS_BEFORE_DEC_POINT;
			}
			else if (state <= STATE_DIGITS_AFTER_DEC_POINT) // fractional part digits
			{
				mantissa += (*pStr - '0') / (pow10 *= 10.);
				state = STATE_DIGITS_AFTER_DEC_POINT;
			}
			else if (state <= STATE_EXP_DIGITS) // exponent digits
			{
				exponent = exponent * 10 + (*pStr - '0');
				state = STATE_EXP_DIGITS;
			}
			else return false;
			break;

		case 'E':
		case 'e':
			if (state > STATE_DIGITS_AFTER_DEC_POINT) return false;
			state = STATE_EXP_CHAR;
			break;

		case 'F':
		case 'f':
			state = STATE_SUFFIX;
			break;

		default:
			return false;
		}
	}

	if (exponent > std::numeric_limits<F>::max_exponent10)
	{
		eu = expSign;
		exponent = std::numeric_limits<F>::max_exponent10;
	}

	result += mantissa;
	if (numSign != 1) result *= numSign;
	if (exponent > 1.0)
	{
		F scale = std::pow(10., exponent);
		result = (expSign < 0) ? (result / scale) : (result * scale);
	}

	return (state != STATE_LEADING_SPACES) && // empty/zero-length string
		!FPEnvironment::isInfinite(result) &&
		!FPEnvironment::isNaN(result);
}


template <typename F>
bool strToFloat (const std::string& s, F& result, char& eu = Impl::DUMMY_EXP_UNDERFLOW, char decSep = '.', char thSep = ',')
	/// Converts string to floating-point number;
	/// This is a wrapper function, for details see see the
	/// bool strToFloat(const char*, F&, char&, char, char) implementation.
{
	return strToFloat(s.c_str(), result, eu, decSep, thSep); 
}


//
// Number to String Conversions
//

namespace Impl {

	class Ptr
		/// Utility char pointer wrapper class.
		/// Class ensures increment/decrement remain within boundaries.
	{
	public:
		Ptr(char* ptr, unsigned offset): _beg(ptr), _cur(ptr), _end(ptr + offset)
		{
		}
	
		char*& operator ++ () // prefix
		{
			check(_cur + 1);
			return ++_cur;
		}

		char* operator ++ (int) // postfix
		{
			check(_cur + 1);
			char* tmp = _cur++;
			return tmp;
		}
	
		char*& operator -- () // prefix
		{
			check(_cur - 1);
			return --_cur;
		}

		char* operator -- (int) // postfix
		{
			check(_cur - 1);
			char* tmp = _cur--;
			return tmp;
		}

		char*& operator += (int incr)
		{
			check(_cur + incr);
			return _cur += incr;
		}

		char*& operator -= (int decr)
		{
			check(_cur - decr);
			return _cur -= decr;
		}

		operator char* () const
		{
			return _cur;
		}

		unsigned span() const
		{
			return _end - _beg;
		}

	private:
		void check(char* ptr)
		{
			if (ptr > _end) throw RangeException();
		}

		const char* _beg;
		char*       _cur;
		const char* _end;
};

} // namespace Impl


template <typename T>
bool intToStr(T value,
	unsigned short base,
	char* result,
	unsigned& size,
	bool prefix = false,
	int width = -1,
	char fill = ' ',
	char thSep = 0)
	/// Converts integer to string. Numeric bases from binary to hexadecimal are supported.
	/// If width is non-zero, it pads the return value with fill character to the specified width.
	/// When padding is zero character ('0'), it is prepended to the number itself; all other
	/// paddings are prepended to the formatted result with minus sign or base prefix included
	/// If prefix is true and base is octal or hexadecimal, respective prefix ('0' for octal, 
	/// "0x" for hexadecimal) is prepended. For all other bases, prefix argument is ignored.
	/// Formatted string has at least [width] total length.
{
	if (base < 2 || base > 0x10)
	{
		*result = '\0';
		return false;
	}

	Impl::Ptr ptr(result, size);
	int thCount = 0;
	T tmpVal;
	do
	{
		tmpVal = value;
		value /= base;
		*ptr++ = "FEDCBA9876543210123456789ABCDEF"[15 + (tmpVal - value * base)];
		if (thSep && (base == 10) && (++thCount == 3))
		{
			*ptr++ = thSep;
			thCount = 0;
		}
	} while (value);

	if ('0' == fill)
	{
		if (tmpVal < 0) --width;
		if (prefix && base == 010) --width;
		if (prefix && base == 0x10) width -= 2;
		while ((ptr - result) < width) *ptr++ = fill;
	}

	if (prefix && base == 010) *ptr++ = '0';
	else if (prefix && base == 0x10)
	{
		*ptr++ = 'x';
		*ptr++ = '0';
	}

	if (tmpVal < 0) *ptr++ = '-';

	if ('0' != fill)
	{
		while ((ptr - result) < width) *ptr++ = fill;
	}

	size = ptr - result;
	poco_assert_dbg (size <= ptr.span());
	poco_assert_dbg ((-1 == width) || (size >= width));
	*ptr-- = '\0';

	char* ptrr = result;
	char tmp;
	while(ptrr < ptr)
	{
		 tmp    = *ptr;
		*ptr--  = *ptrr;
		*ptrr++ = tmp;
	}

	return true;
}


template <typename T>
bool intToStr (T number, unsigned short base, std::string& result, bool prefix = false, int width = -1, char fill = ' ', char thSep = 0)
	/// Converts integer to string; This is a wrapper function, for details see see the
	/// bool intToStr(T, unsigned short, char*, int, int, char, char) implementation.
{
	char res[POCO_MAX_NUM_STRING_LEN] = {0};
	unsigned size = POCO_MAX_NUM_STRING_LEN;
	bool ret = intToStr(number, base, res, size, prefix, width, fill, thSep);
	result.assign(res, size);
	return ret;
}


} // namespace Poco


#endif // Foundation_NumericString_INCLUDED