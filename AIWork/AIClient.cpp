// AIClient.cpp
#include"AIClient.h"

namespace awf {


    AIClient::AIClient(ExceptionCollector& ec) :ec(ec), m_llmType(gpt) {}

    void AIClient::setBase(const QString& url,
        const QString& key,
        const QString& model,
        LLM type)
    {
        m_baseUrl = url;
        m_apiKey = key;
        m_model = model;
        m_llmType = type;
    }

    void AIClient::set_deepSeek_thinking(bool thinking)
    {
        m_thinking = thinking;
    }
    std::unique_ptr<AITask> AIClient::createStreamTask(QVector<ChatMessage> promot)
    {
        // 复用原有的 URL 构建和请求体构造逻辑（与 callOpenAICompatible 类似）
        QString urlStr = m_baseUrl;
        if (!urlStr.endsWith('/'))
            urlStr += '/';
        urlStr += "v1/chat/completions";

        QUrl url(urlStr);
        QNetworkRequest request(url);
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        request.setRawHeader("Authorization", ("Bearer " + m_apiKey).toUtf8());

        // 构造 messages 数组
        QJsonArray jsonMessages;
        for (const auto& msg : promot) {
            QJsonObject obj;
            obj["role"] = roleToString(msg.role);
            obj["content"] = msg.message;
            jsonMessages.append(obj);
        }

        QJsonObject body;
        body["model"] = m_model;
        body["messages"] = jsonMessages;
        body["stream"] = true;   // 启用流式

        // DeepSeek 思考模式兼容
        if (m_llmType == deepSeek && m_thinking) {
            QJsonObject thinking;
            thinking["type"] = "enabled";
            body["thinking"] = thinking;
        }

        QByteArray postData = QJsonDocument(body).toJson(QJsonDocument::Compact);
        QNetworkReply* reply = m_nam.post(request, postData);

        // 创建任务对象，它会接管 reply 的生命周期
        AITask* task = new AITask(ec,reply);
        return std::unique_ptr<AITask>(task);
    }

    // ---------------------------------------------------------------------------
    // 公共接口
    // ---------------------------------------------------------------------------
    QString AIClient::getGen(QVector<ChatMessage> promot, TemplateContext temp)
    {
        switch (temp)
        {
        case awf::genFunc:
            promot.push_front({ system,"你现在是一个C++编程助手,负责生成用于代码补全的代码段,你应当严格遵守用户的需求,只生成用户需要的代码模块,不要提供多个代码版本,不要编写使用示例,无需考虑头文件" });
            break;
        case awf::genClass:
            promot.push_front({ system,"你现在是一个C++编程助手,负责生成用于代码补全的代码段,你应当严格遵守用户的需求,只生成用户需要的代码模块,不要提供多个代码版本,不要编写使用示例,无需考虑头文件" });
            break;
        case awf::none:
            break;
        default:
            break;
        }


        if (m_llmType == gemini) {
            return callGemini(promot);
        }
        else {
            // gpt / deepSeek 都走 OpenAI 兼容接口
            return callOpenAICompatible(promot);
        }
    }

    // ---------------------------------------------------------------------------
    // OpenAI / DeepSeek 兼容 API
    // ---------------------------------------------------------------------------
    QString AIClient::callOpenAICompatible(const QVector<ChatMessage>& messages)
    {
        // 拼接完整 URL
        QString urlStr = m_baseUrl;
        if (!urlStr.endsWith(QLatin1Char('/')))
            urlStr += QLatin1Char('/');
        urlStr += QStringLiteral("v1/chat/completions");

        QUrl url(urlStr);
        QNetworkRequest request(url);
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        request.setRawHeader("Authorization", ("Bearer " + m_apiKey).toUtf8());

        // 构造 messages 数组
        QJsonArray jsonMessages;
        for (const auto& msg : messages) {
            QJsonObject obj;
            switch (msg.role) {
            case user:      obj["role"] = "user";      break;
            case system:    obj["role"] = "system";    break;
            case assistant: obj["role"] = "assistant"; break;
            }
            obj["content"] = msg.message;
            jsonMessages.append(obj);
        }

        QJsonObject body;
        body["model"] = m_model;
        body["messages"] = jsonMessages;
        body["n"] = 1;

        if (m_llmType == deepSeek) {
            QJsonObject thinking;
            thinking["type"] = m_thinking ? "enabled" : "disabled";
            body["thinking"] = thinking;
        }

        QByteArray postData = QJsonDocument(body).toJson(QJsonDocument::Compact);

        QNetworkReply* reply = m_nam.post(request, postData);
        QEventLoop loop;
        QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();

        if (reply->error() != QNetworkReply::NoError) {
            qWarning() << "Network error:" << reply->errorString();
            reply->deleteLater();
            return QString();
        }

        QByteArray responseData = reply->readAll();
        reply->deleteLater();

        QJsonDocument doc = QJsonDocument::fromJson(responseData);
        if (doc.isNull() || !doc.isObject()) {
            qWarning() << "Invalid JSON response";
            return QString();
        }

        // 调试输出：打印完整的 API 响应
        qDebug().noquote() << doc.toJson().toStdString();

        QJsonObject root = doc.object();
        QJsonArray choices = root["choices"].toArray();
        if (choices.isEmpty()) {
            qWarning() << "No choices in response";
            return QString();
        }

        QJsonObject firstChoice = choices.first().toObject();
        QJsonObject message = firstChoice["message"].toObject();
        QString content = message["content"].toString();
        return content;
    }

