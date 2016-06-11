#include "systemtime.h"

#include <memory>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <cstdio>
#include <cassert>
#include <utility>
#include <algorithm>
#include <numeric>
#include <cmath>

namespace moodycamel
{
	struct stats_t
	{
		stats_t(const std::unique_ptr<double[]> &results, std::size_t count)
		{
			std::sort(&results[0], &results[count]);
			
			_min = results[0];
			_max = results[count - 1];
			
			if (count == 1) {
				_q[0] = _q[1] = _q[2] = results[0];
				_avg = results[0];
				_variance = 0;
				return;
			}
			
			// Kahan summation to reduce error (see http://en.wikipedia.org/wiki/Kahan_summation_algorithm)
			double sum = 0;
			double c = 0;	// Error last time around
			for (std::size_t i = 0; i != count; ++i) {
				auto y = results[i] - c;
				auto t = sum + y;
				c = (t - sum) - y;
				sum = t;
			}
			_avg = sum / count;
			
			// Calculate unbiased (corrected) sample variance
			sum = 0, c = 0;
			for (std::size_t i = 0; i != count; ++i) {
				auto y = (results[i] - _avg) * (results[i] - _avg) - c;
				auto t = sum + y;
				c = (t - sum) - y;
				sum = t;
			}
			_variance = sum / (count - 1);
			
			// See Method 3 here: http://en.wikipedia.org/wiki/Quartile
			_q[1] = (count & 1) == 0 ? (results[count / 2 - 1] + results[count / 2]) * 0.5 : results[count / 2];
			if ((count & 1) == 0) {
				_q[0] = (count & 3) == 0 ? (results[count / 4 - 1] + results[count / 4]) * 0.5 : results[count / 4];
				_q[2] = (count & 3) == 0 ? (results[count / 2 + count / 4 - 1] + results[count / 2 + count / 4]) * 0.5 : results[count / 2 + count / 4];
			}
			else if ((count & 3) == 1) {
				_q[0] = results[count / 4 - 1] * 0.25 + results[count / 4] * 0.75;
				_q[2] = results[count / 4 * 3] * 0.75 + results[count / 4 * 3 + 1] * 0.25;
			}
			else {		// (count & 3) == 3
				_q[0] = results[count / 4] * 0.75 + results[count / 4 + 1] * 0.25;
				_q[2] = results[count / 4 * 3 + 1] * 0.25 + results[count / 4 * 3 + 2] * 0.75;	
			}
		}
	
		double min() const { return _min; }
		double max() const { return _max; }
		double range() const { return _max - _min; }
		double avg() const { return _avg; }
		double variance() const { return _variance; }
		double stddev() const { return std::sqrt(_variance); }
		double median() const { return _q[1]; }
		double q1() const { return _q[0]; }
		double q2() const { return _q[1]; }
		double q3() const { return _q[2]; }
		double q(std::size_t which) const { assert(which < 4 && which > 0); return _q[which - 1]; }
        stats_t operator -(double x) const
        {
            auto ret = *this;
            ret._min -= x;
            ret._max -= x;
            ret._avg -= x;
            ret._q[0] -= x;
            ret._q[1] -= x;
            ret._q[2] -= x;
            return ret;
        }
        stats_t operator * ( double x) const
        {
            auto ret = *this;
            ret._min *= x;
            ret._max *= x;
            ret._avg *= x;
            ret._variance *= (x * x);
            ret._q[0] *= x;
            ret._q[1] *= x;
            ret._q[2] *= x;
            return ret;
        }
	    friend std::ostream	&operator << (std::ostream & os, const stats_t &st)
        {
            os << "min:\t" << st.min() << "\tmean:\t" << st.avg() << "\tmedian:\t" << st.median() << "\tmax:\t" << st.max();
            os << "\tstddev:\t" << st.stddev();
            return os;
        }
		double _min;
		double _max;
		double _q[3];
		double _avg;
		double _variance;
	};
	
	
	// Times how long it takes to run a given function for a given number of iterations; this
	// timing process is repeated for a given number of test runs; various statistics of the
	// of timing results are returned in a `stats_t` object.
    template<typename TFunc, std::uint64_t iterations = 1, std::uint32_t testRuns = 100>
	stats_t microbench_stats(TFunc&& func, bool returnTimePerIteration = true)
	{
		assert(testRuns >= 1);
		assert(iterations >= 1);
        auto results = std::make_unique<double[]>(testRuns);
		for (std::uint32_t i = 0; i < testRuns; ++i) {
		    auto val = 0.0;
		    for(std::uint32_t k = 0; k < 16; k ++) {
			auto start = getSystemTime();
			for (std::uint64_t j = 0; j < iterations; ++j) {
				func();
			}
			auto tmp = static_cast<double>(getTimeDelta(start));
			if(val == 0)
			    val = tmp;
			else
			    val = std::min(tmp,val);

		    }
		    results[i] = val;
		    if (returnTimePerIteration) {
			    results[i] /= iterations;
		    }
		}
		return stats_t(results,testRuns);
	}
	
	
	// Times how long it takes to run a given function for a given number of iterations; this
	// timing process is repeated for a given number of test runs; the fastest of the runs is
	// selected, and its time returned (in milliseconds).
	template<typename TFunc, std::uint64_t iterations = 1, std::uint32_t testRuns = 100, int div = 1>
	stats_t microbench(TFunc&& func, bool returnTimePerIteration = true)
	{
//        auto null_lambda = [](){};
//        auto nstats = microbench_stats<decltype(null_lambda),iterations,testRuns>(std::move(null_lambda),returnTimePerIteration);
		auto stats  = microbench_stats<TFunc,iterations,testRuns>(std::forward<TFunc>(func),  returnTimePerIteration);
        return (stats * (1. / div));//- nstats.min()) * (1. / div);
	}
}

