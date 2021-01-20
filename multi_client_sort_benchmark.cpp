#include <vector>
#include <array>
#include <algorithm>
#include <chrono>
#include <iostream>
#include <cstdlib>
#include <iomanip>
#include <locale>
#include <random>
#include <future>

#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/median.hpp>
#include <boost/sort/sort.hpp>

constexpr auto MEASUREMENTS = 23;


std::vector<int> make_random_vector(size_t size, bool mersenne_twister_engine = false) {
    std::vector<int> numbers(size);
    std::iota(numbers.begin(), numbers.end(), 0);
    if (mersenne_twister_engine) {
        std::mt19937_64 urng{ 121216 };
        std::shuffle(numbers.begin(), numbers.end(), urng);
        return numbers;
    }
    auto rng = std::default_random_engine {};
    std::shuffle(numbers.begin(), numbers.end(), rng);
    return numbers;
}

std::vector<size_t> benchmark_task(const size_t size, const bool use_std_sort, const size_t id){
    auto rng = std::default_random_engine {};
    auto result = std::vector<size_t>{};
    auto vec = make_random_vector(size);

    for (auto i = size_t{0}; i < MEASUREMENTS; ++i) {
        std::shuffle(vec.begin(), vec.end(), rng);

        std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
        if (use_std_sort) {
            std::sort(vec.begin(), vec.end());
        } else {
            boost::sort::pdqsort(vec.begin(), vec.end());
        }
        std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
        auto execution_time = static_cast<size_t>(std::chrono::duration_cast<std::chrono::microseconds> (end - begin).count());
        result.emplace_back(execution_time);
        // std::cout << "thread " << id << " took " << execution_time << " ms" << std::endl;
    }
    return result;
}


int benchmark(size_t size, size_t number_worker, const bool use_std_sort){
    std::vector<size_t> results;
    results.reserve(number_worker * MEASUREMENTS);
    std::vector<std::future<std::vector<size_t>>> benchmark_tasks;
    benchmark_tasks.reserve(number_worker);

    for (size_t i = 0; i < number_worker; i++) {
        benchmark_tasks.emplace_back(std::async(std::launch::async, benchmark_task, size, use_std_sort, i));
    }
    for (auto& task : benchmark_tasks){
        const auto task_result = task.get();
        results.insert(results.end(), task_result.begin(), task_result.end());
    }
    boost::accumulators::accumulator_set<size_t, boost::accumulators::stats<boost::accumulators::tag::median>> accumulator;
    
    for (auto result : results) {
        accumulator(result);
    }

    auto median = boost::accumulators::median(accumulator);

    // std::cout << "  -> Sorted vector with " << size << " elements in " << median << "[ms] (median of " << number_worker << " workers)" << std::endl;
    std::cout << "\"" << (use_std_sort ? "std" : "boost") << "\"," << number_worker << "," << MEASUREMENTS << "," << size << "," << median << std::endl;
    // std::cout << std::endl;
    return median;
}



int main(int argc, char const *argv[]){
    // // std::vector<size_t> vector_sizes{ 10'000, 100'000, 500'000, 1'000'000, 5'000'000, 10'000'000, 50'000'000, 100'000'000 };

    std::vector<size_t> vector_sizes{2'000, 4'000, 6'000, 8'000, 10'000, 12'000}; // Should cover the range around 32k L1 caches
    std::vector<size_t> vector_sizes_large{16'000, 32'000, 64'000, 128'000, 256'000, 512'000, 1'000'000}; // 500k should even be too large for 1MB L2 server caches
    vector_sizes.insert(vector_sizes.end(), vector_sizes_large.begin(), vector_sizes_large.end());
    std::vector<size_t> number_workers{ 1, 2, 4, 8, 16, 32 };
    // std::vector<size_t> number_workers{ 2, 4, 8 };

    std::vector<std::vector<int>> results;
    results.reserve(number_workers.size());

    const char separator = ' ';
    const int numWidth = 20;
    std::cout.imbue(std::locale(""));

    for (int i = 0; i < number_workers.size(); i++) {
        results[i].reserve(vector_sizes.size());
        for (int z = 0; z < vector_sizes.size(); z++) {
            for (const auto use_std_sort : {true, false}) {
                // std::cout << "- Run benchmark with " << number_workers[i] << " worker" << " and a vector of size " <<  vector_sizes[z] << " :" << std::endl;
                results[i][z] = benchmark(vector_sizes[z], number_workers[i], use_std_sort);
            }
        }
    }
    std::cout << std::endl;
    std::cout << std::left << std::setw(numWidth + 2) << std::setfill(separator) << "Size";
    for (int i = 0; i < number_workers.size(); i++) {
        std::cout << std::right << std::setw(numWidth + 2) << std::setfill(separator) << "Worker " << number_workers[i];
    }
    std::cout << std::endl;
    for (int z = 0; z < vector_sizes.size(); z++){
        std::cout << std::left << std::setw(numWidth) << std::setfill(separator) << vector_sizes[z];
        for (int i = 0; i < number_workers.size(); i++){
            std::cout << std::right << std::setw(numWidth) << std::setfill(separator) << results[i][z] << "[ms]";
        }
        std::cout << std::endl;
    }   
    return 0;
}
