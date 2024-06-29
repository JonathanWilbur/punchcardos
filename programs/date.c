/* Time handling functions in this library come from timelib. See
timelib.license.txt for the license. Any code in here written by me is released
under an MIT license. */
#ifndef NOLIBC
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <ctype.h>
#include <sys/stat.h>
#include <string.h>
#endif

#ifdef __GLIBC__

char* itoa (long in) {
    char* bufp = malloc(100);
    if (bufp == NULL) {
        return "0";
    }
    sprintf(bufp, "%ld", in);
    return bufp;
}

#endif

typedef int64_t     timelib_sll;
typedef uint64_t    timelib_ull;

#define MINS_PER_HOUR     60
#define SECS_PER_ERA   TIMELIB_LL_CONST(12622780800)
#define SECS_PER_DAY   86400
#define SECS_PER_HOUR   3600
#define USECS_PER_HOUR TIMELIB_LL_CONST(3600000000)

#define DAYS_PER_WEEK      7
#define DAYS_PER_YEAR    365
#define DAYS_PER_LYEAR   366
#define MONTHS_PER_YEAR   12
/* 400*365 days + 97 leap days */
#define DAYS_PER_ERA  146097
#define YEARS_PER_ERA    400
#define HINNANT_EPOCH_SHIFT 719468 /* 0000-03-01 instead of 1970-01-01 */
#define timelib_is_leap(y) ((y) % 4 == 0 && ((y) % 100 != 0 || (y) % 400 == 0))

const char* USAGE_MSG = "Usage: date [OPTION]... [+FORMAT]";
const char* VERSION = "1.0.0";
const char* COPYRIGHT_MSG = "Copyright (c) 2024 Jonathan M. Wilbur";
const char* LICENSE_MSG = "Released under an MIT License";

const char* FMT_EN_US = "%a %e %b %Y %T %p %Z";
const char* FMT_DATE = "%m/%d/%y";
const char* FMT_FULL_DATE = "%+4Y-%m-%d";
const char* FMT_LOCALE_HOUR12_CLOCK = "%I:%M:%S %p"; // e.g., 11:11:04 PM
const char* FMT_HOUR24_AND_MINUTE = "%H:%M";
const char* FMT_TIME = "%H:%M:%S";
const char* FMT_LOCALE_DATE = "%m/%d/%y"; // e.g., 12/31/99
const char* FMT_ASN1_UTCTIME = "%y%m%d%H%I%sZ"; // "YYMMDDhhmm[ss]Z" or "YYMMDDhhmm[ss](+|-)hhmm"
const char* FMT_ASN1_GENTIME = "%Y%m%d%H%I%sZ"; // YYYYMMDDHH[MM[SS[.fff]]]Z

const char* FMT_RFC_3339_DATE = "%Y-%m-%d";
const char* FMT_RFC_3339_SECS = "%Y-%m-%d %H:%M:%S%:z";
const char* FMT_RFC_3339_NS = "%Y-%m-%d %H:%M:%S.%N%:z";

const char* FMT_ISO_8601_DATE = "%Y-%m-%d";
const char* FMT_ISO_8601_SECOND = "%Y-%m-%dT%H:%M:%S%:z";
const char* FMT_ISO_8601_NS = "%Y-%m-%dT%H:%M:%S,%N%:z";
const char* FMT_ISO_8601_HOUR = "%Y-%m-%dT%H%:z";
const char* FMT_ISO_8601_MINUTE = "%Y-%m-%dT%H:%M%:z";

const char* FMT_RFC_EMAIL = "%a, %d %b %Y %H:%M:%S %z";

#define FMT_LOCALE_TIME     FMT_TIME
#define FMT_DEFAULT         FMT_EN_US

#define FLAG_DEBUG          0b0001

#define PAD if (padding > 0) {\
        if (pad_with_spaces) {\
            while (padding--)\
                if (fputc(' ', stdout) < 0)\
                    return EXIT_FAILURE;\
        } else if (pad_with_zeroes) {\
            while (padding--)\
                if (fputc('0', stdout) < 0)\
                    return EXIT_FAILURE;\
        }\
    }

