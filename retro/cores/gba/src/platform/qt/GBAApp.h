/* Copyright (c) 2013-2014 Jeffrey Pfau
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#pragma once

#include <QApplication>
#include <QFileDialog>
#include <QList>
#include <QMap>
#include <QMultiMap>
#include <QObject>
#include <QRunnable>
#include <QString>
#include <QThreadPool>

#include <functional>

#include "CoreManager.h"
#include "MultiplayerController.h"

struct NoIntroDB;

#include <mgba/core/log.h>

mLOG_DECLARE_CATEGORY(QT);

namespace QGBA {

class ConfigController;
class GameController;
class Window;

#ifdef USE_SQLITE3
class GameDBParser : public QObject {
Q_OBJECT

public:
	GameDBParser(NoIntroDB* db, QObject* parent = nullptr);

public slots:
	void parseNoIntroDB();

private:
	NoIntroDB* m_db;
};
#endif

class GBAApp : public QApplication {
Q_OBJECT

public:
	GBAApp(int& argc, char* argv[], ConfigController*);
	~GBAApp();
	static GBAApp* app();

	static QString dataDir();

	Window* newWindow();

	QString getOpenFileName(QWidget* owner, const QString& title, const QString& filter = QString());
	QString getSaveFileName(QWidget* owner, const QString& title, const QString& filter = QString());
	QString getOpenDirectoryName(QWidget* owner, const QString& title);

	const NoIntroDB* gameDB() const { return m_db; }
	bool reloadGameDB();

	qint64 submitWorkerJob(std::function<void ()> job, std::function<void ()> callback = {});
	qint64 submitWorkerJob(std::function<void ()> job, QObject* context, std::function<void ()> callback);
	bool removeWorkerJob(qint64 jobId);
	bool waitOnJob(qint64 jobId, QObject* context, std::function<void ()> callback);

signals:
	void jobFinished(qint64 jobId);

protected:
	bool event(QEvent*);

private slots:
	void finishJob(qint64 jobId);

private:
	class WorkerJob : public QRunnable {
	public:
		WorkerJob(qint64 id, std::function<void ()> job, GBAApp* owner);

	public:
		void run() override;

	private:
		qint64 m_id;
		std::function<void ()> m_job;
		GBAApp* m_owner;
	};

	Window* newWindowInternal();

	void pauseAll(QList<Window*>* paused);
	void continueAll(const QList<Window*>& paused);

	ConfigController* m_configController;
	QList<Window*> m_windows;
	MultiplayerController m_multiplayer;
	CoreManager m_manager;

	QMap<qint64, WorkerJob*> m_workerJobs;
	QMultiMap<qint64, QMetaObject::Connection> m_workerJobCallbacks;
	QThreadPool m_workerThreads;
	qint64 m_nextJob = 1;

	NoIntroDB* m_db = nullptr;
};

}
