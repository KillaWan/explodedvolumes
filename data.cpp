#include "headers/data.h"
#include "nifti1_io.h"   // NIfTI库
#include "file_dialog.h" // 文件对话框


namespace MC
{

    // Load Volume
    float getVolumeValue(const float *data, int x, int y, int z, const int dims[3])
    {
        return data[x + dims[0] * (y + dims[1] * z)];
    }

    // Boundary and Center
    void calculateMeshBounds(const std::vector<Vertex> &vertices,
                             Vec3 &min_bounds, Vec3 &max_bounds, Vec3 &center)
    {
        if (vertices.empty())
        {
            min_bounds = max_bounds = center = Vec3(0, 0, 0);
            return;
        }

        min_bounds = Vec3(std::numeric_limits<float>::max(),
                          std::numeric_limits<float>::max(),
                          std::numeric_limits<float>::max());
        max_bounds = Vec3(std::numeric_limits<float>::lowest(),
                          std::numeric_limits<float>::lowest(),
                          std::numeric_limits<float>::lowest());

        for (const auto &v : vertices)
        {
            min_bounds.x = std::min(min_bounds.x, v.x);
            min_bounds.y = std::min(min_bounds.y, v.y);
            min_bounds.z = std::min(min_bounds.z, v.z);

            max_bounds.x = std::max(max_bounds.x, v.x);
            max_bounds.y = std::max(max_bounds.y, v.y);
            max_bounds.z = std::max(max_bounds.z, v.z);
        }

        center.x = (min_bounds.x + max_bounds.x) / 2.0f;
        center.y = (min_bounds.y + max_bounds.y) / 2.0f;
        center.z = (min_bounds.z + max_bounds.z) / 2.0f;
    }

    // Load NIfTI
    bool loadNiiFile(const std::string &filename, VolumeData &volume)
    {
        nifti_image *nim = nifti_image_read(filename.c_str(), 1);
        if (!nim)
        {
            std::cerr << "Failed to read NIfTI file: " << filename << std::endl;
            return false;
        }

        volume.dims[0] = nim->nx;
        volume.dims[1] = nim->ny;
        volume.dims[2] = nim->nz;

        // Tracking Min/Max
        float min_val = std::numeric_limits<float>::max();
        float max_val = std::numeric_limits<float>::lowest();

        // Allocate
        volume.data = std::make_unique<float[]>(nim->nvox);

        if (nim->datatype == NIFTI_TYPE_FLOAT32)
        {
            float *src = static_cast<float *>(nim->data);
            for (size_t i = 0; i < nim->nvox; i++)
            {
                volume.data[i] = src[i];
                min_val = std::min(min_val, volume.data[i]);
                max_val = std::max(max_val, volume.data[i]);
            }
        }
        else if (nim->datatype == NIFTI_TYPE_UINT8)
        {
            auto *src = static_cast<uint8_t *>(nim->data);
            for (size_t i = 0; i < nim->nvox; i++)
            {
                volume.data[i] = static_cast<float>(src[i]);
                min_val = std::min(min_val, volume.data[i]);
                max_val = std::max(max_val, volume.data[i]);
            }
        }
        else
        {
            std::cerr << "Unsupported data type: " << nim->datatype << std::endl;
            nifti_image_free(nim);
            return false;
        }

        // Min/Max
        volume.minValue = min_val;
        volume.maxValue = max_val;

        std::cout << "Data range: " << min_val << " to " << max_val << std::endl;
        std::cout << "Volume dimensions: " << volume.dims[0] << " x "
                  << volume.dims[1] << " x " << volume.dims[2] << std::endl;

        nifti_image_free(nim);
        return true;
    }

    // 使用文件对话框打开NIfTI文件
    std::string openNiftiFileDialog()
    {
        std::string filePath = FileDialog::openFile(
            "Open NIfTI File",
            std::filesystem::current_path().string(),
            {{"NIfTI Files", "*.nii"}})[0];

        return filePath;
    }

} // namespace MC