typedef struct _timelib_time {
	timelib_sll      y, m, d;     /* Year, Month, Day */
	timelib_sll      h, i, s;     /* Hour, mInute, Second */
	timelib_sll      us;          /* Microseconds */
	int              z;           /* UTC offset in seconds */
	char            *tz_abbr;     /* Timezone abbreviation (display only) */
	signed int       dst;         /* Flag if we were parsing a DST zone */

	timelib_sll      sse;         /* Seconds since epoch */

	unsigned int   have_time, have_date, have_zone, have_relative, have_weeknr_day;

	unsigned int   sse_uptodate; /* !0 if the sse member is up to date with the date/time members */
	unsigned int   tim_uptodate; /* !0 if the date/time members are up to date with the sse member */
	unsigned int   is_localtime; /*  1 if the current struct represents localtime, 0 if it is in GMT */
	unsigned int   zone_type;    /*  1 time offset,
	                              *  3 TimeZone identifier,
	                              *  2 TimeZone abbreviation */
} timelib_time;

static int m_table_common[13] = { -1, 0, 3, 3, 6, 1, 4, 6, 2, 5, 0, 3, 5 }; /* 1 = jan */
static int m_table_leap[13] =   { -1, 6, 2, 3, 6, 1, 4, 6, 2, 5, 0, 3, 5 }; /* 1 = jan */

static timelib_sll positive_mod(timelib_sll x, timelib_sll y)
{
	timelib_sll tmp;

	tmp = x % y;
	if (tmp < 0) {
		tmp += y;
	}

	return tmp;
}

static timelib_sll century_value(timelib_sll j)
{
	return 6 - positive_mod(j, 4) * 2;
}

static timelib_sll timelib_day_of_week(timelib_sll y, timelib_sll m, timelib_sll d, int iso)
{
	timelib_sll c1, y1, m1, dow;

	/* Only valid for Gregorian calendar, commented out as we don't handle
	 * Julian calendar. We just return the 'wrong' day of week to be
	 * consistent. */
	c1 = century_value(positive_mod(y, 400) / 100);
	y1 = positive_mod(y, 100);
	m1 = timelib_is_leap(y) ? m_table_leap[m] : m_table_common[m];
	dow = positive_mod((c1 + y1 + m1 + (y1 / 4) + d), 7);
	if (iso && dow == 0) {
        dow = 7;
	}
	return dow;
}

static void timelib_unixtime2date (timelib_sll ts, timelib_sll *y, timelib_sll *m, timelib_sll *d)
{
	timelib_sll days, era, t;
	timelib_ull day_of_era, year_of_era, day_of_year, month_portion;

	/* Calculate days since algorithm's epoch (0000-03-01) */
	days = ts / SECS_PER_DAY + HINNANT_EPOCH_SHIFT;

	/* Adjustment for a negative time portion */
	t = ts % SECS_PER_DAY;
	days += (t < 0) ? -1 : 0;

	/* Calculate year, month, and day. Algorithm from:
	 * http://howardhinnant.github.io/date_algorithms.html#civil_from_days */
	era = (days >= 0 ? days : days - DAYS_PER_ERA + 1) / DAYS_PER_ERA;
	day_of_era = days - era * DAYS_PER_ERA;
	year_of_era = (day_of_era - day_of_era / 1460 + day_of_era / 36524 - day_of_era / 146096) / DAYS_PER_YEAR;
	*y = year_of_era + era * YEARS_PER_ERA;
	day_of_year = day_of_era - (DAYS_PER_YEAR * year_of_era + year_of_era / 4 - year_of_era / 100);
	month_portion = (5 * day_of_year + 2) / 153;
	*d = day_of_year - (153 * month_portion + 2) / 5 + 1;
	*m = month_portion + (month_portion < 10 ? 3 : -9);
	*y += (*m <= 2);
}

