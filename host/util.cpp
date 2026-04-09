#include "util.h"

#include <vector>

namespace {

bool has_suffix(const std::string &value, const std::string &suffix)
{
    return value.size() >= suffix.size() &&
           value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
}

float *read_fvecs_file(const char *filename, int &nvecs, int &dim)
{
    std::ifstream infile(filename, std::ios::binary | std::ios::ate);
    if (!infile)
    {
        perror("Error opening fvecs file");
        return nullptr;
    }

    std::streamsize total_size = infile.tellg();
    infile.seekg(0, std::ios::beg);

    int dim_in_file = 0;
    infile.read(reinterpret_cast<char *>(&dim_in_file), sizeof(int));
    if (!infile || dim_in_file <= 0)
    {
        fprintf(stderr, "Invalid fvecs header in %s\n", filename);
        return nullptr;
    }

    const std::streamsize record_size = sizeof(int) + static_cast<std::streamsize>(dim_in_file) * sizeof(float);
    if (total_size % record_size != 0)
    {
        fprintf(stderr, "fvecs file size is not aligned in %s\n", filename);
        return nullptr;
    }

    nvecs = static_cast<int>(total_size / record_size);
    dim = dim_in_file;
    float *float_data = new float[static_cast<size_t>(nvecs) * dim]();

    infile.seekg(0, std::ios::beg);
    for (int i = 0; i < nvecs; i++)
    {
        int cur_dim = 0;
        infile.read(reinterpret_cast<char *>(&cur_dim), sizeof(int));
        if (!infile || cur_dim != dim)
        {
            fprintf(stderr, "Inconsistent fvecs record in %s at row %d\n", filename, i);
            delete[] float_data;
            return nullptr;
        }
        infile.read(reinterpret_cast<char *>(float_data + static_cast<size_t>(i) * dim),
                    static_cast<std::streamsize>(dim) * sizeof(float));
        if (!infile)
        {
            fprintf(stderr, "Failed to read fvecs payload in %s at row %d\n", filename, i);
            delete[] float_data;
            return nullptr;
        }
    }

    return float_data;
}

ID_TYPE *read_ivecs_groundtruth(const char *filename, int &n, int &k)
{
    std::ifstream infile(filename, std::ios::binary | std::ios::ate);
    if (!infile)
    {
        perror("Failed to open ivecs groundtruth");
        return nullptr;
    }

    std::streamsize total_size = infile.tellg();
    infile.seekg(0, std::ios::beg);

    int k_in_file = 0;
    infile.read(reinterpret_cast<char *>(&k_in_file), sizeof(int));
    if (!infile || k_in_file <= 0)
    {
        fprintf(stderr, "Invalid ivecs header in %s\n", filename);
        return nullptr;
    }

    const std::streamsize record_size = sizeof(int) + static_cast<std::streamsize>(k_in_file) * sizeof(int);
    if (total_size % record_size != 0)
    {
        fprintf(stderr, "ivecs file size is not aligned in %s\n", filename);
        return nullptr;
    }

    n = static_cast<int>(total_size / record_size);
    k = k_in_file;

    ID_TYPE *data_id = new ID_TYPE[static_cast<size_t>(n) * k]();
    infile.seekg(0, std::ios::beg);

    for (int i = 0; i < n; i++)
    {
        int cur_k = 0;
        infile.read(reinterpret_cast<char *>(&cur_k), sizeof(int));
        if (!infile || cur_k != k)
        {
            fprintf(stderr, "Inconsistent ivecs record in %s at row %d\n", filename, i);
            delete[] data_id;
            return nullptr;
        }

        std::vector<int> row(k);
        infile.read(reinterpret_cast<char *>(row.data()), static_cast<std::streamsize>(k) * sizeof(int));
        if (!infile)
        {
            fprintf(stderr, "Failed to read ivecs payload in %s at row %d\n", filename, i);
            delete[] data_id;
            return nullptr;
        }

        for (int j = 0; j < k; j++)
        {
            data_id[static_cast<size_t>(i) * k + j] = row[j];
        }
    }

    return data_id;
}

} // namespace

// type==0: sift1b
// type==1: space
// type==2: generic fvecs
float *read_query(
    const char *filename,
    int type,
    int &nvecs,
    int &dim)
{
    nvecs = 0;
    dim = 0;

    printf("query path is %s\n", filename);
    if (type == 0)
    {
        FILE *f = fopen(filename, "rb");
        if (!f)
        {
            perror("Error opening file");
            return NULL;
        }

        if (fread(&nvecs, sizeof(int), 1, f) != 1 ||
            fread(&dim, sizeof(int), 1, f) != 1)
        {
            perror("Failed to read query header");
            fclose(f);
            return NULL;
        }

        int total_elements = nvecs * dim;
        uint8_t *int_data = new uint8_t[total_elements]();

        if (fread(int_data, sizeof(uint8_t), total_elements, f) !=
            static_cast<size_t>(total_elements))
        {
            perror("Failed to read query payload");
            delete[] int_data;
            fclose(f);
            return NULL;
        }

        fclose(f);

        float *float_data = new float[total_elements]();
        for (int i = 0; i < total_elements; i++)
        {
            float_data[i] = (float)int_data[i];
        }

        delete[] int_data;

        return float_data;
    }
    else if (type == 1)
    {
       
        FILE *f = fopen(filename, "rb");
        if (!f)
        {
            perror("Error opening file");
            return NULL;
        }

        if (fread(&nvecs, sizeof(int), 1, f) != 1 ||
            fread(&dim, sizeof(int), 1, f) != 1)
        {
            perror("Failed to read query header");
            fclose(f);
            return NULL;
        }

        int total_elements = nvecs * dim;
        int8_t *int_data = new int8_t[total_elements]();
        if (fread(int_data, sizeof(int8_t), total_elements, f) !=
            static_cast<size_t>(total_elements))
        {
            perror("Failed to read query payload");
            delete[] int_data;
            fclose(f);
            return NULL;
        }

        fclose(f);

        float *float_data = new float[total_elements]();
        for (int i = 0; i < total_elements; i++)
        {
            float_data[i] = (float)int_data[i];
        }

        delete[] int_data;

        return float_data;
    }
    else if (type == 2)
    {
        return read_fvecs_file(filename, nvecs, dim);
    }
    else {
        fprintf(stderr, "Error: Invalid type %d\n", type);
        return nullptr;  
    }
}


ID_TYPE *read_groundtruth(const char *filename, int &n, int &k)
{
    if (has_suffix(filename, ".ivecs"))
    {
        return read_ivecs_groundtruth(filename, n, k);
    }

    FILE *file = fopen(filename, "rb");
    if (!file)
    {
        perror("Failed to open file");
        return NULL;
    }

    if (fread(&n, sizeof(int), 1, file) != 1 ||
        fread(&k, sizeof(int), 1, file) != 1)
    {
        perror("Failed to read n or k");
        fclose(file);
        return NULL;
    }

  
    int *data = (int *)malloc(n * k * sizeof(int));
    if (!data)
    {
        perror("Failed to allocate memory");
        fclose(file);
        return NULL;
    }

   
    for (int i = 0; i < n; i++)
    {
       
        if (fread(&data[i * k], sizeof(int), k, file) != k)
        {
            perror("Failed to read data");
            free(data);
            fclose(file);
            return NULL;
        }
    }

   
    ID_TYPE *data_id = new ID_TYPE[n * k];
    for (int i = 0; i < n * k; i++)
    {
        data_id[i] = data[i];
    }

    free(data);

    fclose(file);

    return data_id;
}
