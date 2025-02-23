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

#include "widgets/NewsWriterDialog.h"

#include "IconCache.h"
#include "SpineConfig.h"

#include "https/Https.h"

#include "utils/Config.h"
#include "utils/Conversion.h"
#include "utils/Database.h"
#include "utils/LanguageConverter.h"

#include "widgets/GeneralSettingsWidget.h"
#include "widgets/NewsWidget.h"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDateEdit>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QScreen>
#include <QScrollArea>
#include <QtConcurrentRun>
#include <QTextBrowser>
#include <QVBoxLayout>

using namespace spine::client;
using namespace spine::common;
using namespace spine::https;
using namespace spine::utils;
using namespace spine::widgets;

NewsWriterDialog::NewsWriterDialog(QWidget * par) : QDialog(par), _newsPreviewWidget(nullptr), _titleEdit(nullptr), _dateEdit(nullptr), _bodyEdit(nullptr), _imageReferencesEdit(nullptr) {
	auto * l = new QHBoxLayout();
	l->setAlignment(Qt::AlignTop);

	_newsPreviewWidget = new NewsWidget(News(), true, this);
	l->addWidget(_newsPreviewWidget);

	{
		auto * vl = new QVBoxLayout();
		vl->setAlignment(Qt::AlignTop);

		_titleEdit = new QLineEdit(this);
		_titleEdit->setPlaceholderText(QApplication::tr("TitlePlaceholder"));
		_languageBox = new QComboBox(this);
		_languageBox->addItems(LanguageConverter::getLanguages());
		_dateEdit = new QDateEdit(this);
		_dateEdit->setAlignment(Qt::AlignRight);
		_dateEdit->setCalendarPopup(true);
		_dateEdit->setDate(QDate::currentDate());
		_dateEdit->setMinimumDate(QDate::currentDate());
		auto * hbl = new QHBoxLayout();
		hbl->addWidget(_titleEdit);
		hbl->addWidget(_languageBox);
		hbl->addWidget(_dateEdit);
		vl->addLayout(hbl);

		_bodyEdit = new QTextEdit(this);
		_bodyEdit->setPlaceholderText(QApplication::tr("BodyPlaceholder") + Config::NEWSIMAGEDIR);
		_bodyEdit->setProperty("newsText", true);
		_bodyEdit->setFontFamily("Lato Semibold");
		vl->addWidget(_bodyEdit);

		auto * scrollArea = new QScrollArea(this);
		scrollArea->setWidgetResizable(true);
		auto * modContainer = new QWidget(scrollArea);
		modContainer->setProperty("default", true);
		scrollArea->setWidget(modContainer);
		_modListLayout = new QGridLayout();
		modContainer->setLayout(_modListLayout);
		vl->addWidget(scrollArea);

		auto * imageLayout = new QHBoxLayout();
		_imageReferencesEdit = new QLineEdit(this);
		_imageReferencesEdit->setPlaceholderText(QApplication::tr("ImagesPlaceholder"));
		auto * pb = new QPushButton(IconCache::getInstance()->getOrLoadIcon(":/svg/add.svg"), "", this);
		connect(pb, &QPushButton::released, this, &NewsWriterDialog::addImage);
		imageLayout->addWidget(_imageReferencesEdit, 1);
		imageLayout->addWidget(pb);
		vl->addLayout(imageLayout);

		auto * dbb = new QDialogButtonBox(QDialogButtonBox::StandardButton::Apply | QDialogButtonBox::StandardButton::Discard, Qt::Orientation::Horizontal, this);
		vl->addWidget(dbb);

		setLayout(l);

		QPushButton * b = dbb->button(QDialogButtonBox::StandardButton::Apply);
		b->setText(QApplication::tr("Submit"));

		connect(b, &QPushButton::released, this, &NewsWriterDialog::accept);

		b = dbb->button(QDialogButtonBox::StandardButton::Discard);
		b->setText(QApplication::tr("Discard"));

		connect(b, &QPushButton::released, this, &NewsWriterDialog::rejected);
		connect(b, &QPushButton::released, this, &NewsWriterDialog::reject);
		connect(b, &QPushButton::released, this, &NewsWriterDialog::hide);

		l->addLayout(vl);

		connect(_titleEdit, &QLineEdit::textChanged, this, &NewsWriterDialog::changedNews);
		connect(_dateEdit, &QDateEdit::dateChanged, this, &NewsWriterDialog::changedNews);
		connect(_bodyEdit, &QTextEdit::textChanged, this, &NewsWriterDialog::changedNews);
		connect(_languageBox, &QComboBox::currentTextChanged, this, &NewsWriterDialog::changedLanguage);
	}

	qRegisterMetaType<std::vector<Mod>>("std::vector<common::Mod>");
	connect(this, &NewsWriterDialog::receivedModList, this, &NewsWriterDialog::updateModList);

	setLayout(l);

	l->setStretch(0, 50);
	l->setStretch(1, 50);

	setMinimumWidth(1700);
	setMinimumHeight(600);

	setWindowTitle(QApplication::tr("NewsWriter"));
	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

	changedNews();
	requestMods();
}

