#include <vector>
#include <array>
#include <algorithm>
#include <chrono>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <iomanip>
#include <locale>
#include <random>
#include <future>
#include <string>
#include <execution>

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

std::vector<size_t> benchmark_task(const size_t size, const std::string use_std_sort, const size_t id){
    auto rng = std::default_random_engine {};
    auto result = std::vector<size_t>{};
    auto vec = make_random_vector(size);

    for (auto i = size_t{0}; i < MEASUREMENTS; ++i) {
        std::shuffle(vec.begin(), vec.end(), rng);
        std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
        if (use_std_sort.compare("std sort")) {
            std::sort(std::execution::seq, vec.begin(), vec.end());
        } 
        // if (use_std_sort.compare("std sort parallel")) {
        //     std::sort(std::execution::par, vec.begin(), vec.end());
        // }
        // if (use_std_sort.compare("std sort parallel unseq")) {
        //     std::sort(std::execution::par_unseq, vec.begin(), vec.end());
        // }
        if (use_std_sort.compare("boost")) {
            boost::sort::pdqsort(vec.begin(), vec.end());
        }
        std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
        auto execution_time = static_cast<size_t>(std::chrono::duration_cast<std::chrono::microseconds> (end - begin).count());
        result.emplace_back(execution_time);
        // std::cout << "thread " << id << " took " << execution_time << " ms" << std::endl;
    }
    return result;
}


double benchmark(size_t size, size_t number_worker, const std::string use_std_sort){
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
    boost::accumulators::accumulator_set<double, boost::accumulators::stats<boost::accumulators::tag::median>> accumulator;
    
    for (auto result : results) {
        accumulator(result);
    }

    auto median = boost::accumulators::median(accumulator);

    std::cout << "  -> Sorted vector with " << size << " elements in " << median << "[ms] (median of " << number_worker << " workers)" << std::endl;
    std::cout << std::endl;
    return median;
}



int main(int argc, char const *argv[]){
    // // std::vector<size_t> vector_sizes{ 10'000, 100'000, 500'000, 1'000'000, 5'000'000, 10'000'000, 50'000'000, 100'000'000 };
    //std::vector<std::string> sort_algorithms{"std sort", "std sort parallel", "std sort parallel unseq", "boost"};
    std::vector<std::string> sort_algorithms{"std sort", "boost"};
    std::vector<size_t> vector_sizes{2'000, 4'000, 6'000, 8'000, 10'000, 12'000}; // Should cover the range around 32k L1 caches
    //std::vector<size_t> vector_sizes_large{16'000, 32'000, 64'000, 128'000, 256'000, 512'000, 1'024'000, 2'048'000}; // 500k should even be too large for 1MB L2 server caches
    //vector_sizes.insert(vector_sizes.end(), vector_sizes_large.begin(), vector_sizes_large.end());

    //std::vector<size_t> number_workers{ 1, 2, 4, 8, 16, 32 };
    std::vector<size_t> number_workers{ 2, 4, 8 };

    std::vector<std::vector<std::vector<double>>> results;
    results.reserve(number_workers.size());

    std::fstream csv_file;
    csv_file.open("results.csv", std::ios_base::out);
    csv_file << "IMPLEMENTATION,THREAD_COUNT,MEASUREMENTS,SIZE,MEDIAN_RUNTIME_MUS" << std::endl;

    const char separator = ' ';
    const int numWidth = 20;
    std::cout.imbue(std::locale(""));

    for (int worker_index = 0; worker_index < number_workers.size(); worker_index++) {
        results[worker_index].reserve(vector_sizes.size());
        for (int vector_sizes_index = 0; vector_sizes_index < vector_sizes.size(); vector_sizes_index++) {
            results[worker_index][vector_sizes_index].reserve(sort_algorithms.size());
            for (const auto sort_algorithm : sort_algorithms) {
                // std::cout << "- Run benchmark with " << number_workers[i] << " worker" << " and a vector of size " <<  vector_sizes[z] << " :" << std::endl;
                const auto result = benchmark(vector_sizes[vector_sizes_index], number_workers[worker_index], sort_algorithm);
                results[worker_index][vector_sizes_index].push_back(result);
                csv_file << "\"" << sort_algorithm << "\"," << number_workers[worker_index] << "," << MEASUREMENTS << "," << vector_sizes[vector_sizes_index] << "," << result << std::endl;
            }
        }
    }
    csv_file.close();
    for (int sort_algorithm_index = 0; sort_algorithm_index < sort_algorithms.size(); sort_algorithm_index++){
        std::cout << std::endl;
        std::cout << "Sort algorithm: " << sort_algorithms[sort_algorithm_index] << std::endl;
        std::cout << std::left << std::setw(numWidth + 2) << std::setfill(separator) << "Size";
        for (int worker_index = 0; worker_index < number_workers.size(); worker_index++) {
            std::cout << std::right << std::setw(numWidth + 2) << std::setfill(separator) << "Worker " << number_workers[worker_index];
        }
        std::cout << std::endl;
        for (int vector_sizes_index = 0; vector_sizes_index < vector_sizes.size(); vector_sizes_index++){
            std::cout << std::left << std::setw(numWidth) << std::setfill(separator) << vector_sizes[vector_sizes_index];
            for (int worker_index = 0; worker_index < number_workers.size(); worker_index++){
                std::cout << std::right << std::setw(numWidth) << std::setfill(separator) << results[worker_index][vector_sizes_index][sort_algorithm_index] << "[ms]";
            }
            std::cout << std::endl;
        }   
    }
    return 0;
}
