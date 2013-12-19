#include "qsubview.h"
#include <guid.h>
#include <QtGui/QPainter>

#include <QtCore>
#include <QSettings>
#include <QMouseEvent>


QSubView::QSubView(QWidget *parent)
	: QWidget(parent),m_IVideoDecoder(NULL),
	m_IVideoRender(NULL),
	m_IDeviceClient(NULL),
	iInitHeight(0),
	iInitWidth(0),
	bIsInitFlags(false),
	bRendering(false),
	ui(new Ui::titleview)
{
	this->lower();
	this->setAttribute(Qt::WA_PaintOutsidePaintEvent);
	//申请解码器接口
	pcomCreateInstance(CLSID_h264Decoder,NULL,IID_IVideoDecoder,(void**)&m_IVideoDecoder);
	qDebug("IVideoDecoder:%x",m_IVideoDecoder);
	//申请渲染器接口
	pcomCreateInstance(CLSID_DDrawRender,NULL,IID_IVideoRender,(void**)&m_IVideoRender);
	qDebug("m_IVideoRender:%x",m_IVideoRender);
	//申请DeviceClient接口
	pcomCreateInstance(CLSID_DeviceClient,NULL,IID_IDeviceClient,(void**)&m_IDeviceClient);
	qDebug("m_IDeviceClient:%x",m_IDeviceClient);

	ui->setupUi(this);
	QVBoxLayout *layout = new QVBoxLayout;
	layout->addWidget(ui->gridLayoutWidget);
	setLayout(layout);
	connect(ui->pushButton_2,SIGNAL(pressed ()),this,SLOT(ConnectOn()),Qt::DirectConnection);
	connect(ui->pushButton,SIGNAL(pressed()),this,SLOT(ConnectOff()),Qt::DirectConnection);

	m_QSubViewObject.SetDeviceClient(m_IDeviceClient);
}

QSubView::~QSubView()
{
	CloseWndCamera();
	//while(bRendering==true){
	//	dieTime=QTime::currentTime().addMSecs(10);
	//	while(QTime::currentTime()<dieTime){
	//		QCoreApplication::processEvents(QEventLoop::AllEvents,100);
	//	}
	//}
	if (NULL!=m_IDeviceClient)
	{
		m_IDeviceClient->Release();
	}
	if (NULL!=m_IVideoDecoder)
	{
		m_IVideoDecoder->Release();
	}
	if (NULL!=m_IVideoRender)
	{
		m_IVideoRender->Release();
	}
	delete ui;

}


void QSubView::paintEvent( QPaintEvent * e)
{
	if (IDeviceClient::STATUS_CONNECTED==GetWindowConnectionStatus())
	{
		return;
	}
	QPainter p(this);

	QString image;
	QColor LineColor;
	QColor FontColor;
	int FontSize;
	QString FontFamily;

	QString sAppPath = QCoreApplication::applicationDirPath();
	QString path = sAppPath + "/skins/default/css/SubWindowStyle.ini";
	QSettings IniFile(path, QSettings::IniFormat, 0);

	image = IniFile.value("background/background-image", NULL).toString();
	LineColor.setNamedColor(IniFile.value("background/background-color", NULL).toString());
	FontColor.setNamedColor(IniFile.value("font/font-color", NULL).toString());
	FontSize = IniFile.value("font/font-size", NULL).toString().toInt();
	FontFamily = IniFile.value("font/font-family", NULL).toString();

 	QRect rcClient = contentsRect();
 
 	QPixmap pix;
	QString PixPaht = sAppPath + image;
 	bool ret = pix.load(PixPaht);
 
  	pix = pix.scaled(rcClient.width(),rcClient.height(),Qt::KeepAspectRatio);

 	p.drawPixmap(rcClient,pix);

	QPen pen = QPen(LineColor, 2);
 	p.setPen(pen);

	p.drawRect(rcClient);
	if (this->hasFocus())
	{
		int x = 0;
		int y = 0;
		int width = 0;
		int height = 0;
		rcClient.getCoords(&x, &y, &width, &height);
		p.drawRect(QRect(x + 2,y + 2,width - 2, height - 2));
	}
	else
	{
	 	p.drawRect(rcClient);
	}

	QFont font(FontFamily, FontSize, QFont::Bold);
	
	p.setFont(font);

 	pen.setColor(FontColor);
	
 	p.setPen(pen);

	p.drawText(rcClient, Qt::AlignCenter, "No Video");

}

