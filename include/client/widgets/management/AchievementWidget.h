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

#include <QWidget>

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QSpinBox;

namespace spine {
namespace client {
namespace widgets {

	class AchievementWidget : public QWidget {
		Q_OBJECT

	public:
		AchievementWidget(QWidget * par);

		void setAchievement(int32_t modID, ManagementAchievement achievement);
		ManagementAchievement getAchievement();

	private slots:
		void nameLanguageChanged(QString language);
		void descriptionLanguageChanged(QString language);
		void openLockedImage();
		void openUnlockedImage();
		void changedLockedImage(QString path);
		void changedUnlockedImage(QString path);

	private:
		ManagementAchievement _achievement;
		ManagementAchievement _newAchievement;
		QString _currentNameLanguage;
		QString _currentDescriptionLanguage;
		QComboBox * _nameLanguageBox;
		QLineEdit * _nameEdit;
		QComboBox * _descriptionLanguageBox;
		QLineEdit * _descriptionEdit;
		QCheckBox * _hiddenBox;
		QSpinBox * _progressBox;
		QLabel * _lockedImage;
		QLabel * _unlockedImage;
		QLineEdit * _lockedImagePath;
		QLineEdit * _unlockedImagePath;
	};

} /* namespace widgets */
} /* namespace client */
} /* namespace spine */