static int d_table_common[13]  = {  0,   0,  31,  59,  90, 120, 151, 181, 212, 243, 273, 304, 334 };
static int d_table_leap[13]    = {  0,   0,  31,  60,  91, 121, 152, 182, 213, 244, 274, 305, 335 };
// static int ml_table_common[13] = {  0,  31,  28,  31,  30,  31,  30,  31,  31,  30,  31,  30,  31 };
// static int ml_table_leap[13]   = {  0,  31,  29,  31,  30,  31,  30,  31,  31,  30,  31,  30,  31 };

static timelib_sll timelib_day_of_year(timelib_sll y, timelib_sll m, timelib_sll d)
{
	return (timelib_is_leap(y) ? d_table_leap[m] : d_table_common[m]) + d - 1;
}

static void timelib_isoweek_from_date(timelib_sll y, timelib_sll m, timelib_sll d, timelib_sll *iw, timelib_sll *iy)
{
	int y_leap, prev_y_leap, doy, jan1weekday, weekday;

	y_leap = timelib_is_leap(y);
	prev_y_leap = timelib_is_leap(y-1);
	doy = timelib_day_of_year(y, m, d) + 1;
	if (y_leap && m > 2) {
		doy++;
	}
	jan1weekday = timelib_day_of_week(y, 1, 1, 0);
	weekday = timelib_day_of_week(y, m, d, 0);
	if (weekday == 0) weekday = 7;
	if (jan1weekday == 0) jan1weekday = 7;
	/* Find if Y M D falls in YearNumber Y-1, WeekNumber 52 or 53 */
	if (doy <= (8 - jan1weekday) && jan1weekday > 4) {
		*iy = y - 1;
		if (jan1weekday == 5 || (jan1weekday == 6 && prev_y_leap)) {
			*iw = 53;
		} else {
			*iw = 52;
		}
	} else {
		*iy = y;
	}
	/* 8. Find if Y M D falls in YearNumber Y+1, WeekNumber 1 */
	if (*iy == y) {
		int i;

		i = y_leap ? 366 : 365;
		if ((i - (doy - y_leap)) < (4 - weekday)) {
			*iy = y + 1;
			*iw = 1;
			return;
		}
	}
	/* 9. Find if Y M D falls in YearNumber Y, WeekNumber 1 through 53 */
	if (*iy == y) {
		int j;

		j = doy + (7 - weekday) + (jan1weekday - 1);
		*iw = j / 7;
		if (jan1weekday > 4) {
			*iw -= 1;
		}
	}
}

/* Converts a Unix timestamp value into broken down time, in GMT */
static void timelib_unixtime2gmt (timelib_time* tm, timelib_sll ts)
{
	timelib_sll remainder;
	timelib_sll hours, minutes, seconds;

	timelib_unixtime2date(ts, &tm->y, &tm->m, &tm->d);
	remainder = ts % SECS_PER_DAY;
	remainder += (remainder < 0) * SECS_PER_DAY;

	/* That was the date, now we do the time */
	hours = remainder / 3600;
	minutes = (remainder - hours * 3600) / 60;
	seconds = remainder % 60;

	tm->h = hours;
	tm->i = minutes;
	tm->s = seconds;
	tm->z = 0;
	tm->dst = 0;
	tm->sse = ts;
	tm->sse_uptodate = 1;
	tm->tim_uptodate = 1;
	tm->is_localtime = 0;
}

static char * get_month_fullname (timelib_sll m) {
    switch (m) {
    case 1: return "January";
    case 2: return "February";
    case 3: return "March";
    case 4: return "April";
    case 5: return "May";
    case 6: return "June";
    case 7: return "July";
    case 8: return "August";
    case 9: return "September";
    case 10: return "October";
    case 11: return "November";
    case 12: return "December";
    default: return NULL;
    }
}

static char * get_month_shortname (timelib_sll m) {
    switch (m) {
    case 1: return "Jan";
    case 2: return "Feb";
    case 3: return "Mar";
    case 4: return "Apr";
    case 5: return "May";
    case 6: return "Jun";
    case 7: return "Jul";
    case 8: return "Aug";
    case 9: return "Sep";
    case 10: return "Oct";
    case 11: return "Nov";
    case 12: return "Dec";
    default: return NULL;
    }
}

