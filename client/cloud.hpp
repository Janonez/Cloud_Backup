#ifndef __M_CLOUD_H__
#define __M_CLOUD_H__

#include "data.hpp"
#include "httplib.h"

#include <windows.h>

#define SERVER_IP "39.104.83.110"
#define SERVER_PORT 8888

namespace cloud
{
	class Backup
	{
	public:
		Backup(const std::string& back_dir, const std::string& back_file)
			: _back_dir(back_dir)
		{
			_data = new DataManager(back_file);
			//Runmodule();
		}
		std::string GetFileIdentifier(const std::string& filename)
		{
			// ./a.txt-fsize-mtime
			FileUtil fu(filename);
			std::stringstream ss;
			ss << fu.FileName() << "-" << fu.FileSize() << "-" << fu.LastMTime();
			return ss.str();
		}
		bool Upload(const std::string& filename)
		{
			// 1. 获取数据文件
			FileUtil fu(filename);
			std::string body;
			fu.GetContent(&body);
			// 2. 搭建http客户端上传文件数据
			httplib::Client client(SERVER_IP, SERVER_PORT);
			httplib::MultipartFormData item;
			item.content = body;
			item.filename = fu.FileName();
			item.name = "file";
			item.content_type = "application/octet-stream";//二进制流数据
			httplib::MultipartFormDataItems items;
			items.push_back(item);
			auto res = client.Post("/upload", items);
			if (!res || res->status != 200) return false;
			return true;
		}

		bool IsNeedUpload(const std::string &filename)
		{
			// 需要上传的文件判断条件：文件是新增的，不是新增，但是被修改过
			// 文件是新增的：看一下有没有历史备份信息
			std::string id;
			if (_data->GetOneByKey(filename, &id) != false)
			{
				// 有历史信息
				std::string new_id = GetFileIdentifier(filename);
				if (new_id == id)
				{
					return false;// 不需要上传
				}
			}
			// 一个文件比较大，正在拷贝到这个目录下，拷贝需要一个过程
			// 每次遍历都判断标识符不一样，一个文件可能上传几百次
			// 因此判断一个文件一段时间没有被修改过，才能上传
			FileUtil fu(filename);
			if (time(NULL) - fu.LastMTime() < 3)
			{
				// 3秒之内修改过文件，认为还在修改
				return false;
			}
			std::cout << filename << " need upload!\n";
			return true;

		}
		bool Runmodule()
		{
			while (1)
			{
				// 1. 遍历获取指定文件夹中的所有文件
				FileUtil fu(_back_dir); // 备份目录
				std::vector<std::string> array;
				fu.ScanDirectory(&array);
				// 2. 逐步判断文件是否需要上传
				for (auto& a : array)
				{
					if (IsNeedUpload(a) == false) continue;
					// 上传文件，新增文件备份信息
					if (Upload(a) == true)
					{
						_data->Insert(a, GetFileIdentifier(a));
						std::cout << a << " upload success!\n";
					}

				}
				Sleep(1);
			}

		}
	private:
		std::string _back_dir;
		DataManager* _data;
	};
}

#endif // !__M_CLOUD_H__
