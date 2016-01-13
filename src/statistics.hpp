#ifndef __STATISTICS_HPP_INCLUDED__
#define __STATISTICS_HPP_INCLUDED__

#include <algorithm>
#include <cmath>

namespace statistics {
    struct statistics_t {
        unsigned int items {0};
        unsigned long max {0};
        unsigned long min {0};
        double avg {0.0}; // mean
        unsigned long med {0}; // median
        double var {0.0}; // variance
        double sd {0.0}; // standard deviation
        unsigned int sd1 {0}; // standard deviation item count
        unsigned int sd2 {0}; // standard deviation item count
        unsigned int sd3 {0}; // standard deviation item count
        double se {0.0}; // standard error
    };

    // TODO: static assert (list + numeric items)
    template<typename T>
    statistics_t calculate(T data_set) {
        statistics_t s;

        s.items = data_set.size();
        if (s.items == 0)
            return s;

        // Preprocess data set
        std::sort(data_set.begin(), data_set.end());

        // Calculate statistics
        if (s.items  % 2 == 0)
            s.med = (data_set[s.items / 2 - 1] + data_set[s.items / 2]) / 2;
        else
            s.med = data_set[s.items / 2];

        s.max = std::numeric_limits<unsigned long>::min();
        s.min = std::numeric_limits<unsigned long>::max();
        for (const auto &item: data_set) {
            if (item < s.min)
                s.min = item;
            if (item > s.max)
                s.max = item;
            s.avg += item;
        }
        s.avg /= s.items;

        for (const auto &item: data_set) {
            s.var += std::pow(item - s.avg, 2);
        }
        s.var /= s.items;

        s.sd = std::sqrt(s.var);

        for (const auto &item: data_set) {
            if (item >= std::min(std::numeric_limits<double>::min(), s.avg - s.sd) && item <= (s.avg + s.sd))
                s.sd1++;
            if (item >= std::min(std::numeric_limits<double>::min(), s.avg - 2 * s.sd) && item <= (s.avg + 2 * s.sd))
                s.sd2++;
            if (item >= std::min(std::numeric_limits<double>::min(), s.avg - 3 * s.sd) && item <= (s.avg + 3 * s.sd))
                s.sd3++;
        }

        s.se = (s.sd / 1000.0) / std::sqrt(s.items);

        return s;
    }
}

#endif //__STATISTICS_HPP_INCLUDED__
