#ifndef __M_HOT_H__
#define __M_HOT_H__

#include <unistd.h>

#include "data.hpp"

extern cloud::DataManager *_data;

namespace cloud
{
    class HotManager
    {
    public:
        HotManager()
        {
            Config *config = Config::GetInstance();
            _back_dir = config->GetBackDir();
            _pack_dir = config->GetPackDir();
            _pack_suffix = config->GetPackFileSuffix();
            _hot_time = config->GetHotTime();
            // 如果路径不存在就创建
            FileUtil tmp1(_back_dir);
            FileUtil tmp2(_pack_dir);
            tmp1.CreateDirectory();
            tmp2.CreateDirectory();
        }
        bool RunModule()
        {
            while (1)
            {
                // 1. 遍历备份目录，获取所有文件名
                FileUtil fu(_back_dir);
                std::vector<std::string> array;
                fu.ScanDirectory(&array); // 将备份文件名存放到数组中
                // 2. 判断文件是否为非热点文件
                for (auto &a : array)
                {
                    if (HotJudge(a) == true)
                    {
                        continue; // 热点文件,无需处理
                    }
                    // 3. 获取备份信息
                    BackupInfo info;
                    if (_data->GetOneByRealPath(a, &info) == false)
                    {
                        info.NewBackupInfo(a); // 文件存在，但是没有备份信息，新建一个备份信息出来
                    }
                    // 4. 对非热点文件进行压缩处理
                    FileUtil tmp(a);
                    tmp.Compress(info.pack_path);
                    // 5. 删除源文件，修改备份信息
                    tmp.Remove();
                    info.pack_flag = true;
                    _data->Update(info);
                }
                usleep(1000); // 避免空目录循环遍历，消耗CPU资源过高
            }

            return true;
        }

    private:
        // 非热点文件-假    热点文件-真
        bool HotJudge(const std::string &filename)
        {
            FileUtil fu(filename);
            time_t last_atime = fu.LastATime();
            time_t cur_time = time(NULL);
            if ((cur_time - last_atime) > _hot_time)
            {
                return false;
            }
            return true;
        }

    private:
        std::string _back_dir;
        std::string _pack_dir;
        std::string _pack_suffix;
        int _hot_time;
    };
}

#endif