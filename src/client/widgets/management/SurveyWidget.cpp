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
// Copyright 2020 Clockwork Origins

#include "widgets/management/SurveyWidget.h"

#include "SpineConfig.h"

#include "client/widgets/management/SurveyAnswersWidget.h"
#include "client/widgets/management/SurveyEditWidget.h"
#include "client/widgets/management/SurveyEntryWidget.h"

#include "gui/WaitSpinner.h"

#include "https/Https.h"

#include "utils/Config.h"
#include "utils/LanguageConverter.h"

#include <QApplication>
#include <QComboBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QStackedWidget>
#include <QVBoxLayout>

using namespace spine::client;
using namespace spine::client::widgets;
using namespace spine::gui;
using namespace spine::utils;

SurveyWidget::SurveyWidget(QWidget * par) : QWidget(par), _modIndex(-1), _waitSpinner(nullptr), _surveyIndex(-1) {
	auto * l = new QVBoxLayout();
	l->setAlignment(Qt::AlignTop);

	_stackedWidget = new QStackedWidget(this);

	{
		auto * w = new QWidget(nullptr);
		
		auto * vlOuter = new QVBoxLayout();
		
		auto * sa = new QScrollArea(this);
		auto * cw = new QWidget(sa);
		_surveysLayout = new QVBoxLayout();
		_surveysLayout->setAlignment(Qt::AlignTop);
		cw->setLayout(_surveysLayout);
		sa->setWidget(cw);
		sa->setWidgetResizable(true);
		sa->setProperty("default", true);
		cw->setProperty("default", true);

		vlOuter->addWidget(sa, 1);

		{
			auto * hl = new QHBoxLayout();

			auto * lbl = new QLabel(QApplication::tr("Version"), w);
			_majorVersionBox = new QSpinBox(w);
			_majorVersionBox->setMinimum(0);
			_majorVersionBox->setMaximum(127);
			
			_minorVersionBox = new QSpinBox(w);
			_minorVersionBox->setMinimum(0);
			_minorVersionBox->setMaximum(127);
			
			_patchVersionBox = new QSpinBox(w);
			_patchVersionBox->setMinimum(0);
			_patchVersionBox->setMaximum(127);

			_languageBox = new QComboBox(this);
			_languageBox->addItems(LanguageConverter::getLanguages());
			_languageBox->setCurrentIndex(1);
			_languageBox->setEditable(false);

			auto * pb = new QPushButton(QApplication::tr("CreateSurvey"));

			hl->addWidget(lbl);
			hl->addWidget(_majorVersionBox);
			hl->addWidget(_minorVersionBox);
			hl->addWidget(_patchVersionBox);
			hl->addWidget(_languageBox);
			hl->addWidget(pb);

			vlOuter->addLayout(hl);

			connect(pb, &QPushButton::released, this, &SurveyWidget::createSurvey);
		}

		w->setLayout(vlOuter);

		_stackedWidget->addWidget(w);
	}

	{
		_surveyAnswersWidget = new SurveyAnswersWidget(this);

		connect(_surveyAnswersWidget, &SurveyAnswersWidget::backClicked, [this]() {
			_stackedWidget->setCurrentIndex(0);
		});

		_stackedWidget->addWidget(_surveyAnswersWidget);
	}

	{
		_surveyEditWidget = new SurveyEditWidget(this);

		connect(_surveyEditWidget, &SurveyEditWidget::backClicked, [this]() {
			_stackedWidget->setCurrentIndex(0);
		});

		_stackedWidget->addWidget(_surveyEditWidget);
	}

	l->addWidget(_stackedWidget);

	setLayout(l);

	connect(this, &SurveyWidget::removeSpinner, [this]() {
		if (!_waitSpinner) return;
		
		_waitSpinner->deleteLater();
		_waitSpinner = nullptr;
	});

	connect(this, &SurveyWidget::loadedData, this, &SurveyWidget::updateData);
	connect(this, &SurveyWidget::triggerUpdate, this, &SurveyWidget::updateView);
}

void SurveyWidget::updateModList(QList<ManagementMod> modList) {
	_mods = modList;
}

void SurveyWidget::selectedMod(int index) {
	_modIndex = index;

	qDeleteAll(_surveyWidgetList);
	_surveyWidgetList.clear();
}

void SurveyWidget::updateView() {
	if (_modIndex == -1 || _modIndex >= _mods.size())
		return;

	selectedMod(_modIndex);

	setDisabled(_mods[_modIndex].packageID >= 0);

	if (_mods[_modIndex].packageID >= 0)
		return;
	
	_stackedWidget->setCurrentIndex(0);
	
	delete _waitSpinner;
	_waitSpinner = new WaitSpinner(QApplication::tr("Updating"), this);

	QJsonObject requestData;
	requestData["Username"] = Config::Username;
	requestData["Password"] = Config::Password;
	requestData["ProjectID"] = _mods[_modIndex].id;
	
	https::Https::postAsync(MANAGEMENTSERVER_PORT, "getPlayTestSurveys", QJsonDocument(requestData).toJson(QJsonDocument::Compact), [this](const QJsonObject & json, int statusCode) {
		if (statusCode != 200) {
			emit removeSpinner();
			return;
		}
		ManagementSurveys ms;
		ms.read(json);
		
		emit loadedData(ms);
		emit removeSpinner();
	});
}

void SurveyWidget::updateData(ManagementSurveys content) {
	_managementSurveys = content;
	
	for (const auto & ms : content.surveys) {
		auto * const sew = new SurveyEntryWidget(_mods[_modIndex].id, ms, this);
		_surveyWidgetList.append(sew);

		connect(sew, &SurveyEntryWidget::releaseClicked, this, &SurveyWidget::updateView);
		connect(sew, &SurveyEntryWidget::deleteClicked, this, &SurveyWidget::updateView);
		connect(sew, &SurveyEntryWidget::editClicked, [this](int surveyID) {
			_stackedWidget->setCurrentIndex(2);
			_surveyEditWidget->updateView(_mods[_modIndex].id, surveyID);
		});
		connect(sew, &SurveyEntryWidget::showAnswersClicked, [this](int surveyID) {
			_stackedWidget->setCurrentIndex(1);
			_surveyAnswersWidget->updateView(_mods[_modIndex].id, surveyID);
		});
	}
}

void SurveyWidget::createSurvey() {
	QJsonObject requestData;
	requestData["Username"] = Config::Username;
	requestData["Password"] = Config::Password;
	requestData["ProjectID"] = _mods[_modIndex].id;
	requestData["Language"] = _languageBox->currentText();
	requestData["MajorVersion"] = _majorVersionBox->value();
	requestData["MinorVersion"] = _minorVersionBox->value();
	requestData["PatchVersion"] = _patchVersionBox->value();
	
	https::Https::postAsync(MANAGEMENTSERVER_PORT, "createPlayTestSurvey", QJsonDocument(requestData).toJson(QJsonDocument::Compact), [this](const QJsonObject &, int statusCode) {
		if (statusCode != 200) return;

		emit triggerUpdate();
	});
}
