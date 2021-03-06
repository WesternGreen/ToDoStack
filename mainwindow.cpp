#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFile>
#include <QMessageBox>
#include <QTextStream>
#include <QSettings>
#include <QDir>

const QString MainWindow::EVENT_PATH = tr("C:\\Program Files\\ToDoStack\\event", nullptr, -1);
const QString MainWindow::CONFIG_PATH = tr("C:\\Program Files\\ToDoStack\\config", nullptr, -1);

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow)
{
	ui->setupUi(this);

	//setup ui
	newEventLe = new QLineEdit(this);
	pushBtn = new QPushButton(this);
	popBtn = new QPushButton(this);
	pushBtn->setText(tr("Push"));
	popBtn->setText(tr("Pop"));
	eventTable = new QTableWidget(0, 3, this);
	eventTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
	eventTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
	eventTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
	eventTable->horizontalHeader()->hide();
	eventTable->setColumnWidth(1, 200);
	QGridLayout * layout = new QGridLayout(this);
	layout->addWidget(newEventLe, 0, 0, 1, 2);
	layout->addWidget(pushBtn, 1, 0, 1, 1);
	layout->addWidget(popBtn, 1, 1, 1, 1);
	layout->addWidget(eventTable, 2, 0, -1, 2);
	ui->centralWidget->setLayout(layout);

	//set menu on tray
	icon = new QSystemTrayIcon(this);
	QIcon thisIcon(":/img/ToDoStack.png");
	icon->setIcon(thisIcon);
	connect(icon, &QSystemTrayIcon::activated, this, &MainWindow::trayClicked);
	icon->show();
	actionShow = new QAction(tr("Show(&S)"), this);
	actionClose = new QAction(tr("Exit(&E)"), this);
	actionChangeMinimize = new QAction(this);//will set text in getConfig()
	actionChangeWindowOnTop = new QAction(this);//will set text in getConfig()
	actionChangeAutoStart = new QAction(this);//will set text in getCOnfig()-setAutoStarrt()
	menu = new QMenu(this);
	menu->addAction(actionShow);
	menu->addSeparator();
	menu->addAction(actionChangeMinimize);
	menu->addAction(actionChangeWindowOnTop);
	menu->addAction(actionChangeAutoStart);
	menu->addSeparator();
	menu->addAction(actionClose);
	connect(actionShow, &QAction::triggered, this, &MainWindow::getShow);
	connect(actionClose, &QAction::triggered, this, &MainWindow::getClose);
	connect(actionChangeMinimize, &QAction::triggered, this, &MainWindow::changeMinimize);
	connect(actionChangeWindowOnTop, &QAction::triggered, this, &MainWindow::changeWindowOnTop);
	connect(actionChangeAutoStart, &QAction::triggered, this, &MainWindow::changeAutoStart);

	getConfig();

	//get data
	load();

	connect(pushBtn, &QPushButton::clicked, this, &MainWindow::getPush);
	connect(popBtn, &QPushButton::clicked, this, &MainWindow::getPop);
	connect(eventTable, &QTableWidget::itemChanged, this, &MainWindow::getItemChanged);//to save when changed
	connect(eventTable, &QTableWidget::itemChanged, this, &MainWindow::setToolTip);

	setToolTip();
}

MainWindow::~MainWindow()
{
	delete ui;
}

void MainWindow::save(){
	QFile file(EVENT_PATH);
	if (!file.open(QFile::WriteOnly | QFile::Text)){
		//QMessageBox::warning(this, tr("保存失败"), tr("无法打开存档文件"));
		return;
	}

	QTextStream out(&file);

	out << eventTable->rowCount() << endl;

	for (int i = 0; i < eventTable->rowCount(); ++i){
		out << eventTable->item(i, 0)->text() << endl;
	}
}


void MainWindow::load(){
	QFile file(EVENT_PATH);
	if (!file.open(QFile::ReadOnly | QFile::Text)){
		return;
	}

	QTextStream in(&file);

	int n;
	in >> n;

	QStack<QString> stack;

	in.readLine();

	for (int i = 0; i < n; ++i){
		stack.push(in.readLine());
	}
	for (int i = 0; i < n; ++i){
		eventPush(stack.pop());
	}
}

