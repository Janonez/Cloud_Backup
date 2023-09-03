#ifndef __M_DATA_H__
#define __M_DATA_H__

#include <unordered_map>
#include <pthread.h>

#include "util.hpp"
#include "config.hpp"

namespace cloud
{
    typedef struct BackupInfo
    {
        bool pack_flag;        // 是否压缩标志
        size_t fsize;          // 文件大小
        time_t atime;          // 最后一次访问时间
        time_t mtime;          // 最后一次更改时间
        std::string real_path; // 文件实际存储路径
        std::string pack_path; // 压缩文件储存路径
        std::string url_path;  //

        bool NewBackupInfo(const std::string &realpath)
        {
            FileUtil fu(realpath);
            if (fu.Exists() == false)
            {
                std::cout << "NewBackupInfo: file not exists" << std::endl;
                return false;
            }
            Config *config = Config::GetInstance();
            std::string packdir = config->GetPackDir();           // 压缩路径
            std::string packsuffix = config->GetPackFileSuffix(); // 压缩后缀
            std::string downloadprefix = config->GetDownloadPrefix();

            this->pack_flag = false;
            this->fsize = fu.FileSize();
            this->atime = fu.LastATime();
            this->mtime = fu.LastMTime();
            this->real_path = realpath;
            // ./backdir/a.txt --> ./packdir/a.txt.lz
            this->pack_path = packdir + fu.FileName() + packsuffix;
            // ./backdir/a.txt --> ./download/a.txt
            this->url_path = downloadprefix + fu.FileName();

            return true;
        }
    } BackupInfo;

    class DataManager
    {
    public:
        DataManager()
        {
            _backup_file = Config::GetInstance()->GetBackupFile();
            pthread_rwlock_init(&_rwlock, NULL); // 初始化读写锁
            InitLoad();
        }
        ~DataManager()
        {
            pthread_rwlock_destroy(&_rwlock); // 销毁读写锁
        }
        bool Insert(const BackupInfo &info)
        {
            pthread_rwlock_wrlock(&_rwlock); // 写锁 加锁
            _table[info.url_path] = info;    // key-value
            pthread_rwlock_unlock(&_rwlock); // 解锁
            Storage(); // 修改数据后持久化存储
            return true;
        }
        bool Update(const BackupInfo &info)
        {
            pthread_rwlock_wrlock(&_rwlock); // 写锁 加锁
            _table[info.url_path] = info;    // key-value
            pthread_rwlock_unlock(&_rwlock); // 解锁
            Storage(); // 修改数据后持久化存储
            return true;
        }
        bool GetOneByURL(const std::string &url, BackupInfo *info)
        {
            pthread_rwlock_wrlock(&_rwlock); // 写锁 加锁
            auto it = _table.find(url);
            if (it == _table.end())
            {
                pthread_rwlock_unlock(&_rwlock); // 解锁
                return false;
            }
            *info = it->second;
            pthread_rwlock_unlock(&_rwlock); // 解锁
            return true;
        }
        bool GetOneByRealPath(const std::string &realpath, BackupInfo *info)
        {
            pthread_rwlock_wrlock(&_rwlock); // 写锁 加锁
            auto it = _table.begin();
            for (; it != _table.end(); ++it)
            {
                if (it->second.real_path == realpath)
                {
                    *info = it->second;
                    pthread_rwlock_unlock(&_rwlock); // 解锁
                    return true;
                }
            }
            pthread_rwlock_unlock(&_rwlock); // 解锁
            return false;
        }
        bool GetAll(std::vector<BackupInfo> *array)
        {
            pthread_rwlock_wrlock(&_rwlock); // 写锁 加锁
            auto it = _table.begin();
            for (; it != _table.end(); ++it)
            {
                array->push_back(it->second);
            }
            pthread_rwlock_unlock(&_rwlock); // 解锁
            return true;
        }
        bool Storage()
        {
            // 1. 获取所有数据
            std::vector<BackupInfo> array;
            this->GetAll(&array);
            // 2. 添加到Json::Value
            Json::Value root;
            for (int i = 0; i < array.size(); ++i)
            {
                Json::Value item;
                item["pack_flag"] = array[i].pack_flag;
                item["fsize"] = (Json::Int64)array[i].fsize;
                item["atime"] = (Json::Int64)array[i].atime;
                item["mtime"] = (Json::Int64)array[i].mtime;
                item["real_path"] = array[i].real_path;
                item["pack_path"] = array[i].pack_path;
                item["url_path"] = array[i].url_path;

                root.append(item); // 添加数组元素
            }

            // 3. 对Json::Value序列化
            std::string body;
            JsonUtil::Serialize(root, &body); // 序列化数据
            // 4. 写文件
            FileUtil fu(_backup_file);
            fu.SetContent(body);
            return true;
        }

        bool InitLoad()
        {
            // 1. 将数据文件中的数据读取出来
            FileUtil fu(_backup_file);
            if(fu.Exists() == false)
            {
                return true;
            }
            std::string body;
            fu.GetContent(&body);
            // 2. 反序列化
            Json::Value root;
            JsonUtil::UnSerialize(body, &root);
            // 3. 将反序列化得到的Json::Value数据添加到table中
            for(int i = 0; i < root.size(); ++i)
            {
                BackupInfo info;
                info.pack_flag = root[i]["pack_flag"].asBool();
                info.fsize = root[i]["fsize"].asInt64();
                info.atime = root[i]["atime"].asInt64();
                info.mtime = root[i]["mtime"].asInt64();
                info.pack_path = root[i]["pack_path"].asString();
                info.real_path = root[i]["real_path"].asString();
                info.url_path = root[i]["url_path"].asString();
                Insert(info);
            }
            return true;
        }

    private:
        std::string _backup_file;
        pthread_rwlock_t _rwlock; // 读写锁 - 读共享，写互斥
        std::unordered_map<std::string, BackupInfo> _table;
    };
}

#endif