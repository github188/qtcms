#include "mdworkobject.h"
#include "bubbleprotocolex.h"

MDWorkObject::MDWorkObject(QObject * parent)
	: QThread(parent)
{
}

MDWorkObject::~MDWorkObject()
{
	stopMd();
}

void MDWorkObject::registerEvent( QString sEvent,EventProc proc,void * pUser )
{
	EventMap mapTemp;
	mapTemp.proc = proc;
	mapTemp.pUser = pUser;
	m_eventMap.insert(sEvent,mapTemp);
}

void MDWorkObject::eventProcCall( QString sEvent,QVariantMap eParam )
{
	if (m_eventMap.contains(sEvent))
	{
		EventMap mapTemp = m_eventMap[sEvent];
		mapTemp.proc(sEvent,eParam,mapTemp.pUser);
	}
}

void MDWorkObject::setHostInfo( QString sAddress,unsigned int uPort )
{
	m_sAddress = sAddress;
	m_uPort = uPort;
}

void MDWorkObject::setUserInfo( QString sUsername,QString sPassword )
{
	m_sUsername = sUsername;
	m_sPassword = sPassword;

	// for test
/*	m_sUsername = QString("admin");
	m_sPassword = QString("");*/
}

int MDWorkObject::startMd()
{
	m_bQuitMd = false;

	start();
	return 0;
}

int MDWorkObject::stopMd()
{
	m_bQuitMd = true;
	while (isRunning())
	{
		msleep(50);
	}
	return 0;
}

int MDWorkObject::mdWorkProc(QTcpSocket * s)
{
	// 如果需要正常结束整个线程，则返回0，如果需要断线重连，则返回-1


	char sRecvBuffer[1024]; // 接受缓冲
	QString sPack; // 帧缓冲
	QString sLR; // 行尾结束符，通过检测判断行尾结束符是\n还是\r\n，兼容两种结束符
	QString sContent; // http协议负载数据

	QDateTime last = QDateTime::currentDateTimeUtc();


	while (!m_bQuitMd)
	{
		msleep(20);
		if (QDateTime::currentDateTimeUtc().toMSecsSinceEpoch() - last.toMSecsSinceEpoch() > (qint64)1000)
		{
			last = QDateTime::currentDateTimeUtc();
			// 请求用户名密码
			QString sUserandPwd;
			sUserandPwd = m_sUsername + QString(":") + m_sPassword;
			QString sAuthorization(sUserandPwd.toAscii().toBase64());

			// 请求字符串
			QString sReq;
			sReq.clear();
			sReq += QString("GET /NetSDK/Video/motionDetection/channel/1/status HTTP/1.1\r\n");
			sReq += QString("Host: ") + m_sAddress + QString("\r\n");
			sReq += QString("Connection: keep-alive\r\n");
			sReq += QString("Authorization: Basic ") + sAuthorization + QString("\r\n");
			sReq += QString("Cookie: juanipcam_lang=zh-cn\r\n\r\n");


			// 发送数据
			qint64 nTotleWrite = 0;
			do 
			{
				if (m_bQuitMd)
				{
					// 退出Md循环
					s->disconnectFromHost();
					s->close();

					return 0;
				}

				qint64 nWrite = s->write(sReq.toAscii().data() + nTotleWrite,sReq.length());
				if (-1 == nWrite)
				{
					// 写失败,关闭socket,退出本次echo
					s->disconnectFromHost();
					s->close();

					// 需要断线重连，返回-1
					return -1;
				}
				nTotleWrite += nWrite;

				if ( ! s->waitForBytesWritten(1000) )
				{
					// 写超时,关闭socket,退出本次echo
					s->disconnectFromHost();
					s->close();

					// 需要断线重连，返回-1
					return -1;
				}
			} while (nTotleWrite < sReq.length());

			// 接受数据
			// 接收前清空上次的缓存
			sPack.clear();
			while(1) // to be modified 需要加入循环强制退出机制
			{
				if (m_bQuitMd)
				{
					// 退出Md循环
					s->disconnectFromHost();
					s->close();

					return 0;
				}

				if (s->waitForReadyRead(50))
				{
					qint64 nRead = s->read(sRecvBuffer,sizeof(sRecvBuffer) - 1); // 预留结束符位置读取
					if (-1 == nRead)
					{
						// 读失败，关闭socket，退出本次echo
						s->disconnectFromHost();
						s->close();

						// 需要断线重连,返回-1
						return -1;
					}

					sRecvBuffer[nRead] = 0;
					sPack += QString(sRecvBuffer);

					// 检查http头是否接收完整,\r\n\r\n或者\n\n,未接收完则返回循环继续接受，已接收完则继续执行后续代码
					sLR = QString("\r\n");
					if (!sPack.contains(QString("\r\n\r\n")))
					{
						// 未找到\r\n的行尾
						sLR = QString("\n");
						if (!sPack.contains(QString("\n\n")))
						{
							// 未找到\n的行尾，头未收完
							// 下个循环继续接收
							continue;
						}
					}

					// 获取content-length字段
					int nContentLength = 0;
					if (sPack.contains(QString("content-length"),Qt::CaseInsensitive))
					{
						int nContentLengthStart = sPack.indexOf(QString("content-length"),0,Qt::CaseInsensitive);
						int nContentLengthEnd = sPack.indexOf(sLR,nContentLengthStart);
						QString sContentLength = sPack.mid(nContentLengthStart,nContentLengthEnd - nContentLengthStart);
						sContentLength.remove(QChar(' '));
						nContentLength = sContentLength.mid(sContentLength.indexOf(QChar(':')) + 1).toInt();
					}

					if (0 != nContentLength)
					{
						// 数据中带负载
						sContent = sPack.mid(sPack.indexOf(sLR + sLR) + QString(sLR + sLR).length());
						if (sContent.length() == nContentLength)
						{
							// 接收完成
							// 退出接收循环
							break;
						}
						else if (sContent.length() > nContentLength)
						{
							// 数据不正确
							s->disconnectFromHost();
							s->close();

							// 需要断线重连，返回-1
							return -1;
						}
						else
						{
							// 需要更多数据
							continue;
						}
					}
					else
					{
						// 无content，接收完成
						// 退出接收循环
						break;
					}
				}
			}

			// 接收数据解析
			QString sFirstLine = sPack.left(sPack.indexOf(sLR));
			if (sFirstLine.contains(QString("200")))
			{
				// 200 OK
				if (sContent == "true")
				{
					QVariantMap eparam;
					eparam.insert("signal",QVariant(true));
					eventProcCall("mdsignal",eparam);
				}
				else if (sContent == "false")
				{
					QVariantMap eparam;
					eparam.insert("signal",QVariant(false));
					eventProcCall("mdsignal",eparam);
				}
			}
			else if (sFirstLine.contains(QString("404")))
			{
				// 404 page not found
			}
		}
	}

	// 需要断线重连的地方，在循环内直接返回-1，其他情况则是需要结束移动侦测的整个过程
	if (s->isOpen())
	{
		s->close();
	}
	return 0;
}