void NewsWriterDialog::changedNews() {
	News news;

	news.title = q2s(_titleEdit->text());
	news.body = q2s(_bodyEdit->toPlainText());
	news.timestamp = QDate(2000, 1, 1).daysTo(_dateEdit->date());

	NewsEntry newsEntry;
	newsEntry.title = _titleEdit->text();
	newsEntry.body = _bodyEdit->toPlainText();

	_newsEntries[LanguageConverter::convert(_languageBox->currentText())] = newsEntry;

	_newsPreviewWidget->update(news);
}

void NewsWriterDialog::accept() {
	if (Config::Username.isEmpty()) return;

	QJsonObject json;
	json["Username"] = Config::Username;
	json["Password"] = Config::Password;
	json["Timestamp"] = QDate(2000, 1, 1).daysTo(_dateEdit->date());

	QJsonArray jsonEntriesArr;

	for (auto it = _newsEntries.begin(); it != _newsEntries.end(); ++it) {
		const auto language = it.key();
		auto title = it.value().title;
		auto body = it.value().body;

		title = title.trimmed();
		body = body.trimmed();

		if (title.isEmpty()) continue;

		if (body.isEmpty()) continue;

		QJsonObject jsonEntry;

		jsonEntry["Title"] = encodeString(title);
		jsonEntry["Body"] = encodeString(body);
		jsonEntry["Language"] = language;

		jsonEntriesArr << jsonEntry;
	}

	if (jsonEntriesArr.isEmpty()) return;

	json["Entries"] = jsonEntriesArr;

	if (!_imageReferencesEdit->text().isEmpty()) {
		json["Image"] = _imageReferencesEdit->text();
	}

	QJsonArray jsonArr;
	
	for (const QCheckBox * cb : _mods) {
		if (!cb->isChecked()) continue;

		jsonArr << cb->property("modid").toInt();
	}

	if (!jsonArr.isEmpty()) {
		json["Projects"] = jsonArr;
	}

	bool success = false;

	Https::post(DATABASESERVER_PORT, "submitNews", QJsonDocument(json).toJson(QJsonDocument::Compact), [this, &success](const QJsonObject &, int statusCode) {
		success = statusCode == 200;
	});

	if (success) {
		QMessageBox resultMsg(QMessageBox::Icon::Information, QApplication::tr("NewsSuccessful"), QApplication::tr("NewsSuccessfulText"), QMessageBox::StandardButton::Ok);
		resultMsg.setWindowFlags(resultMsg.windowFlags() & ~Qt::WindowContextHelpButtonHint);
		resultMsg.exec();
		emit refresh();
		QDialog::accept();
		return;
	}
	
	QMessageBox resultMsg(QMessageBox::Icon::Warning, QApplication::tr("NewsUnsuccessful"), QApplication::tr("NewsUnsuccessfulText"), QMessageBox::StandardButton::Ok);
	resultMsg.setWindowFlags(resultMsg.windowFlags() & ~Qt::WindowContextHelpButtonHint);
	resultMsg.exec();
	QDialog::accept();
}

void NewsWriterDialog::updateModList(std::vector<Mod> mods) {
	std::sort(mods.begin(), mods.end(), [](const Mod & a, const Mod & b) {
		return a.name < b.name;
	});
	int counter = 0;
	for (const Mod & mod : mods) {
		auto * cb = new QCheckBox(s2q(mod.name), this);
		cb->setProperty("modid", static_cast<int>(mod.id));
		_modListLayout->addWidget(cb, counter / 3, counter % 3);
		_mods.append(cb);
		counter++;
	}
}

void NewsWriterDialog::addImage() {
	const QString path = QFileDialog::getOpenFileName(this, QApplication::tr("SelectImage"), Config::NEWSIMAGEDIR + "/", "Images (*.png *.jpg)");
	if (path.isEmpty()) {
		return;
	}
	const QString folder = QFileInfo(path).path() + "/";
	if (folder != Config::NEWSIMAGEDIR) {
		QFile::copy(path, Config::NEWSIMAGEDIR + "/" + QFileInfo(path).fileName());
	}
	_imageReferencesEdit->setText(_imageReferencesEdit->text() + QFileInfo(path).fileName() + ";");
}

void NewsWriterDialog::changedLanguage()
{
	const auto news = _newsEntries.value(LanguageConverter::convert(_languageBox->currentText()));
	_titleEdit->setText(news.title);
	_bodyEdit->setPlainText(news.body);
}

void NewsWriterDialog::showEvent(QShowEvent * evt) {
	QDialog::showEvent(evt);

	const auto screens = QGuiApplication::screens();
	const auto * screen = screens[0];
	const QRect scr = screen->geometry();
	move(scr.center() - rect().center());
}

void NewsWriterDialog::requestMods() {
	QJsonObject json;
	json["Username"] = Config::Username;
	json["Password"] = Config::Password;
	json["Language"] = Config::Language;
	json["Simplified"] = 1;

	Https::postAsync(DATABASESERVER_PORT, "requestAllProjects", QJsonDocument(json).toJson(QJsonDocument::Compact), [this](const QJsonObject & content, int statusCode) {
		if (statusCode != 200) return;

		if (!content.contains("Projects")) return;

		std::vector<Mod> projects;

		for (const auto jsonRef : content["Projects"].toArray()) {
			const auto jsonProj = jsonRef.toObject();

			Mod project;
			project.id = jsonProj["ProjectID"].toString().toInt();
			project.name = q2s(jsonProj["Name"].toString());

			projects.push_back(project);
		}

		emit receivedModList(projects);
	});
}
