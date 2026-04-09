#include <sys/time.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "third-party/faiss_upmem/faiss/IndexFlat.h"
#include "third-party/faiss_upmem/faiss/IndexIVFPQ.h"
#include "third-party/faiss_upmem/faiss/index_io.h"

namespace {

double elapsed()
{
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    return tv.tv_sec + tv.tv_usec * 1e-6;
}

struct Args
{
    std::string base_path;
    std::string output_path;
    std::string train_path;
    int dim = 0;
    int nlist = 0;
    int pq_m = 0;
    int nbits = 8;
    int train_count = 200000;
    int add_batch = 1000000;
    int64_t add_count = -1;
    bool use_ip = false;
};

bool starts_with(const std::string &value, const std::string &prefix)
{
    return value.rfind(prefix, 0) == 0;
}

void print_usage(const char *argv0)
{
    std::cerr
        << "Usage: " << argv0 << " --base <base.fvecs> --output <out.index> "
        << "--dim <d> --nlist <nlist> --pq-m <M> [--nbits 8] "
        << "[--train <train.fvecs>] [--train-count 200000] [--add-batch 1000000] "
        << "[--add-count N] [--metric l2|ip]\n";
}

Args parse_args(int argc, char **argv)
{
    Args args;
    for (int i = 1; i < argc; i++)
    {
        std::string arg = argv[i];
        auto need_value = [&](const char *name) -> std::string {
            if (i + 1 >= argc)
            {
                throw std::runtime_error(std::string("Missing value for ") + name);
            }
            return argv[++i];
        };

        if (arg == "--base")
        {
            args.base_path = need_value("--base");
        }
        else if (arg == "--output")
        {
            args.output_path = need_value("--output");
        }
        else if (arg == "--train")
        {
            args.train_path = need_value("--train");
        }
        else if (arg == "--dim")
        {
            args.dim = std::stoi(need_value("--dim"));
        }
        else if (arg == "--nlist")
        {
            args.nlist = std::stoi(need_value("--nlist"));
        }
        else if (arg == "--pq-m")
        {
            args.pq_m = std::stoi(need_value("--pq-m"));
        }
        else if (arg == "--nbits")
        {
            args.nbits = std::stoi(need_value("--nbits"));
        }
        else if (arg == "--train-count")
        {
            args.train_count = std::stoi(need_value("--train-count"));
        }
        else if (arg == "--add-batch")
        {
            args.add_batch = std::stoi(need_value("--add-batch"));
        }
        else if (arg == "--add-count")
        {
            args.add_count = std::stoll(need_value("--add-count"));
        }
        else if (arg == "--metric")
        {
            std::string metric = need_value("--metric");
            if (metric == "ip")
            {
                args.use_ip = true;
            }
            else if (metric != "l2")
            {
                throw std::runtime_error("Unsupported metric: " + metric);
            }
        }
        else if (starts_with(arg, "--"))
        {
            throw std::runtime_error("Unknown argument: " + arg);
        }
        else
        {
            throw std::runtime_error("Unexpected positional argument: " + arg);
        }
    }

    if (args.base_path.empty() || args.output_path.empty() || args.dim <= 0 ||
        args.nlist <= 0 || args.pq_m <= 0)
    {
        throw std::runtime_error("Missing required arguments");
    }
    if (args.dim % args.pq_m != 0)
    {
        throw std::runtime_error("dim must be divisible by pq-m");
    }
    if (args.train_count <= 0 || args.add_batch <= 0)
    {
        throw std::runtime_error("train-count and add-batch must be positive");
    }

    return args;
}

std::ifstream open_fvecs(const std::string &path)
{
    std::ifstream in(path, std::ios::binary);
    if (!in)
    {
        throw std::runtime_error("Failed to open " + path);
    }
    return in;
}

int64_t count_fvecs(const std::string &path, int expected_dim)
{
    std::ifstream in(path, std::ios::binary | std::ios::ate);
    if (!in)
    {
        throw std::runtime_error("Failed to open " + path);
    }

    const std::streamsize total_size = in.tellg();
    in.seekg(0, std::ios::beg);

    int dim = 0;
    in.read(reinterpret_cast<char *>(&dim), sizeof(int));
    if (!in || dim != expected_dim)
    {
        throw std::runtime_error("Unexpected fvecs dimension in " + path);
    }

    const std::streamsize record_size = sizeof(int) + static_cast<std::streamsize>(expected_dim) * sizeof(float);
    if (total_size % record_size != 0)
    {
        throw std::runtime_error("Unaligned fvecs file size in " + path);
    }
    return static_cast<int64_t>(total_size / record_size);
}

std::vector<float> read_fvecs_range(
    const std::string &path,
    int expected_dim,
    int64_t start_idx,
    int64_t count)
{
    if (count <= 0)
    {
        return {};
    }

    std::ifstream in = open_fvecs(path);
    const std::streamoff record_size = sizeof(int) + static_cast<std::streamoff>(expected_dim) * sizeof(float);
    in.seekg(start_idx * record_size, std::ios::beg);

    std::vector<float> data(static_cast<size_t>(count) * expected_dim);
    for (int64_t i = 0; i < count; i++)
    {
        int dim = 0;
        in.read(reinterpret_cast<char *>(&dim), sizeof(int));
        if (!in || dim != expected_dim)
        {
            throw std::runtime_error("Invalid fvecs record while reading " + path);
        }
        in.read(reinterpret_cast<char *>(data.data() + i * expected_dim),
                static_cast<std::streamsize>(expected_dim) * sizeof(float));
        if (!in)
        {
            throw std::runtime_error("Failed to read fvecs payload from " + path);
        }
    }

    return data;
}

} // namespace

