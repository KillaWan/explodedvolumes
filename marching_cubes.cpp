/****************************************************************************
 * Marching Cubes Full Example
 * 
 * 1) Reads a NIfTI file via nifti1_io
 * 2) Applies full Marching Cubes (with edgeTable & triTable)
 * 3) Renders the resulting mesh using OpenGL + GLFW + GLAD
 ****************************************************************************/

 #include <iostream>
 #include <vector>
 #include <cmath>
 
 // =========== GLAD & GLFW ==========
 #include <glad/glad.h>    // 需确保已下载并包含 <glad/glad.h>
 #include <GLFW/glfw3.h>   // 需确保已安装 GLFW
 
 // =========== NIfTI ==========
 #include "nifti1_io.h"    // 需自行安装或编译 nifti_clib
 
 // ====================== 工具结构 & 函数 ======================
 
 // 存储网格顶点
 struct Vertex {
     float x, y, z;
 };
 
 // 存储三角形索引
 typedef unsigned int IndexType;
 
 // 存储三维点，用于插值
 struct Vec3 {
     float x, y, z;
     Vec3() : x(0), y(0), z(0) {}
     Vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
 };
 
 // 获取体素值 (x,y,z) -> 1D 索引
 // 假设 data[x + dims[0]*(y + dims[1]*z)]
 inline float getVolumeValue(const float* data, int x, int y, int z, const int dims[3]) {
     // 注意越界判断，这里简化略
     return data[x + dims[0] * (y + dims[1] * z)];
 }
 
 // 线性插值：在 (p1, valp1) 和 (p2, valp2) 之间，以 isoLevel 为阈值
 // 返回在这条边上等值面的交点坐标
 Vec3 interpolate(float isoLevel, const Vec3& p1, const Vec3& p2, float valp1, float valp2) {
     // 避免除 0
     if (std::fabs(isoLevel - valp1) < 1e-6f)
         return p1;
     if (std::fabs(isoLevel - valp2) < 1e-6f)
         return p2;
     if (std::fabs(valp1 - valp2) < 1e-6f)
         return p1;
 
     float mu = (isoLevel - valp1) / (valp2 - valp1);
     return Vec3(
         p1.x + mu * (p2.x - p1.x),
         p1.y + mu * (p2.y - p1.y),
         p1.z + mu * (p2.z - p1.z)
     );
 }
 
 // Marching Cubes Lookup: edgeTable & triTable
 // ========================================================
 // edgeTable[i] 的二进制形式指示了立方体顶点 (8 个) 与 isoLevel 的关系
 // triTable[i][16] 给出对应的三角形顶点构成
 // 参考资料: https://graphics.stanford.edu/~mdfisher/MarchingCubes.html
 
 static const int edgeTable[256] = {
 0x0000, 0x0109, 0x0232, 0x033b, 0x0464, 0x056d, 0x0656, 0x074f,
 0x08c8, 0x09c1, 0x0afa, 0x0bf3, 0x0cac, 0x0da5, 0x0efe, 0x0ff7,
 0x0190, 0x0089, 0x03b2, 0x02bb, 0x05e4, 0x04ed, 0x07d6, 0x06df,
 0x0958, 0x0851, 0x0b6a, 0x0a63, 0x0d3c, 0x0c35, 0x0f0e, 0x0e07,
 0x0320, 0x0229, 0x0112, 0x001b, 0x0764, 0x066d, 0x0556, 0x044f,
 0x0bc8, 0x0ac1, 0x09fa, 0x08f3, 0x0fac, 0x0ea5, 0x0dfe, 0x0cf7,
 0x05b0, 0x04b9, 0x0782, 0x068b, 0x01d4, 0x00dd, 0x03e6, 0x02ef,
 0x0d68, 0x0c61, 0x0f5a, 0x0e53, 0x090c, 0x0805, 0x0b3e, 0x0a37,
 0x0640, 0x0759, 0x0462, 0x056b, 0x0234, 0x033d, 0x0006, 0x010f,
 0x0ec8, 0x0fc1, 0x0cfa, 0x0df3, 0x0aac, 0x0ba5, 0x08fe, 0x09f7,
 0x07b0, 0x06b9, 0x0582, 0x048b, 0x03d4, 0x02dd, 0x01e6, 0x00ef,
 0x0f68, 0x0e61, 0x0d5a, 0x0c53, 0x0b0c, 0x0a05, 0x093e, 0x0837,
 0x08c0, 0x09c9, 0x0af2, 0x0bfb, 0x0ca4, 0x0dad, 0x0ef6, 0x0fff,
 0x0078, 0x0171, 0x024a, 0x0343, 0x041c, 0x0515, 0x062e, 0x0727,
 0x0cc0, 0x0dc9, 0x0ef2, 0x0ffb, 0x08a4, 0x09ad, 0x0af6, 0x0bff,
 0x0478, 0x0571, 0x064a, 0x0743, 0x001c, 0x0115, 0x022e, 0x0327,
 0x0480, 0x0589, 0x06b2, 0x07bb, 0x00e4, 0x01ed, 0x02d6, 0x03df,
 0x0c58, 0x0d51, 0x0e6a, 0x0f63, 0x083c, 0x0935, 0x0a0e, 0x0b07,
 0x0560, 0x0469, 0x0752, 0x065b, 0x0104, 0x000d, 0x0336, 0x023f,
 0x0dc8, 0x0cc1, 0x0ffa, 0x0ef3, 0x09ac, 0x08a5, 0x0b9e, 0x0a97,
 0x0650, 0x0759, 0x0462, 0x056b, 0x0234, 0x033d, 0x0006, 0x010f,
 0x0ec8, 0x0fc1, 0x0cfa, 0x0df3, 0x0aac, 0x0ba5, 0x08fe, 0x09f7,
 0x07b0, 0x06b9, 0x0582, 0x048b, 0x03d4, 0x02dd, 0x01e6, 0x00ef,
 0x0f68, 0x0e61, 0x0d5a, 0x0c53, 0x0b0c, 0x0a05, 0x093e, 0x0837,
 0x08c0, 0x09c9, 0x0af2, 0x0bfb, 0x0ca4, 0x0dad, 0x0ef6, 0x0fff,
 0x0078, 0x0171, 0x024a, 0x0343, 0x041c, 0x0515, 0x062e, 0x0727,
 0x0cc0, 0x0dc9, 0x0ef2, 0x0ffb, 0x08a4, 0x09ad, 0x0af6, 0x0bff,
 0x0478, 0x0571, 0x064a, 0x0743, 0x001c, 0x0115, 0x022e, 0x0327,
 0x0c80, 0x0d89, 0x0eb2, 0x0fbb, 0x08e4, 0x09ed, 0x0ad6, 0x0bdf,
 0x0458, 0x0551, 0x066a, 0x0763, 0x003c, 0x0135, 0x020e, 0x0307,
 0x0cc0, 0x0dc9, 0x0ef2, 0x0ffb, 0x08a4, 0x09ad, 0x0af6, 0x0bff,
 0x0478, 0x0571, 0x064a, 0x0743, 0x001c, 0x0115, 0x022e, 0x0327
 };
 
 // triTable[i] 每组最多 5 个三角形 (3*5=15) + 1 个 -1 结尾
 static const int triTable[256][16] = {{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{0, 8, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{0, 1, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{1, 8, 3, 9, 8, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{1, 2, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{0, 8, 3, 1, 2, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{9, 2, 10, 0, 2, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{2, 8, 3, 2, 10, 8, 10, 9, 8, -1, -1, -1, -1, -1, -1, -1},
{3, 11, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{0, 11, 2, 8, 11, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{1, 9, 0, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{1, 11, 2, 1, 9, 11, 9, 8, 11, -1, -1, -1, -1, -1, -1, -1},
{3, 10, 1, 11, 10, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{0, 10, 1, 0, 8, 10, 8, 11, 10, -1, -1, -1, -1, -1, -1, -1},
{3, 9, 0, 3, 11, 9, 11, 10, 9, -1, -1, -1, -1, -1, -1, -1},
{9, 8, 10, 10, 8, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{4, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{4, 3, 0, 7, 3, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{0, 1, 9, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{4, 1, 9, 4, 7, 1, 7, 3, 1, -1, -1, -1, -1, -1, -1, -1},
{1, 2, 10, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{3, 4, 7, 3, 0, 4, 1, 2, 10, -1, -1, -1, -1, -1, -1, -1},
{9, 2, 10, 9, 0, 2, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1},
{2, 10, 9, 2, 9, 7, 2, 7, 3, 7, 9, 4, -1, -1, -1, -1},
{8, 4, 7, 3, 11, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{11, 4, 7, 11, 2, 4, 2, 0, 4, -1, -1, -1, -1, -1, -1, -1},
{9, 0, 1, 8, 4, 7, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1},
{4, 7, 11, 9, 4, 11, 9, 11, 2, 9, 2, 1, -1, -1, -1, -1},
{3, 10, 1, 3, 11, 10, 7, 8, 4, -1, -1, -1, -1, -1, -1, -1},
{1, 11, 10, 1, 4, 11, 1, 0, 4, 7, 11, 4, -1, -1, -1, -1},
{4, 7, 8, 9, 0, 11, 9, 11, 10, 11, 0, 3, -1, -1, -1, -1},
{4, 7, 11, 4, 11, 9, 9, 11, 10, -1, -1, -1, -1, -1, -1, -1},
{9, 5, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{9, 5, 4, 0, 8, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{0, 5, 4, 1, 5, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{8, 5, 4, 8, 3, 5, 3, 1, 5, -1, -1, -1, -1, -1, -1, -1},
{1, 2, 10, 9, 5, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{3, 0, 8, 1, 2, 10, 4, 9, 5, -1, -1, -1, -1, -1, -1, -1},
{5, 2, 10, 5, 4, 2, 4, 0, 2, -1, -1, -1, -1, -1, -1, -1},
{2, 10, 5, 3, 2, 5, 3, 5, 4, 3, 4, 8, -1, -1, -1, -1},
{9, 5, 4, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{0, 11, 2, 0, 8, 11, 4, 9, 5, -1, -1, -1, -1, -1, -1, -1},
{0, 5, 4, 0, 1, 5, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1},
{2, 1, 5, 2, 5, 8, 2, 8, 11, 4, 8, 5, -1, -1, -1, -1},
{10, 3, 11, 10, 1, 3, 9, 5, 4, -1, -1, -1, -1, -1, -1, -1},
{4, 9, 5, 0, 8, 1, 8, 10, 1, 8, 11, 10, -1, -1, -1, -1},
{5, 4, 0, 5, 0, 11, 5, 11, 10, 11, 0, 3, -1, -1, -1, -1},
{5, 4, 8, 5, 8, 10, 10, 8, 11, -1, -1, -1, -1, -1, -1, -1},
{9, 7, 8, 5, 7, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{9, 3, 0, 9, 5, 3, 5, 7, 3, -1, -1, -1, -1, -1, -1, -1},
{0, 7, 8, 0, 1, 7, 1, 5, 7, -1, -1, -1, -1, -1, -1, -1},
{1, 5, 3, 3, 5, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{9, 7, 8, 9, 5, 7, 10, 1, 2, -1, -1, -1, -1, -1, -1, -1},
{10, 1, 2, 9, 5, 0, 5, 3, 0, 5, 7, 3, -1, -1, -1, -1},
{8, 0, 2, 8, 2, 5, 8, 5, 7, 10, 5, 2, -1, -1, -1, -1},
{2, 10, 5, 2, 5, 3, 3, 5, 7, -1, -1, -1, -1, -1, -1, -1},
{7, 9, 5, 7, 8, 9, 3, 11, 2, -1, -1, -1, -1, -1, -1, -1},
{9, 5, 7, 9, 7, 2, 9, 2, 0, 2, 7, 11, -1, -1, -1, -1},
{2, 3, 11, 0, 1, 8, 1, 7, 8, 1, 5, 7, -1, -1, -1, -1},
{11, 2, 1, 11, 1, 7, 7, 1, 5, -1, -1, -1, -1, -1, -1, -1},
{9, 5, 8, 8, 5, 7, 10, 1, 3, 10, 3, 11, -1, -1, -1, -1},
{5, 7, 0, 5, 0, 9, 7, 11, 0, 1, 0, 10, 11, 10, 0, -1},
{11, 10, 0, 11, 0, 3, 10, 5, 0, 8, 0, 7, 5, 7, 0, -1},
{11, 10, 5, 7, 11, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{10, 6, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{0, 8, 3, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{9, 0, 1, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{1, 8, 3, 1, 9, 8, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1},
{1, 6, 5, 2, 6, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{1, 6, 5, 1, 2, 6, 3, 0, 8, -1, -1, -1, -1, -1, -1, -1},
{9, 6, 5, 9, 0, 6, 0, 2, 6, -1, -1, -1, -1, -1, -1, -1},
{5, 9, 8, 5, 8, 2, 5, 2, 6, 3, 2, 8, -1, -1, -1, -1},
{2, 3, 11, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{11, 0, 8, 11, 2, 0, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1},
{0, 1, 9, 2, 3, 11, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1},
{5, 10, 6, 1, 9, 2, 9, 11, 2, 9, 8, 11, -1, -1, -1, -1},
{6, 3, 11, 6, 5, 3, 5, 1, 3, -1, -1, -1, -1, -1, -1, -1},
{0, 8, 11, 0, 11, 5, 0, 5, 1, 5, 11, 6, -1, -1, -1, -1},
{3, 11, 6, 0, 3, 6, 0, 6, 5, 0, 5, 9, -1, -1, -1, -1},
{6, 5, 9, 6, 9, 11, 11, 9, 8, -1, -1, -1, -1, -1, -1, -1},
{5, 10, 6, 4, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{4, 3, 0, 4, 7, 3, 6, 5, 10, -1, -1, -1, -1, -1, -1, -1},
{1, 9, 0, 5, 10, 6, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1},
{10, 6, 5, 1, 9, 7, 1, 7, 3, 7, 9, 4, -1, -1, -1, -1},
{6, 1, 2, 6, 5, 1, 4, 7, 8, -1, -1, -1, -1, -1, -1, -1},
{1, 2, 5, 5, 2, 6, 3, 0, 4, 3, 4, 7, -1, -1, -1, -1},
{8, 4, 7, 9, 0, 5, 0, 6, 5, 0, 2, 6, -1, -1, -1, -1},
{7, 3, 9, 7, 9, 4, 3, 2, 9, 5, 9, 6, 2, 6, 9, -1},
{3, 11, 2, 7, 8, 4, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1},
{5, 10, 6, 4, 7, 2, 4, 2, 0, 2, 7, 11, -1, -1, -1, -1},
{0, 1, 9, 4, 7, 8, 2, 3, 11, 5, 10, 6, -1, -1, -1, -1},
{9, 2, 1, 9, 11, 2, 9, 4, 11, 7, 11, 4, 5, 10, 6, -1},
{8, 4, 7, 3, 11, 5, 3, 5, 1, 5, 11, 6, -1, -1, -1, -1},
{5, 1, 11, 5, 11, 6, 1, 0, 11, 7, 11, 4, 0, 4, 11, -1},
{0, 5, 9, 0, 6, 5, 0, 3, 6, 11, 6, 3, 8, 4, 7, -1},
{6, 5, 9, 6, 9, 11, 4, 7, 9, 7, 11, 9, -1, -1, -1, -1},
{10, 4, 9, 6, 4, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{4, 10, 6, 4, 9, 10, 0, 8, 3, -1, -1, -1, -1, -1, -1, -1},
{10, 0, 1, 10, 6, 0, 6, 4, 0, -1, -1, -1, -1, -1, -1, -1},
{8, 3, 1, 8, 1, 6, 8, 6, 4, 6, 1, 10, -1, -1, -1, -1},
{1, 4, 9, 1, 2, 4, 2, 6, 4, -1, -1, -1, -1, -1, -1, -1},
{3, 0, 8, 1, 2, 9, 2, 4, 9, 2, 6, 4, -1, -1, -1, -1},
{0, 2, 4, 4, 2, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{8, 3, 2, 8, 2, 4, 4, 2, 6, -1, -1, -1, -1, -1, -1, -1},
{10, 4, 9, 10, 6, 4, 11, 2, 3, -1, -1, -1, -1, -1, -1, -1},
{0, 8, 2, 2, 8, 11, 4, 9, 10, 4, 10, 6, -1, -1, -1, -1},
{3, 11, 2, 0, 1, 6, 0, 6, 4, 6, 1, 10, -1, -1, -1, -1},
{6, 4, 1, 6, 1, 10, 4, 8, 1, 2, 1, 11, 8, 11, 1, -1},
{9, 6, 4, 9, 3, 6, 9, 1, 3, 11, 6, 3, -1, -1, -1, -1},
{8, 11, 1, 8, 1, 0, 11, 6, 1, 9, 1, 4, 6, 4, 1, -1},
{3, 11, 6, 3, 6, 0, 0, 6, 4, -1, -1, -1, -1, -1, -1, -1},
{6, 4, 8, 11, 6, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{7, 10, 6, 7, 8, 10, 8, 9, 10, -1, -1, -1, -1, -1, -1, -1},
{0, 7, 3, 0, 10, 7, 0, 9, 10, 6, 7, 10, -1, -1, -1, -1},
{10, 6, 7, 1, 10, 7, 1, 7, 8, 1, 8, 0, -1, -1, -1, -1},
{10, 6, 7, 10, 7, 1, 1, 7, 3, -1, -1, -1, -1, -1, -1, -1},
{1, 2, 6, 1, 6, 8, 1, 8, 9, 8, 6, 7, -1, -1, -1, -1},
{2, 6, 9, 2, 9, 1, 6, 7, 9, 0, 9, 3, 7, 3, 9, -1},
{7, 8, 0, 7, 0, 6, 6, 0, 2, -1, -1, -1, -1, -1, -1, -1},
{7, 3, 2, 6, 7, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{2, 3, 11, 10, 6, 8, 10, 8, 9, 8, 6, 7, -1, -1, -1, -1},
{2, 0, 7, 2, 7, 11, 0, 9, 7, 6, 7, 10, 9, 10, 7, -1},
{1, 8, 0, 1, 7, 8, 1, 10, 7, 6, 7, 10, 2, 3, 11, -1},
{11, 2, 1, 11, 1, 7, 10, 6, 1, 6, 7, 1, -1, -1, -1, -1},
{8, 9, 6, 8, 6, 7, 9, 1, 6, 11, 6, 3, 1, 3, 6, -1},
{0, 9, 1, 11, 6, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{7, 8, 0, 7, 0, 6, 3, 11, 0, 11, 6, 0, -1, -1, -1, -1},
{7, 11, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{7, 6, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{3, 0, 8, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{0, 1, 9, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{8, 1, 9, 8, 3, 1, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1},
{10, 1, 2, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{1, 2, 10, 3, 0, 8, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1},
{2, 9, 0, 2, 10, 9, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1},
{6, 11, 7, 2, 10, 3, 10, 8, 3, 10, 9, 8, -1, -1, -1, -1},
{7, 2, 3, 6, 2, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{7, 0, 8, 7, 6, 0, 6, 2, 0, -1, -1, -1, -1, -1, -1, -1},
{2, 7, 6, 2, 3, 7, 0, 1, 9, -1, -1, -1, -1, -1, -1, -1},
{1, 6, 2, 1, 8, 6, 1, 9, 8, 8, 7, 6, -1, -1, -1, -1},
{10, 7, 6, 10, 1, 7, 1, 3, 7, -1, -1, -1, -1, -1, -1, -1},
{10, 7, 6, 1, 7, 10, 1, 8, 7, 1, 0, 8, -1, -1, -1, -1},
{0, 3, 7, 0, 7, 10, 0, 10, 9, 6, 10, 7, -1, -1, -1, -1},
{7, 6, 10, 7, 10, 8, 8, 10, 9, -1, -1, -1, -1, -1, -1, -1},
{6, 8, 4, 11, 8, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{3, 6, 11, 3, 0, 6, 0, 4, 6, -1, -1, -1, -1, -1, -1, -1},
{8, 6, 11, 8, 4, 6, 9, 0, 1, -1, -1, -1, -1, -1, -1, -1},
{9, 4, 6, 9, 6, 3, 9, 3, 1, 11, 3, 6, -1, -1, -1, -1},
{6, 8, 4, 6, 11, 8, 2, 10, 1, -1, -1, -1, -1, -1, -1, -1},
{1, 2, 10, 3, 0, 11, 0, 6, 11, 0, 4, 6, -1, -1, -1, -1},
{4, 11, 8, 4, 6, 11, 0, 2, 9, 2, 10, 9, -1, -1, -1, -1},
{10, 9, 3, 10, 3, 2, 9, 4, 3, 11, 3, 6, 4, 6, 3, -1},
{8, 2, 3, 8, 4, 2, 4, 6, 2, -1, -1, -1, -1, -1, -1, -1},
{0, 4, 2, 4, 6, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{1, 9, 0, 2, 3, 4, 2, 4, 6, 4, 3, 8, -1, -1, -1, -1},
{1, 9, 4, 1, 4, 2, 2, 4, 6, -1, -1, -1, -1, -1, -1, -1},
{8, 1, 3, 8, 6, 1, 8, 4, 6, 6, 10, 1, -1, -1, -1, -1},
{10, 1, 0, 10, 0, 6, 6, 0, 4, -1, -1, -1, -1, -1, -1, -1},
{4, 6, 3, 4, 3, 8, 6, 10, 3, 0, 3, 9, 10, 9, 3, -1},
{10, 9, 4, 6, 10, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{4, 9, 5, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{0, 8, 3, 4, 9, 5, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1},
{5, 0, 1, 5, 4, 0, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1},
{11, 7, 6, 8, 3, 4, 3, 5, 4, 3, 1, 5, -1, -1, -1, -1},
{9, 5, 4, 10, 1, 2, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1},
{6, 11, 7, 1, 2, 10, 0, 8, 3, 4, 9, 5, -1, -1, -1, -1},
{7, 6, 11, 5, 4, 10, 4, 2, 10, 4, 0, 2, -1, -1, -1, -1},
{3, 4, 8, 3, 5, 4, 3, 2, 5, 10, 5, 2, 11, 7, 6, -1},
{7, 2, 3, 7, 6, 2, 5, 4, 9, -1, -1, -1, -1, -1, -1, -1},
{9, 5, 4, 0, 8, 6, 0, 6, 2, 6, 8, 7, -1, -1, -1, -1},
{3, 6, 2, 3, 7, 6, 1, 5, 0, 5, 4, 0, -1, -1, -1, -1},
{6, 2, 8, 6, 8, 7, 2, 1, 8, 4, 8, 5, 1, 5, 8, -1},
{9, 5, 4, 10, 1, 6, 1, 7, 6, 1, 3, 7, -1, -1, -1, -1},
{1, 6, 10, 1, 7, 6, 1, 0, 7, 8, 7, 0, 9, 5, 4, -1},
{4, 0, 10, 4, 10, 5, 0, 3, 10, 6, 10, 7, 3, 7, 10, -1},
{7, 6, 10, 7, 10, 8, 5, 4, 10, 4, 8, 10, -1, -1, -1, -1},
{6, 9, 5, 6, 11, 9, 11, 8, 9, -1, -1, -1, -1, -1, -1, -1},
{3, 6, 11, 0, 6, 3, 0, 5, 6, 0, 9, 5, -1, -1, -1, -1},
{0, 11, 8, 0, 5, 11, 0, 1, 5, 5, 6, 11, -1, -1, -1, -1},
{6, 11, 3, 6, 3, 5, 5, 3, 1, -1, -1, -1, -1, -1, -1, -1},
{1, 2, 10, 9, 5, 11, 9, 11, 8, 11, 5, 6, -1, -1, -1, -1},
{0, 11, 3, 0, 6, 11, 0, 9, 6, 5, 6, 9, 1, 2, 10, -1},
{11, 8, 5, 11, 5, 6, 8, 0, 5, 10, 5, 2, 0, 2, 5, -1},
{6, 11, 3, 6, 3, 5, 2, 10, 3, 10, 5, 3, -1, -1, -1, -1},
{5, 8, 9, 5, 2, 8, 5, 6, 2, 3, 8, 2, -1, -1, -1, -1},
{9, 5, 6, 9, 6, 0, 0, 6, 2, -1, -1, -1, -1, -1, -1, -1},
{1, 5, 8, 1, 8, 0, 5, 6, 8, 3, 8, 2, 6, 2, 8, -1},
{1, 5, 6, 2, 1, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{1, 3, 6, 1, 6, 10, 3, 8, 6, 5, 6, 9, 8, 9, 6, -1},
{10, 1, 0, 10, 0, 6, 9, 5, 0, 5, 6, 0, -1, -1, -1, -1},
{0, 3, 8, 5, 6, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{10, 5, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{11, 5, 10, 7, 5, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{11, 5, 10, 11, 7, 5, 8, 3, 0, -1, -1, -1, -1, -1, -1, -1},
{5, 11, 7, 5, 10, 11, 1, 9, 0, -1, -1, -1, -1, -1, -1, -1},
{10, 7, 5, 10, 11, 7, 9, 8, 1, 8, 3, 1, -1, -1, -1, -1},
{11, 1, 2, 11, 7, 1, 7, 5, 1, -1, -1, -1, -1, -1, -1, -1},
{0, 8, 3, 1, 2, 7, 1, 7, 5, 7, 2, 11, -1, -1, -1, -1},
{9, 7, 5, 9, 2, 7, 9, 0, 2, 2, 11, 7, -1, -1, -1, -1},
{7, 5, 2, 7, 2, 11, 5, 9, 2, 3, 2, 8, 9, 8, 2, -1},
{2, 5, 10, 2, 3, 5, 3, 7, 5, -1, -1, -1, -1, -1, -1, -1},
{8, 2, 0, 8, 5, 2, 8, 7, 5, 10, 2, 5, -1, -1, -1, -1},
{9, 0, 1, 5, 10, 3, 5, 3, 7, 3, 10, 2, -1, -1, -1, -1},
{9, 8, 2, 9, 2, 1, 8, 7, 2, 10, 2, 5, 7, 5, 2, -1},
{1, 3, 5, 3, 7, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{0, 8, 7, 0, 7, 1, 1, 7, 5, -1, -1, -1, -1, -1, -1, -1},
{9, 0, 3, 9, 3, 5, 5, 3, 7, -1, -1, -1, -1, -1, -1, -1},
{9, 8, 7, 5, 9, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{5, 8, 4, 5, 10, 8, 10, 11, 8, -1, -1, -1, -1, -1, -1, -1},
{5, 0, 4, 5, 11, 0, 5, 10, 11, 11, 3, 0, -1, -1, -1, -1},
{0, 1, 9, 8, 4, 10, 8, 10, 11, 10, 4, 5, -1, -1, -1, -1},
{10, 11, 4, 10, 4, 5, 11, 3, 4, 9, 4, 1, 3, 1, 4, -1},
{2, 5, 1, 2, 8, 5, 2, 11, 8, 4, 5, 8, -1, -1, -1, -1},
{0, 4, 11, 0, 11, 3, 4, 5, 11, 2, 11, 1, 5, 1, 11, -1},
{0, 2, 5, 0, 5, 9, 2, 11, 5, 4, 5, 8, 11, 8, 5, -1},
{9, 4, 5, 2, 11, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{2, 5, 10, 3, 5, 2, 3, 4, 5, 3, 8, 4, -1, -1, -1, -1},
{5, 10, 2, 5, 2, 4, 4, 2, 0, -1, -1, -1, -1, -1, -1, -1},
{3, 10, 2, 3, 5, 10, 3, 8, 5, 4, 5, 8, 0, 1, 9, -1},
{5, 10, 2, 5, 2, 4, 1, 9, 2, 9, 4, 2, -1, -1, -1, -1},
{8, 4, 5, 8, 5, 3, 3, 5, 1, -1, -1, -1, -1, -1, -1, -1},
{0, 4, 5, 1, 0, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{8, 4, 5, 8, 5, 3, 9, 0, 5, 0, 3, 5, -1, -1, -1, -1},
{9, 4, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{4, 11, 7, 4, 9, 11, 9, 10, 11, -1, -1, -1, -1, -1, -1, -1},
{0, 8, 3, 4, 9, 7, 9, 11, 7, 9, 10, 11, -1, -1, -1, -1},
{1, 10, 11, 1, 11, 4, 1, 4, 0, 7, 4, 11, -1, -1, -1, -1},
{3, 1, 4, 3, 4, 8, 1, 10, 4, 7, 4, 11, 10, 11, 4, -1},
{4, 11, 7, 9, 11, 4, 9, 2, 11, 9, 1, 2, -1, -1, -1, -1},
{9, 7, 4, 9, 11, 7, 9, 1, 11, 2, 11, 1, 0, 8, 3, -1},
{11, 7, 4, 11, 4, 2, 2, 4, 0, -1, -1, -1, -1, -1, -1, -1},
{11, 7, 4, 11, 4, 2, 8, 3, 4, 3, 2, 4, -1, -1, -1, -1},
{2, 9, 10, 2, 7, 9, 2, 3, 7, 7, 4, 9, -1, -1, -1, -1},
{9, 10, 7, 9, 7, 4, 10, 2, 7, 8, 7, 0, 2, 0, 7, -1},
{3, 7, 10, 3, 10, 2, 7, 4, 10, 1, 10, 0, 4, 0, 10, -1},
{1, 10, 2, 8, 7, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{4, 9, 1, 4, 1, 7, 7, 1, 3, -1, -1, -1, -1, -1, -1, -1},
{4, 9, 1, 4, 1, 7, 0, 8, 1, 8, 7, 1, -1, -1, -1, -1},
{4, 0, 3, 7, 4, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{4, 8, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{9, 10, 8, 10, 11, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{3, 0, 9, 3, 9, 11, 11, 9, 10, -1, -1, -1, -1, -1, -1, -1},
{0, 1, 10, 0, 10, 8, 8, 10, 11, -1, -1, -1, -1, -1, -1, -1},
{3, 1, 10, 11, 3, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{1, 2, 11, 1, 11, 9, 9, 11, 8, -1, -1, -1, -1, -1, -1, -1},
{3, 0, 9, 3, 9, 11, 1, 2, 9, 2, 11, 9, -1, -1, -1, -1},
{0, 2, 11, 8, 0, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{3, 2, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{2, 3, 8, 2, 8, 10, 10, 8, 9, -1, -1, -1, -1, -1, -1, -1},
{9, 10, 2, 0, 9, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{2, 3, 8, 2, 8, 10, 0, 1, 8, 1, 10, 8, -1, -1, -1, -1},
{1, 10, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{1, 3, 8, 9, 1, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{0, 9, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{0, 3, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}};
 
 static const Vec3 cubeCornerOffsets[8] = {
     Vec3(0,0,0), Vec3(1,0,0), Vec3(1,1,0), Vec3(0,1,0),
     Vec3(0,0,1), Vec3(1,0,1), Vec3(1,1,1), Vec3(0,1,1)
 };
 
 // =========== 读取 NIfTI 文件 ===========
 bool loadNiiFile(const char* filename, float*& data, int dims[3]) {
    nifti_image* nim = nifti_image_read(filename, 1);
    if (!nim) {
        std::cerr << "读取 NIfTI 文件失败: " << filename << std::endl;
        return false;
    }
    dims[0] = nim->nx;
    dims[1] = nim->ny;
    dims[2] = nim->nz;

    // 如果是 float32，直接用
    if (nim->datatype == NIFTI_TYPE_FLOAT32) {
        data = static_cast<float*>(nim->data);
    }
    // 如果是 uint8 (datatype=2)，做手动转换
    else if (nim->datatype == NIFTI_TYPE_UINT8) {
        auto* src = static_cast<uint8_t*>(nim->data);
        size_t nvox = static_cast<size_t>(nim->nvox);  // 总体素数
        float* temp = new float[nvox];
        for (size_t i = 0; i < nvox; i++) {
            temp[i] = static_cast<float>(src[i]);
        }
        data = temp;
    }
    else {
        std::cerr << "不支持的类型: " << nim->datatype << std::endl;
        nifti_image_free(nim);
        return false;
    }

    // 注意：如果直接使用 nim->data，就别调用 nifti_image_free(nim)；  
    // 如果你做了拷贝(temp)，可以在此处 free(nim)：
    // nifti_image_free(nim);

    return true;
}

 
 // =========== Marching Cubes 核心函数：polygoniseCube ===========
 // 对一个立方体单元进行多边形化，返回生成的三角形顶点
 void polygoniseCube(const float* volume, const int dims[3],
                     int cellX, int cellY, int cellZ,
                     float isoLevel,
                     std::vector<Vec3>& triangleVerts)
 {
     // 立方体 8 个顶点坐标 & 标量值
     float cubeValues[8];
     Vec3 cubePos[8];
     for (int i = 0; i < 8; i++) {
         Vec3 offset = cubeCornerOffsets[i];
         int x = cellX + (int)offset.x;
         int y = cellY + (int)offset.y;
         int z = cellZ + (int)offset.z;
         cubeValues[i] = getVolumeValue(volume, x, y, z, dims);
         cubePos[i] = Vec3((float)x, (float)y, (float)z);
     }
 
     // 计算 cubeIndex
     int cubeIndex = 0;
     if (cubeValues[0] < isoLevel) cubeIndex |= 1;
     if (cubeValues[1] < isoLevel) cubeIndex |= 2;
     if (cubeValues[2] < isoLevel) cubeIndex |= 4;
     if (cubeValues[3] < isoLevel) cubeIndex |= 8;
     if (cubeValues[4] < isoLevel) cubeIndex |= 16;
     if (cubeValues[5] < isoLevel) cubeIndex |= 32;
     if (cubeValues[6] < isoLevel) cubeIndex |= 64;
     if (cubeValues[7] < isoLevel) cubeIndex |= 128;
 
     // 查 edgeTable，若为 0 说明不与 iso 面相交
     int edges = edgeTable[cubeIndex];
     if (edges == 0) return;
 
     // 计算 12 条边的交点
     Vec3 vertList[12];
     if (edges & 1)
         vertList[0] = interpolate(isoLevel, cubePos[0], cubePos[1], cubeValues[0], cubeValues[1]);
     if (edges & 2)
         vertList[1] = interpolate(isoLevel, cubePos[1], cubePos[2], cubeValues[1], cubeValues[2]);
     if (edges & 4)
         vertList[2] = interpolate(isoLevel, cubePos[2], cubePos[3], cubeValues[2], cubeValues[3]);
     if (edges & 8)
         vertList[3] = interpolate(isoLevel, cubePos[3], cubePos[0], cubeValues[3], cubeValues[0]);
     if (edges & 16)
         vertList[4] = interpolate(isoLevel, cubePos[4], cubePos[5], cubeValues[4], cubeValues[5]);
     if (edges & 32)
         vertList[5] = interpolate(isoLevel, cubePos[5], cubePos[6], cubeValues[5], cubeValues[6]);
     if (edges & 64)
         vertList[6] = interpolate(isoLevel, cubePos[6], cubePos[7], cubeValues[6], cubeValues[7]);
     if (edges & 128)
         vertList[7] = interpolate(isoLevel, cubePos[7], cubePos[4], cubeValues[7], cubeValues[4]);
     if (edges & 256)
         vertList[8] = interpolate(isoLevel, cubePos[0], cubePos[4], cubeValues[0], cubeValues[4]);
     if (edges & 512)
         vertList[9] = interpolate(isoLevel, cubePos[1], cubePos[5], cubeValues[1], cubeValues[5]);
     if (edges & 1024)
         vertList[10] = interpolate(isoLevel, cubePos[2], cubePos[6], cubeValues[2], cubeValues[6]);
     if (edges & 2048)
         vertList[11] = interpolate(isoLevel, cubePos[3], cubePos[7], cubeValues[3], cubeValues[7]);
 
     // 根据 triTable，生成三角形
     for (int i = 0; triTable[cubeIndex][i] != -1; i += 3) {
         int idx0 = triTable[cubeIndex][i + 0];
         int idx1 = triTable[cubeIndex][i + 1];
         int idx2 = triTable[cubeIndex][i + 2];
         triangleVerts.push_back(vertList[idx0]);
         triangleVerts.push_back(vertList[idx1]);
         triangleVerts.push_back(vertList[idx2]);
     }
 }
 
 // =========== 完整 Marching Cubes，遍历所有体素 ===========
 void marchingCubesFull(const float* volume, const int dims[3],
                        float isoLevel,
                        std::vector<Vertex>& outVertices,
                        std::vector<IndexType>& outIndices)
 {
     std::vector<Vec3> triangles;  // 临时存放所有三角形顶点
 
     // 遍历体素 (x from 0..dims[0]-2, y from 0..dims[1]-2, z from 0..dims[2]-2)
     // 假设体素间距=1，若实际有像素间距，需要乘以 spacing
     for (int z = 0; z < dims[2] - 1; z++) {
         for (int y = 0; y < dims[1] - 1; y++) {
             for (int x = 0; x < dims[0] - 1; x++) {
                 polygoniseCube(volume, dims, x, y, z, isoLevel, triangles);
             }
         }
     }
 
     // 将 triangles 中的坐标写入 outVertices，并生成对应索引
     // 这里不做顶点合并，也不计算法线，只是直接堆积
     outVertices.reserve(triangles.size());
     outIndices.reserve(triangles.size());
     for (size_t i = 0; i < triangles.size(); i++) {
         Vertex v;
         v.x = triangles[i].x;
         v.y = triangles[i].y;
         v.z = triangles[i].z;
         outVertices.push_back(v);
         outIndices.push_back((IndexType)i);
     }
 }
 
 // =========== 简单的顶点 & 片段着色器 (Raw String Literal) ===========
 const char* vertexShaderSource = R"glsl(
 #version 330 core
 layout (location = 0) in vec3 aPos;
 uniform mat4 MVP;
 
 void main()
 {
     gl_Position = MVP * vec4(aPos, 1.0);
 }
 )glsl";
 
 const char* fragmentShaderSource = R"glsl(
 #version 330 core
 out vec4 FragColor;
 
 void main()
 {
     FragColor = vec4(0.8, 0.5, 0.2, 1.0);
 }
 )glsl";
 
 // 窗口尺寸变动时回调
 static void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
     glViewport(0, 0, width, height);
 }
 
 int main() {
     // =========== 初始化 GLFW ===========
     if (!glfwInit()) {
         std::cerr << "GLFW 初始化失败" << std::endl;
         return -1;
     }
 
     // macOS 使用 4.1 Core Profile
     glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
     glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
     glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
 #ifdef __APPLE__
     glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
 #endif
 
     GLFWwindow* window = glfwCreateWindow(800, 600, "Full Marching Cubes", nullptr, nullptr);
     if (!window) {
         std::cerr << "创建窗口失败" << std::endl;
         glfwTerminate();
         return -1;
     }
     glfwMakeContextCurrent(window);
     glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
 
     // =========== 初始化 GLAD ===========
     if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
         std::cerr << "GLAD 初始化失败" << std::endl;
         return -1;
     }
 
     // =========== 读取 NIfTI 文件 ===========
     float* volumeData = nullptr;
     int dims[3] = {0,0,0};
     const char* niiFilename = "chris_MRA.nii"; // 换成你的 NIfTI 文件
     if (!loadNiiFile(niiFilename, volumeData, dims)) {
         std::cerr << "无法读取 NIfTI 文件" << std::endl;
         return -1;
     }
 
     // =========== Marching Cubes ===========
     std::vector<Vertex> vertices;
     std::vector<IndexType> indices;
     float isoLevel = 0.5f; // 根据数据分布选择合适阈值
     marchingCubesFull(volumeData, dims, isoLevel, vertices, indices);
 
     // =========== 创建 VAO, VBO, EBO ===========
     unsigned int VAO, VBO, EBO;
     glGenVertexArrays(1, &VAO);
     glGenBuffers(1, &VBO);
     glGenBuffers(1, &EBO);
 
     glBindVertexArray(VAO);
 
     // VBO
     glBindBuffer(GL_ARRAY_BUFFER, VBO);
     glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex),
                  vertices.data(), GL_STATIC_DRAW);
 
     // EBO
     glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
     glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(IndexType),
                  indices.data(), GL_STATIC_DRAW);
 
     // 顶点属性
     glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
     glEnableVertexAttribArray(0);
 
     glBindVertexArray(0);
 
     // =========== 编译 & 链接着色器 ===========
     auto compileShader = [](GLenum type, const char* src)->unsigned int {
         unsigned int shader = glCreateShader(type);
         glShaderSource(shader, 1, &src, nullptr);
         glCompileShader(shader);
         int success;
         glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
         if (!success) {
             char infoLog[512];
             glGetShaderInfoLog(shader, 512, nullptr, infoLog);
             std::cerr << "着色器编译失败: " << infoLog << std::endl;
         }
         return shader;
     };
 
     unsigned int vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
     unsigned int fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
 
     unsigned int shaderProgram = glCreateProgram();
     glAttachShader(shaderProgram, vertexShader);
     glAttachShader(shaderProgram, fragmentShader);
     glLinkProgram(shaderProgram);
 
     {
         int success;
         glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
         if (!success) {
             char infoLog[512];
             glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
             std::cerr << "着色器链接失败: " << infoLog << std::endl;
         }
     }
 
     glDeleteShader(vertexShader);
     glDeleteShader(fragmentShader);
 
     // =========== 渲染循环 ===========
     glEnable(GL_DEPTH_TEST);
 
     while (!glfwWindowShouldClose(window)) {
         // 处理输入
         if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
             glfwSetWindowShouldClose(window, true);
         }
 
         // 清屏
         glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
         glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
 
         // 使用着色器
         glUseProgram(shaderProgram);
 
         // 简单用单位矩阵作为 MVP
         float MVP[16] = {
             1,0,0,0,
             0,1,0,0,
             0,0,1,0,
             0,0,0,1
         };
         int mvpLoc = glGetUniformLocation(shaderProgram, "MVP");
         glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, MVP);
 
         // 绘制
         glBindVertexArray(VAO);
         glDrawElements(GL_TRIANGLES, (GLsizei)indices.size(), GL_UNSIGNED_INT, 0);
         glBindVertexArray(0);
 
         glfwSwapBuffers(window);
         glfwPollEvents();
     }
 
     // =========== 清理 ===========
    }
 