static char * get_dow_longname (timelib_sll m) {
    switch (m) {
    case 1: return "Monday";
    case 2: return "Tuesday";
    case 3: return "Wednesday";
    case 4: return "Thursday";
    case 5: return "Friday";
    case 6: return "Saturday";
    case 7: return "Sunday";
    default: return NULL;
    }
}

static char * get_dow_shortname (timelib_sll m) {
    switch (m) {
    case 1: return "Mon";
    case 2: return "Tue";
    case 3: return "Wed";
    case 4: return "Thu";
    case 5: return "Fri";
    case 6: return "Sat";
    case 7: return "Sun";
    default: return NULL;
    }
}

static int get_quarter (timelib_sll m) {
    switch (m) {
    case 1: return 1;
    case 2: return 1;
    case 3: return 1;
    case 4: return 2;
    case 5: return 2;
    case 6: return 2;
    case 7: return 3;
    case 8: return 3;
    case 9: return 3;
    case 10: return 4;
    case 11: return 4;
    case 12: return 4;
    }
}

static char *to_upper (char * s) {
    for (int i = 0; i < strlen(s); i++) {
        s[i] = s[i] | 0b00100000;
    }
}

static int prefix (const char *pre, const char *str)
{
    return strncmp(pre, str, strlen(pre)) == 0;
}

static int unrecognized_encountered = 0;

