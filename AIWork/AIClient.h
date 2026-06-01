#pragma once
#include"ExceptionCollector.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include<memory>
namespace awf {

	enum Role {
		user,
		system,
		assistant      // 拼写修正：assintent -> assistant
	};
	inline const char* roleToString(Role role) {
		switch (role) {
		case user:      return "user";
		case system:    return "system";
		case assistant: return "assistant";
		default:        return "unknown";   // 处理未知角色
		}
	}
	enum TemplateContext {
		genFunc,
		genClass,
		none
	};
	struct ChatMessage {
		Role role;
		QString message;
		QString toString() {
			return QString(
				"%1:"
				"%2").arg(roleToString(role)).arg(message);
		}
	};
	class AITask : public QObject {
		Q_OBJECT
	private:
		ExceptionCollector& ec;
	public:
		explicit AITask(ExceptionCollector& ec,QNetworkReply* reply, QObject* parent = nullptr);
		~AITask();

		QString buffer() const { return m_buffer; }   // 当前累积的全部文本
		void abort();                                  // 中止请求

	signals:
		void deltaReceived(const QString& newText);    // 每次收到的新增文本（增量）
		void finished(bool success, const QString& fullText);
		void errorOccurred(const QString& errorString);

	private slots:
		void onReadyRead();
		void onFinished();

	private:
		void parseSSE(const QByteArray& data);
		void handleSSELine(const QString& line);

		QNetworkReply* m_reply;
		QString m_buffer;          // 累积的完整生成文本
		QByteArray m_sseBuffer;    // 用于拼接不完整的 SSE 行
	};

	class AIClient {
		ExceptionCollector& ec;
	public:
		enum LLM { deepSeek, gemini, gpt };

		AIClient(ExceptionCollector& ec);

		void setBase(const QString& url,
			const QString& key,
			const QString& model,
			LLM type);

		void set_deepSeek_thinking(bool thinking);

		// 参数名保留原样 promot
		std::unique_ptr<AITask>createStreamTask(QVector<ChatMessage> promot);
		QString getGen(QVector<ChatMessage> promopt,TemplateContext temp = none);

	private:
		QString m_baseUrl;
		QString m_apiKey;
		QString m_model;
		LLM m_llmType;
		bool m_thinking = false;   // deepseek 思考模式

		QNetworkAccessManager m_nam;

		QString callOpenAICompatible(const QVector<ChatMessage>& messages);
		QString callGemini(const QVector<ChatMessage>& messages);
	};
}

