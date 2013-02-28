/*
 * Copyright (C) by Daniel Molkentin <danimo@owncloud.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 */

#include "generalsettings.h"
#include "ui_generalsettings.h"

#include "mirall/theme.h"

#include <QIcon>

namespace Mirall {

GeneralSettings::GeneralSettings(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::GeneralSettings)
{
    ui->setupUi(this);

    ui->aboutLabel->setText(Theme::instance()->about());
    ui->aboutLabel->setOpenExternalLinks(true);
    ui->manualSettings->setVisible(false);
    ui->authWidget->setVisible(false);
    connect(ui->manualProxyRadioButton, SIGNAL(toggled(bool)), ui->manualSettings, SLOT(setShown(bool)));
    connect(ui->authRequiredcheckBox, SIGNAL(toggled(bool)), ui->authWidget, SLOT(setShown(bool)));
}

GeneralSettings::~GeneralSettings()
{
    delete ui;
}

} // namespace Mirall