void QSubView::mouseDoubleClickEvent( QMouseEvent * ev)
{

	emit mouseDoubleClick(this,ev);
}



void QSubView::mousePressEvent(QMouseEvent *ev)
{
	setFocus(Qt::MouseFocusReason);
	emit SetCurrentWindSignl(this);
}

int QSubView::GetCurrentWnd()
{
	return 1;
}
int QSubView::SetCameraInWnd(const QString sAddress,unsigned int uiPort,const QString & sEseeId ,unsigned int uiChannelId,unsigned int uiStreamId ,const QString & sUsername,const QString & sPassword ,const QString & sCameraname,const QString & sVendor)
{
	m_DevCliSetInfo.m_sAddress.clear();
	m_DevCliSetInfo.m_sEseeId.clear();
	m_DevCliSetInfo.m_sUsername.clear();
	m_DevCliSetInfo.m_sPassword.clear();
	m_DevCliSetInfo.m_sCameraname.clear();
	m_DevCliSetInfo.m_sVendor.clear();
	m_DevCliSetInfo.m_sAddress=sAddress;
	m_DevCliSetInfo.m_uiPort=uiPort;
	m_DevCliSetInfo.m_sEseeId=sEseeId;
	m_DevCliSetInfo.m_uiChannelId=uiChannelId;
	m_DevCliSetInfo.m_uiStreamId=uiStreamId;
	m_DevCliSetInfo.m_sUsername=sUsername;
	m_DevCliSetInfo.m_sPassword=sPassword;
	m_DevCliSetInfo.m_sCameraname=sCameraname;
	m_DevCliSetInfo.m_sVendor=sVendor;
	m_QSubViewObject.SetCameraInWnd(sAddress,uiPort,sEseeId,uiChannelId,uiStreamId,sUsername,sPassword,sCameraname,sVendor);
	return 0;
}
int QSubView::OpenCameraInWnd(const QString sAddress,unsigned int uiPort,const QString & sEseeId ,unsigned int uiChannelId,unsigned int uiStreamId ,const QString & sUsername,const QString & sPassword ,const QString & sCameraname,const QString & sVendor)
{
	qDebug()<<"QSubView";
	qDebug()<<this;
	//注册事件，需检测是否注册成功
	if (false==bIsInitFlags)
	{
		if (1==cbInit())
		{
			return 1;
		}
	}
	m_QSubViewObject.SetCameraInWnd(sAddress,uiPort,sEseeId,uiChannelId,uiStreamId,sUsername,sPassword,sCameraname,sVendor);
	m_QSubViewObject.OpenCameraInWnd();
	return 0;
}
int QSubView::CloseWndCamera()
{
	m_QSubViewObject.CloseWndCamera();
	return 0;
}
int QSubView::GetWindowConnectionStatus()
{
	if (NULL==m_IDeviceClient)
	{
		return 0;
	}
	return m_IDeviceClient->getConnectStatus();
}
int QSubView::ConnectOn()
{
	int nRet=1;
	nRet=OpenCameraInWnd(m_DevCliSetInfo.m_sAddress,m_DevCliSetInfo.m_uiPort,m_DevCliSetInfo.m_sEseeId,m_DevCliSetInfo.m_uiChannelId,m_DevCliSetInfo.m_uiStreamId,m_DevCliSetInfo.m_sUsername,m_DevCliSetInfo.m_sPassword,m_DevCliSetInfo.m_sCameraname,m_DevCliSetInfo.m_sVendor);
	return nRet;
}
int QSubView::ConnectOff()
{
	int nRet=1;
	nRet=CloseWndCamera();
	return 1;
}
int QSubView::cbInit()
{
	//注册设备服务回调函数
	QString evName="LiveStream";
	IEventRegister *pRegist=NULL;
	if (NULL==m_IDeviceClient)
	{
		return 1;
	}
	m_IDeviceClient->QueryInterface(IID_IEventRegister,(void**)&pRegist);
	if (NULL==pRegist)
	{
		return 1;
	}
	pRegist->registerEvent(evName,cbLiveStream,this);
	evName.clear();
	evName.append("SocketError");
	pRegist->registerEvent(evName,cbConnectError,this);
	evName.clear();
	evName.append("CurrentStatus");
	pRegist->registerEvent(evName,cbStateChange,this);
	pRegist->Release();
	pRegist=NULL;
	//注册解码回调函数
	evName.clear();
	evName.append("DecodedFrame");
	if (NULL==m_IVideoDecoder)
	{
		return 1;
	}
	m_IVideoDecoder->QueryInterface(IID_IEventRegister,(void**)&pRegist);
	if (NULL==pRegist)
	{
		return 1;
	}
	pRegist->registerEvent(evName,cbDecodedFrame,this);
	pRegist->Release();
	pRegist=NULL;
	//初始化渲染器
	if (NULL==m_IVideoRender)
	{
		return 1;
	}
	if (0!=m_IVideoRender->setRenderWnd(ui->widget_display))
	{
		return 1;
	}

	bIsInitFlags=true;
	return 0;
}
int QSubView::PrevPlay(QVariantMap evMap)
{
	unsigned int nLength=evMap.value("length").toUInt();
	char * lpdata=(char *)evMap.value("data").toUInt();
	if (NULL==m_IVideoDecoder)
	{
		return 1;
	}
	m_IVideoDecoder->decode(lpdata,nLength);
	return 0;
}
int QSubView::CurrentStateChange(QVariantMap evMap)
{
	QVariantMap::const_iterator it;
	for (it=evMap.begin();it!=evMap.end();++it)
	{
		QString sKey=it.key();
		int sValue=it.value().toInt();
		if ("reflash"==sKey)
		{
			paintEvent((QPaintEvent*)this);
		}
		if ("CurrentStatus"==sKey)
		{
			emit CurrentStateChangeSignl(sValue,this);
		}
		
	}
	return 0;
}
int QSubView::PrevRender(QVariantMap evMap)
{
	QVariantMap::const_iterator it;
	for (it=evMap.begin();it!=evMap.end();++it)
	{
		QString sKey=it.key();
		QString sValue=it.value().toString();
	}
	if (NULL==m_IVideoRender)
	{
		return 1;
	}
	char* pData=(char*)evMap.value("data").toUInt();
	char* pYdata=(char*)evMap.value("Ydata").toUInt();
	char* pUdata=(char*)evMap.value("Udata").toUInt();
	char* pVdata=(char*)evMap.value("Vdata").toUInt();
	int iWidth=evMap.value("width").toInt();
	int iHeight=evMap.value("height").toInt();
	int iYStride=evMap.value("YStride").toInt();
	int iUVStride=evMap.value("UVStride").toInt();
	int iLineStride=evMap.value("lineStride").toInt();
	QString iPixeFormat=evMap.value("pixelFormat").toString();
	int iFlags=evMap.value("flags").toInt();

	if (iInitHeight!=iHeight||iInitWidth!=iWidth)
	{
		m_IVideoRender->init(iWidth,iHeight);
		iInitHeight=iHeight;
		iInitWidth=iWidth;
	}
	bRendering=true;
	m_IVideoRender->render(pData,pYdata,pUdata,pVdata,iWidth,iHeight,iYStride,iUVStride,iLineStride,iPixeFormat,iFlags);
	bRendering=false;
	return 0;
}

