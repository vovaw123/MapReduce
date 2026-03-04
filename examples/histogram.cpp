/// histogram.cpp – MapReduce histogram / frequency-distribution demo.
///
/// Given a list of integer samples, computes how many values fall into each
/// bucket of a fixed-width histogram.

#include "mr/mapreduce.hpp"

#include <cmath>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

int main()
{
    // Input: sample index → value
    std::vector<std::pair<int, double>> samples;
    for (int i = 0; i < 1000; ++i) {
        // Simple pseudo-random data: sine wave + harmonic
        double v = std::sin(i * 0.1) + std::sin(i * 0.37);
        samples.emplace_back(i, v);
    }

    constexpr double bucket_width = 0.5;

    // Map: (index, value) → [(bucket_label, 1)]
    auto map_fn = [](const int& /*idx*/, const double& val)
        -> std::vector<std::pair<std::string, int>>
    {
        int bucket = static_cast<int>(std::floor(val / bucket_width));
        std::string label = std::to_string(
            static_cast<double>(bucket) * bucket_width);
        return {{label, 1}};
    };

    // Reduce: (bucket_label, [1,1,...]) → count
    auto reduce_fn = [](const std::string& /*label*/,
                        const std::vector<int>& ones) -> int
    {
        int total = 0;
        for (int v : ones) total += v;
        return total;
    };

    mr::MapReduce<int, double,
                  std::string, int,
                  int> engine(map_fn, reduce_fn, /*threads=*/4);

    auto result = engine.run(samples);

    std::cout << "Histogram (bucket start → count):\n";
    for (const auto& [label, count] : result) {
        std::cout << "  [" << label << ", "
                  << label << "+" << bucket_width << ") : "
                  << count << "\n";
    }
}
