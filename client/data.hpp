#ifndef __M_DATA_H__
#define __M_DATA_H__

#include <unordered_map>
#include <sstream>

#include "util.hpp"

namespace cloud
{
	class DataManager
	{
	public:
		DataManager(const std::string &backup_file)
			: _backup_file(backup_file)
		{
			InitLoad();
		}
		bool Storage()
		{
			// 1. ��ȡ���б�����Ϣ
			std::stringstream ss;
			auto it = _table.begin();
			for (; it != _table.end(); ++it)
			{
				// 2. ����Ϣ����ָ���־û���ʽ����֯
				ss << it->first << " " << it->second << "\n";
			}
			// 3. �־û��洢
			FileUtil fu(_backup_file);
			fu.SetContent(ss.str());
			return true;
		}
		int Split(const std::string& str, const std::string& sep, std::vector<std::string>* array)
		{
			size_t pos = 0, index = 0;
			int count = 0;
			while (1)
			{
				// find(�ַ���λ��)
				auto pos = str.find(sep, index);
				if (pos == std::string::npos)
				{
					break; // û���ҵ���ֱ����������ѭ��
				}
				// ��������жϣ��м���������Ҫ���ҵ��ַ�
				if (index == pos) // ��indexλ�ò��ң���λ�þ���Ҫ���ҵ��ַ�
				{
					index = pos + sep.size();
					continue;
				}
				// substr(��ʼλ�ã�����)
				std::string tmp = str.substr(index, pos-index);
				array->push_back(tmp);
				index = pos + sep.size();
				++count;
			}
			// ���⴦�����һ���ַ���
			if (index < str.size()) // ˵������һ���ַ���û�д���
			{
				array->push_back(str.substr(index));
				++count;
			}
			return count;
		}
		bool InitLoad()
		{
			// 1. ���ļ��ж�ȡ����
			FileUtil fu(_backup_file);
			std::string body;
			fu.GetContent(&body);
			// 2. �������ݽ�������ӵ�����
			std::vector<std::string> array;
			Split(body, "\n", &array);
			for (auto& a : array)
			{
				std::vector<std::string> tmp;
				Split(a, " ", &tmp);
				if (tmp.size() != 2)
				{
					continue;
				}
				_table[tmp[0]] = tmp[1];
			}
			return true;
		}
		bool Insert(const std::string& key, const std::string& val)
		{
			_table[key] = val;
			Storage();
			return true;
		}
		bool Update(const std::string& key, const std::string& val)
		{
			_table[key] = val;
			Storage();
			return true;
		}
		bool GetOneByKey(const std::string& key, std::string* val)
		{
			auto it = _table.find(key);
			if (it == _table.end())
			{
				return false;
			}
			*val = it->second;
			return true;
		}
	private:
		std::string _backup_file;// ������Ϣ�ĳ־û��洢�ļ�
		std::unordered_map<std::string, std::string> _table;
	};
}

#endif // !__M_DATA_H__
