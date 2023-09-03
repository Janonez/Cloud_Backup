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
			// 1. ��ȡ�����ļ�
			FileUtil fu(filename);
			std::string body;
			fu.GetContent(&body);
			// 2. �http�ͻ����ϴ��ļ�����
			httplib::Client client(SERVER_IP, SERVER_PORT);
			httplib::MultipartFormData item;
			item.content = body;
			item.filename = fu.FileName();
			item.name = "file";
			item.content_type = "application/octet-stream";//������������
			httplib::MultipartFormDataItems items;
			items.push_back(item);
			auto res = client.Post("/upload", items);
			if (!res || res->status != 200) return false;
			return true;
		}

		bool IsNeedUpload(const std::string &filename)
		{
			// ��Ҫ�ϴ����ļ��ж��������ļ��������ģ��������������Ǳ��޸Ĺ�
			// �ļ��������ģ���һ����û����ʷ������Ϣ
			std::string id;
			if (_data->GetOneByKey(filename, &id) != false)
			{
				// ����ʷ��Ϣ
				std::string new_id = GetFileIdentifier(filename);
				if (new_id == id)
				{
					return false;// ����Ҫ�ϴ�
				}
			}
			// һ���ļ��Ƚϴ����ڿ��������Ŀ¼�£�������Ҫһ������
			// ÿ�α������жϱ�ʶ����һ����һ���ļ������ϴ����ٴ�
			// ����ж�һ���ļ�һ��ʱ��û�б��޸Ĺ��������ϴ�
			FileUtil fu(filename);
			if (time(NULL) - fu.LastMTime() < 3)
			{
				// 3��֮���޸Ĺ��ļ�����Ϊ�����޸�
				return false;
			}
			std::cout << filename << " need upload!\n";
			return true;

		}
		bool Runmodule()
		{
			while (1)
			{
				// 1. ������ȡָ���ļ����е������ļ�
				FileUtil fu(_back_dir); // ����Ŀ¼
				std::vector<std::string> array;
				fu.ScanDirectory(&array);
				// 2. ���ж��ļ��Ƿ���Ҫ�ϴ�
				for (auto& a : array)
				{
					if (IsNeedUpload(a) == false) continue;
					// �ϴ��ļ��������ļ�������Ϣ
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