/*

NOTES:
    This always prints GMT time, not matter what your actual locale is and
    always prints "00000000" for nanoseconds. The locale is always en-US,
    effectively. Maybe I'll change this in the future.

WARNING:
    The following formatting symbols are likely to not have been implemented
    correctly: %U, %W, %V.

*/
int printf_date (timelib_time* tm, const char *fmt, time_t ts) {
	size_t fmt_len = strlen(fmt);
	int esc = 0; // Whether the previous character was a % escape.
    timelib_sll dow, doy, woy, y; // day of week, day of year, week of year, year.
    char *s;

    int do_not_pad = 0;
    int pad_with_spaces = 0;
    int pad_with_zeroes = 0;
    int pad_with_zeroplus = 0; // TODO: Unused.
    int use_upper_case = 0;
    int use_opposite_case = 0; // TODO: Unused. What does this even mean?
    int padding = 0; // The padding goes BEFORE the value, not after.
    int colons = 0; // Used for %:::z

	for (const char *fc = fmt; *fc != '\0'; fc++) {
		if (!esc) {
			if (*fc == '%') {
				esc = 1;
			} else if (putc(*fc, stdout) < 0) { // If not an escape, just print.
                return EXIT_FAILURE;
            }
			continue;
		}
        // leading 0 means "pad with zeros", so we have to ignore a leading 0
        if (isdigit(*fc) && (*fc != '0' || padding != 0)) {
            padding = (padding * 10) + ((*fc) - 0x30);
            continue;
        }
		switch (*fc)
		{
        case '-':
            do_not_pad = 1;
            goto esc_continue;
        
        case '_':
            pad_with_spaces = 1;
            goto esc_continue;

        case '0':
            pad_with_zeroes = 1;
            goto esc_continue;
        
        case '+':
            pad_with_zeroplus = 1;
            goto esc_continue;

        case '^':
            use_upper_case = 1;
            goto esc_continue;

        case '#':
            use_opposite_case = 1;
            goto esc_continue;

		case '%':
			if (fputc('%', stdout) < 0)
				return EXIT_FAILURE;
			goto esc_done;

        case ':':
            colons++;
            goto esc_done;

        case 'a':     // locale's abbreviated weekday name (e.g., Sun)
            dow = timelib_day_of_week(tm->y, tm->m, tm->d, 0);
            PAD
            s = use_upper_case
                ? to_upper(get_dow_shortname(dow))
                : get_dow_shortname(dow);
            if (fputs(s, stdout) < 0)
                return EXIT_FAILURE;
            goto esc_done;

        case 'A':     // locale's full weekday name (e.g., Sunday)
            dow = timelib_day_of_week(tm->y, tm->m, tm->d, 0);
            s = use_upper_case
                ? to_upper(get_dow_longname(dow))
                : get_dow_longname(dow);
            if (fputs(s, stdout) < 0)
                return EXIT_FAILURE;
            goto esc_done;

        case 'b':     // locale's abbreviated month name (e.g., Jan)
        case 'h':     // same as %b
            PAD
            s = use_upper_case
                ? to_upper(get_month_shortname(tm->m))
                : get_month_shortname(tm->m);
            if (fputs(s, stdout) < 0)
                return EXIT_FAILURE;
            goto esc_done;

        case 'B':     // locale's full month name (e.g., January)
            s = use_upper_case
                ? to_upper(get_month_fullname(tm->m))
                : get_month_fullname(tm->m);
            if (fputs(s, stdout) < 0)
                return EXIT_FAILURE;
            goto esc_done;

        case 'c':     // locale's date and time (e.g., Thu Mar  3 23:05:25 2005)
            if (printf_date (tm, FMT_EN_US, 0) != EXIT_SUCCESS)
                return EXIT_FAILURE;
            goto esc_done;

        case 'C':     // century; like %Y, except omit last two digits (e.g., 20)
            PAD
            if (!do_not_pad && (tm->y / 100) < 10 && fputc('0', stdout) < 0)
                return EXIT_FAILURE;
            if (fputs(itoa(tm->y / 100), stdout) < 0)
                return EXIT_FAILURE;
            goto esc_done;

        case 'd':     // day of month (e.g., 01)
            PAD
            if (!do_not_pad && tm->d < 10 && fputc('0', stdout) < 0)
                return EXIT_FAILURE;
            if (fputs(itoa(tm->d), stdout) < 0)
                return EXIT_FAILURE;
            goto esc_done;

        case 'D':     // date; same as %m/%d/%y
            PAD
            if (printf_date (tm, FMT_DATE, 0) != EXIT_SUCCESS)
                return EXIT_FAILURE;
            goto esc_done;

        case 'e':     // day of month, space padded; same as %_d
            if (!do_not_pad && tm->d < 10 && fputc(' ', stdout) < 0)
                return EXIT_FAILURE;
            if (fputs(itoa(tm->d), stdout) < 0)
                return EXIT_FAILURE;
            goto esc_done;

        case 'F':     // full date; like %+4Y-%m-%d
            PAD
            if (printf_date (tm, FMT_FULL_DATE, 0) != EXIT_SUCCESS)
                return EXIT_FAILURE;
            goto esc_done;

        case 'g':     // last two digits of year of ISO week number (see %G)
            timelib_isoweek_from_date(tm->y, tm->m, tm->d, &woy, &y);
            if (fputs(itoa(y % 100), stdout) < 0)
                return EXIT_FAILURE;
            goto esc_done;

        case 'G':     // year of ISO week number (see %V); normally useful only with %V
            timelib_isoweek_from_date(tm->y, tm->m, tm->d, &woy, &y);
            if (fputs(itoa(y), stdout) < 0)
                return EXIT_FAILURE;
            goto esc_done;

        case 'H':     // hour (00..23)
            PAD
            if (!do_not_pad && tm->h < 10 && fputc('0', stdout) < 0)
                return EXIT_FAILURE;
            if (fputs(itoa(tm->h), stdout) < 0)
                return EXIT_FAILURE;
            goto esc_done;

        case 'I':     // hour (01..12)
            PAD
            if (!do_not_pad && tm->h < 10 && fputc('0', stdout) < 0)
                return EXIT_FAILURE;
            if (tm->h > 12) {
                if (fputs(itoa(tm->h - 12), stdout) < 0)
                    return EXIT_FAILURE;
            } else {
                if (fputs(itoa(tm->h), stdout) < 0)
                    return EXIT_FAILURE;
            }
            goto esc_done;

        case 'j':     // day of year (001..366)
            PAD
            doy = timelib_day_of_year(tm->y, tm->m, tm->d);
            if (!do_not_pad) {
                if (doy < 10 && fputs("00", stdout) < 0)
                    return EXIT_FAILURE;
                else if (doy < 100 && fputc('0', stdout) < 0)
                    return EXIT_FAILURE;
                return EXIT_FAILURE;
            }
            if (fputs(itoa(doy), stdout) < 0)
                return EXIT_FAILURE;
            goto esc_done;

        case 'k':     // hour, space padded ( 0..23); same as %_H
            PAD
            if (!do_not_pad && tm->h < 10 && fputc(' ', stdout) < 0)
                return EXIT_FAILURE;
            if (fputs(itoa(tm->h), stdout) < 0)
                return EXIT_FAILURE;
            goto esc_done;

        case 'l':     // hour, space padded ( 1..12); same as %_I
            PAD
            if (!do_not_pad && tm->h < 10 && fputc(' ', stdout) < 0)
                return EXIT_FAILURE;
            if (tm->h > 12) {
                if (fputs(itoa(tm->h - 12), stdout) < 0)
                    return EXIT_FAILURE;
            } else {
                if (fputs(itoa(tm->h), stdout) < 0)
                    return EXIT_FAILURE;
            }
            goto esc_done;

        case 'm':     // month (01..12)
            PAD
            if (!do_not_pad && tm->m < 10 && fputc('0', stdout) < 0)
                return EXIT_FAILURE;
            if (fputs(itoa(tm->m), stdout) < 0)
                return EXIT_FAILURE;
            goto esc_done;

        case 'M':     // minute (00..59)
            PAD
            if (!do_not_pad && tm->i < 10 && fputc('0', stdout) < 0)
                return EXIT_FAILURE;
            if (fputs(itoa(tm->i), stdout) < 0)
                return EXIT_FAILURE;
            goto esc_done;

        case 'n':     // a newline
            PAD
            if (fputc('\n', stdout) < 0)
                return EXIT_FAILURE;
            goto esc_done;

        case 'N':     // nanoseconds (000000000..999999999)
            PAD
            // This is a fingerprinting weapon. Just print a wrong number.
            if (fputs("000000000", stdout) < 0)
                return EXIT_FAILURE;
            goto esc_done;

        case 'p':     // locale's equivalent of either AM or PM; blank if not known
            PAD
            if (tm->h >= 12) {
                if (fputs("PM", stdout) < 0)
                    return EXIT_FAILURE;
            } else {
                if (fputs("AM", stdout) < 0)
                    return EXIT_FAILURE;
            }
            goto esc_done;

        case 'P':     // like %p, but lower case
            PAD
            if (tm->h > 12) {
                if (fputs("pm", stdout) < 0)
                    return EXIT_FAILURE;
            } else {
                if (fputs("am", stdout) < 0)
                    return EXIT_FAILURE;
            }
            goto esc_done;

        case 'q':     // quarter of year (1..4)
            PAD
            if (fputs(itoa(get_quarter(tm->m)), stdout) < 0)
                return EXIT_FAILURE;
            goto esc_done;

        case 'r':     // locale's 12-hour clock time (e.g., 11:11:04 PM)
            PAD
            if (printf_date (tm, FMT_LOCALE_HOUR12_CLOCK, 0) != EXIT_SUCCESS)
                return EXIT_FAILURE;
            goto esc_done;

        case 'R':     // 24-hour hour and minute; same as %H:%M
            PAD
            if (printf_date (tm, FMT_HOUR24_AND_MINUTE, 0) != EXIT_SUCCESS)
                return EXIT_FAILURE;
            goto esc_done;

        case 's':     // seconds since the Epoch (1970-01-01 00:00 UTC)
            if (fputs(itoa(ts), stdout) < 0)
                return EXIT_FAILURE;
            goto esc_done;

        case 'S':     // second (00..60)
            PAD
            if (!do_not_pad && tm->s < 10 && fputc('0', stdout) < 0)
                return EXIT_FAILURE;
            if (fputs(itoa(tm->s), stdout) < 0)
                return EXIT_FAILURE;
            goto esc_done;

        case 't':     // a tab
            PAD
            if (fputc('\t', stdout) < 0)
                return EXIT_FAILURE;
            goto esc_done;

        case 'T':     // time; same as %H:%M:%S
            PAD
            if (printf_date (tm, FMT_TIME, 0) != EXIT_SUCCESS)
                return EXIT_FAILURE;
            goto esc_done;

        case 'u':     // day of week (1..7); 1 is Monday
            dow = timelib_day_of_week(tm->y, tm->m, tm->d, 1);
            if (fputs(itoa(dow), stdout) < 0)
                return EXIT_FAILURE;
            goto esc_done;

        case 'U':     // week number of year, with Sunday as first day of week (00..53)
        case 'W':     // week number of year, with Monday as first day of week (00..53)
            PAD
            timelib_isoweek_from_date(tm->y, tm->m, tm->d, &woy, &y);
            if (fputs(itoa(woy), stdout) < 0)
                return EXIT_FAILURE;
            goto esc_done;

        case 'V':     // ISO week number, with Monday as first day of week (01..53)
            PAD
            timelib_isoweek_from_date(tm->y, tm->m, tm->d, &woy, &y);
            if (fputs(itoa(woy), stdout) < 0)
                return EXIT_FAILURE;
            goto esc_done;

        case 'w':     // day of week (0..6); 0 is Sunday
            PAD
            dow = timelib_day_of_week(tm->y, tm->m, tm->d, 0) - 1;
            if (fputs(itoa(dow), stdout) < 0)
                return EXIT_FAILURE;
            goto esc_done;

        case 'x':     // locale's date representation (e.g., 12/31/99)
            PAD
            if (printf_date (tm, FMT_LOCALE_DATE, 0) != EXIT_SUCCESS)
                return EXIT_FAILURE;
            goto esc_done;

        case 'X':     // locale's time representation (e.g., 23:13:48)
            PAD
            if (printf_date (tm, FMT_LOCALE_TIME, 0) != EXIT_SUCCESS)
                return EXIT_FAILURE;
            goto esc_done;

        case 'y':     // last two digits of year (00..99)
            PAD
            if (((tm->y % 100) < 10) && putc('0', stdout) < 0)
                return EXIT_FAILURE;
            if (fputs(itoa(tm->y % 100), stdout) < 0)
                return EXIT_FAILURE;
            goto esc_done;

        case 'Y':     // year
            if (fputs(itoa(tm->y), stdout) < 0)
                return EXIT_FAILURE;
            goto esc_done;

        case 'z':     // +hhmm numeric time zone (e.g., -0400)
            PAD
            // ':z':   +hh:mm numeric time zone (e.g., -04:00)
            // '::z':  +hh:mm:ss numeric time zone (e.g., -04:00:00)
            // ':::z': numeric time zone with : to necessary precision (e.g., -04, +05:30)
            if (colons == 0) {
                if (fputs("+0000", stdout) < 0)
                    return EXIT_FAILURE;
            }
            else if (colons == 1) {
                if (fputs("+00:00", stdout) < 0)
                    return EXIT_FAILURE;
            }
            else {
                if (fputs("+00:00:00", stdout) < 0)
                    return EXIT_FAILURE;
            }
            goto esc_continue;

        case 'Z':     // alphabetic time zone abbreviation (e.g., EDT)
            PAD
            if (fputs("GMT", stdout) < 0)
                return EXIT_FAILURE;
            goto esc_done;
		
		default:
			// If unrecognized, just print the format character itself.
			if (fputc(*fc, stdout) < 0)
				return EXIT_FAILURE;
            unrecognized_encountered++;
			goto esc_done;
		}

        esc_done:
            esc = 0;
            do_not_pad = 0;
            pad_with_spaces = 0;
            pad_with_zeroes = 0;
            pad_with_zeroes = 0;
            use_upper_case = 0;
            use_opposite_case = 0;
            padding = 0;

        esc_continue:
            ;

	}

    return EXIT_SUCCESS;
}

