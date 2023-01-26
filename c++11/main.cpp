#ifdef __APPLE__
#include <sys/event.h>
#else
#include <event.h>
#endif
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>

void usage(char *name) {
	std::cout << name << " -s [HH:MM] -d [HH:MM] [-w [HH:MM]] [-b [HH:MM-HH:MM]]\n";
}

using timepoint_t = std::chrono::time_point<std::chrono::system_clock>;
using duration_t  = std::chrono::system_clock::duration;
using d_hour_t    = std::chrono::duration<double, std::ratio<3600>>;

/** Convert string to timepoint */
timepoint_t string_to_timepoint(const std::string &input) {
	auto  tmp       = time(nullptr);
	auto *tmpp      = std::localtime(&tmp);
	tmpp->tm_hour   = std::atoi(input.substr(0, 2).c_str());
	tmpp->tm_min    = std::atoi(input.substr(input.find(':') + 1, 2).c_str());
	tmpp->tm_sec    = 0;
	tmp             = std::mktime(tmpp);
	timepoint_t res = std::chrono::system_clock::from_time_t(tmp);

	return res;
}

/** Convert string to durations.  No more constants required */
duration_t string_to_duration(const std::string &input) {
	duration_t h = std::chrono::hours(std::atoi(input.substr(0, 2).c_str()));
	duration_t m = std::chrono::minutes(std::atoi(input.substr(input.find(':') + 1, 2).c_str()));

	duration_t res = h + m;
	return res;
}

/** Calculate length of breaks.  Straight-forward arithmetic operations. */
duration_t break_length(const std::string &input) {
	const size_t pos_dash    = input.find('-');
	auto         break_start = string_to_duration(input.substr(0, pos_dash));
	auto         break_end   = string_to_duration(input.substr(pos_dash + 1));
	auto         break_time  = break_end - break_start;

	return break_time;
}

/** abs is C++17 */
template<typename T> T abs(T value) { return value < T::zero() ? -1 * value : value; }

/** Print duration as HH:MM */
std::string print_duration(duration_t duration) {
	duration = abs(duration);
	auto h   = std::chrono::duration_cast<std::chrono::hours>(duration);
	auto m   = std::chrono::duration_cast<std::chrono::minutes>(duration - h);

	std::stringstream ss;
	ss.fill('0');
	ss << std::setw(2) << h.count() << ':' << std::setw(2) << m.count();

	return ss.str();
}

/** Print duration as floating-value of hours */
std::string print_duration_as_hours(duration_t duration) {
	duration = abs(duration);
	auto h   = d_hour_t(duration);

	std::stringstream ss;
	ss << h.count();

	return ss.str();
}

/** Print timepoint as HH:MM */
std::string print_time(const timepoint_t time) {
	std::string res;
	res.resize(9);

	time_t ttmp = std::chrono::system_clock::to_time_t(time);
	tm    *tmp  = localtime(&ttmp);
	strftime(&res[0], res.size(), "%H:%M:%S", tmp);

	return res;
}

int main(int argc, char **argv) {
	char                     option;
	std::vector<std::string> raw_breaks;
	std::string              raw_start;
	std::string              raw_daily;
	std::string              raw_weekly = "39:00";
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
	// Verify all required options have been set properly
	if (raw_start.empty()) {
		throw std::invalid_argument("Start time must be set");
	}
	if ((!raw_daily.empty() && !raw_weekly.empty()) || (raw_daily.empty() && raw_weekly.empty())) {
		throw std::invalid_argument("Either weekly or daily work time should be set");
	}

	std::vector<duration_t> breaks;
	std::transform(raw_breaks.begin(), raw_breaks.end(), std::back_inserter(breaks),
	               [](const std::string break_string) { return break_length(break_string); });

	auto        now   = std::chrono::system_clock::now();
	timepoint_t start = string_to_timepoint(raw_start);

	duration_t total_time = now - start;
	duration_t todo =
	    (raw_daily.empty()) ? (string_to_duration(raw_weekly) / 5) : string_to_duration(raw_daily);
	duration_t nine             = std::chrono::hours(9);
	duration_t ten              = std::chrono::hours(10);
	auto       total_break_time = std::accumulate(breaks.begin(), breaks.end(), (duration_t) 0);

	duration_t break_small = std::chrono::minutes(30);
	duration_t break_large = std::chrono::minutes(45);
	duration_t work_time   = total_time - total_break_time;
	if (total_break_time == duration_t::zero()) {
		total_break_time = (work_time - break_large) < nine ? break_small : break_large;
		breaks.push_back(total_break_time);
	}
	bool       done = work_time > todo;
	duration_t remaining_time =
	    done ? (work_time + total_break_time - todo) : (total_time - (todo + total_break_time));
	duration_t  max_work_time = start + ten + std::max(total_break_time, break_large) - now;
	std::string text_rem      = (work_time > todo) ? "more" : "remaining";

	std::cout << '[' << print_time(now) << "] start: " << print_time(start) << "; "
	          << print_duration_as_hours(todo)
	          << "h: " << print_time(start + todo + std::max(break_small, total_break_time))
	          << "; 9h: " << print_time(start + nine + std::max(break_large, total_break_time))
	          << "; 10h: " << print_time(start + ten + std::max(break_large, total_break_time))
	          << '\n';
	std::cout << "           already done: " << print_duration(work_time) << "; "
	          << print_duration(remaining_time) << ' ' << text_rem
	          << "; no longer than: " << print_duration(max_work_time) << '\n';
	std::cout << "           total break time: " << print_duration(total_break_time)
	          << "; longest break: "
	          << print_duration(*std::max_element(breaks.begin(), breaks.end())) << '\n';

	return 0;
}
