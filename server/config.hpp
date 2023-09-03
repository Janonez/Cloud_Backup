#ifndef __M_CONFIG_H__
#define __M_CONFIG_H__

#include <mutex>

#include "util.hpp"

#define CONFIG_FILE "./cloud.conf"

namespace cloud
{
    class Config
    {
    private:
        Config()
        {
            ReadConfigFile();
        }
        static Config *_instance;
        static std::mutex _mutex;

    private:
        int _hot_time;                // 热点判断时间
        int _server_port;             // 服务器监听端口
        std::string _server_ip;       // 服务器IP地址
        std::string _download_prefix; // 下载url前缀路径
        std::string _packfile_suffix; // 压缩包后缀名称
        std::string _pack_dir;        // 压缩包存放目录
        std::string _back_dir;        // 备份文件存放目录
        std::string _backup_file;     // 数据信息存放文件

        bool ReadConfigFile()
        {
            FileUtil fu(CONFIG_FILE);
            std::string body;
            if (fu.GetContent(&body) == false)
            {
                std::cout << "Load Config File Failed" << std::endl;
                return false;
            }
            Json::Value root;
            if (JsonUtil::UnSerialize(body, &root) == false)
            {
                std::cout << "Parse Config File Failed" << std::endl;
                return false;
            }
            _hot_time = root["hot_time"].asInt();
            _server_port = root["server_port"].asInt();
            _server_ip = root["server_ip"].asString();
            _download_prefix = root["download_prefix"].asString();
            _packfile_suffix = root["packfile_suffix"].asString();
            _pack_dir = root["pack_dir"].asString();
            _back_dir = root["back_dir"].asString();
            _backup_file = root["backup_file"].asString();
            return true;
        }

    public:
        static Config *GetInstance()
        {
            if (_instance == nullptr) // 二次检测
            {
                _mutex.lock();
                if (_instance == nullptr)
                {
                    _instance = new Config();
                }
                _mutex.unlock();
            }
            return _instance;
        }
        int GetHotTime()
        {
            return _hot_time;
        }
        int GetServerPort()
        {
            return _server_port;
        }
        std::string GetServerIP()
        {
            return _server_ip;
        }
        std::string GetDownloadPrefix()
        {
            return _download_prefix;
        }
        std::string GetPackFileSuffix()
        {
            return _packfile_suffix;
        }
        std::string GetPackDir()
        {
            return _pack_dir;
        }
        std::string GetBackDir()
        {
            return _back_dir;
        }
        std::string GetBackupFile()
        {
            return _backup_file;
        }
    };
    Config *Config::_instance = nullptr;
    std::mutex Config::_mutex;
}

#endif