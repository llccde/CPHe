// LaunchTask.cpp
#include "LaunchTask.h"
#include "AIWorkFlow.h"
#include "Gui/OutPutView.h"
#include <QFileInfo>
#include <QDir>

LaunchTask::LaunchTask(const QString& path, const QString& folder, OutputView* outputView, QObject* parent)
    : QObject(parent)
    , m_path(path)
    , m_folder(folder)
    , m_outputView(outputView)
{
    m_outputId = m_outputView->launch(QFileInfo(path).fileName());
    m_outFileName = "result_" + QFileInfo(path).fileName();
}

LaunchTask::~LaunchTask()
{
    // 确保线程安全退出
    if (m_thread && m_thread->isRunning()) {
        m_thread->quit();
        m_thread->wait();
    }
}

void LaunchTask::start()
{
    m_workflow = new awf::AIWorkFlow(m_folder);
    m_thread = new QThread;

    m_workflow->moveToThread(m_thread);

    // 连接输出
    connect(m_workflow, &awf::AIWorkFlow::outPut, this, &LaunchTask::onOutput, Qt::QueuedConnection);

    // 线程启动 → 开始任务
    connect(m_thread, &QThread::started, this, &LaunchTask::onStarted);

    // 线程结束 → 清理并通知
    connect(m_thread, &QThread::finished, this, &LaunchTask::onThreadFinished);

    m_thread->start();
}

void LaunchTask::cancel()
{
    if (m_workflow) {
        // 假设 AIWorkFlow 有 cancel 机制，否则至少可以 quit 线程
        m_thread->quit();
    }
}

void LaunchTask::onOutput(const QString& data)
{
    m_outputView->output(m_outputId, data + "\n");
}

void LaunchTask::onStarted()
{
    QString fileName = QFileInfo(m_path).fileName();
    m_workflow->launch(fileName, m_outFileName);
    if (m_workflow->ec.hasErr())
        emit m_workflow->outPut(m_workflow->ec.toString());
    else
        emit m_workflow->outPut("task done");
    m_thread->quit(); // 任务执行完毕，退出线程
}

void LaunchTask::onThreadFinished()
{
    m_outputView->finish(m_outputId);
    // 通知外部结果文件路径
    emit resultReady(QDir(m_folder).absoluteFilePath(m_outFileName));
    // 清理对象（延迟删除）
    m_workflow->deleteLater();
    m_thread->deleteLater();
    emit finished();
}