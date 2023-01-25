#include <sys/event.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

void usage(char *name) {
	std::cout << name << " -s [HH:MM] -d [HH:MM] [-w [HH:MM]] [-b [HH:MM-HH:MM]]\n";
}

const int seconds_per_minute = 60;
const int minutes_per_hour   = 60;
const int seconds_per_hour   = minutes_per_hour * seconds_per_minute;

struct tmp_time {
	int hour;
	int minute;

	void normalize() {
		while (minute < 0 && hour > 0) {
			--hour;
			minute += minutes_per_hour;
		}

		if (minute > minutes_per_hour) {
			hour += minute / minutes_per_hour;
			minute = minute % minutes_per_hour;
		}
	}

	tmp_time &operator-(const tmp_time &rhs) {
		hour -= rhs.hour;
		minute -= rhs.minute;

		normalize();
		return *this;
	}

	template<typename T> tmp_time &operator/(const T div) {
		minute += minutes_per_hour * hour;
		hour = 0;
		minute /= div;

		normalize();
		return *this;
	}
};

tmp_time string_to_tm(const std::string &input) {
	tmp_time res;
	res.hour   = std::atoi(input.substr(0, 2).c_str());
	res.minute = std::atoi(input.substr(input.find(':') + 1, 2).c_str());
	return res;
}

size_t break_length(const std::string &input) {
	const size_t pos_dash    = input.find('-');
	tmp_time     break_start = string_to_tm(input.substr(0, pos_dash));
	tmp_time     break_end   = string_to_tm(input.substr(pos_dash + 1));
	tmp_time     break_time  = break_end - break_start;

	return break_time.hour * seconds_per_hour + break_time.minute * seconds_per_minute;
}

std::string print_duration(int duration) {
	std::stringstream ss;
	ss.fill('0');
	duration = abs(duration);
	ss << std::setw(2) << duration / seconds_per_hour << ':';
	duration = duration % seconds_per_hour;
	ss << std::setw(2) << duration / seconds_per_minute;

	return ss.str();
}

std::string print_duration_as_hours(int duration) {
	std::stringstream ss;
	ss.fill('0');
	duration       = abs(duration);
	double hours   = duration / seconds_per_hour;
	int    minutes = (duration % seconds_per_hour) / seconds_per_minute;
	hours += (double) minutes / 60.;
	ss << hours;

	return ss.str();
}

std::string print_time(const time_t time) {
	std::string res;
	res.resize(9);
	tm *tmp = localtime(&time);
	strftime(&res[0], res.size(), "%H:%M:%S", tmp);

	return res;
}

int main(int argc, char **argv) {
	char                     option;
	std::vector<std::string> raw_breaks;
	std::string              raw_start;
	std::string              raw_daily;
	std::string              raw_weekly;

	while ((option = getopt(argc, argv, "b:d:hs:w:")) != -1) {
		switch (option) {
		case 'b':
			raw_breaks.push_back(optarg);
			break;
		case 'd':
			raw_daily = optarg;
			break;
		case 'h':
			usage(argv[0]);
			return 0;
		case 's':
			raw_start = optarg;
			break;
		case 'w':
			raw_weekly = optarg;
			break;
		default:
			usage(argv[0]);
			return -1;
		}
	}
	// verify all required options have been set
	if (raw_start.empty()) {
		throw std::invalid_argument("Start time must be set");
	}
	if ((!raw_daily.empty() && !raw_weekly.empty()) || (raw_daily.empty() && raw_weekly.empty())) {
		throw std::invalid_argument("Either weekly or daily work time should be set");
	}

	tmp_time start_tmp = string_to_tm(raw_start);
	tmp_time daily_tmp;
	if (raw_daily.empty()) {
		daily_tmp = string_to_tm(raw_weekly);
		daily_tmp = daily_tmp / 5;
	} else {
		string_to_tm(raw_daily);
	}
	std::vector<size_t> breaks;
	for (size_t i = 0; i < raw_breaks.size(); ++i) {
		breaks.push_back(break_length(raw_breaks[i]));
	}

	std::time_t now = std::time(nullptr);
	std::tm    *start_tm;
	start_tm          = std::localtime(&now);
	start_tm->tm_hour = start_tmp.hour;
	start_tm->tm_min  = start_tmp.minute;
	start_tm->tm_sec  = 0;
	std::time_t start = std::mktime(start_tm);

	const std::time_t established = now - start;
	const int todo = daily_tmp.hour * seconds_per_hour + daily_tmp.minute * seconds_per_minute;
	const int nine = 9 * seconds_per_hour;
	const int ten  = 10 * seconds_per_hour;
	int       total_break_time = 0;
	for (size_t i = 0; i < breaks.size(); ++i) {
		total_break_time += breaks[i];
	}
	const int break_small     = 30 * seconds_per_minute;
	const int break_large     = 45 * seconds_per_minute;
	const int total_work_time = established - total_break_time;
	if (total_break_time == 0) {
		total_break_time = (total_work_time - break_large) < nine ? break_small : break_large;
	}
	const int   remaining_time = total_work_time - todo - total_break_time;
	const int   max_work_time  = start + ten + std::max(total_break_time, break_large) - now;
	std::string text_rem       = (total_work_time > todo) ? "more" : "remaining";

	std::cout << '[' << print_time(now) << "] start: " << print_time(start) << "; "
	          << print_duration_as_hours(todo) << "h: " << print_time(start + todo + break_small)
	          << "; 9h: " << print_time(start + nine + break_large)
	          << "; 10h: " << print_time(start + ten + break_large) << '\n';
	std::cout << "           already done: " << print_duration(total_work_time - total_break_time)
	          << "; " << print_duration(remaining_time) << ' ' << text_rem
	          << "; no longer than: " << print_duration(max_work_time) << '\n';
	std::cout << "           total break time: " << print_duration(total_break_time) << '\n';

	return 0;
}