void MainWindow::getConfig()
{
	/*
	 * List:
	 * windowOnTop(0 or 1)
	 * minimizeToTray(0 or 1)
	 * windowHeight
	 * windowWidth
	 * autoStart(0 or 1)
	 */
	QFile file(CONFIG_PATH);
	if (!file.open(QFile::ReadOnly | QFile::Text)){
#ifdef DEBUG
		QMessageBox::warning(this, tr("Error - ToDoStack"), tr("Read Config File Failed!!"));
#endif
		//get folder
		QDir dir(tr("C:\\Program Files\\ToDoStack"));
		if (!dir.exists()){
			QDir(tr("C:\\Program Files")).mkdir(tr("ToDoStack"));
		}
	}
	QTextStream in(&file);
	int n;
	in >> n;
	windowOnTop = n == 1;
	in >> n;
	minimizeToTray = n == 1;
	in >> windowHeight >> windowWidth;
	in >> n;
	autoStart = n == 1;

	resize(windowWidth, windowHeight);

	actionChangeMinimize->setText(minimizeToTray ? tr("Do not minimize to tray(&M)") : tr("Minimmize window to tray(&M)"));
	actionChangeWindowOnTop->setText(windowOnTop ? tr("Do NOT Keep Window on Top(&T)") : tr("Keep Window on Top(&T)"));

	if (windowOnTop)setWindowFlags(Qt::WindowStaysOnTopHint);//Window Stay On Top

	setAutoStart();
}

void MainWindow::setConfig()
{
	/*
	 * List:
	 * windowOnTop(0 or 1)
	 * minimizeToTray(0 or 1)
	 * windowHeight
	 * windowWidth
	 * autoStart(0 or 1)
	 */
	QFile file(CONFIG_PATH);
	if (!file.open(QFile::WriteOnly | QFile::Text)){
#ifdef DEBUG
		QMessageBox::warning(this, tr("Error - ToDoStack"), tr("Write Config File Failed!!"));
#endif
	}
	QTextStream out(&file);
	out << (windowOnTop ? 1 : 0) << endl;
	out << (minimizeToTray ? 1 : 0) << endl;
	out << windowHeight << endl << windowWidth << endl;
	out << (autoStart ? 1 : 0) << endl;
}

