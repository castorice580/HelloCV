#include <iostream>
#include <fstream>
#include <string>
#include <cctype>
using namespace std;
// 加密解密类
class Crypto {
public:
    static string jiami(const string& text, int key) {
        string result;
        for (char c : text) {
            if (isalpha(c)) {
                char base = islower(c) ? 'a' : 'A';
                result += (c - base + key) % 26 + base;
            } else {
                result += c;
            }
        }
        return result;
    }
    
    static string jiemi(const string& text, int key) {
        return jiami(text, 26 - key);
    }
};

// 文件处理类
class FileHandler {
public:
    static string read(const string& name) {
        ifstream file(name);
        return string((istreambuf_iterator<char>(file)), 
                     istreambuf_iterator<char>());
    }
    
    static void write(const string& name, const string& data) {
        ofstream file(name);
        file << data;
    }
};

// 菜单类
class Menu {
public:
    void run() {
        int n;
        while (true) {
            cout << "\n1.加密文本 2.解密文本 3.加密文件 4.解密文件 5.退出\n选: ";
            cin >> n;
            cin.ignore();
            
            if (n == 5) break;
            doJob(n);
        }
    }

private:
    void doJob(int n) {
        string s, in, out;
        int k;
        
        if (n == 1 || n == 2) {
            cout << "文本: ";
            getline(cin, s);
            cout << "密码: ";
            cin >> k;
            cin.ignore();
            
            string r = (n == 1) ? Crypto::jiami(s, k) : Crypto::jiemi(s, k);
            cout << "结果: " << r << endl;
            
        } else if (n == 3 || n == 4) {
            cout << "输入文件: ";
            getline(cin, in);
            cout << "输出文件: ";
            getline(cin, out);
            cout << "密码: ";
            cin >> k;
            cin.ignore();
            
            string data = FileHandler::read(in);
            string r = (n == 3) ? Crypto::jiami(data, k) : Crypto::jiemi(data, k);
            FileHandler::write(out, r);
            cout << "完成！" << endl;
        }
    }
};

// 主函数
int main() {
    Menu m;
    m.run();
    return 0;
}
