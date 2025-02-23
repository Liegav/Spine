/*
	This file is part of Spine.

    Spine is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Spine is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Spine.  If not, see <http://www.gnu.org/licenses/>.
 */
// Copyright 2018 Clockwork Origins

#pragma once

#include "ManagementCommon.h"

#include "widgets/management/IManagementWidget.h"

#include <QFutureWatcher>
#include <QMap>
#include <QWidget>

class QPushButton;
class QSpinBox;
class QStandardItem;
class QStandardItemModel;
class QTreeView;
class QWinTaskbarProgress;

namespace spine {
namespace gui {
	class WaitSpinner;
}
namespace client {
namespace widgets {

	class ModFilesWidget : public QWidget, public IManagementWidget {
		Q_OBJECT

	public:
		explicit ModFilesWidget(QWidget * par);
		~ModFilesWidget();

		void updateModList(QList<client::ManagementMod> modList);
		void selectedMod(int index);
		void updateView() override;

	signals:
		void finishedUpload(bool, int);
		void updateUploadText(QString);
		void checkForUpdate(int32_t, bool);
		void updateProgress(int);
		void updateProgressMax(int);
		void removeSpinner();
		void loadedData(ManagementModFilesData);
		void versionUpdated(bool success);

	private slots:
		void addFile();
		void deleteFile();
		void uploadCurrentMod();
		void changedLanguage(QStandardItem * itm);
		void updateVersion();
		void finishUpload(bool success, int updatedCount);
		void testUpdate();
		void updateData(ManagementModFilesData content);
		void addFolder();
		void showVersionUpdate(bool success);

	private:
		struct FileToAdd {
			QString fullPath;
			QString relativePath;
			QString file;
			QString path;
			QFuture<QString> hashFuture;
		};

		QStandardItemModel * _fileList;
		QList<ManagementMod> _mods;
		QTreeView * _fileTreeView;
		int _modIndex;
		QMap<QString, QStandardItem *> _directory;
		QMap<QString, QString> _fileMap;
		QSpinBox * _majorVersionBox;
		QSpinBox * _minorVersionBox;
		QSpinBox * _patchVersionBox;
		QSpinBox * _spineVersionBox;
		QPushButton * _versionButton;
		gui::WaitSpinner * _waitSpinner;
		QWinTaskbarProgress * _taskbarProgress;
		ManagementModFilesData _data;

		QFutureWatcher<void> _futureWatcher;

		QString _filterString;

		void addFile(QString fullPath, QString relativePath, QString file, const QString & hash);
		void addFile(QStandardItem * itm, QString file, QString language);
		void deleteFile(const QModelIndex & idx);
		QString getPathSuggestion(const QString & file) const;

		int64_t getSize(const QString & path) const;
		void markAsChanged(const QString & relativePath);

		QString hashFile(const FileToAdd & fta) const;
		QString hashFile(const QString & path) const;

		static void optimizeIni(const QString& path);
	};

} /* namespace widgets */
} /* namespace client */
} /* namespace spine */
