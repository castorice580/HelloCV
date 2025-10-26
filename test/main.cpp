#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <deque>

using namespace cv;
using namespace std;

class TrafficLightDetector {
private:
    deque<Rect> rectHistory;
    int historySize;

public:
    TrafficLightDetector(int size = 8) : historySize(size) {}
    
    Rect getStableRect(const Rect& currentRect) {
        if (currentRect.area() == 0) {
            rectHistory.clear();
            return Rect();
        }
        
        // 添加到历史记录
        rectHistory.push_back(currentRect);
        if (rectHistory.size() > historySize) {
            rectHistory.pop_front();
        }
        
        // 如果历史记录不足，返回当前矩形
        if (rectHistory.size() < historySize) {
            return currentRect;
        }
        
        // 计算平均位置和大小
        int avg_x = 0, avg_y = 0, avg_width = 0, avg_height = 0;
        for (const auto& rect : rectHistory) {
            avg_x += rect.x;
            avg_y += rect.y;
            avg_width += rect.width;
            avg_height += rect.height;
        }
        
        avg_x /= rectHistory.size();
        avg_y /= rectHistory.size();
        avg_width /= rectHistory.size();
        avg_height /= rectHistory.size();
        
        return Rect(avg_x, avg_y, avg_width, avg_height);
    }
};