int main(int argc, char **argv)
{
    try
    {
        Args args = parse_args(argc, argv);
        const double t0 = elapsed();

        const std::string train_path = args.train_path.empty() ? args.base_path : args.train_path;
        const int64_t total_base = count_fvecs(args.base_path, args.dim);
        const int64_t total_train = count_fvecs(train_path, args.dim);
        const int64_t train_count = std::min<int64_t>(args.train_count, total_train);
        const int64_t add_count = (args.add_count > 0) ? std::min<int64_t>(args.add_count, total_base) : total_base;

        std::cout << "base_path=" << args.base_path << "\n";
        std::cout << "train_path=" << train_path << "\n";
        std::cout << "output_path=" << args.output_path << "\n";
        std::cout << "d=" << args.dim << " nlist=" << args.nlist
                  << " pq_m=" << args.pq_m << " nbits=" << args.nbits << "\n";
        std::cout << "total_base=" << total_base << " total_train=" << total_train
                  << " train_count=" << train_count << " add_count=" << add_count << "\n";

        faiss::IndexFlat *coarse_quantizer = nullptr;
        if (args.use_ip)
        {
            coarse_quantizer = new faiss::IndexFlatIP(args.dim);
        }
        else
        {
            coarse_quantizer = new faiss::IndexFlatL2(args.dim);
        }

        std::unique_ptr<faiss::IndexFlat> coarse_holder(coarse_quantizer);
        faiss::IndexIVFPQ index(
            coarse_quantizer,
            args.dim,
            args.nlist,
            args.pq_m,
            args.nbits,
            args.use_ip ? faiss::METRIC_INNER_PRODUCT : faiss::METRIC_L2);
        index.verbose = true;

        std::vector<float> train_vectors = read_fvecs_range(train_path, args.dim, 0, train_count);
        std::cout << "[" << (elapsed() - t0) << " s] training on " << train_count << " vectors\n";
        index.train(train_count, train_vectors.data());

        for (int64_t start = 0; start < add_count; start += args.add_batch)
        {
            const int64_t chunk = std::min<int64_t>(args.add_batch, add_count - start);
            std::vector<float> base_vectors = read_fvecs_range(args.base_path, args.dim, start, chunk);
            std::cout << "[" << (elapsed() - t0) << " s] add start=" << start
                      << " count=" << chunk << "\n";
            index.add(chunk, base_vectors.data());
        }

        std::cout << "[" << (elapsed() - t0) << " s] writing index\n";
        faiss::write_index(&index, args.output_path.c_str());
        std::cout << "done\n";
        return 0;
    }
    catch (const std::exception &e)
    {
        print_usage(argv[0]);
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
}
