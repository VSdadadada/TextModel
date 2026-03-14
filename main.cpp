/*
平台: Windows 11 (25H2)
编译器: g++ 15.2.0 (MinGW-w64, 64-bit)
C++标准: C++17 (ISO/IEC 14882:2017)

编译: g++ -O2 -std=c++17 -static main.cpp -o main.exe
要求: 为保证兼容性, input文件夹下文件为UTF-8编码, 文件名为英文
使用方法: 在input文件夹下导入文本(.txt文件, UTF-8编码), 运行main.exe, 在output文件夹下查看处理结果
*/

#include <algorithm>
#include <cmath>
#include <filesystem> // C++17标准引入
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <Windows.h> // Windows环境

using namespace std;
namespace fs = std::filesystem;

// 收集字频数据
struct info
{
    string character;
    int frequency;
    int level;
};

// 加载常用字表
void load_common(const string &filename, int level, map<string, int> &char_level)
{
    ifstream file;
    file.open(filename);
    if (!file.is_open())
    {
        return;
    }
    string line;
    while (getline(file, line))
    {
        size_t i = 0;
        // 遍历每一个中文字符
        while (i < line.size())
        {
            unsigned char c = (unsigned char)line[i];
            if (c < 0x80) // 跳过ASCII字符
            {
                i++;
                continue;
            }
            if (i + 2 < line.size() && (c & 0xE0) == 0xE0) // 遍历
            {
                string ch = line.substr(i, 3);
                if (char_level.find(ch) == char_level.end())
                {
                    char_level[ch] = level;
                }
                i += 3;
            }
            else
            {
                i++;
            }
        }
    }
    file.close();
}

// 判断是否为全角句子结束标点（用于分割句子）
bool is_sentence_end(const string &ch)
{
    // 全角常见句子结束/分隔标点
    const set<string> ends = {"，", "。", "！", "？", "；", "：", "…", "……", "﹔", "﹕"};
    return ends.count(ch) > 0;
}

// 字频排序比较函数
bool sort_compare(const info &a, const info &b)
{
    if (a.frequency != b.frequency)
        return a.frequency > b.frequency;
    return a.character < b.character;
}

// 处理单个文件
void process_file(const string &input_path, const map<string, int> &char_level)
{
    fs::path p = input_path;
    string filename = p.stem().string();
    string output_path = "output\\" + filename + ".csv";

    map<string, int> freq;        // 字频
    vector<int> sentence_lengths; // 每句的长度

    ifstream in_file;
    in_file.open(input_path);
    if (!in_file.is_open())
    {
        cout << "Unable to open input file: " << input_path << endl;
        return;
    }

    string line;
    while (getline(in_file, line))
    {
        size_t i = 0;
        int current_sentence_len = 0;
        // 遍历每一个中文字符
        while (i < line.size())
        {
            unsigned char c = (unsigned char)line[i];
            if (c < 0x80) // 跳过ASCII字符
            {
                i++;
                continue;
            }

            if (i + 2 < line.size() && (c & 0xE0) == 0xE0) // 遍历
            {
                string ch = line.substr(i, 3);

                // 如果是句子结束标点
                if (is_sentence_end(ch))
                {
                    if (current_sentence_len > 0)
                    {
                        sentence_lengths.push_back(current_sentence_len);
                    }
                    current_sentence_len = 0;
                }
                else
                {
                    // 不是结束标点，且是常用字 => 计入当前句子长度
                    if (char_level.count(ch))
                    {
                        freq[ch]++;
                        current_sentence_len++;
                    }
                }

                i += 3;
            }
            else
            {
                ++i;
            }
        }

        // 文件每行结束时，如果当前句子有长度，也要记录
        if (current_sentence_len > 0)
        {
            sentence_lengths.push_back(current_sentence_len);
        }
    }
    in_file.close();

    // 计算平均值和方差
    double avg_len = 0.0;
    double variance = 0.0;
    size_t n = sentence_lengths.size();

    if (n > 0)
    {
        long long sum = 0;
        double sum_sq_diff = 0.0;
        for (int len : sentence_lengths)
        {
            sum += len;
        }
        avg_len = (double)sum / n; // 平均数
        for (int len : sentence_lengths)
        {
            double diff = len - avg_len;
            sum_sq_diff += diff * diff;
        }
        variance = sum_sq_diff / n; // 方差
    }

    vector<info> data;
    for (auto p : freq)
    {
        data.push_back({p.first, p.second, char_level.at(p.first)});
    }

    sort(data.begin(), data.end(), sort_compare);

    // 输出 CSV
    ofstream out_file;
    out_file.open(output_path, ios::binary);
    if (!out_file.is_open())
    {
        cout << "Unable to create output file: " << output_path << endl;
        return;
    }

    // 输出BOM开头前三个字节，防止在中文Windows环境下乱码
    unsigned char bom[] = {0xEF, 0xBB, 0xBF};
    out_file.write((char *)bom, sizeof(bom));

    // 标题行
    out_file << "字符,频数,级别\n";

    // 字频部分
    for (info item : data)
    {
        out_file << item.character << "," << item.frequency << "," << item.level << "\n";
    }

    // 空行分隔
    out_file << "\n";

    // 句子统计（放在文件末尾）
    out_file << "统计项,值\n";
    out_file << "句长平均值," << avg_len << "\n";
    out_file << "句长方差," << variance << "\n";
    out_file << "句长标准差," << sqrt(variance) << "\n";

    out_file.close();

    cout << "Processed: " << input_path << " => " << output_path << endl;
}

int main()
{
    // 创建output文件夹
    if (!fs::exists("output"))
    {
        if (!fs::create_directory("output"))
        {
            cout << "Unable to create output folder." << endl;
            return 1;
        }
    }

    // 导入常用字表
    map<string, int> char_level;
    load_common("CommonChar1.txt", 1, char_level);
    load_common("CommonChar2.txt", 2, char_level);
    load_common("CommonChar3.txt", 3, char_level);

    if (char_level.empty())
    {
        cout << "Warning: No common words loaded. Check common word list files." << endl;
    }

    string input_dir = "input";
    // 检查input文件夹
    if (!fs::exists(input_dir) || !fs::is_directory(input_dir))
    {
        cout << "Input folder does not exist." << endl;
        return 1;
    }

    // 处理文件
    int processed_count = 0;
    for (const auto &entry : fs::directory_iterator(input_dir))
    {
        if (entry.is_regular_file())
        {
            fs::path path = entry.path();
            if (path.extension() == ".txt")
            {
                process_file(path.string(), char_level);
                processed_count++;
            }
        }
    }

    // 输出处理结果
    if (processed_count == 0)
    {
        cout << "No .txt files found in input folder." << endl;
    }
    else
    {
        cout << "\nAll processing completed. Total files processed: " << processed_count << endl;
    }

    system("pause");
    return 0;
}