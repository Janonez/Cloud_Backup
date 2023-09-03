#ifndef __M_SERVICE_H__
#define __M_SERVICE_H__

#include <sstream>

#include "data.hpp"
#include "httplib.h"

extern cloud::DataManager *_data;

namespace cloud
{
    class Service
    {
    public:
        Service()
        {
            Config *config = Config::GetInstance();
            _server_ip = config->GetServerIP();
            _server_port = config->GetServerPort();
            _download_prefix = config->GetDownloadPrefix();
        }
        bool RunModule()
        {
            _server.Post("/upload", Upload);
            _server.Get("/listshow", ListShow);
            _server.Get("/", ListShow);
            std::string download_url = _download_prefix + "(.*)";
            _server.Get(download_url, Download);
            _server.listen(_server_ip.c_str(), _server_port);
            return true;
        }

    private:
        static void Upload(const httplib::Request &req, httplib::Response &rsp)
        {
            // post /upload 正文并不全是文件数据
            auto ret = req.has_file("file");
            if (ret == false)
            {
                rsp.status = 400;
                return;
            }
            const auto &file = req.get_file_value("file");
            std::string back_dir = Config::GetInstance()->GetBackDir();
            // file.filename 文件名  file.content 文件内容
            std::string realpath = back_dir + FileUtil(file.filename).FileName();
            FileUtil fu(realpath);
            fu.SetContent(file.content); // 将文件数据写入文件中
            BackupInfo info;
            info.NewBackupInfo(realpath); // 组织备份文件信息
            _data->Insert(info);          // 向数据管理模块添加备份的文件信息
            return;
        }
        // 静态成员函数不能调用非静态成员函数 -- 没有this指针
        static std::string TimetoStr(time_t t)
        {
            std::string tmp = std::ctime(&t);
            return tmp;
        }
        static void ListShow(const httplib::Request &req, httplib::Response &rsp)
        {
            // 1. 获取所有文件备份信息
            std::vector<BackupInfo> array;
            _data->GetAll(&array);
            // 2.根据所有备份信息，组织html文件数据
            std::stringstream ss;
            ss << "<html><head><meta charset='UTF-8'><title>云备份</title></head>";
            ss << "<body><h1>文件列表</h1><table>";
            for (auto &a : array)
            {
                ss << "<tr>";
                std::string filename = FileUtil(a.real_path).FileName();
                ss << "<td><a href='" << a.url_path << "'>" << filename << "</a></td>";
                ss << "<td align='right'>" << TimetoStr(a.atime) << "</td>";
                ss << "<td align='right'>" << a.fsize / 1024 << "k</td>";
                ss << "</tr>";
            }
            ss << "</table></body></html>";
            rsp.body = ss.str();
            rsp.set_header("Content-Type", "text/html");
            rsp.status = 200;
            return;
        }

        static std::string GetETag(const BackupInfo &info)
        {
            // etag: filename-fsize-mtime
            FileUtil fu(info.real_path);
            std::string etag = fu.FileName();
            etag += "-";
            etag += std::to_string(info.fsize);
            etag += "-";
            etag += std::to_string(info.mtime);
            return etag;
        }

        static void Download(const httplib::Request &req, httplib::Response &rsp)
        {
            // 1. 获取客户端请求的资源路径path req.path
            // 2. 根据资源路径，获取文件备份信息
            BackupInfo info;
            _data->GetOneByURL(req.path, &info);
            // 3. 判断文件是否被压缩，如果被压缩，先解压缩
            if (info.pack_flag == true)
            {
                FileUtil fu(info.pack_path);
                fu.Uncompress(info.real_path);
                // 4. 删除压缩包，修改备份信息
                fu.Remove();
                info.pack_flag = false;
                _data->Update(info);
            }
            // 断点续传
            bool ftp = false;
            std::string old_etag;
            if (req.has_header("If-Range"))
            {
                // 有If-Range字段，且etag一致，符合断点续传要求
                old_etag = req.get_header_value("If-Range");
                if (old_etag == GetETag(info))
                {
                    ftp = true;
                }
            }

            // 5. 读取文件数据，放入rsp.body中
            FileUtil fu(info.real_path);
            if (ftp == false)
            {
                fu.GetContent(&rsp.body);
                // 6. 设置响应头部字段ETag，Accept-Rangs
                rsp.set_header("Accept-Ranges", "bytes");
                rsp.set_header("ETag", GetETag(info));
                rsp.set_header("Content-Type", "application/octet-stream"); // 二进制数据流
                rsp.status = 200;
            }
            else
            {
                // httplib 实现了断点续传的请求
                // 将文件数据读取到rsp.body中，内部会自动根据请求区间进行响应
                // std::string range = req.get_header_value("Range"); bytes=start-end;
                fu.GetContent(&rsp.body);
                // 6. 设置响应头部字段ETag，Accept-Rangs
                rsp.set_header("Accept-Ranges", "bytes");
                rsp.set_header("ETag", GetETag(info));
                rsp.set_header("Content-Type", "application/octet-stream"); // 二进制数据流
                //rsp.set_header("Content-Range", "bytes start-end/fsize");
                rsp.status = 206;
            }
        }

    private:
        std::string _server_ip;       // IP地址
        int _server_port;             // 端口号
        std::string _download_prefix; // 下载前缀
        httplib::Server _server;      // http对象
    };
}

#endif