int cbLiveStream(QString evName,QVariantMap evMap,void*pUser)
{
	//检测数据包，把数据包扔给解码器

	qDebug("cbLiveStream");
	if (evName=="LiveStream")
	{
		((QSubView*)pUser)->PrevPlay(evMap);
		return 0;
	}
	else
		return 1;
}

int cbDecodedFrame(QString evName,QVariantMap evMap,void*pUser)
{
	if (evName=="DecodedFrame")
	{
		((QSubView*)pUser)->PrevRender(evMap);
		return 0;
	}
	else 
		return 1;
}

int cbConnectError(QString evName,QVariantMap evMap,void*pUser)
{
	qDebug()<<"cbConnectError";

	QVariantMap::const_iterator it;
	for (it=evMap.begin();it!=evMap.end();++it)
	{
		QString sKey=it.key();
		QString sValue=it.value().toString();
		//qDebug()<<sKey;
		//qDebug()<<sValue;
	}
	return 1;
}

int cbStateChange(QString evName,QVariantMap evMap,void*pUser)
{
	qDebug()<<"cbStateChange";
	QVariantMap::const_iterator it;
	for (it=evMap.begin();it!=evMap.end();++it)
	{
		QString sKey=it.key();
		QString sValue=it.value().toString();
		//qDebug()<<sKey;
		//qDebug()<<sValue;
	}
	if (evName=="CurrentStatus")
	{
		((QSubView*)pUser)->CurrentStateChange(evMap);
		return 0;
	}
	return 1;
}