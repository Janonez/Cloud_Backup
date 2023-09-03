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
			// 1. 获取所有备份信息
			std::stringstream ss;
			auto it = _table.begin();
			for (; it != _table.end(); ++it)
			{
				// 2. 将信息进行指定持久化格式的组织
				ss << it->first << " " << it->second << "\n";
			}
			// 3. 持久化存储
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
				// find(字符，位置)
				auto pos = str.find(sep, index);
				if (pos == std::string::npos)
				{
					break; // 没有找到，直接跳出查找循环
				}
				// 特殊情况判断：中间有连续的要查找的字符
				if (index == pos) // 从index位置查找，该位置就是要查找的字符
				{
					index = pos + sep.size();
					continue;
				}
				// substr(起始位置，长度)
				std::string tmp = str.substr(index, pos-index);
				array->push_back(tmp);
				index = pos + sep.size();
				++count;
			}
			// 特殊处理最后一段字符串
			if (index < str.size()) // 说明还有一段字符串没有处理
			{
				array->push_back(str.substr(index));
				++count;
			}
			return count;
		}
		bool InitLoad()
		{
			// 1. 从文件中读取数据
			FileUtil fu(_backup_file);
			std::string body;
			fu.GetContent(&body);
			// 2. 进行数据解析，添加到表中
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
		std::string _backup_file;// 备份信息的持久化存储文件
		std::unordered_map<std::string, std::string> _table;
	};
}

#endif // !__M_DATA_H__