// 检测交通信号灯颜色
string detectTrafficLightColor(Mat& frame, Rect& lightRect, TrafficLightDetector& greenDetector) {
    Mat hsv;
    cvtColor(frame, hsv, COLOR_BGR2HSV);
    
    // 红色范围
    Scalar red_lower1(0, 120, 100);
    Scalar red_upper1(10, 255, 255);
    Scalar red_lower2(170, 120, 100);
    Scalar red_upper2(180, 255, 255);
    
    // 绿色范围
    Scalar green_lower(40, 60, 60);
    Scalar green_upper(90, 255, 255);
    
    // 创建颜色掩码
    Mat red_mask1, red_mask2, red_mask, green_mask;
    inRange(hsv, red_lower1, red_upper1, red_mask1);
    inRange(hsv, red_lower2, red_upper2, red_mask2);
    inRange(hsv, green_lower, green_upper, green_mask);
    
    red_mask = red_mask1 | red_mask2;
    
    // 形态学操作
    Mat kernel_red = getStructuringElement(MORPH_ELLIPSE, Size(15, 15));
    Mat kernel_green = getStructuringElement(MORPH_ELLIPSE, Size(20, 20));
    
    morphologyEx(red_mask, red_mask, MORPH_CLOSE, kernel_red);
    morphologyEx(green_mask, green_mask, MORPH_CLOSE, kernel_green);
    
    int height = frame.rows;
    int width = frame.cols;
    
    // 上方区域 - 只检测红色
    Rect top_roi(width * 0.1, height * 0.05, width * 0.8, height * 0.4);
    // 下方区域 - 只检测绿色，再往下调整
    Rect bottom_roi(width * 0.1, height * 0.70, width * 0.8, height * 0.25); // 起始高度从65%调到70%
    
    Rect detectedRect;
    string color = "UNKNOWN";
    
    // 先在上方区域检测红色 - 不使用稳定器
    Mat red_top_roi = red_mask(top_roi);
    vector<vector<Point>> red_contours;
    findContours(red_top_roi, red_contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
    
    for (const auto& contour : red_contours) {
        double area = contourArea(contour);
        if (area > 300) {
            Rect rect = boundingRect(contour);
            rect.x += top_roi.x;
            rect.y += top_roi.y;
            
            int expand = 30; // 红色扩展大小
            rect.x = max(0, rect.x - expand);
            rect.y = max(0, rect.y - expand);
            rect.width = min(frame.cols - rect.x, rect.width + expand * 2);
            rect.height = min(frame.rows - rect.y, rect.height + expand * 2);
            
            detectedRect = rect;
            color = "RED";
            lightRect = detectedRect;
            return color;
        }
    }
    
    // 在下方区域检测绿色 - 使用稳定器
    Mat green_bottom_roi = green_mask(bottom_roi);
    vector<vector<Point>> green_contours;
    findContours(green_bottom_roi, green_contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
    
    // 只取面积最大的轮廓
    double max_area = 0;
    Rect best_rect;
    
    for (const auto& contour : green_contours) {
        double area = contourArea(contour);
        if (area > 200 && area > max_area) {
            max_area = area;
            Rect rect = boundingRect(contour);
            rect.x += bottom_roi.x;
            rect.y += bottom_roi.y;
            
            // 绿色扩展
            int expand = 45;
            rect.x = max(0, rect.x - expand);
            rect.y = max(0, rect.y - expand);
            rect.width = min(frame.cols - rect.x, rect.width + expand * 2);
            rect.height = min(frame.rows - rect.y, rect.height + expand * 2);
            
            best_rect = rect;
            color = "GREEN";
        }
    }
    
    if (color == "GREEN") {
        // 绿色使用稳定器
        lightRect = greenDetector.getStableRect(best_rect);
    } else {
        lightRect = Rect();
    }
    
    return color;
}

int main() {
    // 读取视频
    VideoCapture cap("TrafficLight.mp4");
    if (!cap.isOpened()) {
        cerr << "无法打开视频文件!" << endl;
        return -1;
    }
    
    // 获取视频属性
    int frame_width = static_cast<int>(cap.get(CAP_PROP_FRAME_WIDTH));
    int frame_height = static_cast<int>(cap.get(CAP_PROP_FRAME_HEIGHT));
    double fps = cap.get(CAP_PROP_FPS);
    
    // 创建输出视频
    VideoWriter writer("result.avi", 
                      VideoWriter::fourcc('M','J','P','G'), 
                      fps, 
                      Size(frame_width, frame_height));
    
    if (!writer.isOpened()) {
        cerr << "无法创建输出视频!" << endl;
        return -1;
    }
    
    // 创建显示窗口
    namedWindow("Traffic Light Detection", WINDOW_NORMAL);
    resizeWindow("Traffic Light Detection", 800, 600);
    
    // 只为绿色创建稳定器
    TrafficLightDetector greenDetector(8);
    
    Mat frame;
    
    while (true) {
        cap >> frame;
        if (frame.empty()) break;
        
        // 检测交通信号灯
        Rect lightRect;
        string color = detectTrafficLightColor(frame, lightRect, greenDetector);
        
        // 用矩形标出红绿灯位置
        if (lightRect.area() > 0) {
            Scalar boxColor;
            if (color == "RED") {
                boxColor = Scalar(0, 0, 255); // 红色框
            } else {
                boxColor = Scalar(0, 255, 0); // 绿色框
            }
            rectangle(frame, lightRect, boxColor, 4);
            
            // 在矩形框上方显示颜色
            string text = color + " Light";
            putText(frame, text, Point(lightRect.x, lightRect.y - 10), 
                   FONT_HERSHEY_SIMPLEX, 0.8, boxColor, 2);
        }
        
        // 在左上角输出颜色字符串
        Scalar textColor;
        if (color == "RED") {
            textColor = Scalar(0, 0, 255);
        } else if (color == "GREEN") {
            textColor = Scalar(0, 255, 0);
        } else {
            textColor = Scalar(128, 128, 128);
        }
        
        putText(frame, "Detected: " + color, Point(20, 40), 
               FONT_HERSHEY_SIMPLEX, 1.2, textColor, 3);
        
        // 显示视频
        imshow("Traffic Light Detection", frame);
        
        // 写入输出视频
        writer.write(frame);
        
        // 按ESC退出，按空格暂停
        int key = waitKey(30);
        if (key == 27) { // ESC键
            break;
        } else if (key == 32) { // 空格键
            waitKey(0); // 暂停，按任意键继续
        }
    }
    
    // 释放资源
    cap.release();
    writer.release();
    destroyAllWindows();
    
    cout << "处理完成! 输出文件: result.avi" << endl;
    
    return 0;
}