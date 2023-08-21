#include <iostream>
#include <string>
#include <sstream>
#include <memory>
#include <jsoncpp/json/json.h>

int main()
{
    const char *name = "张三";
    int age = 20;
    float score[] = {60.5,70.3,80.6};
    Json::Value root;
    root["姓名"] = name;
    root["年龄"] = age;
    for(int i = 0; i < sizeof(score)/sizeof(score[0]); ++i)
        root["成绩"].append(score[i]);

    Json::StreamWriterBuilder swb;
    std::unique_ptr<Json::StreamWriter> sw(swb.newStreamWriter());
    std::stringstream ss;
    sw->write(root, &ss);
    std::cout << ss.str() << std::endl;
    return 0;
}