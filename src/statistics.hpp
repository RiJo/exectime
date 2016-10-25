#ifndef __STATISTICS_HPP_INCLUDED__
#define __STATISTICS_HPP_INCLUDED__

#include <functional>
#include <algorithm>
#include <cmath>
#include <vector>

namespace statistics {
    template<typename T>
    struct statistics_t {
        unsigned int sample_size {0};
        T maximum {0};
        T minimum {0};
        T range {0};
        double average {0.0}; // mean
        double median {0.0};
        double variance {0.0};
        double standard_deviation {0.0};
        double standard_error {0.0};
        double relative_standard_error {0.0};
    };

    // TODO: static assert (list + numeric items)
    // TODO: make selector parameter optional (nullptr as default value?)
    template<typename TContainer, typename TItem, typename TValue>
    statistics_t<TValue> calculate(const TContainer &data_set, std::function<TValue (const TItem &)> selector) {
        statistics_t<TValue> s;

        s.sample_size = data_set.size();
        if (s.sample_size == 0)
            return s;

        // Convert data set
        std::vector<TValue> values;
        for (const TItem &item: data_set)
            values.push_back(selector(item));
        std::sort(values.begin(), values.end());

        // Calculate statistics
        if (s.sample_size  % 2 == 0)
            s.median = (values[s.sample_size / 2 - 1] + values[s.sample_size / 2]) / 2.0;
        else
            s.median = values[s.sample_size / 2];

        s.maximum = values.back();
        s.minimum = values.front();
        s.range = s.maximum - s.minimum;

        for (const TValue &value: values)
            s.average += value;
        s.average /= s.sample_size;


        for (const TValue &value: values) {
            s.variance += std::pow(value - s.average, 2);
        }
        s.variance /= s.sample_size;

        s.standard_deviation = std::sqrt(s.variance);
        s.standard_error = s.standard_deviation / std::sqrt(s.sample_size);
        if (s.average == 0.0)
            s.relative_standard_error = 1.0;
        else
            s.relative_standard_error = s.standard_error / s.average;
        s.relative_standard_error *= 100.0;

        return s;
    }
}

#endif //__STATISTICS_HPP_INCLUDED__
