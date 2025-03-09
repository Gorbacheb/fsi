#include <vector>
#include <random>
#include <omp.h>
#include <iostream>
#include <cmath>

typedef long long ll;
using namespace std;

double sum_k_random(const vector<double>& A, size_t K) {
    const size_t N = A.size();
    if (N == 0 || K == 0) return 0.0;

    // if K >> N 
    if (K >= N * 100) {
        vector<size_t> counts(N, 0);
        random_device rd;
        mt19937_64 gen(rd());

        //it's hard to parallelize this section
        size_t remaining = K;
        for (size_t i = 0; i < N - 1; ++i) {
            binomial_distribution<size_t> binom(remaining, 1.0 / (N - i));
            counts[i] = binom(gen);
            remaining -= counts[i];
        }
        counts[N - 1] = remaining;

        double sum = 0.0;
        #pragma omp parallel for reduction(+:sum)
        for (ll i = 0; i < N; ++i) {
            sum += A[i] * counts[i];
        }
        return sum;
    }
    else {
        // simple openmp parallel if K ~ N
        double total = 0.0;
        random_device rd;
        const unsigned int base_seed = rd();

        #pragma omp parallel reduction(+:total)
        {
            unsigned int seed = base_seed ^ (omp_get_thread_num() * 12345);
            mt19937_64 gen(seed);
            uniform_int_distribution<size_t> dist(0, N - 1);

            #pragma omp for
            for (ll i = 0; i < K; ++i) {
                total += A[dist(gen)];
            }
        }
        return total;
    }
}


int main()
{
    const size_t N = 10000000;
    const size_t K = 100000000000;
    const double expected_mean = static_cast<double>(N - 1) / (2 * N);
    const double tolerance_pct = 1.0;
    const int num_runs = 100;

    vector<double> A(N);

    #pragma omp parallel for
    for (long long i = 0; i < static_cast<long long>(N); ++i) {
        A[i] = static_cast<double>(i) / N;
    }

    const double expected_sum = K * expected_mean;
    double total_result = 0.0;
    double total_time = 0.0;

    #pragma omp parallel reduction(+:total_result, total_time)
    {
        random_device rd;
        unsigned int seed = rd() + omp_get_thread_num();
        mt19937 local_gen(seed);

        #pragma omp for
        for (int run = 0; run < num_runs; ++run) {
            double start = omp_get_wtime();
            double result = sum_k_random(A, K);
            double elapsed = omp_get_wtime() - start;

            total_result += result;
            total_time += elapsed;
        }
    }

    double avg_result = total_result / num_runs;
    double avg_time = total_time / num_runs;

    cout.precision(12);
    double error_pct = fabs(avg_result - expected_sum) / expected_sum * 100.0;

    cout << "Average result after " << num_runs << " runs: " << avg_result << "\n"
        << "Expected value: " << expected_sum << "\n"
        << "Average deviation: " << error_pct << "%\n"
        << "Average time per run: " << avg_time << " s.\n";

    if (error_pct > tolerance_pct) {
        cerr << "Error: deviation exceeds permissible limit!\n";
        return 1;
    }

    return 0;
}
