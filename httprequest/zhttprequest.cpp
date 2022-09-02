#include "zhttprequest.h"

#include <QUrl>
#include <QEventLoop>

ZHttpRequest::ZHttpRequest(int interval)
	: QObject(nullptr)
	, m_Manager(new QNetworkAccessManager)
	, m_Timer()
	, m_Data()
{
	m_Timer.setInterval(interval);
	m_Timer.setSingleShot(true);
}

void ZHttpRequest::SetTimeout(int interval)
{
	if (m_Timer.isActive())
	{
		m_Timer.stop();
	}

	m_Timer.setInterval(interval);
}

void ZHttpRequest::SetUrl(const char* fmt, ...)
{
	char url[1000] = { 0 };
	va_list args;
	va_start(args, fmt);
	vsprintf(url, fmt, args);
	va_end(args);
	m_Request.reset(new QNetworkRequest(QUrl(url)));
	m_Data.clear();
}

void ZHttpRequest::AddHeader(const char* key, const char* value)
{
	m_Request->setRawHeader(key, value);
}

void ZHttpRequest::SetPost(const char* fmt, ...)
{
	char data[2000] = { 0 };
	va_list args;
	va_start(args, fmt);
	vsprintf(data, fmt, args);
	va_end(args);

	m_Data = data;
}

void ZHttpRequest::SetTokenHeader()
{
	AddHeader("Token", m_Token.c_str());
}

bool ZHttpRequest::Exec(std::string& reply_str)
{
	QNetworkReply* reply = nullptr;
	if (m_Data.isEmpty())
	{
		reply = m_Manager->get(*m_Request);
	}
	else
	{
		reply = m_Manager->post(*m_Request, m_Data.toUtf8());
	}
	
	QEventLoop loop;
	connect(&m_Timer, &QTimer::timeout, &loop, &QEventLoop::quit);
	connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
	m_Timer.start();
	loop.exec();       //block until finish or time out

	if (!m_Timer.isActive())
	{
		disconnect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);

		reply->abort();
		reply->deleteLater();
		reply = nullptr;

		return false;
	}
	
	m_Timer.stop();

	reply_str = std::move(reply->readAll().toStdString());
	reply->deleteLater();
	reply = nullptr;

	return !reply_str.empty();
}

const std::string build_uri(const char* url, const char* target)
{
	std::string uri;
	if (url[strlen(url) - 1] == '/') 
		uri.append(url);
	else
		uri.append(url).append("/");
	uri.append("api.php/v1")
		.append(target);

	return uri;
}
