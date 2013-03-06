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

#include "mirall/mirallconfigfile.h"
#include "mirall/theme.h"

#include <QIcon>
#include <QNetworkProxy>

namespace Mirall {

GeneralSettings::GeneralSettings(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::GeneralSettings)
{
    ui->setupUi(this);

    if (Theme::instance()->about().isEmpty()) {
        ui->aboutGroupBox->hide();
    } else {
        ui->aboutLabel->setText(Theme::instance()->about());
        ui->aboutLabel->setOpenExternalLinks(true);
    }
    ui->manualSettings->setVisible(false);
    ui->authWidget->setVisible(false);
    connect(ui->manualProxyRadioButton, SIGNAL(toggled(bool)), ui->manualSettings, SLOT(setShown(bool)));
    connect(ui->authRequiredcheckBox, SIGNAL(toggled(bool)), ui->authWidget, SLOT(setShown(bool)));

    QButtonGroup *proxyGroup = new QButtonGroup(this);
    proxyGroup->addButton(ui->noProxyRadioButton);
    proxyGroup->addButton(ui->systemProxyRadioButton);
    proxyGroup->addButton(ui->manualProxyRadioButton);

    loadMiscSettings();
    loadProxySettings();

    connect(proxyGroup, SIGNAL(buttonClicked(int)), SLOT(saveProxySettings()));
    connect(ui->hostLineEdit, SIGNAL(editingFinished()), SLOT(saveProxySettings()));
    connect(ui->userLineEdit, SIGNAL(editingFinished()), SLOT(saveProxySettings()));
    connect(ui->passwordLineEdit, SIGNAL(editingFinished()), SLOT(saveProxySettings()));

    connect(ui->portSpinBox, SIGNAL(editingFinished()), SLOT(saveProxySettings()));
    connect(ui->monoIconsCheckBox, SIGNAL(toggled(bool)), SLOT(saveMiscSettings()));
}

GeneralSettings::~GeneralSettings()
{
    delete ui;
}


void GeneralSettings::saveProxySettings()
{

    MirallConfigFile cfgFile;

    if (ui->noProxyRadioButton->isChecked())
    {
        cfgFile.setProxyType(QNetworkProxy::NoProxy);
    }
    if (ui->systemProxyRadioButton->isChecked())
    {
        cfgFile.setProxyType(QNetworkProxy::DefaultProxy);
    }
    if (ui->manualProxyRadioButton->isChecked())
    {
        if (ui->authRequiredcheckBox->isChecked())
        {
            QString user = ui->userLineEdit->text();
            QString pass = ui->passwordLineEdit->text();
            cfgFile.setProxyType(QNetworkProxy::HttpProxy, ui->hostLineEdit->text(),
                                 ui->portSpinBox->value(), user, pass);
        }
        else
        {
            cfgFile.setProxyType(QNetworkProxy::HttpProxy, ui->hostLineEdit->text(),
                                 ui->portSpinBox->value(), QString::null, QString::null);
        }
    }

}

void GeneralSettings::saveMiscSettings()
{
    MirallConfigFile cfgFile;
    bool isChecked = ui->monoIconsCheckBox->isChecked();
    cfgFile.setMonoIcons(isChecked);
    Theme::instance()->setSystrayUseMonoIcons(isChecked);
}

void GeneralSettings::loadProxySettings()
{
    // load current proxy settings
    Mirall::MirallConfigFile cfgFile;
    if (cfgFile.proxyType() == QNetworkProxy::NoProxy)
        ui->noProxyRadioButton->setChecked(true);
    if (cfgFile.proxyType() == QNetworkProxy::DefaultProxy)
        ui->systemProxyRadioButton->setChecked(true);
    if (cfgFile.proxyType() == QNetworkProxy::HttpProxy)
    {
        ui->manualProxyRadioButton->setChecked(true);
        ui->hostLineEdit->setText(cfgFile.proxyHostName());
        ui->portSpinBox->setValue(cfgFile.proxyPort());
        if (!cfgFile.proxyUser().isEmpty())
        {
            ui->authRequiredcheckBox->setChecked(true);
            ui->userLineEdit->setText(cfgFile.proxyUser());
            ui->passwordLineEdit->setText(cfgFile.proxyPassword());
        }
    }
}

void GeneralSettings::loadMiscSettings()
{
    MirallConfigFile cfgFile;
    ui->monoIconsCheckBox->setChecked(cfgFile.monoIcons());
}

} // namespace Mirall
