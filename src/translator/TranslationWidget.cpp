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

#include "translator/TranslationWidget.h"

#include <QHBoxLayout>
#include <QLineEdit>

using namespace spine::translation;

TranslationWidget::TranslationWidget(QString sourceText, QString targetText, QWidget * par) : QWidget(par), _sourceEdit(new QLineEdit(sourceText, this)), _translationEdit(new QLineEdit(targetText, this)) {
	auto * l = new QHBoxLayout();

	_sourceEdit->setReadOnly(true);

	l->addWidget(_sourceEdit);
	l->addWidget(_translationEdit);

	setLayout(l);

	connect(_translationEdit, &QLineEdit::cursorPositionChanged, this, &TranslationWidget::focusChanged);
	connect(_translationEdit, &QLineEdit::textChanged, this, &TranslationWidget::focusChanged);
}

QString TranslationWidget::getTranslation() const {
	return _translationEdit->text();
}

void TranslationWidget::setFocusToTranslation() {
	_translationEdit->setFocus();
	focusChanged();
}

void TranslationWidget::focusChanged() {
	emit selectedSource(_sourceEdit->text());
	emit selectedDestination(_translationEdit->text());
}