void MainWindow::setAutoStart()
{
	/* if auto start for all users
	 * HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Run
	 * else if auto start for current user
	 * HKEY_CURRENT_USER\SOFTWARE\Microsoft\Windows\CurrentVersion\Run
	 */
	QSettings reg("HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);

	QString appName = QApplication::applicationName();

	if (autoStart){
		QString strAppPath=QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
		reg.setValue(appName,strAppPath + tr(" autoStart"));
	} else {
		reg.remove(appName);
	}

	actionChangeAutoStart->setText(autoStart ? tr("Do NOT Auto Start(&A)") : tr("Auto Start(&A)"));
}

void MainWindow::eventPush(const QString &name)
{
	eventTable->setRowCount(eventTable->rowCount() + 1);
	eventTable->setItem(eventTable->rowCount() - 1, 0, new QTableWidgetItem(tr("")));
	//get funcBtn
	upBtnList.append(new funcBtn(eventTable, funcBtn::FuncBtnType::UP, eventTable->rowCount() - 1));
	deleteBtnList.append(new funcBtn(eventTable, funcBtn::FuncBtnType::DELETE, eventTable->rowCount() - 1));
	eventTable->setCellWidget(eventTable->rowCount() - 1, 1, upBtnList[upBtnList.length() - 1]);
	eventTable->setCellWidget(eventTable->rowCount() - 1, 2, deleteBtnList[deleteBtnList.length() - 1]);
	connect(upBtnList[upBtnList.length() - 1], &QPushButton::clicked, upBtnList[upBtnList.length() - 1], &funcBtn::getClick);
	connect(upBtnList[upBtnList.length() - 1], &funcBtn::sendIndex, this, &MainWindow::upEvent);
	connect(deleteBtnList[deleteBtnList.length() - 1], &QPushButton::clicked, deleteBtnList[deleteBtnList.length() - 1], &funcBtn::getClick);
	connect(deleteBtnList[deleteBtnList.length() - 1], &funcBtn::sendIndex, this, &MainWindow::deleteEvent);

	//move item
	for (int i = eventTable->rowCount() - 1; i > 0; --i){
		eventTable->item(i, 0)->setText(eventTable->item(i - 1, 0)->text());
	}

	eventTable->item(0, 0)->setText(name);

	newEventLe->clear();
}

void MainWindow::eventPop()
{
	if (eventTable->rowCount()){
		//move event
		for (int i = 0; i < eventTable->rowCount() - 1; ++i){
			eventTable->item(i, 0)->setText(eventTable->item(i + 1, 0)->text());
		}

		eventTable->removeRow(eventTable->rowCount() - 1);

		upBtnList.removeLast();
		deleteBtnList.removeLast();
	}
}

QString MainWindow::eventTop()
{
	if (eventTable->rowCount()){
		return eventTable->item(0, 0)->text();
	} else {
		return tr("无事件");
	}
}

void MainWindow::setToolTip()
{
	icon->setToolTip(tr("ToDoStack - ") + eventTop());
}

void MainWindow::getPush()
{
	if (newEventLe->text().length() > 0){
		eventPush(newEventLe->text());
		save();
	}
}

void MainWindow::getPop()
{
	eventPop();
	save();
}

void MainWindow::trayClicked(QSystemTrayIcon::ActivationReason reason)
{
	switch (reason){
	case QSystemTrayIcon::Context:
		menu->exec(this->cursor().pos());
		break;
	case QSystemTrayIcon::Trigger:
		show();
		break;
	}
}

void MainWindow::getShow()
{
	show();
}

void MainWindow::getClose()
{
	windowHeight = height();
	windowWidth = width();
	setConfig();
	qApp->quit();//exit(0)
}

void MainWindow::upEvent(int index)
{
	if (index < eventTable->rowCount()){
		//move
		QString t = eventTable->item(index, 0)->text();
		for (int i = index; i > 0; --i){
			eventTable->item(i, 0)->setText(eventTable->item(i - 1, 0)->text());
		}
		eventTable->item(0, 0)->setText(t);
		save();
	}
}

void MainWindow::deleteEvent(int index)
{
	eventTable->removeRow(index);

	upBtnList.removeAt(index);
	deleteBtnList.removeAt(index);
	for (int i = 0; i < upBtnList.length(); ++i){
		upBtnList[i]->m_index = i;
		deleteBtnList[i]->m_index = i;
	}

	save();
}

void MainWindow::changeMinimize()
{
	minimizeToTray = !minimizeToTray;
	setConfig();
	actionChangeMinimize->setText(minimizeToTray ? tr("Do NOT Minimize to Tray(&M)") : tr("Minimmize Window to Tray(&M)"));
}

void MainWindow::changeWindowOnTop()
{
	windowOnTop = !windowOnTop;
	setConfig();
	actionChangeWindowOnTop->setText(windowOnTop ? tr("Do NOT Keep Window on Top(&T)") : tr("Keep Window on Top(&T)"));

	if (windowOnTop){
		if (!isHidden()){
			hide();
			setWindowFlags(Qt::WindowStaysOnTopHint);//Window Stay On Top
			show();
		} else {
			setWindowFlags(Qt::WindowStaysOnTopHint);
		}

	} else {
		if (!isHidden()){
			hide();
			setWindowFlags(!Qt::WindowStaysOnTopHint);//not stay on top
			show();
		} else {
			setWindowFlags(!Qt::WindowStaysOnTopHint);
		}
	}
}

void MainWindow::changeAutoStart()
{
	autoStart = !autoStart;
	setAutoStart();
	setConfig();
}

void MainWindow::getItemChanged(QTableWidgetItem *item)
{
	save();
}

void MainWindow::keyPressEvent(QKeyEvent *e)
{
	switch (e->key()){
	case Qt::Key_Enter:
	case Qt::Key_Return:
		getPush();
		break;
	case Qt::Key_Escape:
		getPop();
		break;
	}
}

void MainWindow::closeEvent(QCloseEvent *e)
{
	if (minimizeToTray){
		e->ignore();
		hide();
	} else {
		windowHeight = height();
		windowWidth = width();
		setConfig();
		e->accept();
	}
}
