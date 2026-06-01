// LaunchTask.h
#pragma once
#include <QObject>
#include <QThread>
#include <functional>

namespace awf { class AIWorkFlow; }
class OutputView;

class LaunchTask : public QObject {
    Q_OBJECT
public:
    // path: 源文件绝对路径，folder: 工作目录，outputView: 用于显示输出
    LaunchTask(const QString& path, const QString& folder, OutputView* outputView,
        QObject* parent = nullptr);
    ~LaunchTask();

    void start();   // 启动任务
    void cancel();  // 可选：取消任务

signals:
    void finished();                          // 任务完成（无论成功失败）
    void resultReady(const QString& resultPath); // 结果文件路径

private slots:
    void onOutput(const QString& data);
    void onStarted();
    void onThreadFinished();

private:
    QString m_path;
    QString m_folder;
    OutputView* m_outputView;
    int m_outputId;

    awf::AIWorkFlow* m_workflow = nullptr;
    QThread* m_thread = nullptr;
    QString m_outFileName;
};