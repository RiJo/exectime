#ifndef __STATISTICS_HPP_INCLUDED__
#define __STATISTICS_HPP_INCLUDED__

#include <algorithm>
#include <cmath>

namespace statistics {
    struct statistics_t {
        unsigned int items {0};
        unsigned long maximum {0};
        unsigned long minimum {0};
        double average {0.0}; // mean
        unsigned long median {0};
        double variance {0.0};
        double standard_deviation {0.0};
        double standard_error {0.0};
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
            s.median = (data_set[s.items / 2 - 1] + data_set[s.items / 2]) / 2;
        else
            s.median = data_set[s.items / 2];

        s.maximum = std::numeric_limits<unsigned long>::min();
        s.minimum = std::numeric_limits<unsigned long>::max();
        for (const auto &item: data_set) {
            if (item < s.minimum)
                s.minimum = item;
            if (item > s.maximum)
                s.maximum = item;
            s.average += item;
        }
        s.average /= s.items;

        for (const auto &item: data_set) {
            s.variance += std::pow(item - s.average, 2);
        }
        s.variance /= s.items;

        s.standard_deviation = std::sqrt(s.variance);
        s.standard_error = (s.standard_deviation / 1000.0) / std::sqrt(s.items);

        return s;
    }
}

#endif //__STATISTICS_HPP_INCLUDED__