void MDWorkObject::run()
{
	typedef enum _enMdThreadStatus{
		MTS_INIT,
		MTS_WORK,
		MTS_TERMINAT,
	}MdThreadStatus;
	MdThreadStatus nThreadStatus = MTS_INIT;

	// 创建socket，在MTS_TERMINATE时销毁
	QTcpSocket * s = new QTcpSocket;

	while (!m_bQuitMd)
	{
		switch(nThreadStatus)
		{
		case MTS_INIT:
			{
				// 初始化socket,进行连接、参数设置等工作
				if (NULL != s)
				{
					int nRet = InitSocket(s);
					if (0 == nRet)
					{
						nThreadStatus = MTS_WORK;
					}
				}
			}
			break;
		case MTS_WORK:
			{
				// 传入socket,开始工作
				// 工作中返回0则正常退出，返回-1则重连
				if (NULL != s)
				{
					int nRet = mdWorkProc(s);
					if (0 == nRet)
					{
						nThreadStatus = MTS_TERMINAT;
					}
					else
					{
						nThreadStatus = MTS_INIT;
					}
				}
			}
			break;
		case MTS_TERMINAT:
			{
				// 释放socket然后退出
				delete s;
				return;
			}
			break;
		default:
			break;
		}
	}
}

int MDWorkObject::InitSocket(QTcpSocket * s)
{
	// 正常工作返回0,连接失败返回-1
	// 连接
	s->connectToHost(m_sAddress,m_uPort,QIODevice::ReadWrite);
	if (!s->waitForConnected(5000))
	{
		qDebug("[%s:%d] %s:%s",__FILE__,__LINE__,__FUNCTION__,s->errorString());
		return -1;
	}

	// keep alive
	s->setSocketOption(QAbstractSocket::KeepAliveOption,QVariant(1));

	// nodelay
	s->setSocketOption(QAbstractSocket::LowDelayOption,QVariant(1));

	return 0;
}