    // ---------------------------------------------------------------------------
    // Gemini API（generateContent）
    // ---------------------------------------------------------------------------
    QString AIClient::callGemini(const QVector<ChatMessage>& messages)
    {
        // URL: https://generativelanguage.googleapis.com/v1/models/{model}:generateContent?key={key}
        // 如果用户传入的 m_baseUrl 已经包含完整路径，也可以直接拼接 model 和 key
        QString urlStr = m_baseUrl;
        if (!urlStr.endsWith(QLatin1Char('/')))
            urlStr += QLatin1Char('/');
        urlStr += m_model + QStringLiteral(":generateContent?key=") + m_apiKey;

        QUrl url(urlStr);
        QNetworkRequest request(url);
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

        // Gemini 的 contents 格式：每个内容是一个 part，role 可选为 "user" / "model"
        QJsonArray contents;
        for (const auto& msg : messages) {
            QJsonObject contentObj;
            QJsonArray parts;
            QJsonObject partObj;
            if (msg.role == system) {
                partObj["text"] = QStringLiteral("System: ") + msg.message;
            }
            else {
                partObj["text"] = msg.message;
            }
            parts.append(partObj);
            contentObj["parts"] = parts;

            if (msg.role == assistant) {
                contentObj["role"] = QStringLiteral("model");
            }
            else {
                contentObj["role"] = QStringLiteral("user");
            }
            contents.append(contentObj);
        }

        QJsonObject body;
        body["contents"] = contents;
        // 可设置 generationConfig，这里忽略
        QByteArray postData = QJsonDocument(body).toJson(QJsonDocument::Compact);

        QNetworkReply* reply = m_nam.post(request, postData);

        QEventLoop loop;
        QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();

        if (reply->error() != QNetworkReply::NoError) {
            qWarning() << "Network error:" << reply->errorString();
            reply->deleteLater();
            return QString();
        }

        QByteArray responseData = reply->readAll();
        reply->deleteLater();

        QJsonDocument doc = QJsonDocument::fromJson(responseData);
        if (doc.isNull() || !doc.isObject()) {
            qWarning() << "Invalid JSON response";
            return QString();
        }

        QJsonObject root = doc.object();
        QJsonArray candidates = root["candidates"].toArray();
        if (candidates.isEmpty()) {
            qWarning() << "No candidates in response";
            return QString();
        }

        // 取第一个 candidate 中的第一个 part 的 text
        QJsonObject firstCandidate = candidates.first().toObject();
        QJsonObject contentObj = firstCandidate["content"].toObject();
        QJsonArray parts = contentObj["parts"].toArray();
        if (parts.isEmpty()) {
            qWarning() << "No parts in first candidate";
            return QString();
        }

        QString text = parts.first().toObject()["text"].toString();
        return text;
    }

    AITask::AITask(ExceptionCollector& ec,QNetworkReply* reply, QObject* parent)
        : QObject(parent), m_reply(reply),ec(ec)
    {
        Q_ASSERT(reply);
        // 连接 readyRead 信号，实现流式读取
        connect(reply, &QNetworkReply::readyRead, this, &AITask::onReadyRead);
        // 连接 finished 信号，处理请求结束
        connect(reply, &QNetworkReply::finished, this, &AITask::onFinished);
    }

    AITask::~AITask()
    {
        if (m_reply && m_reply->isRunning()) {
            m_reply->abort();
        }
        m_reply->deleteLater();
    }

    void AITask::abort()
    {
        if (m_reply) {
            m_reply->abort();
        }
    }

    void AITask::onReadyRead()
    {
        QByteArray newData = m_reply->readAll();
        // 将新数据追加到 SSE 解析缓冲区
        m_sseBuffer.append(newData);

        // 尝试解析缓冲区中完整的 SSE 消息
        // SSE 消息由双换行 "\n\n" 分隔
        while (true) {
            int pos = m_sseBuffer.indexOf("\n\n");
            if (pos == -1)
                break;

            QByteArray message = m_sseBuffer.left(pos);
            m_sseBuffer.remove(0, pos + 2);   // 移除已处理的消息（包括末尾的 \n\n）

            parseSSE(message);
        }
    }

    void AITask::onFinished()
    {
        // 网络请求结束，处理可能残留的 SSE 数据
        if (!m_sseBuffer.isEmpty()) {
            parseSSE(m_sseBuffer);
            m_sseBuffer.clear();
        }

        if (m_reply->error() == QNetworkReply::NoError) {
            emit finished(true, m_buffer);
        }
        else {
            emit errorOccurred(m_reply->errorString());
            emit finished(false, m_buffer);
        }
    }

    void AITask::parseSSE(const QByteArray& data)
    {
        // 按行分割 SSE 消息，每行以 "data: " 开头
        const QList<QByteArray> lines = data.split('\n');
        for (const QByteArray& line : lines) {
            QString lineStr = QString::fromUtf8(line).trimmed();
            if (lineStr.startsWith("data: ")) {
                handleSSELine(lineStr.mid(6)); // 去掉 "data: " 前缀
            }
        }
    }

    void AITask::handleSSELine(const QString& line)
    {
        // OpenAI / DeepSeek 流式响应：data: [DONE] 表示结束
        if (line == "[DONE]") {
            return;
        }

        // 解析 JSON
        QJsonDocument doc = QJsonDocument::fromJson(line.toUtf8());
        if (doc.isNull() || !doc.isObject())
            return;

        QJsonObject root = doc.object();
        QJsonArray choices = root["choices"].toArray();
        if (choices.isEmpty())
            return;

        // OpenAI 流式格式：choices[0].delta.content
        QJsonObject choice = choices.first().toObject();
        QString deltaText = choice["delta"].toObject()["content"].toString();

        if (!deltaText.isEmpty()) {
            m_buffer += deltaText;
            emit deltaReceived(deltaText);
        }
    }


} // namespace awf