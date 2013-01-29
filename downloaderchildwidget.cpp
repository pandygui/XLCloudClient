#include "downloaderchildwidget.h"
#include "ui_downloaderchildwidget.h"
#include "util.h"

DownloaderChildWidget::DownloaderChildWidget(QListWidgetItem *item,
                                             const QString &downloadUrl,
                                             const QString &fileName,
                                             const QString &folderName,
                                             QWidget *parent):
    QWidget(parent),
    m_fileName(fileName),
    m_folderName(folderName),
    m_url(downloadUrl),
    m_percentage (1),
    m_item(item),
    ui(new Ui::DownloaderChildWidget)
{
    ui->setupUi(this);
    setLayout (ui->horizontalLayout_2);

    ui->label->setAttribute(Qt::WA_TranslucentBackground , true);
    ui->label_2->setAttribute(Qt::WA_TranslucentBackground , true);
    ui->label_3->setAttribute(Qt::WA_TranslucentBackground , true);
    ui->openFolderLabel->setAttribute(Qt::WA_TranslucentBackground , true);
    ui->openFileLabel->setAttribute(Qt::WA_TranslucentBackground , true);
    ui->transferStatusLabel->setAttribute(Qt::WA_TranslucentBackground , true);
    ui->label_8->setAttribute(Qt::WA_TranslucentBackground , true);

    ui->label->setText(fileName);

    connect (&m_taskStatusRoutineTimer,
             SIGNAL(timeout()), SLOT(getCurrentTaskStatus()));
    connect (&m_Downloader, SIGNAL(taskStatusChanged(Downloader::TaskStatusX)) ,
             this , SLOT(taskStatusChanged(Downloader::TaskStatusX)));
    ///
    ui->label_8->setPixmap(Util::getFileAttr(m_fileName).icon.pixmap(24));

    QList<QNetworkCookie> cookies = Util::parseMozillaCookieFile(
                QDesktopServices::storageLocation(QDesktopServices::HomeLocation)
                + "/.tdcookie");
    QNetworkCookieJar *cj = new QNetworkCookieJar;

    cj->setCookiesFromUrl(cookies , QUrl (m_url));
    m_Downloader.setCookieJar(cj);

//    setFixedHeight(54);

    m_Downloader.startDownload(m_url , m_folderName + "/" + m_fileName);
}

QSize DownloaderChildWidget::sizeHint()
{
    QSize size (QWidget::sizeHint().width(), 54);
    return size;
}

DownloaderChildWidget::~DownloaderChildWidget()
{
    delete ui;
}

void DownloaderChildWidget::mouseDoubleClickEvent(QMouseEvent *)
{
    if ( (int)m_percentage == 100 )
    {
        on_openFileLabel_linkActivated("");
        return;
    }

    if ( m_Downloader.running )
    {
        m_Downloader.stop();
    }
    else
    {
        m_Downloader.startDownload(m_url,
                                   m_folderName + QDir::separator() + m_fileName);
    }
}

void DownloaderChildWidget::getCurrentTaskStatus()
{
    const Downloader::TaskInfoX & taskInfo = m_Downloader.currentTaskInfo;

    QTime time (0,0,0);

    ui->transferStatusLabel->setText(QString ("%1/%2  %3/s  %4")
                         .arg(Util::toReadableSize(taskInfo.transfered))
                         .arg(Util::toReadableSize(taskInfo.total))
                         .arg(Util::toReadableSize(taskInfo.speed))
                         .arg(time.addSecs(taskInfo.eta).toString()));

    m_percentage = taskInfo.percentage / 100;
    update ();
}

void DownloaderChildWidget::taskStatusChanged(Downloader::TaskStatusX ts)
{
    switch (ts)
    {
    case Downloader::Finished:
        m_taskStatusRoutineTimer.stop();
        m_percentage = 100;
        ui->transferStatusLabel->setText(QString ("%1/%1 finished")
                             .arg(Util::toReadableSize(m_Downloader.getFileSize())) );
        update ();
        break;
    case Downloader::Paused:
        ui->transferStatusLabel->setText(tr("0B/0B Suspended"));
        m_taskStatusRoutineTimer.stop();
        break;
    case Downloader::Failed:
        ui->transferStatusLabel->setText(tr("0B/0B Failed"));
        m_taskStatusRoutineTimer.stop();
        break;
    case Downloader::Running:
        m_taskStatusRoutineTimer.start(1000);
        break;
    default:
        Q_ASSERT(false);
    }
}

void DownloaderChildWidget::copyURLtoClipboard(const QString &txt)
{
    QClipboard *clip = QApplication::clipboard();
    clip->setText(txt);
}

void DownloaderChildWidget::keyPressEvent(QKeyEvent *e)
{
    switch (e->modifiers())
    {
    case Qt::CTRL:
        switch (e->key())
        {
        case Qt::Key_C:
            copyURLtoClipboard(m_url);
            break;
        }

        break;
    }
}

void DownloaderChildWidget::paintEvent(QPaintEvent *)
{
    int mid = width() * m_percentage;

    QPainter painter (this);
    painter.setPen(QPen(QColor(Qt::white)));
    painter.setBrush(QBrush(QColor::fromRgb(213, 231, 245)));
    painter.drawRect(0 , 0 , mid , height() );

    painter.setBrush(QBrush(QColor("#EFF8FF")));
    painter.drawRect(mid , 0 , width() - mid , height());
}

void DownloaderChildWidget::on_openFileLabel_linkActivated(const QString &link)
{
    Q_UNUSED (link);
    QDesktopServices::openUrl(m_folderName + QDir::separator() + m_fileName);
}

void DownloaderChildWidget::on_openFolderLabel_linkActivated(const QString &link)
{
    Q_UNUSED (link);
    QDesktopServices::openUrl(m_folderName);
}

bool DownloaderChildWidget::question(const QString &msg)
{
    return QMessageBox::No ==
            QMessageBox::question(this, tr("Question"),
                                  msg, QMessageBox::Yes, QMessageBox::No);
}

void DownloaderChildWidget::on_trashTaskBtn_clicked()
{
    if (question(tr("Remove task ?")))
    {
        return;
    }

    if (! question(tr("Also remove files ?")) )
    {
        m_Downloader.cancelAndRemove ();
    }

    /// this is stupid , why should one iterate and do this ??
    for ( int i = 0 ; i < m_item->listWidget()->count() ; ++ i )
    {
        if ( m_item->listWidget()->item(i) == m_item )
        {
            delete m_item->listWidget()->takeItem(i);
            break;
        }
    }
}