/*
Padding and other flags only work for some symbols.

--utc is implied. This always returns UTC time using an English locale.
--set will not be supported.
--file will not be supported. (It is stupid. Why does it exist?)
--debug just prints whether any unrecognized symbols were encountered to stderr

*/
int main (int argc, char **argv) {
    const char *fmt = FMT_EN_US;
    timelib_time t;
    struct stat refstat;
    int next_arg_date = 0;
    time_t ts = time(NULL);
    char* file;

    int debug = 0;

    for (int i = 1; i < argc; i++) {
        char* arg = argv[i];
        if (strlen(arg) == 0) {
            return EXIT_FAILURE;
        }
        if (next_arg_date) {
            if (arg[0] == '@') { // It is a Unix timestamp, which is the only type supported.
                if (strlen(arg) == 1) {
                    return EXIT_FAILURE;
                }
                ts = atoi(arg + 1);
            } else {
                return EXIT_FAILURE;
            }
            next_arg_date = 0;
            continue;
        }
        if (arg[0] == '+') { // This is the format string.
            if (strlen(arg) == 1) {
                return EXIT_FAILURE;
            }
            fmt = &arg[1];
        }
        else if (strcmp(arg, "-d") == 0 || strcmp(arg, "--date") == 0) {
            next_arg_date = 1;
        }
        else if (prefix("--date=", arg)) {
            if (arg[7] == '@') { // It is a Unix timestamp, which is the only type supported.
                if (strlen(arg) <= 7) {
                    return EXIT_FAILURE;
                }
                ts = atoi(arg + 8);
            } else {
                return EXIT_FAILURE;
            }
        }
        else if (prefix("--reference=", arg)) {
            file = &arg[sizeof("--reference=") - 1];
            if (stat(file, &refstat) == -1) {
                perror("stat");
                return EXIT_FAILURE;
            }
            ts = refstat.st_mtime;
        }
        else if (strcmp(arg, "--debug") == 0) {
            debug = 1;
        }
        else if (strcmp(arg, "--iso-8601") == 0) {
            fmt = FMT_ISO_8601_DATE;
        }
        else if (strcmp(arg, "--iso-8601=date") == 0) {
            fmt = FMT_ISO_8601_DATE;
        }
        else if (strcmp(arg, "--iso-8601=hours") == 0) {
            fmt = FMT_ISO_8601_HOUR;
        }
        else if (strcmp(arg, "--iso-8601=minutes") == 0) {
            fmt = FMT_ISO_8601_MINUTE;
        }
        else if (strcmp(arg, "--iso-8601=seconds") == 0) {
            fmt = FMT_ISO_8601_SECOND;
        }
        else if (strcmp(arg, "--iso-8601=ns") == 0) {
            fmt = FMT_ISO_8601_NS;
        }
        else if (strcmp(arg, "--iso-3339=date") == 0) {
            fmt = FMT_RFC_3339_DATE;
        }
        else if (strcmp(arg, "--iso-3339=seconds") == 0) {
            fmt = FMT_RFC_3339_SECS;
        }
        else if (strcmp(arg, "--iso-3339=ns") == 0) {
            fmt = FMT_RFC_3339_NS;
        }
        else if (strcmp(arg, "--rfc-email") == 0) {
            fmt = FMT_RFC_EMAIL;
        }
        else if (strcmp(arg, "--help") == 0) {
            if (puts(USAGE_MSG) <= 0)
                return EXIT_FAILURE;
            return EXIT_SUCCESS;
        }
        else if (strcmp(arg, "--version") == 0) {
            if (puts(VERSION) <= 0)
                return EXIT_FAILURE;
            if (puts(COPYRIGHT_MSG) <= 0)
                return EXIT_FAILURE;
            if (puts(LICENSE_MSG) <= 0)
                return EXIT_FAILURE;
            return EXIT_SUCCESS;
        }
        else {
            printf("Unrecognized option: %s\n", arg);
            return EXIT_FAILURE;
        }
    }

    timelib_unixtime2gmt(&t, ts);
    if (printf_date(&t, fmt, ts) != EXIT_SUCCESS)
        return EXIT_FAILURE;

    if (putc('\n', stdout) <= 0)
        return EXIT_FAILURE;

    if (debug
        && unrecognized_encountered
        && (fprintf(stderr, "%d unrecognized symbols used\n", unrecognized_encountered) < 0))
        return EXIT_FAILURE;

    return EXIT_SUCCESS;
}
