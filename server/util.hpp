#ifndef __M_UTIL_H__
#define __M_UTIL_H__

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <experimental/filesystem> //c++17
#include <jsoncpp/json/json.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "bundle.h"

namespace cloud
{
    namespace fs = std::experimental::filesystem;
    class FileUtil
    {
    public:
        FileUtil(const std::string &filename)
            : _filename(filename)
        {
        }
        int64_t FileSize()
        {
            struct stat st;
            if (stat(_filename.c_str(), &st) < 0)
            {
                std::cout << "Get FileSize Failed" << std::endl;
                return -1;
            }
            return st.st_size;
        }
        time_t LastMTime() // 最后更改时间
        {
            struct stat st;
            if (stat(_filename.c_str(), &st) < 0)
            {
                std::cout << "Get Mtime Failed" << std::endl;
                return -2;
            }
            return st.st_mtime;
        }
        time_t LastATime() // 最后访问时间
        {
            struct stat st;
            if (stat(_filename.c_str(), &st) < 0)
            {
                std::cout << "Get Atime Failed" << std::endl;
                return -3;
            }
            return st.st_atime;
        }
        std::string FileName()
        {
            // ./abc/test.txt
            size_t pos = _filename.find_last_of("/");
            if (pos == std::string::npos)
                return _filename;             // 没有找到‘/’说明就是文件名
            return _filename.substr(pos + 1); // 截取从pos+1到末尾的字符串
        }
        bool Remove()
        {
            if(this->Exists() == false)
            {
                return true;
            }
            remove(_filename.c_str());
            return true;
        }
        bool GetPosLen(std::string *body, size_t pos, size_t len) // 获取指定位置 指定长度 的数据
        {
            // 判断要获取范围是否合法
            size_t fsize = this->FileSize();
            if (pos + len > fsize)
            {
                std::cout << "Get FileLen is Error" << std::endl;
                return false;
            }
            // 查看文件是否存在
            std::ifstream ifs;
            ifs.open(_filename, std::ios::binary);
            if (ifs.is_open() == false)
            {
                std::cout << "Read Open File Failed" << std::endl;
                return false;
            }
            // 获取数据
            ifs.seekg(pos, std::ios::beg); // 将文件指针设置为beg+pos
            body->resize(len);
            ifs.read(&(*body)[0], len);
            if (ifs.good() == false)
            {
                std::cout << "Get File Content Failed" << std::endl;
                ifs.close();
                return false;
            }
            ifs.close();
            return true;
        }
        bool GetContent(std::string *body) // 获取所有的数据
        {
            size_t fsize = this->FileSize();
            return GetPosLen(body, 0, fsize);
        }
        bool SetContent(const std::string &body) // 写入数据
        {
            std::ofstream ofs;
            ofs.open(_filename, std::ios::binary);
            if (ofs.is_open() == false)
            {
                std::cout << "Write Open File Failed" << std::endl;
                return false;
            }
            ofs.write(&body[0], body.size());
            if (ofs.good() == false)
            {
                std::cout << "Write File Content Failed" << std::endl;
                ofs.close();
                return false;
            }
            ofs.close();
            return true;
        }
        bool Compress(const std::string &packname) // 压缩文件
        {
            // 1. 获取源文件数据
            std::string body;
            if (this->GetContent(&body) == false)
            {
                std::cout << "Compress Get File Content Failed" << std::endl;
                return false;
            }
            // 2. 对数据进行压缩
            std::string packed = bundle::pack(bundle::LZIP, body); // 将body压缩为packed
            // 3. 将压缩的数据储存到压缩包文件中
            FileUtil fu(packname);
            if (fu.SetContent(packed) == false)
            {
                std::cout << "Compress Write Packed Data Failed" << std::endl;
                return false;
            }
            return true;
        }
        bool Uncompress(const std::string &filename) // 解压缩文件
        {
            // 1. 将压缩包数据读取
            std::string body;
            if (this->GetContent(&body) == false)
            {
                std::cout << "Uncompress Get File Content Failed" << std::endl;
                return false;
            }
            // 2. 将数据解压缩
            std::string unpacked = bundle::unpack(body); // 将body解压缩为packed
            // 3. 将解压缩的数据储存到文件中
            FileUtil fu(filename);
            if (fu.SetContent(unpacked) == false)
            {
                std::cout << "Uncompress Write Unpacked Data Failed" << std::endl;
                return false;
            }
            return true;
        }
        bool Exists()
        {
            return fs::exists(_filename);
        }
        bool CreateDirectory()
        {
            if (this->Exists())
                return true;
            return fs::create_directories(_filename);
        }
        bool ScanDirectory(std::vector<std::string> *array)
        {
            for (auto &p : fs::directory_iterator(_filename))
            {
                if (fs::is_directory(p) == true)
                {
                    continue;
                }
                // relative_path 带有路径的文件名
                array->push_back(fs::path(p).relative_path().string());
            }
            return true;
        }

    private:
        std::string _filename; // 路径+文件名
    };

    class JsonUtil
    {
    public:
        static bool Serialize(const Json::Value &root, std::string *str)
        {
            Json::StreamWriterBuilder swb;
            std::unique_ptr<Json::StreamWriter> sw(swb.newStreamWriter());
            std::stringstream ss;
            int ret = sw->write(root, &ss);
            if(ret != 0)
            {
                std::cout << "Json Write Failed" << std::endl;
                return false;
            }
            *str = ss.str();
            return true;
        }
        static bool UnSerialize(const std::string &str, Json::Value *root)
        {
            Json::CharReaderBuilder crb;
            std::unique_ptr<Json::CharReader> cr(crb.newCharReader());
            std::string err;
            bool ret = cr->parse(str.c_str(), str.c_str()+str.size(), root, &err);
            if(ret == false)
            {
                std::cout << "Parse Error: " << err << std::endl;
                return false;
            }
            return true;
        }
    };
}

#endif